/* linux/drivers/media/video/samsung/jpeg/s3c-jpeg.c
 *
 * Driver file for Samsung JPEG Encoder/Decoder
 *
 * Peter Oh, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*==============================================================================
** s3c_jpeg.c -- Porting from linux-2.6.28-samsung.
**               s3c_jpeg.c.orginal save the diff.
**
** MODIFY HISTORY:
**
** 2012-05-10 wdf Create.
==============================================================================*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/irq.h>		
#include <plat/map.h>	
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/platform_device.h>

#include <linux/version.h>
#include <plat/regs-clock.h>		

#include <linux/time.h>
#include <linux/clk.h>

#include "s3c_jpeg.h"
#include "jpg_mem.h"
#include "jpg_misc.h"
#include "jpg_opr.h"
#include "log_msg.h"


static struct clk		*jpeg_hclk;
static struct clk		*jpeg_sclk;
static void __iomem		*jpeg_base;
static s3c6400_jpg_ctx	JPGMem;
static int				irq_no = 15;
static int				instanceNo = 0;
volatile int			jpg_irq_reason;

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_JPEG);

/*==============================================================================
 * - s3c_jpeg_irq()
 *
 * - jpeg irq interrupt service routine
 */
irqreturn_t s3c_jpeg_irq(int irq, void *dev_id)
{
	unsigned int	intReason;
	unsigned int	status;

	status = JPGMem.v_pJPG_REG->JPGStatus;
	intReason = JPGMem.v_pJPG_REG->JPGIRQStatus;

	if(intReason) {
		intReason &= ((1<<6)|(1<<4)|(1<<3));

		switch(intReason) {
			case 0x08 : 
				jpg_irq_reason = OK_HD_PARSING; 
				break;
			case 0x00 : 
				jpg_irq_reason = ERR_HD_PARSING; 
				break;
			case 0x40 : 
				jpg_irq_reason = OK_ENC_OR_DEC; 
				break;
			case 0x10 : 
				jpg_irq_reason = ERR_ENC_OR_DEC; 
				break;
			default : 
				jpg_irq_reason = ERR_UNKNOWN;
		}
		wake_up_interruptible(&WaitQueue_JPEG);
	}	
	else {
		jpg_irq_reason = ERR_UNKNOWN;
		wake_up_interruptible(&WaitQueue_JPEG);
	}

	return IRQ_HANDLED;
}

/*==============================================================================
 * - s3c_jpeg_open()
 *
 * - open jpeg device. alloc <s3c6400_jpg_ctx> attched to it
 */
int s3c_jpeg_open(struct inode *inode, struct file *file)
{
	s3c6400_jpg_ctx *JPGRegCtx;
	DWORD	ret;

	clk_enable(jpeg_hclk);
	clk_enable(jpeg_sclk);

	log_msg(LOG_TRACE, "s3c_jpeg_open", "JPG_open \r\n");

	JPGRegCtx = (s3c6400_jpg_ctx *)mem_alloc(sizeof(s3c6400_jpg_ctx));
	if (JPGRegCtx == NULL) {
		log_msg(LOG_ERROR, "s3c_jpeg_open", "DD::JPG mem alloc Fail\r\n");
		return -1;
	}
	
	memset(JPGRegCtx, 0x00, sizeof(s3c6400_jpg_ctx));

	ret = lock_jpg_mutex();
	if(!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_open", "DD::JPG Mutex Lock Fail\r\n");
		unlock_jpg_mutex();
		kfree(JPGRegCtx);
		return -1;
	}

	JPGRegCtx->v_pJPG_REG = JPGMem.v_pJPG_REG;
	JPGRegCtx->v_pJPGData_Buff = JPGMem.v_pJPGData_Buff;

	if (instanceNo >= MAX_INSTANCE_NUM){
		log_msg(LOG_ERROR, "s3c_jpeg_open", "DD::Instance Number error-JPEG is running, instance number is %d\n", instanceNo);
		unlock_jpg_mutex();
		kfree(JPGRegCtx);
		return -1;
	}

	instanceNo++;

	unlock_jpg_mutex();

	file->private_data = (s3c6400_jpg_ctx *)JPGRegCtx;


	return 0;
}

