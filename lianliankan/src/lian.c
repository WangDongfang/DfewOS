/*==============================================================================
** lian.c -- core.
**
** MODIFY HISTORY:
**
** 2011-10-12 wdf Create.
** 2012-05-15 wdf Add reorder.
==============================================================================*/
#include "lian.h"

#ifdef OS_DFEWOS
#include <dfewos.h>
#else
#include <vxWorks.h>
#define  MIN(a, b)              (((a) > (b)) ? (b) : (a))
#define  serial_printf          printf
#endif
#include <stdio.h>
#include <stdlib.h>        /* srand(), rand() */
#include "string.h"

/*======================================================================
  config Block types
======================================================================*/
#define _LEVEL_NUM   3
static int _G_type_num[] = {8, 12, 16, 20, 24};

/*======================================================================
  This Game Have Maps for each level
======================================================================*/
static const int _G_level_maps [_LEVEL_NUM][ROW_NUM + 2][COL_NUM + 2] =
{
#define B  (-1)
    {
        {B, B, B, B, B, B, B, B, B, B, B, B},
        {B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, B},
        {B, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B},
        {B, B, B, B, B, B, B, B, B, B, B, B},
    },
    {
        {B, B, B, B, B, B, B, B, B, B, B, B},
        {B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, B},
        {B, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B},
        {B, B, B, B, B, B, B, B, B, B, B, B},
    },
        {
        {B, B, B, B, B, B, B, B, B, B, B, B},
        {B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, B},
        {B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B},
        {B, B, B, B, B, B, B, B, B, B, B, B},
    }
#undef B
};

/*======================================================================
  Current map for user interface
======================================================================*/
int _G_map [ROW_NUM + 2][COL_NUM + 2] =
{
    {-1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1},

    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},
    {-1,      0, 0, 0, 0, 0,  0, 0, 0, 0, 0,      -1},

    {-1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1},
};

/*======================================================================
  store the path
======================================================================*/
COOR _G_way[4];
int  _G_pos = 0;

/*======================================================================
  function forward declare
======================================================================*/
static int _reach (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end);
static int _lr_reach (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end);
static int _lr_lr_reach (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end);

/*==============================================================================
 * - lian_init()
 *
 * - init
 */
int lian_init (int level)
{
    int i;
    int j;
    int fill_cnt = 0;
    int type;
    int *temp = NULL;
    int temp_fill;

    /* check level */
    if (level >= LEVEL_NUM) {
        return -1;
    }

    /* count the will filled block number */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_level_maps[level % _LEVEL_NUM][i][j] != 0) {
                fill_cnt++;
            }
            _G_map[i][j] = 0; /* clear current map */
        }
    }

    /* chech not 0 blocks number */
    if (fill_cnt % 2 != 0) {
        serial_printf ("check the map, It's filled block number is't odd\n");
        return -1;
    }

    /* alloc memory for temp */
    temp = (int *)calloc (fill_cnt, sizeof (int));
    if (temp == NULL) {
        serial_printf ("No memory.\n");
        return -1;
    }

    /* make blocks */
    for (i = 0, type = 0; i < fill_cnt; type = (type+1) % _G_type_num[level / _LEVEL_NUM]) {
        temp[i++] = type + 1;
        temp[i++] = type + 1;
    }

    do {
        /* exchage blocks */
        for (i = 0; i < fill_cnt; i++) {
            int ex = i + rand () % (fill_cnt - i);
            int t = temp[i];
            temp[i] = temp[ex];
            temp[ex] = t;
        }

        /* fill the map */
        temp_fill = fill_cnt;
        for (i = 1; i < ROW_NUM + 1; i++) {
            for (j = 1; j < COL_NUM + 1; j++) {
                if (_G_level_maps[level % _LEVEL_NUM][i][j] != 0) {
                    _G_map[i][j] = temp[--temp_fill];
                }
            }
        }

#if 0 /* Check whether this order can delete all */
    } while (lian_left (_G_map, fill_cnt) != 0);
#else
    } while (0);
#endif

    free (temp);
    
    return fill_cnt;
}

/*==============================================================================
 * - lian_reorder()
 *
 * - reorder the left blocks
 */
