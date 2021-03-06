/* linux/drivers/media/video/samsung/post/s3c_pp_6400.c
 *
 * Driver file for Samsung Post processor
 *
 * Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/errno.h> 	/* error codes */
#include <asm/div64.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <mach/map.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/mman.h>
#include <linux/version.h>

#include <plat/regs-pp.h>
#include <plat/regs-lcd.h>
#include <plat/clock.h>
#include <plat/regs-clock.h>
#include <plat/pm.h>

#include "s3c_pp.h"   		/* ioctl */
#include "s3c_pp_common.h" 	/* internal used struct & type */
#include "plat/media.h"

#undef DEBUG

#ifdef DEBUG
#define dprintk printk
#else
#define dprintk(format,args...)
#endif

#define PFX "s3c_pp"

/* if you want to modify src/dst buffer size, modify below defined size */
#define PP_RESERVED_MEM_SIZE		8*1024*1024
#define pp_reserved_mem_addr_phy	0x57800000
#define PP_RESERVED_MEM_ADDR_PHY	(UINT32) s3c_get_media_memory(S3C_MDEV_POST)

#define ALLOC_KMEM				1

#define PP_MAX_NO_OF_INSTANCES			4

#define PP_VALUE_CHANGED_NONE			0x00
#define PP_VALUE_CHANGED_PARAMS			0x01
#define PP_VALUE_CHANGED_SRC_BUF_ADDR_PHY	0x02
#define PP_VALUE_CHANGED_DST_BUF_ADDR_PHY	0x04

#define PP_INSTANCE_FREE			0
#define PP_INSTANCE_READY			1
#define PP_INSTANCE_INUSE_DMA_ONESHOT		2
#define PP_INSTANCE_INUSE_FIFO_FREERUN		3

#define S3C_PP_SAVE_START_ADDR			0x0
#define S3C_PP_SAVE_END_ADDR			0xA0



typedef struct {
	int		running_instance_no;
	int		last_running_instance_no;
	int		fifo_mode_instance_no;
	unsigned int	wincon0_value_before_fifo_mode;
	int		dma_mode_instance_count;
	int		in_use_instance_count;
	unsigned char	instance_state[PP_MAX_NO_OF_INSTANCES];
} s3c_pp_instance_info_t;

static s3c_pp_instance_info_t s3c_pp_instance_info;



static void __iomem *s3c_pp_base;


static struct clk *s3c_pp_hclk;
static unsigned long s3c_pp_hclk_rate;

static struct mutex *h_mutex;
static struct mutex *mem_alloc_mutex;

static wait_queue_head_t waitq;





void set_scaler_register(s3c_pp_scaler_info_t * scaler_info, 
			s3c_pp_instance_context_t *pp_instance)
{
	__raw_writel((scaler_info->pre_v_ratio << 7) | (scaler_info->pre_h_ratio << 0), 
			s3c_pp_base + S3C_VPP_PRESCALE_RATIO);
	__raw_writel((scaler_info->pre_dst_height << 12) | (scaler_info->pre_dst_width << 0), 
			s3c_pp_base + S3C_VPP_PRESCALEIMGSIZE);
	__raw_writel(scaler_info->sh_factor, s3c_pp_base + S3C_VPP_PRESCALE_SHFACTOR);
	__raw_writel(scaler_info->dx, s3c_pp_base + S3C_VPP_MAINSCALE_H_RATIO);
	__raw_writel(scaler_info->dy, s3c_pp_base + S3C_VPP_MAINSCALE_V_RATIO);
	__raw_writel((pp_instance->src_height << 12) | (pp_instance->src_width), 
			s3c_pp_base + S3C_VPP_SRCIMGSIZE);
	__raw_writel((pp_instance->dst_height << 12) | (pp_instance->dst_width), 
			s3c_pp_base + S3C_VPP_DSTIMGSIZE);
}


