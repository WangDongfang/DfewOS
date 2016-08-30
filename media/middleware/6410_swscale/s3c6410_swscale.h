/****************************************Copyright (c)****************************************************
**                         Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                               http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           s3c6410_swscale.h
** Last modified Date:  2012-01-13
** Last Version:        1.0
** Descriptions:        s3c6410的视频场景比例缩放、色彩映射转换驱动程序for ffmpeg
**--------------------------------------------------------------------------------------------------------
** Created by:          Chenmingji
** Created date:        2012-01-13
** Version:             1.0
** Descriptions:        The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
*********************************************************************************************************/
#ifndef __S3C6410_SWSCALE_H
#define __S3C6410_SWSCALE_H

#include <dfewos.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  用于配置代码的宏
*********************************************************************************************************/
#define SWS_S3C6410_SWSCALE

/*********************************************************************************************************
  覆盖标准的swscale
*********************************************************************************************************/
#undef  sws_getContext
#define sws_getContext      sws_6410_getContext
#undef  sws_scale
#define sws_scale           sws_6410_scale
#undef  sws_freeContext
#define sws_freeContext     sws_6410_freeContext

/*********************************************************************************************************
** Function name:           sws_6410_getContext
** Descriptions:            Allocates and returns a SwsContext. You need it to perform scaling/conversion
**                          operations using sws_scale().
**                          @note this function is to be removed after a saner alternative is written
**                          @deprecated Use sws_getCachedContext() instead.
** input parameters:        srcW:      the width of the source image
**                          srcH:      the height of the source image
**                          srcFormat: the source image format
**                          dstW:      the width of the destination image
**                          dstH:      the height of the destination image
**                          dstFormat: the destination image format
**                          flags:     specify which algorithm and options to use for rescaling
** output parameters:       none
** Returned value:          a pointer to an allocated context, or NULL in case of error
*********************************************************************************************************/
struct SwsContext *sws_6410_getContext(int srcW, int srcH, enum PixelFormat srcFormat,
                                       int dstW, int dstH, enum PixelFormat dstFormat,
                                       int flags, SwsFilter *srcFilter,
                                       SwsFilter *dstFilter, const double *param);

/*********************************************************************************************************
** Function name:           sws_6410_scale
** Descriptions:            Scales the image slice in srcSlice and puts the resulting scaled slice in the 
**                          image in dst. A slice is a sequence of consecutive rows in an image.
**                          Slices have to be provided in sequential order, either in top-bottom or 
**                          bottom-top order. If slices are provided in non-sequential order the behavior 
**                          of the function is undefined.
** input parameters:        context:   the scaling context previously created with sws_getContext()
**                          srcSlice:  the array containing the pointers to the planes of the source slice
**                          srcStride: the array containing the strides for each plane of the source image
**                          srcSliceY: the position in the source image of the slice to process, that is 
**                                     the number (counted starting from
**                          srcSliceH: the height of the source slice, that is the number of rows in the 
**                                     slice
**                          dstStride: the array containing the strides for each plane of the destination 
**                                     image
** output parameters:       dst: the array containing the pointers to the planes of the destination image
** Returned value:          the height of the output slice
*********************************************************************************************************/
int sws_6410_scale(struct SwsContext *context, const uint8_t* const srcSlice[], const int srcStride[],
                   int srcSliceY, int srcSliceH, uint8_t* const dst[], const int dstStride[]);

/*********************************************************************************************************
** Function name:           sws_6410_freeContext
** Descriptions:            Frees the swscaler context swsContext. 
**                          If swsContext is NULL, then does nothing.
** input parameters:        context:   the scaling context previously created with sws_getContext()
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void sws_6410_freeContext(struct SwsContext *swsContext);

/*********************************************************************************************************
** Function name:           sws_6410_set_lcd
** Descriptions:            设置液晶的宽和高

** input parameters:        context:   the scaling context previously created with sws_getContext()
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void sws_6410_set_lcd(struct SwsContext *pscContext, int iWidth, int iHeight);

/*********************************************************************************************************
** Function name:           sws_6410_set_lcd
** Descriptions:            设置液晶的宽和高

** input parameters:        context:   the scaling context previously created with sws_getContext()
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void sws_6410_set_des_xy(struct SwsContext *pscContext, int x, int y);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __S3C6410_SWSCALE_H         */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