int lian_reorder()
{
    int i;
    int j;
    int fill_cnt = 0;
    int temp[ROW_NUM * COL_NUM];
    int temp_fill;

    /* count the will filled block number & store old order to temp */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_map[i][j] != 0) {
                temp[fill_cnt++] = _G_map[i][j];
            }
        }
    }

    /* calculate new order */
    for (i = 0; i < fill_cnt; i++) {
        int ex = i + rand () % (fill_cnt - i);
        int t = temp[i];
        temp[i] = temp[ex];
        temp[ex] = t;
    }

    /* fill new order to the map */
    temp_fill = fill_cnt;
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_map[i][j] != 0) {
                _G_map[i][j] = temp[--temp_fill];
            }
        }
    }

    return fill_cnt;
}

/*==============================================================================
 * - _reach()
 *
 * - 一步可达 
 */
static int _reach (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end)
{
    int status = 0;  /* init can't reache */
    int dir;

    int i_x = start->x;
    int i_y = start->y;

    /* find possible dir */
    if ((start->x > end->x) && (start->y == end->y)) {
        dir = LEFT;
    } else if ((start->x < end->x) && (start->y == end->y)) {
        dir = RIGHT;
    } else if ((start->x == end->x) && (start->y > end->y)) {
        dir = UP;
    } else if ((start->x == end->x) && (start->y < end->y)) {
        dir = DOWN;
    } else {
        return NO_WAY;
    }

    /* check whether can reach if go straight */
    switch (dir) {
        case LEFT:
            while ((--i_x > end->x) && (map[start->y][i_x] == 0)) ;
            status = (i_x == end->x) ? CAN_GO : NO_WAY;
            break;
        case RIGHT:
            while ((++i_x < end->x) && (map[start->y][i_x] == 0)) ;
            status = (i_x == end->x) ? CAN_GO : NO_WAY;
            break;
        case UP:
            while ((--i_y > end->y) && (map[i_y][start->x] == 0)) ;
            status = (i_y == end->y) ? CAN_GO : NO_WAY;
            break;
        case DOWN:
            while ((++i_y < end->y) && (map[i_y][start->x] == 0)) ;
            status = (i_y == end->y) ? CAN_GO : NO_WAY;
            break;
        default:
            break;
    }

    return status;
}

/*==============================================================================
 * - _lr_reach()
 *
 * - 拐
 */
static int _lr_reach (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end)
{
    COOR turn[2];

    /* find two possible turn points */
    if ((start->x > end->x) && (start->y > end->y)) { /* 1 */
        turn[0].x = end->x - 1; /* try go LEFT */
        turn[0].y = start->y;

        turn[1].x = start->x;   /* try go UP */
        turn[1].y = end->y - 1;
    } else if ((start->x < end->x) && (start->y > end->y)) { /* 2 */
        turn[0].x = end->x + 1; /* try go RIGHT */
        turn[0].y = start->y;

        turn[1].x = start->x;   /* try go UP */
        turn[1].y = end->y - 1;
    } else if ((start->x > end->x) && (start->y < end->y)) { /* 3 */
        turn[0].x = end->x - 1; /* try go LEFT */
        turn[0].y = start->y;

        turn[1].x = start->x;   /* try go DOWN */
        turn[1].y = end->y + 1;
    } else if ((start->x < end->x) && (start->y < end->y)) { /* 4 */
        turn[0].x = end->x + 1; /* try go RIGHT */
        turn[0].y = start->y;

        turn[1].x = start->x;   /* try go DOWN */
        turn[1].y = end->y + 1;
    } else {
        return NO_WAY;
    }

    /* fisrt reach <turn[0]> then turn to <end> */
    if (_reach (map, start, &turn[0]) == CAN_GO) {
        turn[0].x = end->x;
        _G_way[_G_pos++] = turn[0];
        if (_reach (map, &turn[0], end) == CAN_GO) {
            return CAN_GO;
        }
        _G_pos--;
    }

    /* fisrt reach <turn[1]> then turn to <end> */
    if (_reach (map, start, &turn[1]) == CAN_GO) {
        turn[1].y = end->y;
        _G_way[_G_pos++] = turn[1];
        if (_reach (map, &turn[1], end) == CAN_GO) {
            return CAN_GO;
        }
        _G_pos--;
    }

    return NO_WAY;
}

