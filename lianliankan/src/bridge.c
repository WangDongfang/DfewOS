/*==============================================================================
** bridge.c -- tty and lian moudle bridge.
**
** MODIFY HISTORY:
**
** 2011-10-13 wdf Create.
==============================================================================*/
#include "lian.h"
#include "tty.h"

#ifdef OS_DFEWOS /* OS_DFEWOS */
#include <dfewos.h>
extern int sprintf (char *buffer, const char *fmt, ...);

static  OS_TCB  *_G_time_taskid;
#define OS_CREATE_TIME_TASK  _G_time_taskid = task_create ("tLianTime", 4096, 200, _T_time, 0, 0);
#define OS_DELETE_TIME_TASK  if (_G_time_taskid != NULL) { task_delete (_G_time_taskid); }
#define OS_TIME_TASK_OVER    _G_time_taskid = NULL;
#define OS_DELAY_ONE_SECOND  delayQ_delay (SYS_CLK_RATE);

#else            /* OS_VXWORKS */

#include <vxWorks.h>
#include <taskLib.h>
#include <sysLib.h>
#include <stdio.h>

static  int  _G_time_taskid;
#define OS_CREATE_TIME_TASK  _G_time_taskid = taskSpawn ("tLianTime", 200, 0, 4096, \
                                     (FUNCPTR)_T_time, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0);
#define OS_DELETE_TIME_TASK  if (_G_time_taskid != ERROR) { taskDelete (_G_time_taskid); }
#define OS_TIME_TASK_OVER    _G_time_taskid = ERROR;
#define OS_DELAY_ONE_SECOND  taskDelay (sysClkRateGet());
#endif           /* OS_DEFWOS */

/*======================================================================
  configs
======================================================================*/
#define ONE_LEVEL_TIME  200
#define DELAY_TIME   100000

#define MOVE_BEEP_TIMES  1
#define MARK_BEEP_TIMES  1
#define EXIT_BEEP_TIMES  3

/*======================================================================
  Global variables
======================================================================*/
static COOR _G_cur    = {0, 0}; /* The block cursor on */
static COOR _G_marked = {0, 0}; /* User have marked block */
static int  _G_left   = 0;      /* The left blocks number */
static int  _G_level  = 0;      /* Current Level */
static int  _G_time   = 0; /* because two tasks access, it should be
                              atomic_t type in vxworks. but dfewos not
                              support this type, so I am ostrich */

/*======================================================================
  functions forward declare
======================================================================*/
static int  _start_level (int level);
static int  _delete_matched ();
static void _make_cur (int start_row, int start_col);
static void _leave (COOR *coor);
static void _delay (int delay_time);
static void _T_time ();

/*==============================================================================
 * - bridge_init()
 *
 * - init lian moudle and bridge global variables
 */
int bridge_init ()
{
    _G_level = 0;

    if (_start_level(_G_level) <= 0) {
        return -1;
    }

    _G_time = ONE_LEVEL_TIME;
    /* create count time task */
    OS_CREATE_TIME_TASK;

    return _G_left;
}

/*==============================================================================
 * - bridge_next()
 *
 * - move cursor to next block
 */
void bridge_next ()
{
    /* check time */
    if (_G_time <= 0) return;

    tty_beep (MOVE_BEEP_TIMES);

    _make_cur (_G_cur.y, _G_cur.x + 1);
}

/*==============================================================================
 * - bridge_move()
 *
 * - move cursor
 */
int bridge_move (int dir)
{
    int i_x;
    int i_y;

    int interval = 0;

    /* check time */
    if (_G_time <= 0) return 0;

    switch (dir) {
        case LEFT:
            i_x = _G_cur.x;
            while (_G_map[_G_cur.y][--i_x] == 0) ;
            if (i_x != 0) {
                interval = _G_cur.x - i_x;
                _leave (&_G_cur);
                _G_cur.x = i_x;
                tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MOVE);
            }
            break;
        case UP:
            i_y = _G_cur.y;
            while (_G_map[--i_y][_G_cur.x] == 0) ;
            if (i_y != 0) {
                interval = _G_cur.y - i_y;
                _leave (&_G_cur);
                _G_cur.y = i_y;
                tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MOVE);
            }
            break;
         case RIGHT:
            i_x = _G_cur.x;
            while (_G_map[_G_cur.y][++i_x] == 0) ;
            if (i_x != COL_NUM + 1) {
                interval = i_x - _G_cur.x;
                _leave (&_G_cur);
                _G_cur.x = i_x;
                tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MOVE);
            }
            break;
         case DOWN:
            i_y = _G_cur.y;
            while (_G_map[++i_y][_G_cur.x] == 0) ;
            if (i_y != ROW_NUM + 1) {
                interval = i_y - _G_cur.y;
                _leave (&_G_cur);
                _G_cur.y = i_y;
                tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MOVE);
            }
            break;

        default:
            break;
    }

    /* beep alarm */
    if (interval > 0) {
        tty_beep (MOVE_BEEP_TIMES);
    }

    return interval;
}

