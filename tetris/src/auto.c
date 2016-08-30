/*==============================================================================
** auto.c -- The Algorithm for Auto find Path.
**
** MODIFY HISTORY:
**
** 2011-09-25 wdf Create.
==============================================================================*/

#include <stdio.h>
#include "tetris.h"

/*======================================================================
  debug config
======================================================================*/
#define AUTO_DEBUG_LEVEL             0  /* dig how much deep: 0 meter */
#define AUTO_DEBUG(level, fmt, ...)  if (level <= AUTO_DEBUG_LEVEL) { \
                                        printf(fmt, ##__VA_ARGS__); \
                                     }

/*======================================================================
  functions forward declare
======================================================================*/
static int get_score (SCREEN *scr, const BLOCK *blk);
static int get_type_column_num (BLOCK_TYPE type);
static int get_height (const SCREEN *scr);
static int get_turn_num (BLOCK_TYPE *p_type);
static int cal_all_holes_in_scr (const SCREEN *scr);
#if 0
static int cal_holes_under_block_columns (const SCREEN *scr, const BLOCK *blk);
#endif
static int cal_holes_in_one_column (const SCREEN *scr, int col);
static int min_interval_between_blk_scr (const SCREEN *scr, const BLOCK *blk);

/*==============================================================================
 * - find_steps()
 *
 * - The Algorigthm enter point
 */
void find_steps (SCREEN *scr, const BLOCK *blk, STEPS *p_steps)
{
	int    temp_score;              /* store current type score */
    int    max_score = 0x80000000;  /* init as minimal integer */

    int    turn_num;        /* this blk can turn times */
    int    block_col_num;   /* temp store blk col num */

    int    max_base_x;
    int    max_turn_type;

    BLOCK  temp_blk = *blk;
    int    i;

    BLOCK_TYPE best_type = blk->type;
    int        best_base_x = blk->base.x;

    /* 
     * change <temp_blk.type> to it's group first type
     * and get the times this type can max turn
     */
    turn_num = get_turn_num (&temp_blk.type);
    max_turn_type = temp_blk.type + turn_num;

    AUTO_DEBUG(2,"row_num = %d  turn_num = %d\n", scr->row_num, turn_num);

    /* for each type at each column, find max score */
    for (; temp_blk.type < max_turn_type; temp_blk.type++) {

        AUTO_DEBUG(2,"===================== type = %d ====================\n", temp_blk.type);

        block_col_num = get_type_column_num (temp_blk.type);
        max_base_x = scr->column_num - block_col_num + 1;

        for (temp_blk.base.x = 0; temp_blk.base.x < max_base_x; temp_blk.base.x++) {
            AUTO_DEBUG(2,"----- base_x = %d -----\n", temp_blk.base.x);
            temp_score = get_score (scr, &temp_blk);
            AUTO_DEBUG(2," ====> score = %d\n", temp_score);
            if (max_score < temp_score) {
                max_score = temp_score;

                best_type   = temp_blk.type;
                best_base_x = temp_blk.base.x;

            }
        }
    }

    /* 
     * calculate steps
     */
    /* press TURN msg */
    if (best_type < blk->type) {
        best_type += turn_num;
    }
    for (i = blk->type; i < best_type; i++) {
        p_steps->msgs[p_steps->step_num++] = TETRIS_MSG_TURN;
    }

    /* press LEFT|RIGHT msg */
    if (blk->base.x < best_base_x) {
        for (i = blk->base.x; i < best_base_x; i++) {
            p_steps->msgs[p_steps->step_num++] = TETRIS_MSG_RIGHT;
        }
    } else {
        for (i = best_base_x; i < blk->base.x; i++) {
            p_steps->msgs[p_steps->step_num++] = TETRIS_MSG_LEFT;
        }
    }

    /* press BOTTOM msg */
    p_steps->msgs[p_steps->step_num++] = TETRIS_MSG_BOTTOM;

}


