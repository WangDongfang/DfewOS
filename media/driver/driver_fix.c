/*==============================================================================
** driver_fix.c -- These Media Drviers Porting from Linux-2.6.28-samsung.
**                 This file support some functions those need by the driver.
**
** MODIFY HISTORY:
**
** 2012-03-22 wdf Create.
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
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#include <asm/cacheflush.h>
#include <asm/memory.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <plat/regs-clock.h>
#include <plat/regs-mfc.h>
#include <plat/map.h>
#include <plat/media.h>

#include "s3c_6410_config.h"
#include "mfcClk.h"
#include "ppClk.h"
#include "jpegClk.h"

#include <dfewos.h>
#include "../../bsp/int.h"

/*********************************************************************************************************
  结构体定义
*********************************************************************************************************/
struct clk {
    const char  *name;
    void       (*enable)(void);
    void       (*disable)(void);
};

/*********************************************************************************************************
  定义全局变量
*********************************************************************************************************/
static const struct clk __GclkThis[] = {
    {"hclk_mfc", s3c_mfc_hclk_enable, s3c_mfc_hclk_disable},
    {"sclk_mfc", s3c_mfc_sclk_enable, s3c_mfc_sclk_disable},
    {"pclk_mfc", s3c_mfc_pclk_enable, s3c_mfc_pclk_disable},
    {"post",     s3c_pp_clk_enable,     s3c_pp_clk_disable},
    {"hclk_jpeg", s3c_jpeg_hclk_enable, s3c_jpeg_hclk_disable},
    {"sclk_jpeg", s3c_jpeg_sclk_enable, s3c_jpeg_sclk_disable},
    {NULL}
};

/*********************************************************************************************************
** Function name:           clk_get
** Descriptions:            ilookup and obtain a reference to a clock producer.
** input parameters:        dev: device for clock "consumer"
**                          id: clock comsumer ID
** output parameters:       none
** Returned value:          Returns a struct clk corresponding to the clock producer, or
**                          valid IS_ERR() condition containing errno.  The implementation
**                          uses @dev and @id to determine the clock consumer, and thereby
**                          the clock producer.  (IOW, @id may be identical strings, but
**                          clk_get may return different clock producers depending on @dev.)
**                          Drivers must assume that the clock source is not enabled.
**                          clk_get should not be called from within interrupt context.
*********************************************************************************************************/
struct clk *clk_get (struct device *dev, const char *id)
{
    struct clk *pclkRt = (struct clk *)__GclkThis;

    while (pclkRt->name != NULL) {
        if (strcmp(pclkRt->name, id) == 0) {
            return pclkRt;
        }
        pclkRt++;
    }
    return NULL;
}

/*********************************************************************************************************
** Function name:           clk_enable
** Descriptions:            inform the system when the clock source should be running.
** input parameters:        clk: clock source
** output parameters:       none
** Returned value:          If the clock can not be enabled/disabled, this should return success.
**                          Returns success (0) or negative errno.
*********************************************************************************************************/
int clk_enable (struct clk *clk)
{
    if (clk == NULL) {
        return -EINVAL;
    }
    clk->enable();
    return 0;
}

/*********************************************************************************************************
** Function name:           clk_disable
** Descriptions:            inform the system when the clock source is no longer required.
** input parameters:        clk: clock source
** output parameters:       none
** Returned value:          Inform the system that a clock source is no longer required by
**                          a driver and may be shut down.
**                          Implementation detail: if the clock source is shared between
**                          multiple drivers, clk_enable() calls must be balanced by the
**                          same number of clk_disable() calls for the clock source to be
**                          disabled.
*********************************************************************************************************/
void clk_disable (struct clk *clk)
{
    clk->disable();
}


/*********************************************************************************************************
** Function name:           clk_get_rate
** Descriptions:            obtain the current clock rate (in Hz) for a clock source.
**                          This is only valid once the clock source has been enabled.
** input parameters:        clk: clock source
** output parameters:       none
** Returned value:          Inform the system that a clock source is no longer required by
**                          a driver and may be shut down.
**                          Implementation detail: if the clock source is shared between
**                          multiple drivers, clk_enable() calls must be balanced by the
**                          same number of clk_disable() calls for the clock source to be
**                          disabled.
*********************************************************************************************************/
unsigned long clk_get_rate(struct clk *clk)
{
    if (strcmp(clk->name, "post") == 0) {
        return 33000000;
    }
    return 0;
}