/*==============================================================================
 * - bridge_exit()
 *
 * - exit game. delete the count time task
 */
void bridge_exit ()
{
    OS_DELETE_TIME_TASK;

    _G_time = 0;

    tty_beep (EXIT_BEEP_TIMES);
}

/*==============================================================================
 * - bridge_mark()
 *
 * - mark a block, if this is second mark, try to delete the two blocks
 */
int bridge_mark ()
{
    /* check time */
    if (_G_time <= 0) return UNMARKED;

    if (_G_marked.x == 0) { /* first mark */
        tty_beep (MARK_BEEP_TIMES);
        tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MARK);
        _G_marked = _G_cur;
        return MARKED;
    } else {             /* second mark */
    	int i;
        if ((_G_marked.x != _G_cur.x) || (_G_marked.y != _G_cur.y)) { /* not equal first coor */
            if (lian_check ((const int (*)[COL_NUM + 2])_G_map,
                            &_G_marked, &_G_cur) == CAN_GO) { /* fisrt can reach second */
                
                tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MARK);

                _G_left = _delete_matched ();

                /* there is no block is left */
                if (_G_left == 0) {
                    if ((++_G_level) < LEVEL_NUM) {
                        _G_time += ONE_LEVEL_TIME; /* add left time to new level */

                        _G_left = _start_level (_G_level); /* start next level */
                        if (_G_left <= 0) { /* this level map have error */
                            bridge_exit ();
                            return GAME_OVER;
                        }
                    } else {    /* OH,Yeah! Go through all level */
                        char str[100];
                        sprintf (str, "Congratulation! Your Score: %d", _G_time);
                        tty_msg (str);

                        bridge_exit ();
                        return GAME_OVER;
                    }
                } else { /* there is still some blocks left */

                    /* no marked */
                    _G_marked.x = 0;

                    /* move current cusor */
                    for (i = LEFT; i <= DOWN; i++) {
                        if (bridge_move (i) != 0) {
                            break;
                        }
                    }
                    /* cur go to begin */
                    if (i > DOWN) {
                        _make_cur (1, 1); /* from the first block start */
                    }
                }

                return UNMARKED;
            } else { /* fisrt can't reach second */
                return MARKED;
            }
        } else { /* equal first coor */
            tty_beep (MARK_BEEP_TIMES);
            /* cancel the mark */
            tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MOVE);
            _G_marked.x = 0;
            /* no marked */
            return UNMARKED;
        }
    }
}

/*==============================================================================
 * - bridge_help()
 *
 * - find a couple of blocks to delete
 */
int bridge_help ()
{
    /* check left blocks */
    if (_G_left <= 0) return 0;

    _G_op_cnt = 0;

    if (lian_left (_G_map, _G_left) == 0) { /* This Map is Good */
        _leave (&_G_cur);
        _G_marked = _G_ops[0].start;
        _G_cur    = _G_ops[0].end;
        bridge_mark ();
        return 1;
    } else {                                /* This Map is Bad */
        if (_G_op_cnt == 0) { /* can't find a couple to delete */
            tty_msg ("  You are DEAD  ");
        } else {              /* can delete but not all */
            tty_msg ("  I am trying!  ");
            _leave (&_G_cur);
            _G_marked = _G_ops[0].start;
            _G_cur    = _G_ops[0].end;
            bridge_mark ();
        }
        OS_DELAY_ONE_SECOND;
        OS_DELAY_ONE_SECOND;
        OS_DELAY_ONE_SECOND;

        return 0;
    }
}

/*==============================================================================
 * - _start_level()
 *
 * - start a new level
 */
