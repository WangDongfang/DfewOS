/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               jpg_clk.c
** Last modified Date:      2012-04-24
** Last Version:            0.01
** Descriptions:            JPEG ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
**
**--------------------------------------------------------------------------------------------------------
** Created by:              WangDongfang
** Created date:            2012-04-24
** Version:                 0.01
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
*********************************************************************************************************/
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

#include "s3c_clock.h"
#include "jpegClk.h"
/*********************************************************************************************************
** Function name:           s3c_jpeg_sclk_enable
** Descriptions:            Ê¹ÄÜ JPEG ±à½âÂëÆ÷µÄ SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2012-04-24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void s3c_jpeg_sclk_enable (void)
{
    s3c64xx_gate((UINT32)S3C_SCLK_GATE, S3C_CLKCON_SCLK_JPEG, TRUE);
}
/*********************************************************************************************************
** Function name:           s3c_jpeg_sclk_disable
** Descriptions:            ½ûÄÜ JPEG ±à½âÂëÆ÷µÄ SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2012-04-24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void s3c_jpeg_sclk_disable (void)
{
    s3c64xx_gate((UINT32)S3C_SCLK_GATE, S3C_CLKCON_SCLK_JPEG, FALSE);
}
/*********************************************************************************************************
** Function name:           s3c_jpeg_hclk_enable
** Descriptions:            Ê¹ÄÜ JPEG ±à½âÂëÆ÷µÄ HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2012-04-24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void s3c_jpeg_hclk_enable (void)
{
    s3c64xx_gate((UINT32)S3C_HCLK_GATE, S3C_CLKCON_HCLK_JPEG, TRUE);
}
/*********************************************************************************************************
** Function name:           s3c_jpeg_hclk_disable
** Descriptions:            ½ûÄÜ JPEG ±à½âÂëÆ÷µÄ HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2012-04-24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void s3c_jpeg_hclk_disable (void)
{
    s3c64xx_gate((UINT32)S3C_HCLK_GATE, S3C_CLKCON_HCLK_JPEG, FALSE);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

