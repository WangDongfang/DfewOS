/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               s3c6410_mpeg4_hwaccel.c
** Last modified Date:      2012-2-2
** Last Version:            1.0.0
** Descriptions:            MPEG4 Ӳ��������
**
**--------------------------------------------------------------------------------------------------------
** Created by:              JiaoJinXing
** Created date:            2011-3-25
** Version:                 1.0.0
** Descriptions:            MPEG4 Ӳ��������
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             chenmingji
** Modified date:           2012.01.13
** Version:                 1.0.0
** Descriptions:            ��ֲ��VxWorks 6.8 DKM
**
** Modified by:             chenmingji
** Modified date:           2012.02.02
** Version:                 1.0.0
** Descriptions:            �򻯴���,�����ڴ�ռ��,�������ݿ���,�������
**
*********************************************************************************************************/
#include <libavutil/intmath.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/mpeg4video.h>

#include "../../driver/src/mfc10/s3c_mfc.h"
#include "../../driver/src/mfc10/s3c_mfc_params.h"

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

/*********************************************************************************************************
    �����ⲿ�ı����ͺ���
*********************************************************************************************************/
extern int s3c_mfc_open(struct inode *inode, struct file *file);
extern int s3c_mfc_release(struct inode *inode, struct file *file);
extern int s3c_mfc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
extern u32 s3c_mfc_mmap(struct file *filp);
extern int s3c_mfc_init(void);