/*==============================================================================
 * - s3c_jpeg_release()
 *
 * - close jpeg device. free it's <s3c6400_jpg_ctx>
 */
int s3c_jpeg_release(struct inode *inode, struct file *file)
{
	DWORD			ret;
	s3c6400_jpg_ctx	*JPGRegCtx;

	log_msg(LOG_TRACE, "s3c_jpeg_release", "JPG_Close\n");

	JPGRegCtx = (s3c6400_jpg_ctx *)file->private_data;
	if(!JPGRegCtx){
		log_msg(LOG_ERROR, "s3c_jpeg_release", "DD::JPG Invalid Input Handle\r\n");
		return -1;
	}

	ret = lock_jpg_mutex();
	if(!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_release", "DD::JPG Mutex Lock Fail\r\n");
		return -1;
	}

	if((--instanceNo) < 0)
		instanceNo = 0;
	kfree(JPGRegCtx);
	
	unlock_jpg_mutex();

	clk_disable(jpeg_hclk);
	clk_disable(jpeg_sclk);

	return 0;
}

/*==============================================================================
 * - s3c_jpeg_ioctl()
 *
 * - ioctl to a jpeg device.
 */
int s3c_jpeg_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	static s3c6400_jpg_ctx		*JPGRegCtx;
	JPG_DEC_PROC_PARAM	DecReturn;
	JPG_ENC_PROC_PARAM	EncParam;
	BOOL				result = TRUE;
	DWORD				ret;
	int out;
	

	JPGRegCtx = (s3c6400_jpg_ctx *)file->private_data;
	if(!JPGRegCtx){
		log_msg(LOG_ERROR, "s3c_jpeg_ioctl", "DD::JPG Invalid Input Handle\r\n");
		return -1;
	}

	ret = lock_jpg_mutex();
	if(!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_ioctl", "DD::JPG Mutex Lock Fail\r\n");
		return -1;
	}

	switch (cmd) 
	{
		case IOCTL_JPG_DECODE:
			
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPEG_DECODE\n");

			out = copy_from_user(&DecReturn, (JPG_DEC_PROC_PARAM *)arg, sizeof(JPG_DEC_PROC_PARAM));
			result = decode_jpg(JPGRegCtx, &DecReturn);

			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "width : %d hegiht : %d size : %d\n", 
					DecReturn.width, DecReturn.height, DecReturn.dataSize);

			out = copy_to_user((void *)arg, (void *)&DecReturn, sizeof(JPG_DEC_PROC_PARAM));
			break;

		case IOCTL_JPG_ENCODE:
		
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPEG_ENCODE\n");

			out = copy_from_user(&EncParam, (JPG_ENC_PROC_PARAM *)arg, sizeof(JPG_ENC_PROC_PARAM));

			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "width : %d hegiht : %d\n", 
					EncParam.width, EncParam.height);

			result = encode_jpg(JPGRegCtx, &EncParam);

			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "encoded file size : %d\n", EncParam.fileSize);

			out = copy_to_user((void *)arg, (void *)&EncParam,  sizeof(JPG_ENC_PROC_PARAM));

			break;

		case IOCTL_JPG_GET_STRBUF:
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPG_GET_STRBUF\n");
			unlock_jpg_mutex();
			return arg;      

		case IOCTL_JPG_GET_THUMB_STRBUF:
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPG_GET_THUMB_STRBUF\n");
			unlock_jpg_mutex();
			return arg + JPG_STREAM_BUF_SIZE;

		case IOCTL_JPG_GET_FRMBUF:
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPG_GET_FRMBUF\n");
			unlock_jpg_mutex();
			return arg + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE;

		case IOCTL_JPG_GET_THUMB_FRMBUF:
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPG_GET_THUMB_FRMBUF\n");
			unlock_jpg_mutex();
			return arg + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE;

		case IOCTL_JPG_GET_PHY_FRMBUF:
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPG_GET_PHY_FRMBUF\n");
			unlock_jpg_mutex();
			return jpg_data_base_addr + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE;

		case IOCTL_JPG_GET_PHY_THUMB_FRMBUF:
			log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "IOCTL_JPG_GET_PHY_THUMB_FRMBUF\n");
			unlock_jpg_mutex();
			return jpg_data_base_addr + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE;

        case IOCTL_JPG_MMAP:
            *(unsigned char **)arg = (unsigned char *)jpg_data_base_addr;  /*  返回 JPEG 编解码器的        */
            break;                                                          /*  数据缓冲的基地址            */

		default : 
			log_msg(LOG_ERROR, "s3c_jpeg_ioctl", "DD::JPG Invalid ioctl : 0x%X\r\n", cmd);
	}

	unlock_jpg_mutex();

	return result;
}

