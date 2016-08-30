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
 * - ��תһ��ͼƬ����ͼƬ���������ݱ��������ڴ�<srcAddr>�������, ���ظ�ʽΪ
 *   RGB565, ��ת����������������ط�����<dstAddr>�ڴ���
 */
int rotator (void *srcAddr, void *dstAddr, int src_width, int src_height,
             ROTATE_DEGREE degree);

/*==============================================================================
 * - rotator_scr()
 *
 * - ��תLCD��ͼ��
 *   ��LCD���ϴ�(<src_x>, <src_y>)��, ��СΪ(<src_width>, src_height>)��ͼ��,
 *   ����<degree>�ȵ���ת, ��ʾ��(<dst_x>, <dst_y>)��
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