/*********************************************************************************************************
** Function name:           msleep
** Descriptions:            sleep safely even with waitqueue interruptions
** input parameters:        msecs: Time in milliseconds to sleep for
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void msleep(unsigned int msecs)
{
    msecs = msecs * SYS_CLK_RATE / 1000ul;
    msecs == 0 ? delayQ_delay (1) : delayQ_delay (msecs);
}

/*********************************************************************************************************
** Function name:           udelay
** Descriptions:            Use only for very small delays ( < 1 msec).  Should probably use a lookup
**                          table, really, as the multiplications take much too long with short delays.
**                          This is a "reasonable" implementation, though (and the first constant
**                          multiplications gets optimized away if the delay is a constant)
** input parameters:        usecs: Time in microsecond  to delay for
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void udelay (unsigned long usecs)
{
    volatile int i = usecs;
    volatile int j;

    while (i--) {
        j = 1000;
        while (j--)
            ;
    }
}

/*********************************************************************************************************
** Function name:           request_irq
** Descriptions:            allocate an interrupt line
** input parameters:        irq: Interrupt line to allocate
**                          handler: Function to be called when the IRQ occurs
**                          irqflags: Interrupt type flags
**                          devname: An ascii name for the claiming device
**                          dev_id: A cookie passed back to the handler function
** output parameters:       none
** Returned value:          This call allocates interrupt resources and enables the interrupt line and
**                          IRQ handling. From the point this call is made your handler function may be
**                          invoked. Since your handler function must clear any interrupt the board
**                          raises, you must take care both to initialise your hardware and to set up
**                          the interrupt handler in the right order. Dev_id must be globally unique.
**                          Normally the address of the device data structure is used as the cookie.
**                          Since the handler receives this value it makes sense to use it. If your
**                          interrupt is shared you must pass a non NULL dev_id as this is required when
**                          freeing the interrupt.
**                          Flags:
**                                IRQF_SHARED   Interrupt is shared
**                                IRQF_DISABLED Disable local interrupts while processing
**                                IRQF_SAMPLE_RANDOM The interrupt can be used for entropy IRQF_TRIGGER_*       Specify active edge(s) or level
*********************************************************************************************************/
int request_irq (unsigned int  irq,
                 irq_handler_t handler,
                 unsigned long irqflags,
                 const char   *devname,
                 void         *dev_id)
{
#if 0
    ((struct platform_device *)dev_id)->dev.handler = handler;
    ((struct platform_device *)dev_id)->dev.irqflags = irqflags;
#endif

    int_connect (irq, (FUNC_ISR)handler, (void *)irqflags);
    int_enable (irq);

    return 0;
}

/*********************************************************************************************************
** Function name:           free_irq
** Descriptions:            free an interrupt
** input parameters:        irq: Interrupt line to free
**                          dev_id: Device identity to free
** output parameters:       none
** Returned value:          Remove an interrupt handler. The handler is removed and if the interrupt
**                          line is no longer in use by any driver it is disabled. On a shared IRQ the
**                          caller must ensure the interrupt is disabled on the card it drives before
**                          calling this function. The function does not return until any executing
**                          interrupts for this IRQ have completed.
**                          This function must not be called from interrupt context.
*********************************************************************************************************/
void free_irq(unsigned int irq, void *dev_id)
{
#if 0
	FUNC_ISR routine;
    int      parameter;

    routine   = (FUNC_ISR)(((struct platform_device *)dev_id)->dev.handler);
    parameter = ((struct platform_device *)dev_id)->dev.irqflags;
#endif

    int_disable (irq);
    int_disconnect (irq);
}


/*********************************************************************************************************
  以下根据三星自己的代码稍微修改
*********************************************************************************************************/

#define SZ_1K                           0x00000400

static struct s3c_media_device s3c_mdevs[S3C_MDEV_MAX] = {
	{
		.id = S3C_MDEV_FIMC,
		.name = "fimc",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},

	{
		.id = S3C_MDEV_POST,
		.name = "pp",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_POST
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_POST * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},

	{
		.id = S3C_MDEV_TV,
		.name = "tv",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},

	{
		.id = S3C_MDEV_MFC,
		.name = "mfc",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},

	{
		.id = S3C_MDEV_JPEG,
		.name = "jpeg",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},

	{
		.id = S3C_MDEV_CMM,
		.name = "cmm",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_CMM
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_CMM * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},
	
	{
		.id = S3C_MDEV_RP,
		.name = "rp",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_RP
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_RP * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},
	
	{
		.id = S3C_MDEV_G3D,
		.name = "s3c-g3d",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_G3D
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_G3D * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	}
};

void *nocache_malloc (unsigned int size)
{
    static void *nocache_memory = (void *)0x56C00000; /* CONFIG_NOCACHE_START */

    /* grown up */
    nocache_memory += size;

    return (nocache_memory - size);
}

static struct s3c_media_device *s3c_get_media_device(int dev_id)
{
	struct s3c_media_device *mdev = NULL;
	int i, found;

	if (dev_id < 0 || dev_id >= S3C_MDEV_MAX)
		return NULL;

	i = 0;
	found = 0;
	while (!found && (i < S3C_MDEV_MAX)) {
		mdev = &s3c_mdevs[i];
		if (mdev->id == dev_id)
			found = 1;
		else
			i++;
	}

	if (!found) {
		mdev = NULL;
	} else {
        if (mdev->memsize != 0 && mdev->paddr == 0) {
            mdev->paddr = (dma_addr_t)nocache_malloc(mdev->memsize);
        }
    }

	return mdev;
}

dma_addr_t s3c_get_media_memory(int dev_id)
{
	struct s3c_media_device *mdev;

	mdev = s3c_get_media_device(dev_id);
	if (!mdev){
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	if (!mdev->paddr) {
		printk(KERN_ERR "no memory for %s\n", mdev->name);
		return 0;
	}

	return mdev->paddr;
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
