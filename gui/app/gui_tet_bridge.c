/*==============================================================================
** bridge.c -- This moudle links GUI Display and Tetris.
**
** MODIFY HISTORY:
**
** 2011-09-24 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "../../tetris/src/tet_for_bri.h"

/*======================================================================
  Debug config
======================================================================*/
#define BRIDGE_DEBUG  serial_printf

/*======================================================================
  Bridge Control. One bridge have a instance of this struct
======================================================================*/
typedef struct Bridge_Ctrl {
    void *Display_ID;
    int (*dsply_show_routine)(void *Display_ID, int x0, int y0, int x1, int y1, int show); 
    void *Tetris_ID;

    int   window_lt_x;         /* the window to show Tetris left top "x" coordinate */
    int   window_lt_y;         /* the window to show Tetris left top "y" coordinate */
    int   node_width;
} BRIDGE_CTRL;

/*======================================================================
  forward functions declare
======================================================================*/
static int bri_show_routine (void *Bridge_ID, COOR *coor, int show);
static BRIDGE_CTRL *_G_Bridge_ID = NULL;

/*==============================================================================
 * - Russian_main()
 *
 * - The Display Module call this routine to start a Game
 */
void bri_Russian_main (void *Display_ID,
              int fd,
              int left_top_x, int left_top_y,
              int right_bottom_x, int right_bottom_y,
              int node_width,
              int (*dsply_show_routine)(void *Display_ID, int x0, int y0, int x1, int y1, int show))
{

    BRIDGE_CTRL *Bridge_ID = NULL;
    int  column_num;
    int  row_num;


    Bridge_ID = (BRIDGE_CTRL *)malloc (sizeof (BRIDGE_CTRL));
    if (Bridge_ID == NULL) {
        BRIDGE_DEBUG ("There is no memory for <Bridge_ID>!\n");
        return;
    }

    _G_Bridge_ID = Bridge_ID;

    column_num = (right_bottom_x - left_top_x) / node_width;
    row_num = (right_bottom_y - left_top_y) / node_width;

    Bridge_ID->Display_ID = Display_ID;
    Bridge_ID->dsply_show_routine = dsply_show_routine;
    Bridge_ID->window_lt_x = left_top_x;
    Bridge_ID->window_lt_y = left_top_y;
    Bridge_ID->node_width  = node_width;

    Bridge_ID->Tetris_ID = rb_init (Bridge_ID, column_num, row_num, bri_show_routine);
}

/*==============================================================================
 * - bri_bridge_free()
 *
 * - Display Module call this routine to free Bridge Module resource
 */
void bri_bridge_free ()
{
    /* free Bridge module control struct */
    if (_G_Bridge_ID != NULL) {
        free (_G_Bridge_ID);
    }
    _G_Bridge_ID = NULL;
}

/*==============================================================================
 * - bri_send_msg()
 *
 * - Dislplay Module call this routine to send msg to tetris core
 */
int bri_send_msg (int type)
{
    int send_ret;

    if (_G_Bridge_ID == NULL) {
        return -1;
    }

    switch (type) {
        case 0:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_QUIT);
            break;
        case 1:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_TURN);
            break;
        case 2:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_DOWN);
            break;
        case 3:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_RIGHT);
            break;
        case 4:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_LEFT);
            break;
        case 5:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_AUTO);
            break;
        case 6:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_BOTTOM);
            break;
        case 7:
            send_ret = rb_send_msg (_G_Bridge_ID->Tetris_ID, TETRIS_MSG_HOLD);
            break;
        default:
            break;
    }

    if (send_ret == SEND_GAME_OVER) {
        return -1;
    } else {
        return 0;
    }
}

/*==============================================================================
 * - bri_show_routine()
 *
 * - convert the node coor in screen to pix coor for display show routine
 *   and call display show routine erase or show the rectange
 */
static int bri_show_routine (void *Bridge_ID, COOR *coor, int show)
{
    BRIDGE_CTRL *_Bridge_ID = (BRIDGE_CTRL *)Bridge_ID;
    int node_width = _Bridge_ID->node_width;

    int x0 = _Bridge_ID->window_lt_x + coor->x * node_width;
    int y0 = _Bridge_ID->window_lt_y + coor->y * node_width;
    int x1 = x0 + node_width - 1;
    int y1 = y0 + node_width - 1;

    return _Bridge_ID->dsply_show_routine (_Bridge_ID->Display_ID, x0, y0, x1, y1, show);
}

/*==============================================================================
** FILE END
==============================================================================*/

