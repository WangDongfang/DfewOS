/*==============================================================================
** lian.h -- this file is lian Moudle support for bridge Moudle.
**
** MODIFY HISTORY:
**
** 2011-10-13 wdf Create.
==============================================================================*/

#ifndef __LIAN_H__
#define __LIAN_H__

#include "common.h"

/*======================================================================
  This Game have levels
======================================================================*/
#define LEVEL_NUM   15        /* _LEVEL_NUM * N_ELEMENTS(_G_type_num) */

/*======================================================================
  lian_check() can return values
======================================================================*/
#define NO_WAY      0
#define CAN_GO      1

/*======================================================================
  the coordinate of a node in screen
======================================================================*/
typedef struct coor {
    short x;
    short y;
} COOR;

typedef struct op_delete {
    COOR start;
    COOR end;
} OP_DELETE;

/*======================================================================
  Global variables for bridge Moudle
======================================================================*/
extern int _G_map [ROW_NUM + 2][COL_NUM + 2];
extern COOR _G_way[4];
extern int  _G_pos;
extern int       _G_op_cnt;
extern OP_DELETE _G_ops[ROW_NUM * COL_NUM / 2];

/*======================================================================
  support API
======================================================================*/
int lian_init();
int lian_reorder();
int lian_check(const int (* map)[COL_NUM+2], const COOR *start, const COOR *end);
int lian_left(int (* map)[COL_NUM+2], int left_blocks);
int lian_get_op (const int (* map)[COL_NUM+2]);

#endif /* __LIAN_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

