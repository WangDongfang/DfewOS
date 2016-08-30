/*==============================================================================
** tetris.c -- Tetris core.
**
** MODIFY HISTORY:
**
** 2011-09-24 wdf Create.
==============================================================================*/

#include <stdio.h>         /* prinf() */
#include <stdlib.h>        /* srand(), rand() */

#include "tetris.h"

/*======================================================================
  debug config
======================================================================*/
#define TETRIS_DEBUG      serial_printf

/*======================================================================
  forward function declare
======================================================================*/
static void down_task (TETRIS_CTRL *Tetris_ID);
static void job_task (TETRIS_CTRL *Tetris_ID);
static void auto_task (TETRIS_CTRL *Tetris_ID);
static void new_block (TETRIS_CTRL *Tetris_ID);
static void fix_block (TETRIS_CTRL *Tetris_ID);
static int  delete_this_line (TETRIS_CTRL *Tetris_ID, int y);
static void quit (TETRIS_CTRL *Tetris_ID);
static TETRIS_MSG rb_recv_msg (TETRIS_CTRL *Tetris_ID);

/*======================================================================
  Auto Module Support API for Tetris Module
  This Function according to the current situation find a series of
  steps(left|right, trun motion) stored in <p_steps> array
======================================================================*/
void find_steps (SCREEN *scr, const BLOCK *blk, STEPS *p_steps);

/*==============================================================================
 * - rb_init()
 *
 * - init a Tetris control struct and return it. called by upper layer
 */
void *rb_init (void *Bridge_ID, int column_num, int row_num,
             int (*bri_show_routine)(void *Bridge_ID, COOR *coor, int show))
{
    TETRIS_CTRL *Tetris_ID = NULL;

    /* alloc Tetris_ID */
    Tetris_ID = (TETRIS_CTRL *)malloc (sizeof (TETRIS_CTRL));
    
    if (Tetris_ID == NULL) {
        TETRIS_DEBUG ("There is no memory for <Tetris_ID>!\n");
        return NULL;
    }

    /* init Tetris_ID and alloc scr_map */
    Tetris_ID->Bridge_ID        = Bridge_ID;
    Tetris_ID->bri_show_routine = bri_show_routine;
    Tetris_ID->scr.column_num   = TETERIS_MIN (TETRIS_MAX_COLUMN, column_num);
    Tetris_ID->scr.row_num      = row_num;
    Tetris_ID->scr.scr_map      = (unsigned int *)calloc (row_num * 2, sizeof (unsigned int));
    Tetris_ID->full_mask        = (1 << Tetris_ID->scr.column_num) - 1;
    Tetris_ID->full_line        = 0;
    Tetris_ID->score            = 0;

    if (Tetris_ID->scr.scr_map == NULL) {
        TETRIS_DEBUG ("There is no memory for <scr_map>!\n");
        free (Tetris_ID);
        return NULL;
    }

#ifndef OMIT_NODE_COLOR
    /* alloc node_color_map */
    Tetris_ID->node_color_map = (unsigned char *)calloc (
                            row_num * Tetris_ID->scr.column_num,
                            sizeof (unsigned char));

    if (Tetris_ID->node_color_map == NULL) {
        TETRIS_DEBUG ("There is no memory for <node_color_map>!\n");
        free (Tetris_ID->scr.scr_map);
        free (Tetris_ID);
        return NULL;
    }
#endif

    /* create msgQ */
    Tetris_ID->job_queue = os_msgQ_create (JOB_QUEUE_MAX, sizeof (TETRIS_MSG));

    /* auto mode */
    Tetris_ID->in_auto_mode = 0;
    Tetris_ID->temp_scr_map = Tetris_ID->scr.scr_map + row_num;
    Tetris_ID->auto_task_id = OS_TASK_ID_ERROR;

    /* create the first block */
    srand (os_tick_get());
    new_block (Tetris_ID);

    /* create do job task */
    os_task_create ("tRb_job", 40960, JOB_TASK_PRI, (void *)job_task, (int)Tetris_ID, 2);
    os_task_create ("tRb_dwn", 10240, DOWN_TASK_PRI, (void *)down_task, (int)Tetris_ID, 2);

    /* show score 0 */
    Tetris_ID->bri_show_routine (Tetris_ID->Bridge_ID,
            &Tetris_ID->blk.base,               /* not used */
            Tetris_ID->score | 0x80000000
            );
    return Tetris_ID;
}


