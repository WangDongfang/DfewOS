/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mfcClk.h
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
#ifndef MFC_CLK_H_
#define MFC_CLK_H_
/*********************************************************************************************************
** Function name:           s3c_mfc_pclk_enable
** Descriptions:            使能 MFC 的 PCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_pclk_enable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_pclk_disable
** Descriptions:            禁能 MFC 的 PCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_pclk_disable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_sclk_enable
** Descriptions:            使能 MFC 的 SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_sclk_enable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_sclk_disable
** Descriptions:            禁能 MFC 的 SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_sclk_disable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_hclk_enable
** Descriptions:            使能 MFC 的 HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_hclk_enable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_hclk_disable
** Descriptions:            禁能 MFC 的 HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_hclk_disable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_setup_clock
** Descriptions:            设置 MFC 的时钟
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          TRUE
*********************************************************************************************************/
BOOL s3c_mfc_setup_clock (VOID);

#endif                                                                  /*  MFC_CLK_H_                  */
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