/*======================================================================
  LR_REACH
======================================================================*/
#define LR_REACH() { \
    _G_way[_G_pos++] = turn; \
    if (_lr_reach (map, &turn, end) == CAN_GO) {return CAN_GO;} \
    _G_pos--;}

/*==============================================================================
 * - _lr_lr_reach()
 *
 * - 拐-拐
 */
static int _lr_lr_reach (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end)
{
    COOR turn;

    /* try LEFT */
    turn = *start;
    while (map[turn.y][--turn.x] == 0) LR_REACH();

    /* try RIGHT */
    turn = *start;
    while (map[turn.y][++turn.x] == 0) LR_REACH();

    /* try UP */
    turn = *start;
    while (map[--turn.y][turn.x] == 0) LR_REACH();

    /* try DOWN */
    turn = *start;
    while (map[++turn.y][turn.x] == 0) LR_REACH();

    return NO_WAY;
}

/*==============================================================================
 * - lian_check()
 *
 * - lian_check <start> can reach <end> in three line
 */
int lian_check (const int (* map)[COL_NUM+2], const COOR *start, const COOR *end)
{
    /* coordinate equal */
    if (start->x == end->x && start->y == end->y) {
        return NO_WAY;
    }

    /* no block */
    if (map[start->y][start->x] == 0) {
        return NO_WAY;
    }
    /* type not match */
    if (map[start->y][start->x] != map[end->y][end->x]) {
        return NO_WAY;
    }

    _G_pos = 0;
    
    _G_way[_G_pos++] = *start;

    /************************** one line ****************************/
    /* start 一往无前之 */
    if (_reach (map, start, end) == CAN_GO) {
        _G_way[_G_pos++] = *end;
        return CAN_GO;
    }

    /************************** two lines ***************************/
    /* start 拐之 */
    if (_lr_reach (map, start, end) == CAN_GO) {
        _G_way[_G_pos++] = *end;
        return CAN_GO;
    }

    /************************** three lines *************************/
    /* start 拐-拐之 */
    if (_lr_lr_reach (map, start, end) == CAN_GO) {
        _G_way[_G_pos++] = *end;
        return CAN_GO;
    }

    return NO_WAY;
}

/*======================================================================
  check whether this map can delete all blocks.          START
======================================================================*/
int              _G_op_cnt = 0;
OP_DELETE        _G_ops[ROW_NUM * COL_NUM / 2];

static int       _G_op_cnt_tmp = 0;
static OP_DELETE _G_ops_tmp[ROW_NUM * COL_NUM / 2];

/*==============================================================================
 * - _push_op()
 *
 * - push a op_detete in temp operations array
 */
static void _push_op (COOR *p_start, COOR *p_end)
{
    _G_ops_tmp[_G_op_cnt_tmp].start = *p_start;
    _G_ops_tmp[_G_op_cnt_tmp].end   = *p_end;

    _G_op_cnt_tmp++;
}
/*==============================================================================
 * - _pop_op()
 *
 * - pop a op_detete in temp operations array
 */
static void _pop_op ()
{
    _G_op_cnt_tmp--;
}

/*==============================================================================
 * - _save_op()
 *
 * - copy temp operations array to Global operations array
 */
static void _save_op ()
{
    if (_G_op_cnt_tmp > _G_op_cnt) {
        _G_op_cnt = _G_op_cnt_tmp;
        memcpy (_G_ops, _G_ops_tmp, sizeof (OP_DELETE) * _G_op_cnt);
    }
}

/*==============================================================================
 * - lian_left()
 *
 * - calculate the minimal left blocks in current map
 */
