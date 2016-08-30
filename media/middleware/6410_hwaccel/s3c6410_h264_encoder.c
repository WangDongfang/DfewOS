/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               s3c6410_h264_encoder.c
** Last modified Date:      2011-3-25
** Last Version:            1.0.0
** Descriptions:            H.264 Ӳ��������
**
**--------------------------------------------------------------------------------------------------------
** Created by:              JiaoJinXing
** Created date:            2011-3-25
** Version:                 1.0.0
** Descriptions:            H.264 Ӳ��������
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             chenmingji
** Modified date:           2012.01.13
** Version:                 1.0.0
** Descriptions:            ��ֲ��VxWorks 6.8 DKM
**
*********************************************************************************************************/

#include <libavcodec/avcodec.h>
#include <libavcodec/h264.h>

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
    H.264 Ӳ��������������
*********************************************************************************************************/
typedef struct {
    struct file             fFd;                                        /*  MFC �豸�ļ�������          */
    unsigned char          *pucMmapAddr;                                /*  �ڴ�ӳ��Ļ�ַ              */
    AVFrame                 EncodeFrame;                                /*  ����֡                      */
} HWEncoderContext;
/*********************************************************************************************************
** Function name:           H264Encoder_Init
** Descriptions:            ��ʼ�� H.264 Ӳ��������
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
static int H264Encoder_Init(AVCodecContext  *pCodecCtx)
{
    HWEncoderContext  *pHWEncoderCtx = pCodecCtx->priv_data;            /*  H.264 Ӳ��������������      */
    unsigned char     *pucYUVBuffer;
    s3c_mfc_args_t     MFCParam;
    int                iError;

    s3c_mfc_init();

    /*
     * �� MFC �豸
     */
    iError = s3c_mfc_open(NULL, &(pHWEncoderCtx->fFd));
    if (iError < 0) {
        return  (-1);
    }

    /*
     * �ڴ�ӳ�� MFC �豸
     */
    pHWEncoderCtx->pucMmapAddr = (uint8 *)s3c_mfc_mmap(&(pHWEncoderCtx->fFd));
    if (pHWEncoderCtx->pucMmapAddr == NULL) {
        s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));
        return  (-1);
    }

    /*
     * ����Ƭģʽ
     */
    MFCParam.set_config.in_config_param    = S3C_MFC_SET_CONFIG_ENC_SLICE_MODE;
    MFCParam.set_config.in_config_value[0] = 0;
    MFCParam.set_config.in_config_value[1] = 0;
    iError = s3c_mfc_ioctl(NULL, &(pHWEncoderCtx->fFd), S3C_MFC_IOCTL_MFC_SET_CONFIG, (unsigned long)&MFCParam);
    if ((iError < 0) || (MFCParam.set_config.ret_code < 0)) {
        s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));
        return  (-1);
    }

    /*
     * ��ʼ�� H.264 Ӳ��������
     */
    MFCParam.enc_init.in_width        = pCodecCtx->width;
    MFCParam.enc_init.in_height       = pCodecCtx->height;
    MFCParam.enc_init.in_bitrate      = pCodecCtx->bit_rate;
    MFCParam.enc_init.in_gopNum       = pCodecCtx->gop_size;
    MFCParam.enc_init.in_frameRateRes = pCodecCtx->time_base.den;
    MFCParam.enc_init.in_frameRateDiv = 0;
    /*
     * TODO: δ���õĲ���
     */
#if 0
    int                 IN_iInitQP;                                     /*  �������: ��ʼ����������    */
    int                 IN_iMaxQP;                                      /*  �������: ������������    */
    FP32                IN_fpGamma;                                     /*  �������: �˶�Ԥ���٤��ϵ��*/
