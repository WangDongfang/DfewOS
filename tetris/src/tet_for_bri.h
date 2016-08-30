/*==============================================================================
** tet_for_bri.h -- Tetris Supports Interface for Bridge.
**
** MODIFY HISTORY:
**
** 2011-09-24 wdf Create.
==============================================================================*/
#ifndef __TET_FOR_BRI_H__
#define __TET_FOR_BRI_H__

/*======================================================================
  the coordinate of a node in screen
======================================================================*/
typedef struct coor {
    short x;
    short y;
} COOR;

/*======================================================================
  Tetris can process these messages
======================================================================*/
typedef enum tetris_msg {
    TETRIS_MSG_QUIT,
    TETRIS_MSG_TURN,
    TETRIS_MSG_DOWN,
    TETRIS_MSG_RIGHT,
    TETRIS_MSG_LEFT,
    TETRIS_MSG_AUTO,
    TETRIS_MSG_BOTTOM,
    TETRIS_MSG_HOLD
} TETRIS_MSG;

/*======================================================================
  send message return values
======================================================================*/
typedef enum send_msg_status {
    SEND_OK,
    SEND_TIMEOUT = -1,
    SEND_GAME_OVER = -2
} SEND_MSG_STATUS;

/*======================================================================
  Tetris Module Support API for Bridge Module
======================================================================*/
void *rb_init (void *Bridge_ID, int column_num, int row_num,
               int (*bri_show_routine)(void *Bridge_ID, COOR *coor, int show));
int   rb_send_msg (void *Tetris_ID, TETRIS_MSG msg);

#define TETRIS_FOREVER for (;;)

#endif /* __TET_FOR_BRI_H__ */
/*==============================================================================
** FILE END
==============================================================================*/