/*==============================================================================
 * - s3c_jpeg_clock_setup()
 *
 * - helper routine. setup jpeg clock rate
 */
static BOOL s3c_jpeg_clock_setup(void)
{
	unsigned int	jpg_clk;
	
	// JPEG clock was set as 66 MHz
	jpg_clk = readl(S3C_CLK_DIV0);
	jpg_clk = (jpg_clk & ~(0xF << 24)) | (3 << 24);
	__raw_writel(jpg_clk, S3C_CLK_DIV0);

	return TRUE;

}

/*==============================================================================
 * - s3c_jpeg_probe()
 *
 * - init jpeg hardware, and init some global variables
 */
static int s3c_jpeg_probe(struct platform_device *pdev)
{
	static int		ret;
	HANDLE 			h_Mutex;


	// JPEG clock enable 
	jpeg_hclk	= clk_get(&pdev->dev, "hclk_jpeg");
	if (!jpeg_hclk || IS_ERR(jpeg_hclk)) {
		printk(KERN_ERR "failed to get jpeg hclk source\n");
		return -ENOENT;
	}
	clk_enable(jpeg_hclk);

	jpeg_sclk	= clk_get(&pdev->dev, "sclk_jpeg");
	if (!jpeg_sclk || IS_ERR(jpeg_sclk)) {
		printk(KERN_ERR "failed to get jpeg scllk source\n");
		return -ENOENT;
	}
	clk_enable(jpeg_sclk);

    // JPEG waitquene init
	init_waitqueue_head(&WaitQueue_JPEG);

    // JPEG irq enable
	ret = request_irq(irq_no, (irq_handler_t) s3c_jpeg_irq, 0, pdev->name, pdev);
	if (ret != 0) {
		printk(KERN_INFO "failed to install irq (%d)\n", ret);
		return ret;
	}

    // JPEG registers map
	jpeg_base = ioremap(JPG_REG_BASE_ADDR, 2 * 4096);
	if (jpeg_base == 0) {
		printk(KERN_INFO "failed to ioremap() region\n");
		return -EINVAL;
	}

	// JPEG clock was set as 66 MHz
	if (s3c_jpeg_clock_setup() == FALSE)
		return -ENODEV;
	
	log_msg(LOG_TRACE, "s3c_jpeg_probe", "JPG_Init\n");

	// Mutex initialization
	h_Mutex = create_jpg_mutex();
	if (h_Mutex == NULL) 
	{
		log_msg(LOG_ERROR, "s3c_jpeg_probe", "DD::JPG Mutex Initialize error\r\n");
		return -1;
	}

	ret = lock_jpg_mutex();
	if (!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_probe", "DD::JPG Mutex Lock Fail\n");
		return -1;
	}

	// Memory initialization
	if( !jpeg_mem_mapping(&JPGMem) ){ /* get register address */
		log_msg(LOG_ERROR, "s3c_jpeg_probe", "DD::JPEG-HOST-MEMORY Initialize error\r\n");
		unlock_jpg_mutex();
		return -1;
	}
	else {
		if (!jpg_buff_mapping(&JPGMem)){ /* get data buffer address */
			log_msg(LOG_ERROR, "s3c_jpeg_probe", "DD::JPEG-DATA-MEMORY Initialize error : %d\n");
			unlock_jpg_mutex();
			return -1;	
		}
	}

	instanceNo = 0;

	unlock_jpg_mutex();

	/* ret = misc_register(&s3c_jpeg_miscdev); */ /* DfewOS */

	clk_disable(jpeg_hclk);
	clk_disable(jpeg_sclk);

	return TRUE;
}

