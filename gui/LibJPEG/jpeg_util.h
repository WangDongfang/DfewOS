/*==============================================================================
** jpeg_util.h -- 
**
** MODIFY HISTORY:
**
** 2011-11-07 wdf Create.
==============================================================================*/

#ifndef __JPEG_UTIL_H__
#define __JPEG_UTIL_H__

uint16   *get_pic_pixel (const char *file_name, int *p_width, int *p_height);
OS_STATUS jpeg_dump_pic (const char *pic_file_name,
                         const GUI_COOR *p_left_up,
                         GUI_SIZE       *p_expect_size);

#endif /* __JPEG_UTIL_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

