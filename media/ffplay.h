/*==============================================================================
** ffplay.h -- ffplay interface.
**
** MODIFY HISTORY:
**
** 2012-04-10 wdf Create.
==============================================================================*/

#ifndef __FFPLAY_H__
#define __FFPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

OS_STATUS media_play (const char *media_file_name);
OS_STATUS media_little_forward ();
OS_STATUS media_large_forward ();
OS_STATUS media_little_backward ();
OS_STATUS media_large_backward ();
OS_STATUS media_pause ();
OS_STATUS media_continue ();
OS_STATUS media_stop ();
OS_STATUS media_set_over_callback (FUNC_PTR over_func, uint32 arg1, uint32 arg2);
OS_STATUS media_goto (int percent);
OS_STATUS media_size_set (int width, int height);
OS_STATUS media_position_set (int x, int y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FFPLAY_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