static int _start_level (int level)
{
    int i, j;

    _G_left = lian_init (level); /* start level 0 */
    if (_G_left <= 0) {
        return _G_left;
    }

    _G_marked.x = 0;        /* no block is marked */
    _G_cur.x = 0;

    /* find the first block */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_map[i][j] != 0) {
                if (_G_cur.x == 0) {
                    _G_cur.x = j;
                    _G_cur.y = i;
                }

                /* show */
                tty_show (i, j, _G_map[i][j], ACT_SHOW);
            }
        }
    }

    /* move to cur */
    tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MOVE);

    return _G_left;
}

/*==============================================================================
 * - _delete_matched()
 *
 * - delete the matched two block <_G_marked> -- <_G_cur>
 */
static int _delete_matched ()
{
    int i;

    /* draw line */
    for (i = 0; i < _G_pos - 1; i++) {
        tty_line (_G_way[i].y, _G_way[i].x, _G_way[i+1].y, _G_way[i+1].x,
                _G_map[_G_cur.y][_G_cur.x]);
    }
    tty_beep (1);
    _delay (DELAY_TIME); /* delay */

    tty_show (_G_cur.y, _G_cur.x, 0, ACT_SHOW); /* hide */
    tty_show (_G_marked.y, _G_marked.x, 0, ACT_SHOW);
    tty_beep (1);
    _delay (DELAY_TIME); /* delay */

    tty_show (_G_cur.y, _G_cur.x, _G_map[_G_cur.y][_G_cur.x], ACT_MARK); /* show */
    tty_show (_G_marked.y, _G_marked.x, _G_map[_G_marked.y][_G_marked.x], ACT_MARK);
    tty_beep (1);
    _delay (DELAY_TIME); /* delay */

    tty_show (_G_cur.y, _G_cur.x, 0, ACT_SHOW); /* hide */
    tty_show (_G_marked.y, _G_marked.x, 0, ACT_SHOW);
    /* hide line */
    for (i = 0; i < _G_pos - 1; i++) {
        tty_line (_G_way[i].y, _G_way[i].x, _G_way[i+1].y, _G_way[i+1].x, 0);
    }

    /* clear */
    _G_map[_G_cur.y][_G_cur.x] = 0;
    _G_map[_G_marked.y][_G_marked.x] = 0;

    _G_left -= 2;

    return _G_left;
}

/*==============================================================================
 * - _make_cur()
 *
 * - start <start_row> <start_col> find a not 0 block, set current cursor
 */
static void _make_cur (int start_row, int start_col)
{
    int i, j;

#define _FIND_BLOCK() \
        if (_G_map[i][j] != 0) { \
            _leave (&_G_cur);    \
            _G_cur.x = j;        \
            _G_cur.y = i;        \
            tty_show (i, j, _G_map[i][j], ACT_MOVE); \
            return ;\
        }
    
    /* forward find the first block */
    i = start_row;
    for (j = start_col; j < COL_NUM + 1; j++) {
        _FIND_BLOCK()
    }
    for (i = start_row + 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            _FIND_BLOCK();
        }
    }
    
    /* backward find the first block */
    for (i = 1; i < start_row; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            _FIND_BLOCK();
        }
    }
    i = start_row;
    for (j = 1; j < start_col; j++) {
        _FIND_BLOCK()
    }
}

/*==============================================================================
 * - _leave()
 *
 * - cursor leave a block
 */
static void _leave (COOR *coor)
{
    if ((coor->x != _G_marked.x) || (coor->y != _G_marked.y)) {
        tty_show (coor->y, coor->x, _G_map[coor->y][coor->x], ACT_SHOW);
    } else {
        tty_show (coor->y, coor->x, _G_map[coor->y][coor->x], ACT_MARK);
    }
}

/*==============================================================================
 * - _delay()
 *
 * - delay some time for user
 */
static void _delay (int delay_time)
{
    volatile int i;
    volatile int j;
    for (i = 0; i < 100; i++) {
        j = delay_time;
        while (j--);
    }
}

/*==============================================================================
 * - _T_time()
 *
 * - the count time task
 */
static void _T_time ()
{
    char str[100];

    while (_G_time >= 0) {
        sprintf (str, "You Time : %4d", _G_time);
        tty_msg (str);
        OS_DELAY_ONE_SECOND;
        _G_time--;
    }
    OS_TIME_TASK_OVER;

    tty_msg ("    Game Over!    ");
}

/*==============================================================================
** FILE END
==============================================================================*/

