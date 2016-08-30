/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mfcClk.c
** Last modified Date:      2010-10-14
** Last Version:            1.0.0
** Descriptions:            MFC 的时钟配置
**
**--------------------------------------------------------------------------------------------------------
** Created by:              jiaojinxing
** Created date:            2010-10-14
** Version:                 1.0.0
** Descriptions:            MFC 的时钟配置
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
#include "mfcClk.h"

/*********************************************************************************************************
** Function name:           s3c_mfc_pclk_enable
** Descriptions:            使能 MFC 的 PCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_mfc_pclk_enable (VOID)
{
    clkSrcGate((UINT32)S3C_PCLK_GATE, S3C_CLKCON_PCLK_MFC, TRUE);
}
/*********************************************************************************************************
** Function name:           s3c_mfc_pclk_disable
** Descriptions:            禁能 MFC 的 PCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_mfc_pclk_disable (VOID)
{
    clkSrcGate((UINT32)S3C_PCLK_GATE, S3C_CLKCON_PCLK_MFC, FALSE);
}
/*********************************************************************************************************
** Function name:           s3c_mfc_sclk_enable
** Descriptions:            使能 MFC 的 SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_mfc_sclk_enable (VOID)
{
    clkSrcGate((UINT32)S3C_SCLK_GATE, S3C_CLKCON_SCLK_MFC, TRUE);
}
/*********************************************************************************************************
** Function name:           s3c_mfc_sclk_disable
** Descriptions:            禁能 MFC 的 SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_mfc_sclk_disable (VOID)
{
    clkSrcGate((UINT32)S3C_SCLK_GATE, S3C_CLKCON_SCLK_MFC, FALSE);
}
/*********************************************************************************************************
** Function name:           s3c_mfc_hclk_enable
** Descriptions:            使能 MFC 的 HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_mfc_hclk_enable (VOID)
{
    clkSrcGate((UINT32)S3C_HCLK_GATE, S3C_CLKCON_HCLK_MFC, TRUE);
}
/*********************************************************************************************************
** Function name:           s3c_mfc_hclk_disable
** Descriptions:            禁能 MFC 的 HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
VOID s3c_mfc_hclk_disable (VOID)
{
    clkSrcGate((UINT32)S3C_HCLK_GATE, S3C_CLKCON_HCLK_MFC, FALSE);
}
/*********************************************************************************************************
** Function name:           s3c_mfc_setup_clock
** Descriptions:            设置 MFC 的时钟
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          TRUE
** Created by:              jiaojinxing
** Created Date:            2010-10-14
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
BOOL s3c_mfc_setup_clock (VOID)
{
    UINT32  uiRatio;

    /*
     * 公式1: MFC_CLK = MFC_CLKin / (MFC_RATIO + 1)
     *
     * 公式2: MFC_CLKin = HCLK x 2
     *
     * MFC_CLK 要为 133 MHz, 当 HCLK = 133 MHz 时, MFC_RATIO = 1
     */
#define MFC_RATIO       1

    uiRatio = readl(S3C_CLK_DIV0);

    uiRatio = (uiRatio & ~(0xF << 28)) | (MFC_RATIO << 28);

    writel(uiRatio, S3C_CLK_DIV0);

    return  (TRUE);
}

/*********************************************************************************************************
    END FILE
*********************************************************************************************************/

