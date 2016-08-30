/*==============================================================================
** gui_cbf.c -- this file support some useful routine for cbi callback.
**
** MODIFY HISTORY:
**
** 2011-11-07 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include "gui.h"
#include "driver/lcd.h"

/*======================================================================
  other file support function
======================================================================*/
extern void app_home ();

/*======================================================================
  config
======================================================================*/
#define CBI_FRAME_THICK         4

/*==============================================================================
 * - reverse_frame()
 *
 * - reverse the cbi's frame color
 */
static void reverse_frame (GUI_CBI *pCBI)
{
    int i;
    int j;

    /* consider frame buffer move */
    pCBI->left_up.x += lcd_get_start_x();
    pCBI->right_down.x += lcd_get_start_x();

    /* if frame is too thick, reverse cbi's all pixel */
    if ( (pCBI->right_down.x - pCBI->left_up.x + 1 <= CBI_FRAME_THICK * 2) ||
         (pCBI->right_down.y - pCBI->left_up.y + 1 <= CBI_FRAME_THICK * 2) ) {

        for (i = pCBI->left_up.y; i <= pCBI->right_down.y; i++) {
            for (j = pCBI->left_up.x; j <= pCBI->right_down.x; j++) {
                lcd_reverse_pixel (i, j); /* reverse pixel color */
            }
        }

        return;
    }

    /* top frame */
    for (i = pCBI->left_up.y; i < pCBI->left_up.y + CBI_FRAME_THICK; i++) {

        for (j = pCBI->left_up.x; j <= pCBI->right_down.x; j++) { /* one loop one line */
            lcd_reverse_pixel (i, j); /* reverse pixel color */
        }
    }

    /* left -- right frame */
    for (i = pCBI->left_up.y + CBI_FRAME_THICK;
         i <= pCBI->right_down.y - CBI_FRAME_THICK;
         i++) {
        
        /* left */
        for (j = pCBI->left_up.x; j <  pCBI->left_up.x + CBI_FRAME_THICK; j++) {
            lcd_reverse_pixel (i, j); /* reverse pixel color */
        }
        /* right */
        for (j = pCBI->right_down.x - CBI_FRAME_THICK + 1; j <=  pCBI->right_down.x; j++) {
            lcd_reverse_pixel (i, j); /* reverse pixel color */
        }
    }

    /* bottom frame */
    for (i = pCBI->right_down.y - CBI_FRAME_THICK + 1; i <= pCBI->right_down.y; i++) {

        for (j = pCBI->left_up.x; j <= pCBI->right_down.x; j++) {
            lcd_reverse_pixel (i, j); /* reverse pixel color */
        }
    }

    /* consider frame buffer move */
    pCBI->left_up.x -= lcd_get_start_x();
    pCBI->right_down.x -= lcd_get_start_x();
}

/*==============================================================================
 * - cbf_default_press()
 *
 * - user touch this cbi
 */
OS_STATUS cbf_default_press (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    reverse_frame (pCBI);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbf_default_leave()
 *
 * - user leave this cbi with press touch screen
 */
OS_STATUS cbf_default_leave (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    reverse_frame (pCBI);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbf_default_release()
 *
 * - user release touch screen on this cbi
 */
OS_STATUS cbf_default_release (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    reverse_frame (pCBI);
    return OS_STATUS_OK;
}


/*==============================================================================
 * - cbf_no_drag()
 *
 * - no operation when user try to drag the cbi
 */
OS_STATUS cbf_no_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor)
{
    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbf_hw_drag()
 *
 * - use hareware scroll the screen
 */
OS_STATUS cbf_hw_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor)
{
    lcd_fb_move (p_offset_coor->x, p_offset_coor->y);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbf_do_drag()
 *
 * - user move around on this cbi with press touch screen
 */
OS_STATUS cbf_do_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor)
{
    int cbi_width  = pCBI->right_down.x - pCBI->left_up.x + 1;
    int cbi_height = pCBI->right_down.y - pCBI->left_up.y + 1;

    GUI_SIZE offset_size;

    if ((p_offset_coor->x == 0) && (p_offset_coor->y == 0)) {
        return OS_STATUS_OK;
    }

    /* erase old */
    offset_size.w = cbi_width;
    offset_size.h = cbi_height;
    gra_block (&pCBI->left_up, &offset_size, NULL);

    /* change to new coordinate */
    pCBI->left_up.x += p_offset_coor->x;
    pCBI->left_up.y += p_offset_coor->y;
    pCBI->right_down.x += p_offset_coor->x;
    pCBI->right_down.y += p_offset_coor->y;

    /* draw new */
    gra_block (&pCBI->left_up, &offset_size, pCBI->data);
    /* this cbi must be pressed, so we mark it */
    reverse_frame (pCBI);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbf_noop()
 *
 * - no operation
 */
OS_STATUS cbf_noop (GUI_CBI *pCBI, GUI_COOR *p_coor)
{
    return OS_STATUS_OK;
}

/*==============================================================================
 * - cbf_go_home()
 *
 * - start home application
 */
OS_STATUS cbf_go_home (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    app_home ();

    return OS_STATUS_OK;
}

/*==============================================================================
** FILE END
==============================================================================*/

