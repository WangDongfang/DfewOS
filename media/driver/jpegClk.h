/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               jpg_clk.h
** Last modified Date:      2012-04-24
** Last Version:            0.01
** Descriptions:            JPEG ���������ʱ������
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
#ifndef JPG_CLK_H_
#define JPG_CLK_H_
/*********************************************************************************************************
** Function name:           s3c_jpeg_sclk_enable
** Descriptions:            ʹ�� JPEG ��������� SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void s3c_jpeg_sclk_enable (void);
/*********************************************************************************************************
** Function name:           s3c_jpeg_sclk_disable
** Descriptions:            ���� JPEG ��������� SCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void s3c_jpeg_sclk_disable (void);
/*********************************************************************************************************
** Function name:           s3c_jpeg_hclk_enable
** Descriptions:            ʹ�� JPEG ��������� HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void s3c_jpeg_hclk_enable (void);
/*********************************************************************************************************
** Function name:           s3c_jpeg_hclk_disable
** Descriptions:            ���� JPEG ��������� HCLK
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void s3c_jpeg_hclk_disable (void);

#endif /* JPG_CLK_H_ */
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
