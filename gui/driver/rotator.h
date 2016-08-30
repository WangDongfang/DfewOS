/*==============================================================================
** rotator.h -- S3C6410X Rotator Driver interface.
**
** MODIFY HISTORY:
**
** 2012-03-17 wdf Create.
==============================================================================*/

#ifndef __ROTATOR_H__
#define __ROTATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================
  Rotator Controller Support Degrees Enumeration
======================================================================*/
typedef enum rotate_degree {
    DEGREE_0   = 0,
    DEGREE_90  = 1,
    DEGREE_180 = 2,
    DEGREE_270 = 3
} ROTATE_DEGREE;

/*==============================================================================
 * - rotator()
 *
 * - 旋转一张图片。此图片的像素数据必须是在内存<srcAddr>连续存放, 像素格式为
 *   RGB565, 旋转后的像素数据连续地放置在<dstAddr>内存中
 */
int rotator (void *srcAddr, void *dstAddr, int src_width, int src_height,
             ROTATE_DEGREE degree);

/*==============================================================================
 * - rotator_scr()
 *
 * - 旋转LCD屏图像。
 *   将LCD屏上从(<src_x>, <src_y>)处, 大小为(<src_width>, src_height>)的图像,
 *   经过<degree>度的旋转, 显示到(<dst_x>, <dst_y>)处
 */
int rotator_scr (int src_x, int src_y, int src_width, int src_height,
                 int dst_x, int dst_y, ROTATE_DEGREE degree);

#ifdef __cplusplus
}
#endif

#endif /* __ROTATOR_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

