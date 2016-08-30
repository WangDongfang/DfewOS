/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               s3c_clock.c
** Last modified Date:      2010-10-14
** Last Version:            1.0.0
** Descriptions:            S3C6410 的时钟配置
**
**--------------------------------------------------------------------------------------------------------
** Created by:              jiaojinxing
** Created date:            2010-10-14
** Version:                 1.0.0
** Descriptions:            S3C6410 的时钟配置
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
/*********************************************************************************************************
** Function name:           s3c64xx_gate
** Descriptions:            配置 S3C6410 的 S3C_HCLK_GATE 等寄存器
** input parameters:        uiReg               寄存器(S3C_HCLK_GATE, S3C_SCLK_GATE, S3C_PCLK_GATE)
**                          uiCtrlBit           控制位(S3C_CLKCON_XCLK_XXX, 见 "regs-clock.h" 文件)
**                          bEnable             是否使能
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c64xx_gate(UINT32  uiReg,
                  UINT32  uiCtrlBit,
                  BOOL    bEnable)
{
    UINT32  uiValue;

    uiValue = *(volatile UINT32 *)(uiReg);
    if (bEnable) {
        uiValue |=  uiCtrlBit;
    } else {
        uiValue &= ~uiCtrlBit;
    }
    *(volatile UINT32 *)(uiReg) = uiValue;
}
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