void set_src_addr_register(s3c_pp_buf_addr_t *buf_addr, s3c_pp_instance_context_t *pp_instance)
{
	__raw_writel(buf_addr->src_start_y, s3c_pp_base + S3C_VPP_ADDRSTART_Y);
	__raw_writel(buf_addr->offset_y, s3c_pp_base + S3C_VPP_OFFSET_Y);
	__raw_writel(buf_addr->src_end_y, s3c_pp_base + S3C_VPP_ADDREND_Y);

	if (pp_instance->src_color_space == YC420) {
		__raw_writel(buf_addr->src_start_cb, s3c_pp_base + S3C_VPP_ADDRSTART_CB);
		__raw_writel(buf_addr->offset_cr, s3c_pp_base + S3C_VPP_OFFSET_CB);
		__raw_writel(buf_addr->src_end_cb, s3c_pp_base + S3C_VPP_ADDREND_CB);
		__raw_writel(buf_addr->src_start_cr, s3c_pp_base + S3C_VPP_ADDRSTART_CR);
		__raw_writel(buf_addr->offset_cb, s3c_pp_base + S3C_VPP_OFFSET_CR);
		__raw_writel(buf_addr->src_end_cr, s3c_pp_base + S3C_VPP_ADDREND_CR);
	}
}

void set_dest_addr_register(s3c_pp_buf_addr_t *buf_addr, s3c_pp_instance_context_t *pp_instance)
{
	if (PP_INSTANCE_INUSE_DMA_ONESHOT == 
		s3c_pp_instance_info.instance_state[pp_instance->instance_no]) {
		__raw_writel(buf_addr->dst_start_rgb, s3c_pp_base + S3C_VPP_ADDRSTART_RGB);
		__raw_writel(buf_addr->offset_rgb, s3c_pp_base + S3C_VPP_OFFSET_RGB);
		__raw_writel(buf_addr->dst_end_rgb, s3c_pp_base + S3C_VPP_ADDREND_RGB);

		if (pp_instance->dst_color_space == YC420) {
			__raw_writel(buf_addr->out_src_start_cb, 
					s3c_pp_base + S3C_VPP_ADDRSTART_OCB);
			__raw_writel(buf_addr->out_offset_cb, s3c_pp_base + S3C_VPP_OFFSET_OCB);
			__raw_writel(buf_addr->out_src_end_cb, s3c_pp_base + S3C_VPP_ADDREND_OCB);
			__raw_writel(buf_addr->out_src_start_cr, 
					s3c_pp_base + S3C_VPP_ADDRSTART_OCR);
			__raw_writel(buf_addr->out_offset_cr, s3c_pp_base + S3C_VPP_OFFSET_OCR);
			__raw_writel(buf_addr->out_src_end_cr, s3c_pp_base + S3C_VPP_ADDREND_OCR);
		}
	}
}

void set_src_next_addr_register(s3c_pp_buf_addr_t *buf_addr, 
				s3c_pp_instance_context_t *pp_instance)
{
	__raw_writel(buf_addr->src_start_y, s3c_pp_base + S3C_VPP_NXTADDRSTART_Y);
	__raw_writel(buf_addr->src_end_y, s3c_pp_base + S3C_VPP_NXTADDREND_Y);

	if (pp_instance->src_color_space == YC420) {
		__raw_writel(buf_addr->src_start_cb, s3c_pp_base + S3C_VPP_NXTADDRSTART_CB);
		__raw_writel(buf_addr->src_end_cb, s3c_pp_base + S3C_VPP_NXTADDREND_CB);
		__raw_writel(buf_addr->src_start_cr, s3c_pp_base + S3C_VPP_NXTADDRSTART_CR);
		__raw_writel(buf_addr->src_end_cr, s3c_pp_base + S3C_VPP_NXTADDREND_CR);
	}
}

