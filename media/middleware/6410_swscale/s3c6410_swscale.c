/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info--------------------------------------------------------------------------------
** File name:           s3c6410_swscale.c
** Last modified Date:  2012-01-13
** Last Version:        V1.00
** Descriptions:        s3c6410的视频场景比例缩放、色彩映射转换驱动程序for ffmpeg
**------------------------------------------------------------------------------------------------------
** Created by:          chenmingji
** Created date:        2012-01-13
** Version:             V1.00
** Descriptions:        The original version
**
**------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
*********************************************************************************************************/

#include  "s3c6410_swscale.h"
#include  "s3c6410_swscale_cfg.h"
#undef malloc
#undef free

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

/*********************************************************************************************************
  允许调用标准的swscale
*********************************************************************************************************/
#undef  sws_getContext
#undef  sws_scale
#undef  sws_freeContext

/*********************************************************************************************************
  ffmpeg颜色空间与硬件支持的颜色空间对照表,不保证正确
*********************************************************************************************************/
struct sws_6410_color_space_table {
    enum PixelFormat    pfFFmpegColorSpace;
    s3c_color_space_t   pcHwColorSpace;
};

static const struct sws_6410_color_space_table __GcstTable[] = {
    {PIX_FMT_NONE, PAL1},
    {PIX_FMT_NONE, PAL2},
    {PIX_FMT_NONE, PAL4},
    {PIX_FMT_PAL8, PAL8},
    {PIX_FMT_RGB8, RGB8},
    {PIX_FMT_NONE, ARGB8},
    {PIX_FMT_RGB565LE, RGB16},
    {PIX_FMT_NONE, ARGB16},
    {PIX_FMT_NONE, RGB18},
    {PIX_FMT_RGB24, RGB24},
    {PIX_FMT_NONE, RGB30},
    {PIX_FMT_ARGB, ARGB24},
    {PIX_FMT_YUV420P, YC420},
    {PIX_FMT_YUYV422, YC422},
    {PIX_FMT_NONE, CRYCBY},
    {PIX_FMT_UYVY422, CBYCRY},
    {PIX_FMT_NONE, YCRYCB},
    {PIX_FMT_YUYV422, YCBYCR},
    {PIX_FMT_YUV444P,  YUV444}

};

/*********************************************************************************************************
  点尺寸, 0为未知，不保证正确
*********************************************************************************************************/
static const float __GfPixSize[] = {
    0.125,                                                              /*  PAL1,                       */
    0.25,                                                               /*  PAL2,                       */
    0.5,                                                                /*  PAL4,                       */
    1,                                                                  /*  PAL8,                       */
    1,                                                                  /*  RGB8,                       */
    2,                                                                  /*  ARGB8,                      */
    2,                                                                  /*  RGB16,                      */
    3,                                                                  /*  ARGB16,                     */
    3,                                                                  /*  RGB18,                      */
    3,                                                                  /*  RGB24,                      */
    4,                                                                  /*  RGB30,                      */
    4,                                                                  /*  ARGB24,                     */
    1.5,                                                                /*  YC420,                      */
    2,                                                                  /*  YC422,                      */
    0,                                                                  /*  CRYCBY,                     */
    2,                                                                  /*  CBYCRY,                     */
    0,                                                                  /*  YCRYCB,                     */
    2,                                                                  /*  YCBYCR,                     */
    3                                                                   /*  YUV444                      */
};

/*********************************************************************************************************
  内部的sws信息
*********************************************************************************************************/
typedef struct sws_6410_context {
    void               *av_class;                                       /*  标志,与struct SwsContext兼容*/
    struct file         fPost;
    s3c_pp_params_t     ppPostParam;                                    /*  PostProcessor work parameter*/
    int                 piDstStride[1];
    int                 iLcdWith;
    int                 iLcdHeight;
    double              height_ratio;
} sws_6410_context;

/*********************************************************************************************************
  后端处理器接口函数
*********************************************************************************************************/
int s3c_pp_open(struct inode *inode, struct file *file);
int s3c_pp_release(struct inode *inode, struct file *file);
int s3c_pp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
int s3c_pp_init(void);