/*==============================================================================
 * - is_occupied()
 *
 * - on screen whether this coordinate is occupied
 *
 * - return value
 *   0: not occupied
 *  !0: occupied
 */
static int is_occupied (const SCREEN *p_scr, const COOR *coor)
{
    if (((coor->x >= 0) && (coor->x < p_scr->column_num)) &&
        ((coor->y >= 0) && (coor->y < p_scr->row_num))) {
        return (BIT_VAL(p_scr->scr_map[coor->y], coor->x));
    } else {
        return 1;
    }
}

/*==============================================================================
 * - update_block()
 *
 * - erase of show a block
 */
static void update_block (TETRIS_CTRL *Tetris_ID, int show)
{
    int   i;
    COOR  coor;
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        coor.x = COOR_X(i);
        coor.y = COOR_Y(i);

        /* TETRIS_DEBUG("coor x = %d y = %d\n", coor.x, coor.y); */
        Tetris_ID->bri_show_routine (Tetris_ID->Bridge_ID, &coor, show);
    }
}

/*==============================================================================
 * - left()
 *
 * - try to move a block to left
 *
 * - return value
 *   MOTION_STATUS_OK:    ok
 *   MOTION_STATUS_ERROR: can't move
 */
static int left (TETRIS_CTRL *Tetris_ID)
{
    int   i;
    COOR  coor;

    /* make sure blk every node's new position is no occupied */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {

        coor.x = COOR_X(i) - 1;
        coor.y = COOR_Y(i);

        if (is_occupied (&Tetris_ID->scr, &coor)) {
            return MOTION_STATUS_ERROR;
        }
    }

    /* can move left, erase old show new */
    update_block (Tetris_ID, 0);
    Tetris_ID->blk.base.x--;
    update_block (Tetris_ID, _G_color_map[Tetris_ID->blk.type]);

    return MOTION_STATUS_OK;
}


/*==============================================================================
 * - right()
 *
 * - try to move a block to right
 *
 * - return value
 *   MOTION_STATUS_OK:    ok
 *   MOTION_STATUS_ERROR: can't right
 */
static int right (TETRIS_CTRL *Tetris_ID)
{
    int   i;
    COOR coor;

    /* make sure every node is't occypied */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {

        coor.x = COOR_X(i) + 1;
        coor.y = COOR_Y(i);

        if (is_occupied (&Tetris_ID->scr, &coor)) {
            return MOTION_STATUS_ERROR;
        }
    }

    /* can move right, erase old show new */
    update_block (Tetris_ID, 0);
    Tetris_ID->blk.base.x++;
    update_block (Tetris_ID, _G_color_map[Tetris_ID->blk.type]);

    return MOTION_STATUS_OK;
}

/*==============================================================================
 * - down()
 *
 * - try to move a block to down
 *
 * - return value
 *   MOTION_STATUS_OK:        ok
 *   MOTION_STATUS_ERROR:     can't down 
 *   MOTION_STATUS_GAME_OVER: game over
 */