/*********************************************************************************************************
    MPEG4 Ӳ��������������
*********************************************************************************************************/
typedef struct {
    struct file         fFd;                                            /*  MFC �豸�ļ�������          */
    uint8              *pucMmapAddr;                                    /*  �ڴ�ӳ��ĵ�ַ              */

    uint8              *pucStreamBuffer;                                /*  ��Ƶ��������                */
    uint8              *pucStreamIn;                                    /*  ��Ƶ����д���              */

    BOOL                bConfig;                                        /*  �Ƿ��Ѿ����� MFC            */

    uint32                uiWidth;                                        /*  ��Ƶ���                    */
    uint32                uiHeight;                                       /*  ��Ƶ�߶�                    */
} HWAccelContext;
/*********************************************************************************************************
** Function name:           MPEG4_StartFrame
** Descriptions:            ��ʼ�µ�һ֡ʱ�ĵ���
** input parameters:        pCodecCtx           ������������
**                          pucFrame            ֡
**                          uiFrameSize         ֡�Ĵ�С
** output parameters:       NONE
** Returned value:          ERROR_CODE
** Created by:              JiaoJinXing
** Created Date:            2011-3-25
**--------------------------------------------------------------------------------------------------------
** Modified by:             chenmingji
** Modified date:           2012.01.12
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int MPEG4_StartFrame(AVCodecContext  *pCodecCtx,
                            const uint8_t   *pucFrame,
                            uint32_t         uiFrameSize)
{
    HWAccelContext  *pHWAccelCtx;
    s3c_mfc_args_t   MFCParam;
    int              iError;

    /*
     *  �鿴�Ƿ�֧��Ӳ�������Ƶ
     */
    if (pCodecCtx->width > 720 || pCodecCtx->height > 480) {
        return -1;
    }

    pHWAccelCtx = pCodecCtx->hwaccel_context;
    if (pHWAccelCtx == NULL) {                                          /*  ���δ����Ӳ��������������  */
        /*
         * ����Ӳ��������������
         */
        pHWAccelCtx = (HWAccelContext *)av_malloc(sizeof(HWAccelContext));
        if (pHWAccelCtx == NULL) {
            pCodecCtx->hwaccel = NULL;
            return -1;
        }
        pCodecCtx->hwaccel_context = pHWAccelCtx;
        memset(pHWAccelCtx, 0, sizeof(HWAccelContext));

        s3c_mfc_init();
        /*
         * �� MFC �豸
         */
        iError = s3c_mfc_open(NULL, &(pHWAccelCtx->fFd));
        if (iError < 0) {
            av_free(pHWAccelCtx);
            pCodecCtx->hwaccel_context = NULL;
            pCodecCtx->hwaccel         = NULL;
            return -1;
        }

        /*
         * �ڴ�ӳ�� MFC �豸
         */
        pHWAccelCtx->pucMmapAddr = (uint8 *)s3c_mfc_mmap(&(pHWAccelCtx->fFd));
        if (pHWAccelCtx->pucMmapAddr == NULL) {
            s3c_mfc_release(NULL, &(pHWAccelCtx->fFd));
            av_free(pHWAccelCtx);
            pCodecCtx->hwaccel_context = NULL;
            pCodecCtx->hwaccel         = NULL;
            return -1;
        }

        /*
         * �����Ƶ���������Ļ�ַ
         */
        MFCParam.get_buf_addr.in_usr_data = (int)pHWAccelCtx->pucMmapAddr;
        iError = s3c_mfc_ioctl(NULL, &(pHWAccelCtx->fFd), S3C_MFC_IOCTL_MFC_GET_LINE_BUF_ADDR, (unsigned long)&MFCParam);
        if ((iError < 0) || (MFCParam.get_buf_addr.ret_code < 0)) {
            s3c_mfc_release(NULL, &(pHWAccelCtx->fFd));
            av_free(pHWAccelCtx);
            pCodecCtx->hwaccel_context = NULL;
            pCodecCtx->hwaccel         = NULL;
            return -1;
        }
        pHWAccelCtx->pucStreamBuffer   = (uint8 *)MFCParam.get_buf_addr.out_buf_addr;

        pHWAccelCtx->bConfig           = FALSE;                         /*  δ���� MFC                  */

        pHWAccelCtx->pucStreamIn       = pHWAccelCtx->pucStreamBuffer;  /*  ������Ƶ����д���          */
    }

    if (uiFrameSize > 0) {
        memcpy(pHWAccelCtx->pucStreamIn, pucFrame, uiFrameSize);        /*  �����Ƶ��                  */
        pHWAccelCtx->pucStreamIn  += uiFrameSize;
    }

    return 0;
}
/*********************************************************************************************************
** Function name:           MPEG4_DecodeSlice
** Descriptions:            ����һ����Ƭ
** input parameters:        pCodecCtx           ������������
**                          pucSlice            ��Ƭ
**                          uiSliceSize         ��Ƭ�Ĵ�С
** output parameters:       NONE
** Returned value:          ERROR_CODE
** Created by:              JiaoJinXing
** Created Date:            2011-3-25
**--------------------------------------------------------------------------------------------------------
** Modified by:             chenmingji
** Modified date:           2012.01.12
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int MPEG4_DecodeSlice(AVCodecContext  *pCodecCtx,
                             const uint8_t   *pucSlice,
                             uint32_t         uiSliceSize)
{
    HWAccelContext  *pHWAccelCtx = pCodecCtx->hwaccel_context;
    s3c_mfc_args_t   MFCParam;
    int              iError;

    if (pucSlice == NULL && uiSliceSize == 0) {                         /*  �ر�Ӳ��������              */
        s3c_mfc_release(NULL, &(pHWAccelCtx->fFd));
        av_free(pHWAccelCtx);
        pCodecCtx->hwaccel_context = NULL;
        pCodecCtx->hwaccel         = NULL;
        return -1;
    } else {
        memcpy(pHWAccelCtx->pucStreamIn, pucSlice, uiSliceSize);
        pHWAccelCtx->pucStreamIn +=uiSliceSize;

        if (pHWAccelCtx->bConfig == FALSE) {                            /*  ���δ���� MFC              */

            /*
             * ��ʼ�� MPEG4 ������
             */
            MFCParam.dec_init.in_strmSize = pHWAccelCtx->pucStreamIn - pHWAccelCtx->pucStreamBuffer;

            iError = s3c_mfc_ioctl(NULL, &(pHWAccelCtx->fFd),
                                   S3C_MFC_IOCTL_MFC_MPEG4_DEC_INIT, (unsigned long)&MFCParam);
            if ((iError < 0) || (MFCParam.dec_init.ret_code < 0)) {
                s3c_mfc_release(NULL, &(pHWAccelCtx->fFd));
                av_free(pHWAccelCtx);
                pCodecCtx->hwaccel_context = NULL;
                pCodecCtx->hwaccel         = NULL;
                return -1;
            }

            pHWAccelCtx->bConfig  = TRUE;                               /*  �Ѿ����� MFC                */

            pHWAccelCtx->uiWidth  = MFCParam.dec_init.out_width;        /*  ��Ƶ���                    */
            pHWAccelCtx->uiHeight = MFCParam.dec_init.out_height;       /*  ��Ƶ�߶�                    */
        }

        return 0;
    }
}
/*********************************************************************************************************
** Function name:           MPEG4_EndFrame
** Descriptions:            ����һ֡ʱ�ĵ���
** input parameters:        pCodecCtx           ������������
** output parameters:       NONE
** Returned value:          ERROR_CODE
** Created by:              JiaoJinXing
** Created Date:            2011-3-25
**--------------------------------------------------------------------------------------------------------
** Modified by:             chenmingji
** Modified date:           2012.01.12
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int MPEG4_EndFrame(AVCodecContext  *pCodecCtx)
{
    HWAccelContext  *pHWAccelCtx = pCodecCtx->hwaccel_context;
    MpegEncContext  *pMpegEncCtx = pCodecCtx->priv_data;
    uint8           *pucYUVBuffer;
    s3c_mfc_args_t   MFCParam;
    int              iError;

    /*
     * ִ�н���
     */
    MFCParam.dec_exe.in_strmSize = pHWAccelCtx->pucStreamIn - pHWAccelCtx->pucStreamBuffer;

    pHWAccelCtx->pucStreamIn     = pHWAccelCtx->pucStreamBuffer;        /*  ������Ƶ����д���          */

    iError = s3c_mfc_ioctl(NULL, &(pHWAccelCtx->fFd), S3C_MFC_IOCTL_MFC_MPEG4_DEC_EXE, (unsigned long)&MFCParam);
    if ((iError < 0) || (MFCParam.dec_exe.ret_code < 0)) {
        s3c_mfc_release(NULL, &(pHWAccelCtx->fFd));
        av_free(pHWAccelCtx);
        pCodecCtx->hwaccel_context = NULL;
        pCodecCtx->hwaccel         = NULL;
        return  (-1);
    }

    /*
     * ��� YUV420 ֡�������Ļ�ַ
     */
    MFCParam.get_buf_addr.in_usr_data = (int)pHWAccelCtx->pucMmapAddr;
    iError = s3c_mfc_ioctl(NULL, &(pHWAccelCtx->fFd), S3C_MFC_IOCTL_MFC_GET_YUV_BUF_ADDR, (unsigned long)&MFCParam);
    if ((iError < 0) || (MFCParam.get_buf_addr.ret_code < 0)) {
        s3c_mfc_release(NULL, &(pHWAccelCtx->fFd));
        av_free(pHWAccelCtx);
        pCodecCtx->hwaccel_context = NULL;
        pCodecCtx->hwaccel         = NULL;
        return  (-1);
    }
    pucYUVBuffer = (uint8 *)MFCParam.get_buf_addr.out_buf_addr;

    /*
     * ����һ�� AVFrame ���ϲ�
     */
    pMpegEncCtx->current_picture_ptr->data[0]     = pucYUVBuffer;
    pMpegEncCtx->current_picture_ptr->data[1]     = pucYUVBuffer +   pHWAccelCtx->uiWidth * pHWAccelCtx->uiHeight;
    pMpegEncCtx->current_picture_ptr->data[2]     = pucYUVBuffer + ((pHWAccelCtx->uiWidth * pHWAccelCtx->uiHeight * 5) >> 2);

    pMpegEncCtx->current_picture_ptr->linesize[0] = pHWAccelCtx->uiWidth;
    pMpegEncCtx->current_picture_ptr->linesize[1] = pHWAccelCtx->uiWidth >> 1;
    pMpegEncCtx->current_picture_ptr->linesize[2] = pHWAccelCtx->uiWidth >> 1;

    ff_draw_horiz_band(pMpegEncCtx, 0, pMpegEncCtx->avctx->height);

    return  (0);
}
/*********************************************************************************************************
    MPEG4 Ӳ��������
*********************************************************************************************************/
AVHWAccel ff_s3c6410_mpeg4_hwaccel = {
        .name           = "s3c6410_mpeg4_hwaccel",
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = CODEC_ID_MPEG4,
        .pix_fmt        = PIX_FMT_YUV420P,
        .capabilities   = 0,
        .start_frame    = MPEG4_StartFrame,
        .end_frame      = MPEG4_EndFrame,
        .decode_slice   = MPEG4_DecodeSlice,
        .priv_data_size = 0,
};
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