void set_data_format_register(s3c_pp_instance_context_t *pp_instance)
{
	u32 tmp;

	tmp = __raw_readl(s3c_pp_base + S3C_VPP_MODE);
	tmp |= (0x1 << 16);
	tmp |= (0x2 << 10);

	// set the source color space
	switch (pp_instance->src_color_space) {
	case YC420:
		tmp &= ~((0x1 << 3) | (0x1 << 2));
		tmp |= (0x1 << 8) | (0x1 << 1);
		break;
	case CRYCBY:
		tmp &= ~((0x1 << 15) | (0x1 << 8) | (0x1 << 3) | (0x1 << 0));
		tmp |= (0x1 << 2) | (0x1 << 1);
		break;
	case CBYCRY:
		tmp &= ~((0x1 << 8) | (0x1 << 3) | (0x1 << 0));
		tmp |= (0x1 << 15) | (0x1 << 2) | (0x1 << 1);
		break;
	case YCRYCB:
		tmp &= ~((0x1 << 15) | (0x1 << 8) | (0x1 << 3));
		tmp |= (0x1 << 2) | (0x1 << 1) | (0x1 << 0);
		break;
	case YCBYCR:
		tmp &= ~((0x1 << 8) | (0x1 << 3));
		tmp |= (0x1 << 15) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0);
		break;
	case RGB24:
		tmp &= ~(0x1 << 8);
		tmp |= (0x1 << 3) | (0x1 << 2) | (0x1 << 1);
		break;
	case RGB16:
		tmp &= ~((0x1 << 8) | (0x1 << 1));
		tmp |= (0x1 << 3) | (0x1 << 2);
		break;
	default:
		break;
	}

	// set the destination color space
	if (PP_INSTANCE_INUSE_DMA_ONESHOT == 
		s3c_pp_instance_info.instance_state[pp_instance->instance_no]) {
		switch (pp_instance->dst_color_space) {
		case YC420:
			tmp &= ~(0x1 << 18);
			tmp |= (0x1 << 17);
			break;
		case CRYCBY:
			tmp &= ~((0x1 << 20) | (0x1 << 19) | (0x1 << 18) | (0x1 << 17));
			break;
		case CBYCRY:
			tmp &= ~((0x1 << 19) | (0x1 << 18) | (0x1 << 17));
			tmp |= (0x1 << 20);
			break;
		case YCRYCB:
			tmp &= ~((0x1 << 20) | (0x1 << 18) | (0x1 << 17));
			tmp |= (0x1 << 19);
			break;
		case YCBYCR:
			tmp &= ~((0x1 << 18) | (0x1 << 17));
			tmp |= (0x1 << 20) | (0x1 << 19);
			break;
		case RGB24:
			tmp |= (0x1 << 18) | (0x1 << 4);
			break;
		case RGB16:
			tmp &= ~(0x1 << 4);
			tmp |= (0x1 << 18);
			break;
		default:
			break;
		}
	} else if (PP_INSTANCE_INUSE_FIFO_FREERUN == 
			s3c_pp_instance_info.instance_state[pp_instance->instance_no]) {
		if (pp_instance->dst_color_space == RGB30) {
			tmp |= (0x1 << 18) | (0x1 << 13);

		} else if (pp_instance->dst_color_space == YUV444) {
			tmp |= (0x1 << 13);
			tmp &= ~(0x1 << 18) | (0x1 << 17);
		}
	}

	__raw_writel(tmp, s3c_pp_base + S3C_VPP_MODE);
}

static void set_clock_src(s3c_pp_clk_src_t clk_src)
{
	u32 tmp;

	tmp = __raw_readl(s3c_pp_base + S3C_VPP_MODE);

	if (clk_src == PP_HCLK) {
		if ( s3c_pp_hclk_rate > 66000000 ) {
			tmp &= ~(0x7f<<23);
			tmp |= (1<<24);
			tmp |= (1<<23);
		} else {
			tmp &=~ (0x7f<<23);
		}
	} else if (clk_src == PLL_EXT) {
	} else {
		tmp &= ~(0x7f << 23);
	}

	tmp = (tmp & ~(0x3 << 21)) | (clk_src << 21);

	__raw_writel(tmp, s3c_pp_base + S3C_VPP_MODE);
}


static void post_int_enable(u32 int_type)
{
	u32 tmp;

	tmp = __raw_readl(s3c_pp_base + S3C_VPP_MODE);

	if (int_type == 0) {		/* Edge triggering */
		tmp &= ~(S3C_MODE_IRQ_LEVEL);
	} else if (int_type == 1) {	/* level triggering */
		tmp |= S3C_MODE_IRQ_LEVEL;
	}

	tmp |= S3C_MODE_POST_INT_ENABLE;

	__raw_writel(tmp, s3c_pp_base + S3C_VPP_MODE);
}












static void start_processing(void)
{
	__raw_writel(0x1 << 31, s3c_pp_base + S3C_VPP_POSTENVID);
}

s3c_pp_state_t post_get_processing_state(void)
{
	s3c_pp_state_t	state;
	u32 tmp;

	tmp = __raw_readl(s3c_pp_base + S3C_VPP_POSTENVID);

	if (tmp & S3C_VPP_POSTENVID) {
		state = POST_BUSY;
	} else {
		state = POST_IDLE;
	}

	dprintk("Post processing state = %d\n", state);

	return state;
}