static int down (TETRIS_CTRL *Tetris_ID)
{
    int  i;
    COOR coor;

    /* make sure every node is't occypied */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {

        coor.x = COOR_X(i);
        coor.y = COOR_Y(i) + 1;

        if (is_occupied (&Tetris_ID->scr, &coor)) {
            if (Tetris_ID->blk.base.y > 0) { /* to the bottom and not game over */
                fix_block (Tetris_ID);
                new_block (Tetris_ID);
                return MOTION_STATUS_ERROR;
            } else { /* there is no space fo a new block, so game over */
                return MOTION_STATUS_GAME_OVER;
            }
        }
    }

    /* can move down, erase old show new */
    update_block (Tetris_ID, 0);
    Tetris_ID->blk.base.y++;
    update_block (Tetris_ID, _G_color_map[Tetris_ID->blk.type]);

    return MOTION_STATUS_OK;
}

/*==============================================================================
 * - bottom()
 *
 * - try to send a block to bottom
 *
 * - return value
 *   MOTION_STATUS_ERROR:     go to the bottom and new a block
 *   MOTION_STATUS_GAME_OVER: can't go to the bottom, becase current is at
 */
static int bottom (TETRIS_CTRL *Tetris_ID)
{
    int  i;
    COOR coor;
    int  status = MOTION_STATUS_OK;

    /* erase old block */
    update_block (Tetris_ID, 0);

    while (status == MOTION_STATUS_OK) {

        /* make sure every node is't occypied */
        for (i = 0; i < BLOCK_NODE_NUM; i++) {

            coor.x = COOR_X(i);
            coor.y = COOR_Y(i) + 1;

            if (is_occupied (&Tetris_ID->scr, &coor)) {
                if (Tetris_ID->blk.base.y > 0) { /* to the bottom and not game over */
                    status = MOTION_STATUS_ERROR;
                    break; /* break for (...) */
                } else { /* there is no space fo a new block, so game over */
                    status = MOTION_STATUS_GAME_OVER;
                    break; /* break for (...) */
                }
            }
        }

        if (status == MOTION_STATUS_OK) {
            Tetris_ID->blk.base.y++;
        } else if (status == MOTION_STATUS_ERROR) {
            /* show block at new position */
            update_block (Tetris_ID, _G_color_map[Tetris_ID->blk.type]);
            fix_block (Tetris_ID);
            new_block (Tetris_ID);
        }
    }

    return status;
}

/*==============================================================================
 * - turn()
 *
 * - try to change a block from current type to another
 *
 * - return value
 *   MOTION_STATUS_OK:    ok
 *   MOTION_STATUS_ERROR: can't spin it round
 */
static int turn (TETRIS_CTRL *Tetris_ID)
{
    int        i;
    BLOCK_TYPE new_type;
    COOR       coor;

    /* get a new type */
    switch (Tetris_ID->blk.type) {
        case O_0:         /* O */
            return MOTION_STATUS_OK;
        case I_1:         /* I */
            new_type = I_0;
            break;
        case Z_1:         /* Z */
            new_type = Z_0;
            break;
        case S_1:         /* S */
            new_type = S_0;
            break;
        case T_3:         /* T */
            new_type = T_0;
            break;
        case J_3:         /* J */
            new_type = J_0;
            break;
        case L_3:         /* L */
            new_type = L_0;
            break;
        default:
            new_type = Tetris_ID->blk.type + 1;
            break;
    }

    /* check whether the new type is ok */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {

        coor.x = Tetris_ID->blk.base.x + _G_block_map[new_type].pos[i].x;
        coor.y = Tetris_ID->blk.base.y + _G_block_map[new_type].pos[i].y;

        if (is_occupied (&Tetris_ID->scr, &coor)) {
            return MOTION_STATUS_ERROR;
        }
    }

    /* erase old block and show new block */
    update_block (Tetris_ID, 0);
    Tetris_ID->blk.type = new_type;
    update_block (Tetris_ID, _G_color_map[Tetris_ID->blk.type]);

    return MOTION_STATUS_OK;
}

/*==============================================================================
 * - new_block()
 *
 * - create a new block on screen middle top
 */