#endif

    iError = s3c_mfc_ioctl(NULL, &(pHWEncoderCtx->fFd), S3C_MFC_IOCTL_MFC_H264_ENC_INIT, (unsigned long)&MFCParam);
    if ((iError < 0) || (MFCParam.enc_init.ret_code < 0)) {
        s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));
        return  (-1);
    }

    /*
     * ��� YUV420 ֡�������Ļ�ַ
     */
    MFCParam.get_buf_addr.in_usr_data = (int)pHWEncoderCtx->pucMmapAddr;
    iError = s3c_mfc_ioctl(NULL, &(pHWEncoderCtx->fFd), S3C_MFC_IOCTL_MFC_GET_YUV_BUF_ADDR, (unsigned long)&MFCParam);
    if ((iError < 0) || (MFCParam.get_buf_addr.ret_code < 0)) {
        s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));
        return  (-1);
    }
    pucYUVBuffer = (unsigned char *)MFCParam.get_buf_addr.out_buf_addr;

    /*
     * ����һ�� AVFrame ���ϲ�
     */
    pHWEncoderCtx->EncodeFrame.data[0]     = pucYUVBuffer;
    pHWEncoderCtx->EncodeFrame.data[1]     = pucYUVBuffer +   pCodecCtx->width * pCodecCtx->height;
    pHWEncoderCtx->EncodeFrame.data[2]     = pucYUVBuffer + ((pCodecCtx->width * pCodecCtx->height * 5) >> 2);

    pHWEncoderCtx->EncodeFrame.linesize[0] = pCodecCtx->width;
    pHWEncoderCtx->EncodeFrame.linesize[1] = pCodecCtx->width >> 1;
    pHWEncoderCtx->EncodeFrame.linesize[2] = pCodecCtx->width >> 1;

    pCodecCtx->coded_frame                 = &pHWEncoderCtx->EncodeFrame;

    /*
     * û�� B ֡
     */
    pCodecCtx->has_b_frames                = FALSE;

    return  (0);
}
/*********************************************************************************************************
** Function name:           H264Encoder_Encode
** Descriptions:            ִ�б���
** input parameters:        pCodecCtx           ������������
**                          pucStreamBuffer     ��Ƶ���������Ļ�ַ
**                          iStreamBufferSize   ��Ƶ���������Ĵ�С
**                          pvData              ������� YUV420 ֡
** output parameters:       NONE
** Returned value:          ��Ƶ���Ĵ�С
** Created by:              JiaoJinXing
** Created Date:            2011-3-25
**--------------------------------------------------------------------------------------------------------
** Modified by:             chenmingji
** Modified date:           2012.01.12
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int H264Encoder_Encode(AVCodecContext  *pCodecCtx,
                              unsigned char           *pucStreamBuffer,
                              int              iStreamBufferSize,
                              VOID            *pvData)
{
    HWEncoderContext  *pHWEncoderCtx = pCodecCtx->priv_data;
    AVFrame           *pEncodeFrame  = pvData;
    uint32               uiStreamSize;
    s3c_mfc_args_t     MFCParam;
    int                iError;

    if (pEncodeFrame != NULL) {
        /*
         * ��� YUV420 ֡���ڱ������� YUV420 ֡������, ��ô��Ҫ��һ�ο���
         */
        if (pEncodeFrame != &pHWEncoderCtx->EncodeFrame) {
            memcpy(pHWEncoderCtx->EncodeFrame.data[0],
                   pEncodeFrame->data[0],
                   (pCodecCtx->width * pCodecCtx->height * 3) >> 1);
        }

        /*
         * ���� IDR
         */
        MFCParam.set_config.in_config_param    = S3C_MFC_SET_CONFIG_ENC_CUR_PIC_OPT;
        MFCParam.set_config.in_config_value[0] = S3C_ENC_PIC_OPT_IDR;
        MFCParam.set_config.in_config_value[1] = 0;
        s3c_mfc_ioctl(NULL, &(pHWEncoderCtx->fFd), S3C_MFC_IOCTL_MFC_SET_CONFIG, (unsigned long)&MFCParam);

        /*
         * ִ�б���
         */
        iError = s3c_mfc_ioctl(NULL, &(pHWEncoderCtx->fFd), S3C_MFC_IOCTL_MFC_H264_ENC_EXE, (unsigned long)&MFCParam);
        if ((iError < 0) || (MFCParam.enc_exe.ret_code < 0)) {
            s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));
            return  (-1);
        }
        uiStreamSize = MFCParam.enc_exe.out_encoded_size;

        /*
         * �����Ƶ���������Ļ�ַ
         */
        MFCParam.get_buf_addr.in_usr_data = (int)pHWEncoderCtx->pucMmapAddr;
        iError = s3c_mfc_ioctl(NULL, &(pHWEncoderCtx->fFd), S3C_MFC_IOCTL_MFC_GET_LINE_BUF_ADDR, (unsigned long)&MFCParam);
        if ((iError < 0) || (MFCParam.get_buf_addr.ret_code < 0)) {
            s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));
            return  (-1);
        }

        /*
         * �ϲ����ͨ�� extradata ֱ�ӷ�����Ƶ��������
         */
        pCodecCtx->extradata      = (unsigned char *)MFCParam.get_buf_addr.out_buf_addr;
        pCodecCtx->extradata_size = uiStreamSize;

        if (pucStreamBuffer != NULL) {
            memcpy(pucStreamBuffer,
                   (void *)MFCParam.get_buf_addr.out_buf_addr,
                   uiStreamSize < iStreamBufferSize ? uiStreamSize : iStreamBufferSize);
        }

        /*
         * TODO: ��Ч��������Щ����
         */
        pHWEncoderCtx->EncodeFrame.pts       = 0;
        pHWEncoderCtx->EncodeFrame.pict_type = FF_I_TYPE;
        pHWEncoderCtx->EncodeFrame.key_frame = TRUE;
        pHWEncoderCtx->EncodeFrame.quality   = 1;

        return  (uiStreamSize);
    }

    return  (0);
}
/*********************************************************************************************************
** Function name:           H264Encoder_Close
** Descriptions:            �ر� H.264 Ӳ��������
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
static int H264Encoder_Close(AVCodecContext  *pCodecCtx)
{
    HWEncoderContext  *pHWEncoderCtx = pCodecCtx->priv_data;

    /*
     * �ر� MFC �豸
     */
    s3c_mfc_release(NULL, &(pHWEncoderCtx->fFd));

    /*
     * ���� ffmpeg �ڲ��ͷ� extradata
     */
    pCodecCtx->extradata      = NULL;
    pCodecCtx->extradata_size = 0;

    return  (0);
}
/*********************************************************************************************************
    H.264 Ӳ��������
*********************************************************************************************************/
AVCodec ff_s3c6410_h264_encoder = {
        .name           = "s3c6410_h264_encoder",
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = CODEC_ID_H264,
        .priv_data_size = sizeof(HWEncoderContext),
        .init           = H264Encoder_Init,
        .encode         = H264Encoder_Encode,
        .close          = H264Encoder_Close,
        .capabilities   = CODEC_CAP_DELAY,
        .pix_fmts       = (const enum PixelFormat[]) { PIX_FMT_YUV420P, PIX_FMT_NONE },
};
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/