int lian_left (int (* map)[COL_NUM+2], int left_blocks)
{
    int  i, j, p, q;
    COOR start, end;
    int  min_left = left_blocks;
    int  sub_left;
    int  t_start, t_end;

    _G_op_cnt = 0;

    if (left_blocks == 0) {
        goto over_check;
    }

    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            start.y = i;
            start.x = j;
            
            /* throught current row */
            for (q = start.x + 1; q < COL_NUM + 1; q++) {
                end.y = start.y;
                end.x = q;

                if (lian_check((const int (*)[COL_NUM + 2])map,
                                &start, &end) == CAN_GO) {
                    t_start = map[start.y][start.x];
                    t_end   = map[end.y][end.x];

                    map[start.y][start.x] = 0;
                    map[end.y][end.x]     = 0;
                    _push_op (&start, &end);  /* save this operation */

                    sub_left = lian_left (map, left_blocks - 2);

                    _pop_op ();               /* pop this operation */
                    map[start.y][start.x] = t_start;
                    map[end.y][end.x]     = t_end;

                    min_left = MIN(min_left, sub_left);
                    if (min_left == 0) {
                        goto over_check;
                    }
                } /* if (..CAN_GO) */
            }

            /* next row to map over */
            for (p = start.y + 1; p < ROW_NUM + 1; p++) {
                for (q = 1; q < COL_NUM + 1; q++) {
                    end.y = p;
                    end.x = q;

                    if (lian_check((const int (*)[COL_NUM + 2])map,
                                    &start, &end) == CAN_GO) {
                        t_start = map[start.y][start.x];
                        t_end   = map[end.y][end.x];

                        map[start.y][start.x] = 0;
                        map[end.y][end.x]     = 0;
                        _push_op (&start, &end);  /* save this operation */

                        sub_left = lian_left (map, left_blocks - 2);

                        _pop_op ();               /* pop this operation */
                        map[start.y][start.x] = t_start;
                        map[end.y][end.x]     = t_end;

                        min_left = MIN(min_left, sub_left);
                        if (min_left == 0) {
                            goto over_check;
                        }
                    } /* if (..CAN_GO) */
                } /* q++ */
            } /* p++ */
        } /* j++ */
    } /* i++ */

over_check:
    _save_op ();

    return min_left;
}

/*==============================================================================
 * - lian_get_op()
 *
 * - get first coulpe of blocks those can be deleted
 */
int lian_get_op (const int (* map)[COL_NUM+2])
{
    int  i, j, p, q;
    COOR start, end;

    _G_op_cnt = 0;

    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            start.y = i;
            start.x = j;
            
            /* throught current row */
            for (q = start.x + 1; q < COL_NUM + 1; q++) {
                end.y = start.y;
                end.x = q;

                if (lian_check((const int (*)[COL_NUM + 2])map,
                                &start, &end) == CAN_GO) {

                    _G_op_cnt = 1;
                    _G_ops[0].start = start;
                    _G_ops[0].end = end;

                    goto over_check;
                } /* if (..CAN_GO) */
            }

            /* next row to map over */
            for (p = start.y + 1; p < ROW_NUM + 1; p++) {
                for (q = 1; q < COL_NUM + 1; q++) {
                    end.y = p;
                    end.x = q;

                    if (lian_check((const int (*)[COL_NUM + 2])map,
                                    &start, &end) == CAN_GO) {

                        _G_op_cnt = 1;
                        _G_ops[0].start = start;
                        _G_ops[0].end = end;

                        goto over_check;
                    } /* if (..CAN_GO) */
                } /* q++ */
            } /* p++ */
        } /* j++ */
    } /* i++ */

over_check:
    return _G_op_cnt;
}

/*======================================================================
  check whether this map can delete all blocks.          END
======================================================================*/

/*==============================================================================
 * - lian_show()
 *
 * - show_map
 */
int lian_show ()
{
    int i, j;
    /* show the map */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            serial_printf ("%d   ", _G_map[i][j]);
        }
        serial_printf (": y = %d\n", i);
    }
    
    return 0;
}

/*==============================================================================
 * - lian_test()
 *
 * - command operat
 */
int lian_test (int x0, int y0, int x1, int y1)
{
    COOR point0 = {x0, y0};
    COOR point1 = {x1, y1};

    if (lian_check ((const int (*)[COL_NUM + 2])_G_map,
                    &point0, &point1) == CAN_GO) {
        int i = 0;
        for (; i < _G_pos; i++) {
            serial_printf ("{%d, %d}\t", _G_way[i].x, _G_way[i].y);
        }
        serial_printf ("\n-----------------------------\n");

        _G_map[_G_way[0].y][_G_way[0].x] = 0;
        _G_map[_G_way[_G_pos-1].y][_G_way[_G_pos-1].x] = 0;

        lian_show();

        return 1;
    } else {
        serial_printf ("Start: {%d, %d} = %d   End: {%d, %d} = %d ",
        		x0,y0, _G_map[y0][x0], x1,y1, _G_map[y1][x1]);
        serial_printf ("Can't find path.\n");
        return 0;
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

