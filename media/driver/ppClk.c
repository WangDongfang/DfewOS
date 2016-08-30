/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               ppClk.c
** Last modified Date:      2011-4-22
** Last Version:            1.0.0
** Descriptions:            PostProcessor 的时钟配置
**
**--------------------------------------------------------------------------------------------------------
** Created by:              JiaoJinXing
** Created date:            2011-4-22
** Version:                 1.0.0
** Descriptions:            PostProcessor 的时钟配置
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
#include "ppClk.h"

/*********************************************************************************************************
** Function name:           s3c_pp_scaler_clk_src
** Descriptions:            设置 PostProcessor 的时钟源
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              JiaoJinXing
** Created Date:            2011-4-22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_pp_scaler_clk_src (VOID)
{
    UINT32  uiValue;

    uiValue  = __raw_readl(S3C_CLK_SRC);

    uiValue &= ~(0x3 << 28);
    uiValue |=  (0x0 << 28);

    __raw_writel(uiValue, S3C_CLK_SRC);
}
/*********************************************************************************************************
** Function name:           s3c_pp_clk_enable
** Descriptions:            使能 PostProcessor 的时钟源
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              JiaoJinXing
** Created Date:            2011-4-22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_pp_clk_enable (VOID)
{
    clkSrcGate((UINT32)S3C_HCLK_GATE, S3C_CLKCON_HCLK_POST0, TRUE);

    clkSrcGate((UINT32)S3C_SCLK_GATE, S3C_CLKCON_SCLK_POST0 | S3C_CLKCON_SCLK_POST0_27, TRUE);
}
/*********************************************************************************************************
** Function name:           s3c_pp_clk_disable
** Descriptions:            禁能 PostProcessor 的时钟源
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              JiaoJinXing
** Created Date:            2011-4-22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_pp_clk_disable (VOID)
{
    clkSrcGate((UINT32)S3C_HCLK_GATE, S3C_CLKCON_HCLK_POST0, FALSE);

    clkSrcGate((UINT32)S3C_SCLK_GATE, S3C_CLKCON_SCLK_POST0 | S3C_CLKCON_SCLK_POST0_27, FALSE);
}
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