static void new_block (TETRIS_CTRL *Tetris_ID)
{
    Tetris_ID->blk.base.x = Tetris_ID->scr.column_num / 2 - 1;
    Tetris_ID->blk.base.y = 0;
    Tetris_ID->blk.type = rand() % BLOCK_TYPE_NUM;

    /* TETRIS_DEBUG("new block x = %d blk.type = %d\n", Tetris_ID->blk.base.x, Tetris_ID->blk.type); */
    /* 
     * only not in auto mode, clear the message queue
     * because if in auto mode, maybe flush TETRIS_MSG_AUTO message
     */
    if (Tetris_ID->in_auto_mode == 0) {
        os_msgQ_flush (Tetris_ID->job_queue);
    }

    /* show the new block */
    update_block (Tetris_ID, _G_color_map[Tetris_ID->blk.type]);

    /* start auto algorithm task if in auto mode */
    if (Tetris_ID->in_auto_mode) {
        if (Tetris_ID->auto_task_id != OS_TASK_ID_ERROR) {
            os_task_delete (Tetris_ID->auto_task_id);
        }

        Tetris_ID->auto_task_id = 
        /* if can't spawn, os_task_create() return OS_TASK_ID_ERROR */
        os_task_create ("tRb_auto", 40960, AUTO_TASK_PRI, (void *)auto_task, (int)Tetris_ID, 2);
    }
}

/*==============================================================================
 * - fix_node()
 *
 * - fix a node in scr map
 *
 * - return value:
 *    -1: this line not full
 *   >=0: this line is full and return y coordinate
 */
static int fix_node (TETRIS_CTRL *Tetris_ID, const COOR *coor)
{
    BIT_SET (Tetris_ID->scr.scr_map[coor->y], coor->x);
#ifndef OMIT_NODE_COLOR
    Tetris_ID->node_color_map[coor->y * Tetris_ID->scr.column_num + coor->x] =
                                _G_color_map[Tetris_ID->blk.type];
#endif

    if (Tetris_ID->scr.scr_map[coor->y] == Tetris_ID->full_mask) {
        Tetris_ID->full_line++;
        /* TETRIS_DEBUG("in fix_node() *** full_line = %d",Tetris_ID->full_line); */
        /* TETRIS_DEBUG("    x = %d, y = %d\n",coor->x, coor->y); */
        return coor->y;
    }

    return -1;
}

/*==============================================================================
 * - fix_block()
 *
 * - when a block down to bottom call me
 */
static void fix_block (TETRIS_CTRL *Tetris_ID)
{
    int  i;
    COOR coor;
    int  bottom_full_line = -1;
    int  fix_line;

    /* find the bottomest line that is full */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        coor.x = COOR_X(i);
        coor.y = COOR_Y(i);
        /* TETRIS_DEBUG("fix node(%d)\n", i); */
        fix_line = fix_node (Tetris_ID, &coor);
        bottom_full_line = TETERIS_MAX(fix_line, bottom_full_line);
    }

    /* TETRIS_DEBUG("bottom_full_line = %d\n", bottom_full_line); */

    /* delete the full line(s) */
    if (Tetris_ID->full_line != 0) {
        Tetris_ID->score += _G_score_map[Tetris_ID->full_line];
        /* show score */
        /* TETRIS_DEBUG("full_line = %d  score = %d\n",Tetris_ID->full_line, Tetris_ID->score); */
        Tetris_ID->bri_show_routine (Tetris_ID->Bridge_ID,
                                     &coor,               /* not used */
                                     Tetris_ID->score | 0x80000000  /* show score */
                                     );

        for (i = bottom_full_line; i >= 0; i--) {
            if (Tetris_ID->full_line == 0) {
                break;
            }

            if (Tetris_ID->scr.scr_map[i] != Tetris_ID->full_mask) {
                continue;
            } else {
                delete_this_line(Tetris_ID, i);
            }
        }

    }
}

/*==============================================================================
 * - update_line()
 *
 * - show one line in new value
 */
