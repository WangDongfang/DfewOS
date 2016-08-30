/*==============================================================================
** gui_cbi.c -- cbi control.
**
** MODIFY HISTORY:
**
** 2011-10-29 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include "gui.h"
#include "driver/lcd.h"
#include "LibJPEG/jpeg_util.h"

/*======================================================================
  this list link all cbi appear on lcd screen
======================================================================*/
static DL_LIST  _G_cbi_list = {NULL, NULL};
static GUI_CBI *_G_pCBI_cover = NULL; /* save the cover cbi */

/*==============================================================================
 * - _cbi_find_in_which()
 *
 * - dlist_each() callback function, check whether the coor in the cbi
 *   if yes return OS_STATUS_ERROR to make dlist_each() return the cbi point,
 *   if not return OS_STATUS_OK to make dlist_each() continue chech next cbi.
 */
static OS_STATUS _cbi_find_in_which (GUI_CBI *pCBI, GUI_COOR *pCoor)
{
    if ( (pCBI->left_up.x <= pCoor->x) && (pCBI->right_down.x >= pCoor->x) &&
         (pCBI->left_up.y <= pCoor->y) && (pCBI->right_down.y >= pCoor->y) ) {

        /* find it, stop dlist_each */
        return OS_STATUS_ERROR;
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbi_in_which()
 *
 * - check the coordinate in which CBI area those in <_G_cbi_list>
 */
GUI_CBI *cbi_in_which (const GUI_COOR *p_scr_coor, GUI_COOR *p_cbi_coor)
{
    GUI_CBI *p_find_cbi = NULL;

    p_find_cbi = (GUI_CBI *)dlist_each (&_G_cbi_list, 
                                    (EACH_FUNC_PTR)_cbi_find_in_which,
                                    (int)p_scr_coor);

    /* calculate the relative coor */
    if ((p_find_cbi != NULL) && (p_cbi_coor != NULL)) {
        p_cbi_coor->x = p_scr_coor->x - p_find_cbi->left_up.x;
        p_cbi_coor->y = p_scr_coor->y - p_find_cbi->left_up.y;
    }

    return p_find_cbi;
}

/*==============================================================================
 * - cbi_init_cb()
 *
 * - init a cbi with default process functions
 */
OS_STATUS cbi_init_cb (GUI_CBI *pCBI)
{
    if (pCBI == NULL) {
        return OS_STATUS_ERROR;
    }

    pCBI->func_press   = cbf_default_press;
    pCBI->func_leave   = cbf_default_leave;
    pCBI->func_release = cbf_default_release;
    pCBI->func_drag    = cbf_no_drag;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbi_create_default()
 *
 * - create a cbi from a picture file, init it callback function with default,
 *   and register it.
 */
GUI_CBI *cbi_create_default (const char     *pic_file_name,
                             const GUI_COOR *p_left_up,
                             GUI_SIZE       *p_cbi_size,
                             BOOL            save_data)
{
    int i;
    GUI_CBI *pCBI = NULL;

    if (p_left_up == NULL) {
        goto _finish;
    }

    /* show picture at <p_left_up> with <p_cbi_size> size */
    /* if <p_cbi_size> is biger than picture size the left is black,
     * otherwise failed */
    /* if <p_cbi_size> is {0, 0}, then it will be changed to equal the pic size */
    if (jpeg_dump_pic (pic_file_name, p_left_up, p_cbi_size)
            == OS_STATUS_ERROR) {
        goto _finish;
    }

    /* alloc cbi struct */
    pCBI = malloc (sizeof (GUI_CBI));

    if (pCBI == NULL) {
        goto _finish;
    }

    /* calculate left_up and right_down coordinate  */
    pCBI->left_up.x = p_left_up->x;
    pCBI->left_up.y = p_left_up->y;
    pCBI->right_down.x = p_left_up->x + p_cbi_size->w - 1;
    pCBI->right_down.y = p_left_up->y + p_cbi_size->h - 1;

    /* save picture data if need */
    if (save_data) {
        pCBI->data = (uint16 *)malloc (p_cbi_size->w * p_cbi_size->h * sizeof (uint16));
        if (pCBI->data != NULL) {
            for (i = pCBI->left_up.y; i <= pCBI->right_down.y; i++) {
                memcpy (pCBI->data + (i - pCBI->left_up.y) * p_cbi_size->w,   /* dst */
                        lcd_get_addr (i, pCBI->left_up.x),                    /* src */
                        p_cbi_size->w * sizeof (uint16));                     /* size */
            }
        }
    } else {
        pCBI->data = NULL;
    }

    /* init cbi's name as picture file nanme */
    strlcpy (pCBI->name, pic_file_name, GUI_CBI_NAME_LEN_MAX);

    /* init cbi's callback function as default */
    cbi_init_cb (pCBI);

_finish:
    GUI_ASSERT (pCBI != NULL);
    return pCBI;
}

/*==============================================================================
 * - cbi_create_blank()
 *
 * - create a cbi that have no picture
 */
GUI_CBI *cbi_create_blank (GUI_COOR *p_left_up, GUI_COOR *p_right_bottom,
                           CB_FUNC_PTR user_func_press)
{
    GUI_CBI *pCBI = NULL;

    /* alloc cbi struct */
    pCBI = malloc (sizeof (GUI_CBI));

    if (pCBI == NULL) {
        return NULL;
    }

    /* calculate left_up and right_down coordinate  */
    pCBI->left_up.x = p_left_up->x;
    pCBI->left_up.y = p_left_up->y;
    pCBI->right_down.x = p_right_bottom->x;
    pCBI->right_down.y = p_right_bottom->y;

    pCBI->data = NULL;

    /* init cbi's callback function as default */
    cbi_init_cb (pCBI);
    pCBI->func_press = user_func_press;

    cbi_register (pCBI);

    return pCBI;
}

/*==============================================================================
 * - cbi_register()
 *
 * - register a cbi into list
 */
OS_STATUS cbi_register (GUI_CBI *pCBI)
{
#if 0
    dlist_add (&_G_cbi_list, (DL_NODE *)pCBI); /* insert tail */
#else
    dlist_insert (&_G_cbi_list, NULL, (DL_NODE *)pCBI); /* insert head */
#endif

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbi_unregister()
 *
 * - remove a cbi from list, and free it's memory, but not erase the pic
 */
OS_STATUS cbi_unregister (GUI_CBI *pCBI)
{
    if (dlist_check (&_G_cbi_list, (DL_NODE *)pCBI)) {
        dlist_remove (&_G_cbi_list, (DL_NODE *)pCBI);
        if (pCBI->data != NULL) {
            free (pCBI->data);
        }
        free (pCBI);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbi_delete_all()
 *
 * - clear the CBI list, and free all cbi's memory
 */
OS_STATUS cbi_delete_all ()
{
    GUI_CBI *pCBI = (GUI_CBI *)dlist_get (&_G_cbi_list);

    while (pCBI != NULL) {
        if (pCBI->data != NULL) {
            free (pCBI->data);
        }
        free (pCBI);
        pCBI = (GUI_CBI *)dlist_get (&_G_cbi_list);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbi_cover_all()
 *
 * - cover current all cbis
 */
OS_STATUS cbi_cover_all ()
{
    GUI_COOR cover_left_up = {0, 0};
    GUI_COOR cover_right_down = {gra_scr_w() - 1, gra_scr_h() - 1};

    _G_pCBI_cover = cbi_create_blank(&cover_left_up, &cover_right_down, cbf_noop);
    _G_pCBI_cover->func_release = cbf_noop;
    _G_pCBI_cover->func_leave = cbf_noop;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbi_uncover()
 *
 * - unconver all cbis
 */
OS_STATUS cbi_uncover ()
{
    cbi_unregister (_G_pCBI_cover);

    return OS_STATUS_OK;
}
/*==============================================================================
** FILE END
==============================================================================*/