/*********************************************************************************************************
** Function name:           sws_6410_getContext
** Descriptions:            Allocates and returns a SwsContext. You need it to perform scaling/conversion
**                          operations using sws_scale().
**                          @note this function is to be removed after a saner alternative is written
**                          @deprecated Use sws_getCachedContext() instead.
** input parameters:        iSrcWith:      the width of the source image
**                          iSrcHeight:    the height of the source image
**                          pfSrcFormat:   the source image format
**                          iDstWith:      the width of the destination image
**                          iDstHeight:    the height of the destination image
**                          pfDstFormat:   the destination image format
**                          iFlags:        specify which algorithm and options to use for rescaling
** output parameters:       none
** Returned value:          a pointer to an allocated context, or NULL in case of error
*********************************************************************************************************/
struct SwsContext *sws_6410_getContext (int iSrcWith, int iSrcHeight, enum PixelFormat pfSrcFormat,
                                        int iDstWith, int iDstHeight, enum PixelFormat pfDstFormat,
                                        int iFlags, SwsFilter *psfSrcFilter,
                                        SwsFilter *psfDstFilter, const double *pdParam)
{
    int               i;
    int               color_table_elements = (sizeof(__GcstTable) / sizeof(__GcstTable[0]));
    sws_6410_context *pscThis   = NULL;
    s3c_color_space_t csSrcColorSpace;
    s3c_color_space_t csDstColorSpace;

    /*
     *  查看是否在支持列表中
     */
    for (i = 0; i < color_table_elements; i++) {
        if (__GcstTable[i].pfFFmpegColorSpace == pfSrcFormat) {
            csSrcColorSpace = __GcstTable[i].pcHwColorSpace;
            break;
        }
    }
    if (i >= color_table_elements) {
        return sws_getContext(iSrcWith, iSrcHeight, pfSrcFormat, iDstWith, iDstHeight, pfDstFormat, 
                              iFlags, psfSrcFilter, psfDstFilter, pdParam);
    }
    for (i = 0; i < color_table_elements; i++) {
        if (__GcstTable[i].pfFFmpegColorSpace == pfDstFormat) {
            csDstColorSpace = __GcstTable[i].pcHwColorSpace;
            break;
        }
    }
    if (i >= color_table_elements) {
        return sws_getContext(iSrcWith, iSrcHeight, pfSrcFormat, iDstWith, iDstHeight, pfDstFormat, 
                              iFlags, psfSrcFilter, psfDstFilter, pdParam);
    }

    /*
     *  使用硬件后端处理
     */
    pscThis = malloc(sizeof(sws_6410_context));
    if (pscThis == NULL) {
        return sws_getContext(iSrcWith, iSrcHeight, pfSrcFormat, iDstWith, iDstHeight, pfDstFormat, 
                              iFlags, psfSrcFilter, psfDstFilter, pdParam);
    }
    memset(pscThis, 0, sizeof(sws_6410_context));

    s3c_pp_init();                                                      /*  初始化后端处理器            */

    /*
     *  打开pp设备
     */
    if (s3c_pp_open(NULL, &(pscThis->fPost)) != 0) {
        free(pscThis);
        return sws_getContext(iSrcWith, iSrcHeight, pfSrcFormat, iDstWith, iDstHeight, pfDstFormat, 
                              iFlags, psfSrcFilter, psfDstFilter, pdParam);
    }
    pscThis->av_class = sws_6410_getContext;
    
    /*
     *  保存参数
     */
    pscThis->ppPostParam.src_full_width  = iSrcWith;
    pscThis->ppPostParam.src_full_height = iSrcHeight;
    pscThis->ppPostParam.src_start_x     = 0;
    pscThis->ppPostParam.src_start_y     = 0;
    pscThis->ppPostParam.src_width       = iSrcWith;
    pscThis->ppPostParam.src_height      = iSrcHeight;
    pscThis->ppPostParam.src_color_space = csSrcColorSpace;
    
    if (iDstWith > __SWSCALE_LCD_WIDTH) {
        iDstWith = __SWSCALE_LCD_WIDTH;
    }
    if (iDstHeight > __SWSCALE_LCD_HEIGHT) {
        iDstHeight = __SWSCALE_LCD_HEIGHT;
    }
    pscThis->ppPostParam.dst_full_width  = __SWSCALE_LCD_WIDTH;
    pscThis->ppPostParam.dst_full_height = __SWSCALE_LCD_HEIGHT;
    pscThis->ppPostParam.dst_start_x     = 0;
    pscThis->ppPostParam.dst_start_y     = 0;
    pscThis->ppPostParam.dst_width       = iDstWith;
    pscThis->ppPostParam.dst_height      = iDstHeight;
    pscThis->ppPostParam.dst_color_space   = csDstColorSpace;

    pscThis->ppPostParam.out_path        = DMA_ONESHOT;
    pscThis->ppPostParam.scan_mode       = PROGRESSIVE_MODE;
    pscThis->piDstStride[0]              = 0;
    
    pscThis->height_ratio                = (double)iDstHeight / iSrcHeight;

    return (struct SwsContext *)pscThis;
}