static int update_line (TETRIS_CTRL *Tetris_ID, int y, unsigned int new_val
#ifndef OMIT_NODE_COLOR
        , unsigned char *new_colors
#endif
        )
{
    int  i;
    COOR coor;

    for (i = 0; i < Tetris_ID->scr.column_num; i++) {
#ifndef OMIT_NODE_COLOR
        coor.x = i;
        coor.y = y;
        if (new_colors != NULL) {
            Tetris_ID->node_color_map[y * Tetris_ID->scr.column_num + i] = new_colors[i];
        } else {
            Tetris_ID->node_color_map[y * Tetris_ID->scr.column_num + i] = NO_COLOR;
        }
        Tetris_ID->bri_show_routine (Tetris_ID->Bridge_ID, &coor,
                Tetris_ID->node_color_map[y * Tetris_ID->scr.column_num + i]);
#else
        if (BIT_VAL(Tetris_ID->scr.scr_map[y], i) != BIT_VAL(new_val, i)) {
            coor.x = i;
            coor.y = y;

            Tetris_ID->bri_show_routine (Tetris_ID->Bridge_ID, &coor, BIT_VAL(new_val, i));
        }
#endif
    }

    Tetris_ID->scr.scr_map[y] = new_val;
    
    return 0;
}

/*==============================================================================
 * - delete_this_line()
 *
 * - make current line is not full
 */
static int delete_this_line (TETRIS_CTRL *Tetris_ID, int y)
{
    int i = y;

    /* fall one line */
    for (i = y; i > 0; i--) {
        update_line (Tetris_ID, i, Tetris_ID->scr.scr_map[i - 1]
#ifndef OMIT_NODE_COLOR
                , Tetris_ID->node_color_map + (i-1) * Tetris_ID->scr.column_num
#endif
                );

        if (Tetris_ID->scr.scr_map[i] == 0) {
            break;
        }
    }
    if (i == 0) {
        update_line (Tetris_ID, i, 0
#ifndef OMIT_NODE_COLOR
                , NULL
#endif
                );
    }

    Tetris_ID->full_line--;

    if (Tetris_ID->scr.scr_map[y] == Tetris_ID->full_mask) {
        delete_this_line (Tetris_ID, y);
    }
    
    return 0;
}

/*==============================================================================
 * - down_task()
 *
 * - a task that send TETRIS_MSG_DOWN regularly
 */
static void down_task (TETRIS_CTRL *Tetris_ID)
{
    TETRIS_FOREVER {
        if (rb_send_msg (Tetris_ID, TETRIS_MSG_DOWN) == SEND_GAME_OVER) {
            break;
        }
        os_task_delay (1000 / BLOCK_SPEED);
    }
}

/*==============================================================================
 * - job_task()
 *
 * - a task that receive TETRIS_MSG and process
 */
static void job_task (TETRIS_CTRL *Tetris_ID)
{
    TETRIS_MSG msg_job;
    int hold_on = 0;

    TETRIS_FOREVER {
        msg_job = rb_recv_msg (Tetris_ID);

        switch (msg_job) {
            case TETRIS_MSG_LEFT:
                left (Tetris_ID);
                break;
            case TETRIS_MSG_RIGHT:
                right (Tetris_ID);
                break;
            case TETRIS_MSG_TURN:
                turn (Tetris_ID);
                break;
            case TETRIS_MSG_DOWN:
                if (hold_on) { break; }
                /* let's try to down the block */
                if (down (Tetris_ID) == MOTION_STATUS_GAME_OVER) {
                    /* TETRIS_DEBUG("Motion Down Game Over!\n"); */
                    goto game_over;
                }
                break;
            case TETRIS_MSG_BOTTOM:
                if (hold_on) { break; }
                /* let's try to send the block to bottom */
                if (bottom (Tetris_ID) == MOTION_STATUS_GAME_OVER) {
                    /* TETRIS_DEBUG("Motion Down Game Over!\n"); */
                    goto game_over;
                }
                break;
            case TETRIS_MSG_AUTO:
                Tetris_ID->in_auto_mode = 1 - Tetris_ID->in_auto_mode;
                break;
            case TETRIS_MSG_HOLD:
                hold_on = 1 - hold_on;
                break;
            case TETRIS_MSG_QUIT:
                goto game_over;
            default:
                break;
        }
    }

game_over:
    quit (Tetris_ID);
}

