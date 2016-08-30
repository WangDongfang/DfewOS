/*==============================================================================
** jpeg.h -- jpeg hardware decode interface.
**
** MODIFY HISTORY:
**
** 2012-05-10 wdf Create.
==============================================================================*/

#ifndef __JPEG_H__
#define __JPEG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int jpeg_show_file (const char *file_name, int x, int y, int w, int h);
OS_STATUS jpeg_hw_dump (const char *file_name, const int *p_x, const int *p_y,
                        int *p_expect_width, int *p_expect_height,
                        int *p_width, int *p_height);
uint16 *jpeg_hw_pixel (const char *file_name, int *w, int *h);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __JPEG_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