/*********************************************************************************************************
** Function name:           sws_6410_scale
** Descriptions:            Scales the image slice in srcSlice and puts the resulting scaled slice in the 
**                          image in dst. A slice is a sequence of consecutive rows in an image.
**                          Slices have to be provided in sequential order, either in top-bottom or 
**                          bottom-top order. If slices are provided in non-sequential order the behavior 
**                          of the function is undefined.
** input parameters:        pscContext:  the scaling context previously created with sws_getContext()
**                          pucSrcSlice: the array containing the pointers to the planes of the source 
**                                       slice
**                          piSrcStride: the array containing the strides for each plane of the source 
**                                       image
**                          iSrcSliceY:  the position in the source image of the slice to process, that  
**                                       is the number (counted starting from zero) in the image of the
**                                       first row of the slice
**                          iSrcSliceH:  the height of the source slice, that is the number of rows in  
**                                       the slice
**                          piDstStride: the array containing the strides for each plane of the  
**                                       destination image
** output parameters:       ucDst: the array containing the pointers to the planes of the destination image
** Returned value:          the height of the output slice
*********************************************************************************************************/
int sws_6410_scale (struct SwsContext *pscContext, const uint8_t* const pucSrcSlice[], const int piSrcStride[],
                    int iSrcSliceY, int iSrcSliceH, uint8_t* const ucDst[], const int piDstStride[])
{
    sws_6410_context *pscThis = NULL;
    int               iParamChange;

    pscThis = (sws_6410_context *)pscContext;
    if (pscThis->av_class != (void *)sws_6410_getContext) {
        return sws_scale(pscContext, pucSrcSlice, piSrcStride, iSrcSliceY, iSrcSliceH, ucDst, piDstStride);
    }

    /*
     *  使用6410硬件的后端处理加速器
     */
    iParamChange = FALSE;
    if (pscThis->piDstStride[0] != piDstStride[0]) {
        pscThis->ppPostParam.dst_full_width = piDstStride[0] /
                                            __GfPixSize[pscThis->ppPostParam.dst_color_space];
        pscThis->piDstStride[0]  = piDstStride[0];
        iParamChange             = TRUE;

        /*
         *  为硬解码调整参数
         */
        if (pscThis->ppPostParam.src_full_width != piSrcStride[0]) {
            pscThis->ppPostParam.src_full_width  = piSrcStride[0];
        }
    }

    /* 更新视频帧裁剪参数 */
    if (pscThis->ppPostParam.src_start_y != iSrcSliceY || pscThis->ppPostParam.src_height != iSrcSliceH) {
        pscThis->ppPostParam.src_start_y  = iSrcSliceY; /* 从源的第 <iSrcSliceY> 行开始转换 */
        pscThis->ppPostParam.src_height   = iSrcSliceH; /* 需对源转换 <iSrcSliceH> 行 */
        pscThis->ppPostParam.dst_start_y += iSrcSliceY * pscThis->height_ratio;
        pscThis->ppPostParam.dst_height   = iSrcSliceH * pscThis->height_ratio;

        iParamChange                      = TRUE;
    }

    if (iParamChange == TRUE) {
        s3c_pp_ioctl(NULL, &(pscThis->fPost), S3C_PP_SET_PARAMS, (unsigned long)&(pscThis->ppPostParam));
    }

    pscThis->ppPostParam.src_buf_addr_phy[0] = (unsigned int)pucSrcSlice[0];
    pscThis->ppPostParam.src_buf_addr_phy[1] = (unsigned int)pucSrcSlice[1];
    pscThis->ppPostParam.src_buf_addr_phy[2] = (unsigned int)pucSrcSlice[2];
    pscThis->ppPostParam.src_buf_addr_phy[3] = (unsigned int)pucSrcSlice[3];


    s3c_pp_ioctl(NULL, &(pscThis->fPost), S3C_PP_SET_SRC_BUF_ADDR_PHY, 
                (unsigned long)&(pscThis->ppPostParam));

    pscThis->ppPostParam.dst_buf_addr_phy[0] = (unsigned int)ucDst[0];
    pscThis->ppPostParam.dst_buf_addr_phy[1] = (unsigned int)ucDst[1];
    pscThis->ppPostParam.dst_buf_addr_phy[2] = (unsigned int)ucDst[2];
    pscThis->ppPostParam.dst_buf_addr_phy[3] = (unsigned int)ucDst[3];
    s3c_pp_ioctl(NULL, &(pscThis->fPost), S3C_PP_SET_DST_BUF_ADDR_PHY, 
                (unsigned long)&(pscThis->ppPostParam));

    s3c_pp_ioctl(NULL, &(pscThis->fPost), S3C_PP_START, 0);
    return pscThis->ppPostParam.dst_height;
}