/*==============================================================================
 * - auto_task()
 *
 * - when a new block is create may start this task auto send message
 */
static void auto_task (TETRIS_CTRL *Tetris_ID)
{
    int    i;
    SCREEN temp_scr;
    BLOCK  temp_blk;
    STEPS  steps;

    /* copy current screen map to temp_scr_map for auto algorithm to use */
    memcpy (Tetris_ID->temp_scr_map, Tetris_ID->scr.scr_map,
            sizeof(unsigned int) * Tetris_ID->scr.row_num);

    /* init auto used screen struct */
    temp_scr.scr_map    = Tetris_ID->temp_scr_map;
    temp_scr.column_num = Tetris_ID->scr.column_num;
    temp_scr.row_num    = Tetris_ID->scr.row_num;

    /* init auto algorithm used block struct */
    temp_blk = Tetris_ID->blk;

    /* init auto alogrithm used step struct */
    steps.step_num = 0;

    /* auto algorithm */
    find_steps (&temp_scr, &temp_blk, &steps);

    /* post the steps to job queue */
    for (i = 0; i < steps.step_num; i++) {
        if(rb_send_msg (Tetris_ID, steps.msgs[i]) == SEND_GAME_OVER)  {
        	break;
        }
    }

    /* sign auto task is over */
    Tetris_ID->auto_task_id = OS_TASK_ID_ERROR;
}

/*==============================================================================
 * - quit()
 *
 * - game over
 */
static void quit (TETRIS_CTRL *Tetris_ID)
{
    /* free scr_map */
    free (Tetris_ID->scr.scr_map);
#ifndef OMIT_NODE_COLOR
    free (Tetris_ID->node_color_map);
#endif

    /* if auto task is runing, delete it */
    if (Tetris_ID->auto_task_id != OS_TASK_ID_ERROR) {
        /* TETRIS_DEBUG ("Delete auto_task.\n"); */
        os_task_delete (Tetris_ID->auto_task_id);
    }

    /* delete job_queue */
    os_msgQ_delete (Tetris_ID->job_queue);
    Tetris_ID->job_queue = NULL;

    /* free Tetris module control struct */
    /* TETRIS_DEBUG ("Tetris_ID = %p\n", Tetris_ID); */
    free (Tetris_ID);
}

/*==============================================================================
 * - rb_send_msg()
 *
 * - send a message to job queue
 *
 * -        SEND_OK: send to the message queue success
 *     SEND_TIMEOUT: the message queue is full no sapce for new message
 *   SEND_GAME_OVER: the message queue is delete by job_task
 */
int rb_send_msg (void *Tetris_ID, TETRIS_MSG msg)
{
    SEND_MSG_STATUS  status = SEND_OK;

    int os_send_ret = os_msgQ_send (((TETRIS_CTRL *)Tetris_ID)->job_queue,
                                     (char *)&msg, sizeof (TETRIS_MSG));
    if (os_send_ret == -1) {
        status = SEND_TIMEOUT;
    } else if (os_send_ret == -2) {
        status = SEND_GAME_OVER;
    }
        
    return status;
}

/*==============================================================================
 * - rb_recv_msg()
 *
 * - receive message from job queue. Only used by job_task
 */
static TETRIS_MSG rb_recv_msg (TETRIS_CTRL *Tetris_ID)
{
    TETRIS_MSG msg_job;

    os_msgQ_receive (Tetris_ID->job_queue, (char *)&msg_job, sizeof (TETRIS_MSG));

    return msg_job;
}

/*==============================================================================
** FILE END
==============================================================================*/

