/*==============================================================================
** bridge.c -- This moudle links GUI Display and Tetris.
**
** MODIFY HISTORY:
**
** 2011-09-24 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../../string.h"
#include "tet_for_bri.h"

/*======================================================================
  Debug config
======================================================================*/
#define BRIDGE_DEBUG  serial_printf

/*======================================================================
  KeyBoard Strings
======================================================================*/
#define KEY_ESC     "\033"
#define KEY_UP      "\033[A"
#define KEY_DOWN    "\033[B"
#define KEY_RIGHT   "\033[C"
#define KEY_LEFT    "\033[D"
#define KEY_BOTTOM  " "         /* fix block at bottom */
#define KEY_HOLD    "h"         /* freeze tetris */
#define KEY_AUTO    "`"         /* the key below [ESC] and left [1] */

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

/*==============================================================================
 * - Russian_main()
 *
 * - The Display Module call this routine to start a Game
 */
void Russian_main (void *Display_ID,
              int fd,
              int left_top_x, int left_top_y,
              int right_bottom_x, int right_bottom_y,
              int node_width,
              int (*dsply_show_routine)(void *Display_ID, int x0, int y0, int x1, int y1, int show))
{
    int  nread;
    char key[4];

    BRIDGE_CTRL *Bridge_ID = NULL;
    int  column_num;
    int  row_num;


    Bridge_ID = (BRIDGE_CTRL *)malloc (sizeof (BRIDGE_CTRL));
    if (Bridge_ID == NULL) {
        BRIDGE_DEBUG ("There is no memory for <Bridge_ID>!\n");
        return;
    }

    column_num = (right_bottom_x - left_top_x) / node_width;
    row_num = (right_bottom_y - left_top_y) / node_width;

    Bridge_ID->Display_ID = Display_ID;
    Bridge_ID->dsply_show_routine = dsply_show_routine;
    Bridge_ID->window_lt_x = left_top_x;
    Bridge_ID->window_lt_y = left_top_y;
    Bridge_ID->node_width  = node_width;

    Bridge_ID->Tetris_ID = rb_init (Bridge_ID, column_num, row_num, bri_show_routine);

#define _SEND_MSG(m)    if(rb_send_msg (Bridge_ID->Tetris_ID, m) == SEND_GAME_OVER) { break;}
    FOREVER {
        nread = serial_read (key, 3);
        key[nread] = '\0';

        if (strcmp (key, KEY_DOWN) == 0) {          /* Down */
            _SEND_MSG(TETRIS_MSG_DOWN);
            _SEND_MSG(TETRIS_MSG_DOWN);
        } else if (strcmp (key, KEY_BOTTOM) == 0) { /* ' ': bottom */
            _SEND_MSG(TETRIS_MSG_BOTTOM);
        } else if (strcmp (key, KEY_UP) == 0) {     /* Up */
            _SEND_MSG(TETRIS_MSG_TURN);
        } else if (strcmp (key, KEY_LEFT) == 0) {   /* Left */
            _SEND_MSG(TETRIS_MSG_LEFT);
        } else if (strcmp (key, KEY_RIGHT) == 0) {  /* Right */
            _SEND_MSG(TETRIS_MSG_RIGHT);
        } else if (strcmp (key, KEY_AUTO) == 0) {   /* '`': auto */
            _SEND_MSG(TETRIS_MSG_AUTO);
        } else if (strcmp (key, KEY_HOLD) == 0) {   /* 'h': hold on */
            _SEND_MSG(TETRIS_MSG_HOLD);
        } else if (strcmp (key, KEY_ESC) == 0) {    /* Esc: quit */
            _SEND_MSG(TETRIS_MSG_QUIT);
            break;
        }
    }

    /* 
     * wait tetris moudle is over.
     * to avoid it use Bridge module control struct after below free
     */
    while (1) {
        _SEND_MSG(TETRIS_MSG_QUIT);
        delayQ_delay (SYS_CLK_RATE / 10);
    }

    /* free Bridge module control struct */
    free (Bridge_ID);
#undef _SEND_MSG
}

/*==============================================================================
 * - bri_show_routine()
 *
 * - conver the node coor in screen to pix coor for display show routine
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

