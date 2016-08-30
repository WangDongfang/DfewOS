/*==============================================================================
** tetris.h -- Tetris Private Structs and Macros.
**
** MODIFY HISTORY:
**
** 2011-09-24 wdf Create.
==============================================================================*/

#ifndef  __TETRIS_H__
#define  __TETRIS_H__

#include "os.h"

#include "tet_for_bri.h"

/*======================================================================
  configs
======================================================================*/
#define BLOCK_SPEED       4  /* down interval : 1s / BLOCK_SPEED */

#define JOB_QUEUE_MAX     60
#define AUTO_MSG_MAX  	  50 

#define DOWN_TASK_PRI     100
#define JOB_TASK_PRI      100
#define AUTO_TASK_PRI     111

/*======================================================================
  characteristic
======================================================================*/
#define BLOCK_NODE_NUM    4         /* one block have node num */
#define BLOCK_TYPE_NUM    19        /* totally block have type num */
#define TETRIS_MAX_COLUMN 32        /* line map max */

/*======================================================================
  operate scr_map
======================================================================*/
#define BIT_SET(val, pos)       ((val) |= (1 << (pos)))
#define BIT_CLR(val, pos)       ((val) &= ~(1 << (pos)))
#define BIT_VAL(val, pos)       (((val) >> (pos)) & 0x1)

/*======================================================================
  return current node screen coordinate
======================================================================*/
#define COOR_X(i) (Tetris_ID->blk.base.x + _G_block_map[Tetris_ID->blk.type].pos[i].x)
#define COOR_Y(i) (Tetris_ID->blk.base.y + _G_block_map[Tetris_ID->blk.type].pos[i].y)

/*======================================================================
  MIN and MAX macro
======================================================================*/
#define TETERIS_MIN(a, b)               (((a) > (b)) ? (b) : (a))
#define TETERIS_MAX(a, b)               (((a) > (b)) ? (a) : (b))

/*======================================================================
  when move a block, may return below vaules
======================================================================*/
typedef enum motion_status {
    MOTION_STATUS_OK = 0,
    MOTION_STATUS_ERROR = -1,
    MOTION_STATUS_GAME_OVER = -2
} MOTION_STATUS;

/*======================================================================
  Block type intdentify O I Z S T J L
======================================================================*/
typedef enum block_type {
    O_0,
    I_0, I_1,
    Z_0, Z_1,
    S_0, S_1,
    T_0, T_1, T_2, T_3,
    J_0, J_1, J_2, J_3,
    L_0, L_1, L_2, L_3
} BLOCK_TYPE;

/*======================================================================
  block's color
======================================================================*/
typedef enum type_color {
    NO_COLOR = 0,

    O_COLOR = 1,
    I_COLOR,
    Z_COLOR,
    S_COLOR,
    T_COLOR,
    J_COLOR,
    L_COLOR
} TYPE_COLOR;

/*======================================================================
  Auto Algorithm find path store in this struct
======================================================================*/
typedef struct steps {
    TETRIS_MSG  msgs[AUTO_MSG_MAX];
    int         step_num;
} STEPS;

/*======================================================================
  A block
======================================================================*/
typedef struct block {
    COOR       base;
    BLOCK_TYPE type;
} BLOCK;

/*======================================================================
  The screen
======================================================================*/
typedef struct screen {
    unsigned int *scr_map;
    int           column_num;
    int           row_num;
} SCREEN;

/*======================================================================
  A Tetris Game Instance
======================================================================*/
typedef struct Tetris_Ctrl {
    void    *Bridge_ID;
    int    (*bri_show_routine)(void *Bridge_ID, COOR *coor, int show);

#ifndef OMIT_NODE_COLOR
    unsigned char *node_color_map;
#endif

    SCREEN        scr;
    unsigned int  full_mask;
    int           full_line;
    int           score;

    BLOCK         blk;

    OS_MSG_Q_ID   job_queue;

    volatile int  in_auto_mode;
    unsigned int *temp_scr_map;
    OS_TASK_ID    auto_task_id;
} TETRIS_CTRL;

/*======================================================================
  The node offset to a block's base
======================================================================*/
typedef struct pos {
    unsigned x : 4;
    unsigned y : 4;
} POS;

/*======================================================================
  A block map. store the nodes' offset to base
======================================================================*/
typedef struct block_map {
    POS pos[BLOCK_NODE_NUM];
} BLOCK_MAP;

/*======================================================================
  All block types' map array
======================================================================*/
static const BLOCK_MAP _G_block_map[BLOCK_TYPE_NUM] =
{
    /* O */
    { {{0,0}, {1,0}, {0,1}, {1,1}} },

    /* I */
    { {{0,0}, {0,1}, {0,2}, {0,3}} },
    { {{0,0}, {1,0}, {2,0}, {3,0}} },

    /* Z */
    { {{0,0}, {1,0}, {1,1}, {2,1}} },
    { {{1,0}, {0,1}, {1,1}, {0,2}} },

    /* S */
    { {{1,0}, {2,0}, {0,1}, {1,1}} },
    { {{0,0}, {0,1}, {1,1}, {1,2}} },

    /* T */
    { {{0,0}, {1,0}, {2,0}, {1,1}} },
    { {{1,0}, {0,1}, {1,1}, {1,2}} },
    { {{1,0}, {0,1}, {1,1}, {2,1}} },
    { {{0,0}, {0,1}, {1,1}, {0,2}} },

    /* J */
    { {{1,0}, {1,1}, {0,2}, {1,2}} },
    { {{0,0}, {0,1}, {1,1}, {2,1}} },
    { {{0,0}, {1,0}, {0,1}, {0,2}} },
    { {{0,0}, {1,0}, {2,0}, {2,1}} },

    /* L */
    { {{0,0}, {0,1}, {0,2}, {1,2}} },
    { {{0,0}, {1,0}, {2,0}, {0,1}} },
    { {{0,0}, {1,0}, {1,1}, {1,2}} },
    { {{2,0}, {0,1}, {1,1}, {2,1}} }
};

/*======================================================================
  Conver full_lines to score
======================================================================*/
static const int _G_score_map[BLOCK_NODE_NUM + 1] = {0, 1, 3, 7, 15};

/*======================================================================
  Conver BLOCK_TYPE to TYPE_COLOR
======================================================================*/
static const TYPE_COLOR _G_color_map[BLOCK_TYPE_NUM] =
{
    O_COLOR,
    I_COLOR, I_COLOR,
    Z_COLOR, Z_COLOR,
    S_COLOR, S_COLOR,
    T_COLOR, T_COLOR, T_COLOR, T_COLOR,
    J_COLOR, J_COLOR, J_COLOR, J_COLOR,
    L_COLOR, L_COLOR, L_COLOR, L_COLOR
};
#endif /* __TETRIS_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

