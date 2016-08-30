/*==============================================================================
** file_mgr.h -- file manager.
**
** MODIFY HISTORY:
**
** 2012-04-10 wdf Create.
==============================================================================*/

#ifndef __FILE_MGR_H__
#define __FILE_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int       file_mgr_show (const char *path, const char *filter);
GUI_CBI  *file_mgr_reg_one (const char *full_name, const char *name, CB_FUNC_PTR open_func);
void      file_mgr_clear ();
OS_STATUS file_mgr_fresh ();
OS_STATUS file_mgr_updir ();
OS_STATUS file_mgr_register (const char *filter,  CB_FUNC_PTR file_open_func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FILE_MGR_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