static void pp_dma_mode_set_and_start(void)
{
	unsigned int temp;

	__raw_writel(0x0 << 31, s3c_pp_base + S3C_VPP_POSTENVID);

	temp = __raw_readl(s3c_pp_base + S3C_VPP_MODE);

	temp &= ~(0x1 << 31);   // must be 0
	temp &= ~(0x1 << 13);   // dma
	temp &= ~(0x1 << 14);   // per frame mode

	__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE);

	__raw_writel(0x1 << 31, s3c_pp_base + S3C_VPP_POSTENVID); // PP start
}

static void pp_fifo_mode_set_and_start(s3c_pp_instance_context_t *current_instance)
{
	unsigned int temp;

	/* line count check */
	while (__raw_readl(S3C_VIDCON1) & 0x07FF0000) {
	}

	temp = __raw_readl(S3C_WINCON0);
	temp &= ~S3C_WINCONx_ENWIN_F_ENABLE;
	__raw_writel(temp, S3C_WINCON0);

	__raw_writel(0x0 << 31, s3c_pp_base + S3C_VPP_POSTENVID);

	temp = __raw_readl(s3c_pp_base + S3C_VPP_MODE);

	temp &= ~(0x1 << 31);

	if (PROGRESSIVE_MODE == current_instance->scan_mode) {
		temp &= ~(0x1 << 12);	// 0: progressive mode, 1: interlace mode
	} else {
		temp |= (0x1 << 12);	// 0: progressive mode, 1: interlace mode
	}

	temp |= (0x1 << 13);		// fifo
	temp |= (0x1 << 14);		// free run

	__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE);

	__raw_writel(0x1 << 31, s3c_pp_base + S3C_VPP_POSTENVID); // PP start


	temp = __raw_readl(S3C_WINCON0);

	temp = current_instance->dst_width * current_instance->dst_height;
	__raw_writel(temp, S3C_VIDOSD0C);

	temp = __raw_readl(S3C_WINCON0);
	temp &= ~(S3C_WINCONx_BPPMODE_F_MASK | (0x3 << 9));
	temp &= ~(S3C_WINCONx_ENLOCAL_POST | S3C_WINCONx_INRGB_MASK);
	temp &= ~(S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BYTSWP_ENABLE | 
			S3C_WINCONx_BITSWP_ENABLE);	// swap disable
	temp |= S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BURSTLEN_4WORD;
	temp |= S3C_WINCONx_ENLOCAL_POST | S3C_WINCONx_ENWIN_F_ENABLE;

	__raw_writel(temp, S3C_WINCON0);
}

static irqreturn_t s3c_pp_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	u32 temp;

	temp = __raw_readl(s3c_pp_base + S3C_VPP_MODE);
	temp &= ~(S3C_MODE_POST_PENDING | S3C_MODE_POST_INT_ENABLE);
	__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE);

	s3c_pp_instance_info.running_instance_no = -1;

	wake_up_interruptible(&waitq);

	return IRQ_HANDLED;
}


int s3c_pp_open(struct inode *inode, struct file *file)
{
	s3c_pp_instance_context_t *current_instance;
	int i;

	mutex_lock(h_mutex);

	/* check instance of fifo mode */ 
	if (s3c_pp_instance_info.fifo_mode_instance_no != -1) {
		printk(KERN_ERR "PP instance allocation is fail: Fifo Mode was already opened.\n");
		mutex_unlock(h_mutex);
		return -1;
	}

	/* check instance pool */
	if (PP_MAX_NO_OF_INSTANCES <= s3c_pp_instance_info.in_use_instance_count) {
		printk(KERN_ERR "PP instance allocation is fail: No more instance.\n");
		mutex_unlock(h_mutex);
		return -1;
	}

	/* allocating the post processor instance */
	current_instance = (s3c_pp_instance_context_t *) 
				kmalloc(sizeof(s3c_pp_instance_context_t), 
					GFP_DMA | GFP_ATOMIC);

	if (current_instance == NULL) {
		printk(KERN_ERR "PP instance allocation is fail: Kmalloc failed.\n");
		mutex_unlock(h_mutex);
		return -1;
	}

	/* Find free instance */
	for (i = 0; i < PP_MAX_NO_OF_INSTANCES; i++) {
		if (PP_INSTANCE_FREE == s3c_pp_instance_info.instance_state[i]) {
			s3c_pp_instance_info.instance_state[i] = PP_INSTANCE_READY;
			s3c_pp_instance_info.in_use_instance_count++;
			break;
		}
	}

	if (PP_MAX_NO_OF_INSTANCES == i) {
		kfree(current_instance);
		printk(KERN_ERR "PP instance allocation is fail: No more instance.\n");
		mutex_unlock(h_mutex);
		return -1;
	}

	memset(current_instance, 0, sizeof(s3c_pp_instance_context_t));
	current_instance->instance_no = i;

	/* check first instance */
	if (1 == s3c_pp_instance_info.in_use_instance_count) {
		/* PP HW Initalize or clock enable; for Power Saving */
		clk_enable(s3c_pp_hclk);
	}

	dprintk(KERN_DEBUG "%s PP instance allocation is success. (%d)\n", __FUNCTION__, i);

	file->private_data = (s3c_pp_instance_context_t *) current_instance;

	mutex_unlock(h_mutex);

	return 0;
}





















































































