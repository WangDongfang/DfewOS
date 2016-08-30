/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               mfcClk.h
** Last modified Date:      2010-10-14
** Last Version:            1.0.0
** Descriptions:            MFC ��ʱ������
**
**--------------------------------------------------------------------------------------------------------
** Created by:              jiaojinxing
** Created date:            2010-10-14
** Version:                 1.0.0
** Descriptions:            MFC ��ʱ������
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
** Descriptions:            ʹ�� MFC �� PCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_pclk_enable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_pclk_disable
** Descriptions:            ���� MFC �� PCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_pclk_disable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_sclk_enable
** Descriptions:            ʹ�� MFC �� SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_sclk_enable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_sclk_disable
** Descriptions:            ���� MFC �� SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_sclk_disable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_hclk_enable
** Descriptions:            ʹ�� MFC �� HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_hclk_enable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_hclk_disable
** Descriptions:            ���� MFC �� HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID s3c_mfc_hclk_disable (VOID);
/*********************************************************************************************************
** Function name:           s3c_mfc_setup_clock
** Descriptions:            ���� MFC ��ʱ��
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          TRUE
*********************************************************************************************************/
BOOL s3c_mfc_setup_clock (VOID);

#endif                                                                  /*  MFC_CLK_H_                  */
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