/*==============================================================================
 * - get_score()
 *
 * - score when a <type> block based <base_x> in <scr->scr_map>
 */
static int get_score (SCREEN *scr, const BLOCK *blk)
{
    int score = 0x80000000; /* min integer value */
    int full_line = 0;
    int space_to_bottom;
    int hole_num;
    int full_mask = ((1 << scr->column_num) - 1);
    int col, row;
    int i;
    int height;

#if 0
    hole_num = cal_holes_under_block_columns (scr, blk);
#else
    hole_num = cal_all_holes_in_scr (scr);
#endif

    /* fix the block and cal full lines */
    space_to_bottom = min_interval_between_blk_scr (scr, blk);
    if (space_to_bottom < 0) {
        goto cal_score_over;
    }

    AUTO_DEBUG(3,"space_to_bottom = %d  ", space_to_bottom);
    /* fix blk and change scr_map, we will restore soon */
    AUTO_DEBUG(3,"change row: ");
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        col = blk->base.x + _G_block_map[blk->type].pos[i].x;
        row = blk->base.y + _G_block_map[blk->type].pos[i].y + space_to_bottom;

        AUTO_DEBUG(3,"%d ", row);

        BIT_SET(scr->scr_map[row], col);

        if (scr->scr_map[row] == full_mask) {
            full_line++;
        }
    }

    /* calculate full line affect score */
    AUTO_DEBUG(2,"full line(s): %d  ", full_line);
    score = _G_score_map[full_line];

    /* calculate space_to_bottom affect score */
    score += space_to_bottom * 2;

    /* calculate height affect score */
#define _DANGERIOUS_HEIGHT(rows)    (rows * 2 / 10)
#define _DANGERIOUS_MINUS            2
    height = get_height (scr) - full_line;
    AUTO_DEBUG(2,"height: %d  ", height);
    if (height > _DANGERIOUS_HEIGHT(scr->row_num)) {
        score -= (height - _DANGERIOUS_HEIGHT(scr->row_num)) * _DANGERIOUS_MINUS;
    }

    /* calculate holes affect score */
#define _HOLE_MINUS    5
#if 0
    hole_num = cal_holes_under_block_columns (scr, blk) - hole_num;
#else
    hole_num = cal_all_holes_in_scr (scr) - hole_num;
#endif
    AUTO_DEBUG(2,"add hole(s): %d  ", hole_num);
    score -= hole_num * _HOLE_MINUS;

    /* restore for next get score */
    AUTO_DEBUG(3,"  <--->   ");
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        col = blk->base.x + _G_block_map[blk->type].pos[i].x;
        row = blk->base.y + _G_block_map[blk->type].pos[i].y + space_to_bottom;

        AUTO_DEBUG(3,"%d ", row);

        BIT_CLR(scr->scr_map[row], col);
    }
    AUTO_DEBUG(2," : row restore\n");

cal_score_over:
    return score;
}

/*==============================================================================
 * - get_type_column_num()
 *
 * - return the specially block type has column number
 */
static int get_type_column_num (BLOCK_TYPE type)
{
    int i;
    int col_num = 0;
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        col_num = TETERIS_MAX (col_num, _G_block_map[type].pos[i].x);
    }
    col_num++;

    return col_num;
}

#if 0
/*==============================================================================
 * - cal_holes_under_block_columns()
 *
 * - when a block is down, cal current holes under it
 */
static int cal_holes_under_block_columns (const SCREEN *scr, const BLOCK *blk)
{
    int hole_num = 0;
    int col = blk->base.x;
    int end_col = blk->base.x + get_type_column_num(blk->type);

    for (; col < end_col; col++) {
        hole_num += cal_holes_in_one_column (scr, col);
    }

    return hole_num;
}
#endif

/*==============================================================================
 * - cal_all_holes_in_scr()
 *
 * - all holes
 */
static int cal_all_holes_in_scr (const SCREEN *scr)
{
    int i;
    int hole_num = 0;

    for (i = 0; i < scr->column_num; i++) {
        hole_num += cal_holes_in_one_column(scr, i);
    }
    
    return hole_num;
}

