/*==============================================================================
** tty.h -- this file is tty Moudle support for bridge Moudle.
**
** MODIFY HISTORY:
**
** 2011-10-13 wdf Create.
==============================================================================*/

#ifndef __LIAN_TTY_H__
#define __LIAN_TTY_H__

#include "common.h"

/*======================================================================
  tty_show() can accept actions
======================================================================*/
#define ACT_SHOW        0
#define ACT_MOVE        1
#define ACT_MARK        2

#define ACTION_NUM      3

/*======================================================================
  bridge_mark() return values
======================================================================*/
#define GAME_OVER       0
#define MARKED          1
#define UNMARKED        2

int tty_show (int row, int col, int color, int action);
int tty_line (int row0, int col0, int row1, int col1, int color);
int tty_msg  (char *msg);
int tty_beep (int times);

#endif /* __LIAN_TTY_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