/*********************************************************************************************************
** Function name:           sws_6410_freeContext
** Descriptions:            Frees the swscaler context SwsContext. 
**                          If pscContext is NULL, then does nothing.
** input parameters:        pscContext:   the scaling context previously created with sws_getContext()
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void sws_6410_freeContext (struct SwsContext *pscContext)
{
    sws_6410_context *pscThis = NULL;

    pscThis = (sws_6410_context *)pscContext;
    if (pscThis->av_class != (void *)sws_6410_getContext) {
        return sws_freeContext(pscContext);
    }

    /*
     *  使用6410硬件的后端处理加速器
     */
    s3c_pp_release(NULL, &(pscThis->fPost));
    free(pscContext);
}

/*********************************************************************************************************
** Function name:           sws_6410_set_lcd
** Descriptions:            改变LCD屏幕大小

** input parameters:        context:   the scaling context previously created with sws_getContext()
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void sws_6410_set_lcd (struct SwsContext *pscContext, int iWidth, int iHeight)
{
    sws_6410_context *pscThis = NULL;

    pscThis = (sws_6410_context *)pscContext;
    if (pscThis->av_class != (void *)sws_6410_getContext) {
        return;
    }

    if ((pscThis->ppPostParam.dst_start_x + pscThis->ppPostParam.dst_width) > iWidth) {
        pscThis->ppPostParam.dst_start_x = 0;
    }
    if ((pscThis->ppPostParam.dst_start_y + pscThis->ppPostParam.dst_height) > iHeight) {
        pscThis->ppPostParam.dst_start_y = 0;
    }

    pscThis->iLcdWith   = iWidth;
    pscThis->iLcdHeight = iHeight;
}

/*********************************************************************************************************
** Function name:           sws_6410_set_xy
** Descriptions:            设置视频显示的位置(左上角坐标)

** input parameters:        context:   the scaling context previously created with sws_getContext()
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void sws_6410_set_des_xy (struct SwsContext *pscContext, int x, int y)
{
    sws_6410_context *pscThis = NULL;

    pscThis = (sws_6410_context *)pscContext;
    if (pscThis->av_class != (void *)sws_6410_getContext) {
        return;
    }
    
    if ((x + pscThis->ppPostParam.dst_width) > pscThis->ppPostParam.dst_full_width) {
        x = 0;
    }
    if ((y + pscThis->ppPostParam.dst_height) > pscThis->ppPostParam.dst_full_height) {
        y = 0;
    }
    pscThis->ppPostParam.dst_start_x = x;
    pscThis->ppPostParam.dst_start_y = y;
}


/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