int s3c_pp_release(struct inode *inode, struct file *file)
{
	s3c_pp_instance_context_t *current_instance;

	mutex_lock(h_mutex);

	dprintk("%s: Enterance\n", __FUNCTION__);

	current_instance = (s3c_pp_instance_context_t *) file->private_data;

	if (NULL == current_instance) {
		mutex_unlock(h_mutex);
		return -1;
	}

	if (PP_INSTANCE_INUSE_DMA_ONESHOT == 
		s3c_pp_instance_info.instance_state[current_instance->instance_no]) {
		s3c_pp_instance_info.dma_mode_instance_count--;
	} else if (PP_INSTANCE_INUSE_FIFO_FREERUN == 
			s3c_pp_instance_info.instance_state[current_instance->instance_no]) {

		s3c_pp_instance_info.fifo_mode_instance_no = -1;

		do {
			unsigned int temp;

			temp = __raw_readl(S3C_VIDCON0) & ~S3C_VIDCON0_ENVID_ENABLE;
			__raw_writel(temp, S3C_VIDCON0);

			while (__raw_readl(S3C_VIDCON1) & 0x07FF0000) {
			}

			/* set Per Frame mode */
			temp = __raw_readl(s3c_pp_base + S3C_VPP_MODE) & ~S3C_MODE_AUTOLOAD_ENABLE; 
			__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE);

			temp = s3c_pp_instance_info.wincon0_value_before_fifo_mode & 
				~S3C_WINCONx_ENWIN_F_ENABLE;
			__raw_writel(temp, S3C_WINCON0);

			temp = __raw_readl(S3C_VIDCON0) | S3C_VIDCON0_ENVID_ENABLE | 
				S3C_VIDCON0_ENVID_F_ENABLE;
			__raw_writel(temp, S3C_VIDCON0);
		} while (0);
	}

	if (current_instance->instance_no == s3c_pp_instance_info.last_running_instance_no) {
		s3c_pp_instance_info.last_running_instance_no = -1;
	}

	s3c_pp_instance_info.instance_state[current_instance->instance_no] = PP_INSTANCE_FREE;

	if (0 < s3c_pp_instance_info.in_use_instance_count && 
		s3c_pp_instance_info.in_use_instance_count <= PP_MAX_NO_OF_INSTANCES) {
		s3c_pp_instance_info.in_use_instance_count--;

		if (0 == s3c_pp_instance_info.in_use_instance_count) {
			s3c_pp_instance_info.last_running_instance_no = -1;

			clk_disable(s3c_pp_hclk);
		}
	}

	dprintk("%s: handle=%d, count=%d\n", 
		__FUNCTION__, current_instance->instance_no, 
		s3c_pp_instance_info.in_use_instance_count);

	kfree(current_instance);

	mutex_unlock(h_mutex);

	return 0;
}