/*==============================================================================
 * - cal_holes_in_one_column()
 *
 * - calculate holes in one column
 *   when row is full line omit the row
 */
static int cal_holes_in_one_column (const SCREEN *scr, int col)
{
    int i;
    int full_mask = ((1 << scr->column_num) - 1);
    int cover = 0;
    int hole_num = 0;

    /* from top to bottom find the space position */
    for (i = 0; i < scr->row_num; i++) {
        if (scr->scr_map[i] != full_mask) {
            if (BIT_VAL(scr->scr_map[i], col)) { /* this (row, col) is set */
                cover = 1;
            } else { /* space */
                hole_num += cover;
            }
        }
    }
    return hole_num;
}

/*==============================================================================
 * - get_height()
 *
 * - return the specially <scr> height
 */
static int get_height (const SCREEN *scr)
{
    int i = 0;

    while ((i < scr->row_num) && scr->scr_map[i] == 0) {
        i++;
    }

    return (scr->row_num - i);
}

/*==============================================================================
 * - min_interval_between_blk_scr()
 *
 * - return the space number between blk scr
 */
static int min_interval_between_blk_scr (const SCREEN *scr, const BLOCK *blk)
{
    int i;
    int block_col_num;
    int blk_max_y[BLOCK_NODE_NUM];
    int scr_map_min_y[BLOCK_NODE_NUM];
    int space_to_bottom = scr->row_num;

    int left_col;
    int offset;

    /* init */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        blk_max_y[i] = blk->base.y;
        scr_map_min_y[i] = scr->row_num;
    }

    block_col_num = get_type_column_num (blk->type);
    /* find max_y per col */
    for (i = 0; i < BLOCK_NODE_NUM; i++) {
        blk_max_y[_G_block_map[blk->type].pos[i].x] = TETERIS_MAX (
                             blk_max_y[_G_block_map[blk->type].pos[i].x],
                             blk->base.y + _G_block_map[blk->type].pos[i].y);
    }

    left_col = block_col_num;

    /* find min_y per col in scr->scr_map */
    for (i = 0; i < scr->row_num; i++) {

        for (offset = 0; offset < block_col_num; offset++) {
            if (scr_map_min_y[offset] != scr->row_num) {
                continue;
            } else {
                if (BIT_VAL(scr->scr_map[i], blk->base.x + offset)) {
                    left_col--;
                    scr_map_min_y[offset] = i;
                }
            }
        }

        if (left_col == 0) {
            break;
        }
    }

    /* find space_to_bottom in all col */
    for (i = 0; i < block_col_num; i++) {
        space_to_bottom = TETERIS_MIN(space_to_bottom, (scr_map_min_y[i] - blk_max_y[i] - 1));
    }

    return space_to_bottom;
}

/*==============================================================================
 * - get_turn_num()
 *
 * - for a specially type, return it can turned times
 *   and set it to it's type group fisrt type
 */
static int get_turn_num (BLOCK_TYPE *p_type)
{
    int turn_num = 0;

    switch (*p_type) {
        case O_0:
            turn_num = 1;
            break;
        case I_0:
        case I_1:
            *p_type = I_0;
            turn_num = 2;
            break;
        case Z_0:
        case Z_1:
            *p_type = Z_0;
            turn_num = 2;
            break;
        case S_0:
        case S_1:
            *p_type = S_0;
            turn_num = 2;
            break;
        case T_0:
        case T_1:
        case T_2:
        case T_3:
            *p_type = T_0;
            turn_num = 4;
            break;
        case J_0:
        case J_1:
        case J_2:
        case J_3:
            *p_type = J_0;
            turn_num = 4;
            break;
        case L_0:
        case L_1:
        case L_2:
        case L_3:
            *p_type = L_0;
            turn_num = 4;
            break;
        default:
            break;
    }

    return turn_num;
}

/*==============================================================================
** FILE END
==============================================================================*/