/*==============================================================================
 * - s3c_jpeg_remove()
 *
 * - opposite to <s3c_jpeg_probe>
 */
int s3c_jpeg_remove(struct platform_device *dev)
{
	free_irq(irq_no, dev);

	return 0;
}


static struct platform_device s3c_jpeg_device = {
    .name		= "s3c-jpeg",
};

/*==============================================================================
 * - s3c_jpeg_init()
 *
 * - jpeg device init
 */
int __init s3c_jpeg_init(void)
{
    static int initialized= 0;
    
    if (initialized) {
    	return 0;
    }
    initialized = 1;

	return s3c_jpeg_probe(&s3c_jpeg_device);
}

/*==============================================================================
 * - s3c_jpeg_exit()
 *
 * - jpeg device deinit
 */
void __exit s3c_jpeg_exit(void)
{
	DWORD	ret;

	log_msg(LOG_TRACE, "s3c_jpeg_exit", "JPG_Deinit\n");

	ret = lock_jpg_mutex();
	if(!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_exit", "DD::JPG Mutex Lock Fail\r\n");
	}

	jpg_buff_free(&JPGMem);
	jpg_mem_free(&JPGMem);
	unlock_jpg_mutex();

	delete_jpg_mutex();	

	printk("S3C JPEG driver module exit\n");
}



int s3c_jpeg_direct_encode(JPG_ENC_PROC_PARAM *EncParam)
{
	BOOL result = TRUE;
	DWORD ret;
    s3c6400_jpg_ctx *g_JPGRegCtx;

	clk_enable(jpeg_hclk);
	clk_enable(jpeg_sclk);

	log_msg(LOG_TRACE, "s3c_jpeg_open", "JPG_open \r\n");

	g_JPGRegCtx = (s3c6400_jpg_ctx *)mem_alloc(sizeof(s3c6400_jpg_ctx));
	memset(g_JPGRegCtx, 0x00, sizeof(s3c6400_jpg_ctx));

	ret = lock_jpg_mutex();
	if(!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_open", "DD::JPG Mutex Lock Fail\r\n");
		unlock_jpg_mutex();
		return -1;
	}

	g_JPGRegCtx->v_pJPG_REG = JPGMem.v_pJPG_REG;
	g_JPGRegCtx->v_pJPGData_Buff = JPGMem.v_pJPGData_Buff;

	if (instanceNo > MAX_INSTANCE_NUM){
		log_msg(LOG_ERROR, "s3c_jpeg_open", "DD::Instance Number error-JPEG is running, instance number is %d\n", instanceNo);
		unlock_jpg_mutex();
		return -1;
	}

	instanceNo++;

	unlock_jpg_mutex();

	log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "width : %d hegiht : %d\n", 
			EncParam->width, EncParam->height);

	result = encode_jpg(g_JPGRegCtx, EncParam);

	log_msg(LOG_TRACE, "s3c_jpeg_ioctl", "encoded file size : %d\n", EncParam->fileSize);

	ret = lock_jpg_mutex();
	if(!ret){
		log_msg(LOG_ERROR, "s3c_jpeg_release", "DD::JPG Mutex Lock Fail\r\n");
		return -1;
	}

	instanceNo = 0;

	unlock_jpg_mutex();

	kfree(g_JPGRegCtx);

	clk_disable(jpeg_hclk);
	clk_disable(jpeg_sclk);

	return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