int s3c_pp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	s3c_pp_instance_context_t *current_instance;
	s3c_pp_params_t *parg;

	unsigned int temp = 0;

	mutex_lock(h_mutex);

	current_instance	= (s3c_pp_instance_context_t *) file->private_data;
	parg	            = (s3c_pp_params_t *) arg;

	switch (cmd) {
	case S3C_PP_SET_PARAMS: {
		s3c_pp_out_path_t temp_out_path;
		unsigned int temp_src_width, temp_src_height, temp_dst_width, temp_dst_height;
		s3c_color_space_t temp_src_color_space, temp_dst_color_space;

		get_user(temp_out_path, &parg->out_path);

		if ((-1 != s3c_pp_instance_info.fifo_mode_instance_no)
		    || ((s3c_pp_instance_info.dma_mode_instance_count) && 
		    	(FIFO_FREERUN == temp_out_path))) {
			printk(KERN_ERR 
				"\n%s: S3C_PP_SET_PARAMS can't be executed.\n", __FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}

		get_user(temp_src_width,  &parg->src_width);
		get_user(temp_src_height, &parg->src_height);
		get_user(temp_dst_width,  &parg->dst_width);
		get_user(temp_dst_height, &parg->dst_height);

		/*
		 * S3C6410 support that the source image is up to 4096 x 4096
		 *     and the destination image is up to 2048 x 2048.
		 */
		if ((temp_src_width > 4096) || (temp_src_height > 4096)	
			|| (temp_dst_width > 2048) || (temp_dst_height > 2048)) {
			printk(KERN_ERR "\n%s: Size is too big to be supported.\n", __FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}

		get_user(temp_src_color_space, &parg->src_color_space);
		get_user(temp_dst_color_space, &parg->dst_color_space);

		if (((temp_src_color_space == YC420) && (temp_src_width % 8))
			|| ((temp_src_color_space == RGB16) && (temp_src_width % 2))
			|| ((temp_out_path == DMA_ONESHOT) 
		    		&& (((temp_dst_color_space == YC420) && (temp_dst_width % 8))
					|| ((temp_dst_color_space == RGB16) 
						&& (temp_dst_width % 2))))) {
			printk(KERN_ERR "\n%s: YUV420 image width must be a multiple of 8.\n", 
				__FUNCTION__);
			printk(KERN_ERR "%s: RGB16 must be a multiple of 2.\n", __FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}


		get_user(current_instance->src_full_width,  &parg->src_full_width);
		get_user(current_instance->src_full_height, &parg->src_full_height);
		get_user(current_instance->src_start_x,     &parg->src_start_x);
		get_user(current_instance->src_start_y,     &parg->src_start_y);
		current_instance->src_width       = temp_src_width;
		current_instance->src_height      = temp_src_height;
		current_instance->src_color_space = temp_src_color_space;

		get_user(current_instance->dst_full_width,  &parg->dst_full_width);
		get_user(current_instance->dst_full_height, &parg->dst_full_height);
		get_user(current_instance->dst_start_x,     &parg->dst_start_x);
		get_user(current_instance->dst_start_y,     &parg->dst_start_y);
		current_instance->dst_width       = temp_dst_width;
		current_instance->dst_height      = temp_dst_height;
		current_instance->dst_color_space = temp_dst_color_space;

		current_instance->out_path = temp_out_path;

		if (DMA_ONESHOT == current_instance->out_path) {
			s3c_pp_instance_info.instance_state[current_instance->instance_no] 
				= PP_INSTANCE_INUSE_DMA_ONESHOT;
			s3c_pp_instance_info.dma_mode_instance_count++;
		} else {
			get_user(current_instance->scan_mode, &parg->scan_mode);

			current_instance->dst_color_space = RGB30;

			s3c_pp_instance_info.instance_state[current_instance->instance_no] 
				= PP_INSTANCE_INUSE_FIFO_FREERUN;
			s3c_pp_instance_info.fifo_mode_instance_no 
				= current_instance->instance_no;
			s3c_pp_instance_info.wincon0_value_before_fifo_mode 
				= __raw_readl(S3C_WINCON0);

			//.[ REDUCE_VCLK_SYOP_TIME
			if (current_instance->src_height > current_instance->dst_height) {
				int i;

				for (i = 2; 
					(current_instance->src_height 
						>= (i * current_instance->dst_height)) && (i < 8); 
					i++) {
				}

				current_instance->src_full_width  *= i;
				current_instance->src_full_height /= i;
				current_instance->src_height      /= i;
			}
			//.] REDUCE_VCLK_SYOP_TIME
		}

		current_instance->value_changed |= PP_VALUE_CHANGED_PARAMS;
	}
	break;

	case S3C_PP_START:
		dprintk("%s: S3C_PP_START last_instance=%d, curr_instance=%d\n", __FUNCTION__,
			s3c_pp_instance_info.last_running_instance_no, 
			current_instance->instance_no);

		if (PP_INSTANCE_READY 
			== s3c_pp_instance_info.instance_state[current_instance->instance_no]) {
			printk(KERN_ERR 
				"%s: S3C_PP_START must be executed " 
				"after running S3C_PP_SET_PARAMS.\n", 
				__FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}

		if (current_instance->instance_no 
			!= s3c_pp_instance_info.last_running_instance_no) {
			__raw_writel(0x0 << 31, s3c_pp_base + S3C_VPP_POSTENVID);

			temp = S3C_MODE2_ADDR_CHANGE_DISABLE | S3C_MODE2_CHANGE_AT_FRAME_END 
				| S3C_MODE2_SOFTWARE_TRIGGER;
			__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE_2);

			set_clock_src(PP_HCLK);

			/* setting the src/dst color space */
			set_data_format(current_instance);

			/* setting the src/dst size */
			set_scaler(current_instance);

			/* setting the src/dst buffer address */
			set_src_addr(current_instance);
			set_dest_addr(current_instance);

			current_instance->value_changed = PP_VALUE_CHANGED_NONE;

			s3c_pp_instance_info.last_running_instance_no 
				= current_instance->instance_no;
			s3c_pp_instance_info.running_instance_no = current_instance->instance_no;

			if (PP_INSTANCE_INUSE_DMA_ONESHOT 
			    == s3c_pp_instance_info.instance_state[current_instance->instance_no]) {
				dprintk("%s: DMA_ONESHOT mode\n", __FUNCTION__);

				post_int_enable(1);
				pp_dma_mode_set_and_start();

				if (!(file->f_flags & O_NONBLOCK)) {
					if (interruptible_sleep_on_timeout(&waitq, 500) == 0) {
						printk(KERN_ERR 
							" %s: Waiting for interrupt is timeout\n", 
							__FUNCTION__);
					}
				}
			} else {
				dprintk("%s: FIFO_freerun mode\n", __FUNCTION__);
				s3c_pp_instance_info.fifo_mode_instance_no 
					= current_instance->instance_no;

				post_int_enable(1);
				pp_fifo_mode_set_and_start(current_instance);
			}
		} else {
			if (current_instance->value_changed != PP_VALUE_CHANGED_NONE) {
				__raw_writel(0x0 << 31, s3c_pp_base + S3C_VPP_POSTENVID);

				if (current_instance->value_changed & PP_VALUE_CHANGED_PARAMS) {
					set_data_format(current_instance);
					set_scaler(current_instance);
				}

				if (current_instance->value_changed 
					& PP_VALUE_CHANGED_SRC_BUF_ADDR_PHY) {
					set_src_addr(current_instance);
				}

				if (current_instance->value_changed 
					& PP_VALUE_CHANGED_DST_BUF_ADDR_PHY) {
					set_dest_addr(current_instance);
				}

				current_instance->value_changed = PP_VALUE_CHANGED_NONE;
			}

			s3c_pp_instance_info.running_instance_no = current_instance->instance_no;

			post_int_enable(1);
			start_processing();

			if (!(file->f_flags & O_NONBLOCK)) {
				if (interruptible_sleep_on_timeout(&waitq, 500) == 0) {
					printk(KERN_ERR 
						"\n%s: Waiting for interrupt is timeout\n", 
						__FUNCTION__);
				}
			}
		}

		break;

	case S3C_PP_GET_SRC_BUF_SIZE:

		if (PP_INSTANCE_READY 
			== s3c_pp_instance_info.instance_state[current_instance->instance_no]) {
			dprintk("%s: S3C_PP_GET_SRC_BUF_SIZE must be executed "
				"after running S3C_PP_SET_PARAMS.\n", __FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}

		temp = cal_data_size(current_instance->src_color_space, 
					current_instance->src_full_width, 
					current_instance->src_full_height);

		mutex_unlock(h_mutex);
		return temp;


	case S3C_PP_SET_SRC_BUF_ADDR_PHY:

#if 0
		get_user(current_instance->src_buf_addr_phy, &parg->src_buf_addr_phy);
#else 
		get_user(current_instance->src_buf_addr_phy[0], &parg->src_buf_addr_phy[0]);
		get_user(current_instance->src_buf_addr_phy[1], &parg->src_buf_addr_phy[1]);
		get_user(current_instance->src_buf_addr_phy[2], &parg->src_buf_addr_phy[2]);
		get_user(current_instance->src_buf_addr_phy[3], &parg->src_buf_addr_phy[3]);
#endif                                                                  /*  change by cmj               */
		current_instance->value_changed |= PP_VALUE_CHANGED_SRC_BUF_ADDR_PHY;
		break;

	case S3C_PP_SET_SRC_BUF_NEXT_ADDR_PHY:

		if (current_instance->instance_no != s3c_pp_instance_info.fifo_mode_instance_no) { 
			dprintk(KERN_DEBUG 
				"%s: S3C_PP_SET_SRC_BUF_NEXT_ADDR_PHY can't be executed.\n", 
				__FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}

		get_user(current_instance->src_next_buf_addr_phy, &parg->src_next_buf_addr_phy);

		temp = __raw_readl(s3c_pp_base + S3C_VPP_MODE_2);
		temp |= (0x1 << 4);
		__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE_2);

		set_src_next_buf_addr(current_instance);

		temp = __raw_readl(s3c_pp_base + S3C_VPP_MODE_2);
		temp &= ~(0x1 << 4);
		__raw_writel(temp, s3c_pp_base + S3C_VPP_MODE_2);
		break;

	case S3C_PP_GET_DST_BUF_SIZE:

		if (PP_INSTANCE_READY 
			== s3c_pp_instance_info.instance_state[current_instance->instance_no]) {
			dprintk("%s: S3C_PP_GET_DST_BUF_SIZE must be executed "
				"after running S3C_PP_SET_PARAMS.\n", __FUNCTION__);
			mutex_unlock(h_mutex);
			return -EINVAL;
		}

		temp = cal_data_size(current_instance->dst_color_space, 
					current_instance->dst_full_width, 
					current_instance->dst_full_height);

		mutex_unlock(h_mutex);
		return temp;

	case S3C_PP_SET_DST_BUF_ADDR_PHY:

#if 0
		get_user(current_instance->dst_buf_addr_phy, &parg->dst_buf_addr_phy);
#else 
		get_user(current_instance->dst_buf_addr_phy[0], &parg->dst_buf_addr_phy[0]);
		get_user(current_instance->dst_buf_addr_phy[1], &parg->dst_buf_addr_phy[1]);
		get_user(current_instance->dst_buf_addr_phy[2], &parg->dst_buf_addr_phy[2]);
		get_user(current_instance->dst_buf_addr_phy[3], &parg->dst_buf_addr_phy[3]);
#endif                                                                  /*  change by cmj               */
		current_instance->value_changed |= PP_VALUE_CHANGED_DST_BUF_ADDR_PHY;
		break;










































































	default:
		mutex_unlock(h_mutex);
		return -EINVAL;
	}

	mutex_unlock(h_mutex);

	return 0;
}































































	





static int s3c_pp_probe(struct platform_device *pdev)
{


	int ret;
	int i;
	






















#define S3C6400_PA_POST     (0x77000000)                                /*  POST 的 SFR 区域的物理基地址*/
#define S3C_SZ_POST         (4096)                                      /*  POST 的 SFR 区域的大小      */
	s3c_pp_base = ioremap(S3C6400_PA_POST, S3C_SZ_POST);

	if (s3c_pp_base == NULL) {
		printk(KERN_ERR PFX "failed ioremap\n");
		return -ENOENT;
	}

	s3c_pp_hclk = clk_get(&pdev->dev, "post");

	if (!s3c_pp_hclk || IS_ERR(s3c_pp_hclk)) {
		printk("failed to get post hclk source\n");
		return -ENOENT;
	}

	s3c_pp_hclk_rate = clk_get_rate(s3c_pp_hclk);

	init_waitqueue_head(&waitq);








#define VIC_CHANNEL_POST0       9
	ret = request_irq(VIC_CHANNEL_POST0, (irq_handler_t) s3c_pp_isr, IRQF_DISABLED,
			  pdev->name, NULL);

	if (ret) {
		printk(KERN_ERR "request_irq(PP) failed.\n");
		return ret;
	}






	mutex_init(h_mutex);






	mutex_init(mem_alloc_mutex);

	/* initialzie instance infomation */
	s3c_pp_instance_info.running_instance_no = -1;
	s3c_pp_instance_info.last_running_instance_no = -1;
	s3c_pp_instance_info.in_use_instance_count = 0;
	s3c_pp_instance_info.dma_mode_instance_count = 0;
	s3c_pp_instance_info.fifo_mode_instance_no = -1;

	for (i = 0; i < PP_MAX_NO_OF_INSTANCES; i++)
		s3c_pp_instance_info.instance_state[i] = PP_INSTANCE_FREE;

	return 0;
}


















	






	



	















static struct platform_device s3c_pp_driver = {
	.name  = "s3c-vpp",

};






int __init s3c_pp_init(void)
{
    static int iFlg        = 0;
    
    if (iFlg != 0) {
    	return 0;
    }
    iFlg = 1;


	return s3c_pp_probe(&s3c_pp_driver);
}
















