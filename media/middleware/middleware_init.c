/*==============================================================================
** middleware_init.c -- Init middleware.
**
** MODIFY HISTORY:
**
** 2012-04-01 wdf Create.
==============================================================================*/
#include <libavcodec/avcodec.h>

extern AVHWAccel ff_s3c6410_h264_hwaccel;
extern AVHWAccel ff_s3c6410_mpeg4_hwaccel;
extern AVHWAccel ff_s3c6410_h263_hwaccel;

int middleware_init(void)
{
    av_register_hwaccel(&ff_s3c6410_h264_hwaccel);
    av_register_hwaccel(&ff_s3c6410_mpeg4_hwaccel);
    av_register_hwaccel(&ff_s3c6410_h263_hwaccel);

    return 0;
}


/*==============================================================================
** FILE END
==============================================================================*/
