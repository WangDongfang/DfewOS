/*==============================================================================
** msg_box.h -- message box interface.
**
** MODIFY HISTORY:
**
** 2012-03-20 wdf Create.
==============================================================================*/

#ifndef __MSG_BOX_H__
#define __MSG_BOX_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

OS_STATUS msg_box_create (const char *msg);
OS_STATUS msg_box_image (const char *image_name);

int txt_get_line (const char *text, int width);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MSG_BOX_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

