/*==============================================================================
** image.c -- image browser.
**
** MODIFY HISTORY:
**
** 2012-05-14 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../gui.h"
#include "msg_box.h"
#include "file_mgr.h"

/*==============================================================================
 * - _image_cb_show()
 *
 * - show a image file. when user [release] a [*.jpg] file icon, call this
 */
static OS_STATUS _image_cb_show (GUI_CBI *pCBI_file, GUI_COOR *pCoor)
{
    /* restore the frame */
    cbf_default_release(pCBI_file, pCoor);
    
    /* show image */
    msg_box_image (pCBI_file->name);

    return OS_STATUS_OK;
}

void image_register ()
{
    file_mgr_register ("*.jpg|*.JPG|oo", _image_cb_show);
}

/*==============================================================================
** FILE END
==============================================================================*/

