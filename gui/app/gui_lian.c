/*==============================================================================
** lian.c -- a lian lian kan game in gui.
**
** MODIFY HISTORY:
**
** 2011-11-05 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "../gui.h"
#include "../LibJPEG/jpeg_util.h"
#include "../../lianliankan/src/lian.h"

/*======================================================================
  Global variables
======================================================================*/
static int      _G_level  = 0;      /* Current Level */
static GUI_CBI *_G_p_marked_cbi = NULL;
static int      _G_left   = 0;      /* The left blocks number */
static int      _G_time   = 0; 
static OS_TCB  *_G_time_taskid;

/*======================================================================
  configs
======================================================================*/
#define LIAN_START_X         0            /* block start x coordinate */
#define LIAN_START_Y         0            /* block start y coordinate */
#define LIAN_BLOCK_SZ        60           /* block picture size */
#define LIAN_MSG_X          (LIAN_BLOCK_SZ * COL_NUM)   /* msg x coor */
#define LIAN_MSG_Y          (gra_scr_h() / 2)           /* msg y coor */
#define LIAN_PIC_PATH       "/n1/lian/a.jpg" /* block pic name start  */
#define SUCCESS_PIC_PATH    "/n1/album/champagne.jpg"  /* success pic */

#define LINE_SHOW_TIME       100000
#define LIAN_LINE_WIDTH      2

#define ONE_LEVEL_TIME       200
#define HELP_TIME            10


/*======================================================================
  functions forward declare
======================================================================*/
static void _lian_delay (int delay_time);
static int  _lian_start_level (int level);
static void _lian_exchange_blocks ();
static void _lian_msg (const char *msg, GUI_COLOR color);
static void _lian_exit ();
static void _T_time ();
static OS_STATUS _lian_cb_home (GUI_CBI *pCBI_home, GUI_COOR *coor);
static OS_STATUS _lian_cb_help (GUI_CBI *pCBI_auto, GUI_COOR *p_cbi_coor);

/*==============================================================================
 * - app_lian()
 *
 * - the lianliankan application entry, called by home app
 */
OS_STATUS app_lian (GUI_CBI *pCBI_lian, GUI_COOR *coor)
{ 
    int i;
    GUI_CBI *pCBI;
    GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, 0};
    GUI_SIZE size = {0, 0};

    ICON_CBI ic_right[] = {
        {"/n1/gui/cbi_home.jpg",     _lian_cb_home},
        {"/n1/gui/cbi_android.jpg",  _lian_cb_help}
    };

    /* clear screen */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR);
    gra_set_show_fb (0);

    /* register block cbis */
    _G_left = _lian_start_level (_G_level);

    /* register help and home cbi */
    pCBI = cbi_create_default (ic_right[0].name, &left_up, &size, FALSE);
    pCBI->func_release = ic_right[0].func; /* home */
    cbi_register (pCBI);
    for (i = 1; i < N_ELEMENTS(ic_right); i++) {
        left_up.y = gra_scr_h() - (N_ELEMENTS(ic_right) - i) * ICON_SIZE;

        pCBI = cbi_create_default (ic_right[i].name, &left_up, &size, FALSE);
        pCBI->func_press = ic_right[i].func;   /* help */

        cbi_register (pCBI);
    }

    _G_time_taskid = task_create ("tLianTime", 4096, 200, _T_time, 0, 0);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _delete_matched()
 *
 * - delete the matched two blocks <_G_marked> -- <_G_cur>
 */
static int _delete_matched (GUI_CBI *pCBI_cur, GUI_CBI* pCBI_marked)
{
    int i;
    GUI_COOR line_start;
    GUI_COOR line_end;
    GUI_SIZE icon_size = {LIAN_BLOCK_SZ, LIAN_BLOCK_SZ};

    /* draw line */
    for (i = 0; i < _G_pos - 1; i++) {
        line_start.x = _G_way[i].x * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;
        line_start.y = _G_way[i].y * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;

        line_end.x = _G_way[i+1].x * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;
        line_end.y = _G_way[i+1].y * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;

        gra_line (&line_start, &line_end, GUI_COLOR_RED, LIAN_LINE_WIDTH);
    }

    _lian_delay (LINE_SHOW_TIME); /* delay */

    cbi_unregister (pCBI_cur);
    gra_rect (&pCBI_cur->left_up, &icon_size, GUI_BG_COLOR);
    cbi_unregister (pCBI_marked);
    gra_rect (&pCBI_marked->left_up, &icon_size, GUI_BG_COLOR);

    /* hide line */
    for (i = 0; i < _G_pos - 1; i++) {
        line_start.x = _G_way[i].x * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;
        line_start.y = _G_way[i].y * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;

        line_end.x = _G_way[i+1].x * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;
        line_end.y = _G_way[i+1].y * LIAN_BLOCK_SZ - LIAN_BLOCK_SZ / 2;

        gra_line (&line_start, &line_end, GUI_BG_COLOR, LIAN_LINE_WIDTH);
    }

    /* change map */
    _G_map[_G_way[0].y][_G_way[0].x] = 0;
    _G_map[_G_way[_G_pos - 1].y][_G_way[_G_pos - 1].x] = 0;

    _G_left -= 2;

    return _G_left;
}

/*==============================================================================
 * - _lian_exit()
 *
 * - stop lianliankan game.
 */
static void _lian_exit ()
{
    if (_G_time_taskid != NULL) {
        task_delete (_G_time_taskid); 
    }

    if (_G_level >= LEVEL_NUM) {
        _G_level = 0;
    }

    _G_time = 0;
}

/*==============================================================================
 * - _lian_cb_mark()
 *
 * - mark a block, if this is second mark, try to delete the two blocks
 */
OS_STATUS _lian_cb_mark (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    /* check time */
    if (_G_time <= 0) return OS_STATUS_ERROR;

    if (_G_p_marked_cbi == NULL) { /* first mark */
        _G_p_marked_cbi  = pCBI;
        return OS_STATUS_OK;
    } else {                       /* second mark */
        /* convert cbi to lian inner coor */
        COOR marked_coor = {(_G_p_marked_cbi->left_up.x - LIAN_START_X) / LIAN_BLOCK_SZ + 1,
                            (_G_p_marked_cbi->left_up.y - LIAN_START_Y) / LIAN_BLOCK_SZ + 1};
        COOR cur_coor = {(pCBI->left_up.x - LIAN_START_X) / LIAN_BLOCK_SZ + 1,
                         (pCBI->left_up.y - LIAN_START_Y) / LIAN_BLOCK_SZ + 1};

        if (pCBI != _G_p_marked_cbi) { /* not equal first marked item */
            if (lian_check ((const int (*)[COL_NUM + 2])_G_map,
                            &marked_coor, &cur_coor) == CAN_GO) { /* marked can reach cur */
                
                _G_left = _delete_matched (pCBI, _G_p_marked_cbi);

                /* there is no block is left */
                if (_G_left == 0) {
                    if ((++_G_level) < LEVEL_NUM) {

                        _G_left = _lian_start_level (_G_level); /* start next level */
                        if (_G_left <= 0) { /* this level map have error */
                            _lian_exit ();
                            return OS_STATUS_ERROR;
                        }
                    } else {    /* OH,Yeah! Go through all level */
                        GUI_SIZE size = {0, 0};
                        GUI_COOR msg_coor = {LIAN_MSG_X, LIAN_MSG_Y};
                        font_printf (&msg_coor, GUI_COLOR_RED, "  Score: %4d ", _G_time);

                        msg_coor.x = 0;
                        msg_coor.y = 0;
                        jpeg_dump_pic (SUCCESS_PIC_PATH, &msg_coor, &size);

                        _lian_exit ();
                        return OS_STATUS_ERROR;
                    }
                } else { /* there is still some blocks left */

                    /* no marked */
                    _G_p_marked_cbi = NULL;
                }

                return OS_STATUS_OK;
            } else { /* marked can't reach cur */
#if 0
                cbf_default_leave (pCBI, p_cbi_coor);
#else
                cbf_default_release (_G_p_marked_cbi, NULL); /* unmark last cbi */
                _G_p_marked_cbi = pCBI;
#endif
                return OS_STATUS_ERROR;
            }
        } else { /* equal first coor */
            /* cancel the mark */
            _G_p_marked_cbi = NULL;

            /* no marked */
            return OS_STATUS_OK;
        }
    }
}

/*==============================================================================
 * - bridge_help()
 *
 * - find a couple of blocks to delete
 */
OS_STATUS _lian_cb_help (GUI_CBI *pCBI_auto, GUI_COOR *p_cbi_coor)
{
    GUI_CBI *pCBI_cur = NULL;

    GUI_COOR marked_left_up;
    GUI_COOR cur_left_up;

    /* check left blocks */
    if (_G_left <= 0) return 0;

    /* if have marked one, leave it */
    if (_G_p_marked_cbi != NULL) {
        cbf_default_leave (_G_p_marked_cbi, NULL);
        _G_p_marked_cbi = NULL;        /* no block is marked */
    }

    /* start find the way */
    if (lian_get_op ((const int (*)[COL_NUM + 2])_G_map) == 0) { /* can't find a couple to delete */
        _lian_msg (" You are DEAD ", GUI_COLOR_RED);
        _G_time += HELP_TIME;

        /* exchange blocks */
        _lian_exchange_blocks ();

        return OS_STATUS_ERROR;
    }

    _G_time = MAX(0, _G_time - HELP_TIME);

    marked_left_up.x = (_G_ops[0].start.x - 1) * LIAN_BLOCK_SZ;
    marked_left_up.y = (_G_ops[0].start.y - 1) * LIAN_BLOCK_SZ;
    cur_left_up.x = (_G_ops[0].end.x - 1) * LIAN_BLOCK_SZ;
    cur_left_up.y = (_G_ops[0].end.y - 1) * LIAN_BLOCK_SZ;

    _G_p_marked_cbi = cbi_in_which (&marked_left_up, NULL);
    pCBI_cur = cbi_in_which (&cur_left_up, NULL);

    _lian_cb_mark (pCBI_cur, NULL);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _lian_msg()
 *
 * - show a string on lcd
 */
static void _lian_msg (const char *msg, GUI_COLOR color)
{
    GUI_COOR msg_coor = {LIAN_MSG_X, LIAN_MSG_Y};
    font_draw_string (&msg_coor, color, (const uint8 *)msg, strlen (msg));
}

/*==============================================================================
 * - _lian_cb_home()
 *
 * - exit lianliankan game, then start home application
 */
static OS_STATUS _lian_cb_home (GUI_CBI *pCBI_home, GUI_COOR *coor)
{    
    /* delete time task, and clean up global variables */
    _lian_exit ();

    /* clear screen */
    gra_clear (GUI_BG_COLOR);

    /* start home application */
    cbf_go_home (pCBI_home, coor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _lian_register_cbi()
 *
 * - register a cbi. <num> [1, 24]
 */
static void _lian_register_cbi (int x, int y, int num)
{
    char pic_name[40] = LIAN_PIC_PATH;
    GUI_CBI *pCBI;
    GUI_COOR left_up = {(x-1) * LIAN_BLOCK_SZ, (y-1) * LIAN_BLOCK_SZ};
    GUI_SIZE size = {0, 0};

    pic_name[strlen(pic_name) - 5] += num - 1;

    pCBI = cbi_create_default (pic_name, &left_up, &size, FALSE);

    pCBI->func_release = _lian_cb_mark; 

    cbi_register (pCBI);
}

/*==============================================================================
 * - _lian_unregister_cbi()
 *
 * - unregister a cbi which at <y> row, <x> col
 */
static void _lian_unregister_cbi (int x, int y)
{
    GUI_COOR left_up = {(x-1) * LIAN_BLOCK_SZ, (y-1) * LIAN_BLOCK_SZ};
    GUI_CBI *pCBI = cbi_in_which (&left_up, NULL);

    cbi_unregister (pCBI);
}

/*==============================================================================
 * - _lian_start_level()
 *
 * - start a level, register the blocks a cbis
 */
static int _lian_start_level (int level) 
{
    int i, j;
    char *level_layer = "---------";
    GUI_COOR layer_coor = {gra_scr_w() - (ICON_SIZE + strlen(level_layer) * GUI_FONT_WIDTH) / 2,
                           gra_scr_h() - ICON_SIZE};

    /* lian core start <level> */
    _G_left = lian_init (level);
    if (_G_left <= 0) {
        return _G_left;
    }

    _G_p_marked_cbi = NULL;        /* no block is marked */

    /* register the block as cbi */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_map[i][j] != 0) {
                _lian_register_cbi (j, i, _G_map[i][j]);
            }
        }
    }

    /* draw level */
    for (i = 0; i <= level; i++) {
        layer_coor.y -= GUI_FONT_HEIGHT;
        font_printf (&layer_coor, GUI_COLOR_CYAN, level_layer);
    }
    layer_coor.y -= GUI_FONT_HEIGHT;
    font_printf (&layer_coor, GUI_COLOR_MAGENTA, "Level: %d", level);

    _G_time += ONE_LEVEL_TIME; /* add left time to new level */

    return _G_left;
}

/*==============================================================================
 * - _lian_exchange_blocks()
 *
 * - when dead, we should exchange blocks
 */
static void _lian_exchange_blocks ()
{
	int i;
	int j;

    /* unregister old block cbis */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_map[i][j] != 0) {
                _lian_unregister_cbi (j, i);
            }
        }
    }

    /* _lian_msg ("Exchnage !!!!!", GUI_COLOR_YELLOW); */

    /* lian core reorder */
    lian_reorder ();

    /* register new block as cbi */
    for (i = 1; i < ROW_NUM + 1; i++) {
        for (j = 1; j < COL_NUM + 1; j++) {
            if (_G_map[i][j] != 0) {
                _lian_register_cbi (j, i, _G_map[i][j]);
            }
        }
    }

    /* _lian_msg ("Exchnage ----- DONE.", GUI_COLOR_YELLOW); */
}

/*==============================================================================
 * - _lian_delay()
 *
 * - delay some time when remove blocks
 */
static void _lian_delay (int delay_time)
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
    GUI_COOR msg_coor = {LIAN_MSG_X, LIAN_MSG_Y};

    while (_G_time >= 0) {

        if (_G_time > 20) {
            font_printf (&msg_coor, GUI_COLOR_GREEN, "You Time: %4d", _G_time);
        } else {
            font_printf (&msg_coor, GUI_COLOR_RED, "You Time: %4d", _G_time);
        }

        delayQ_delay (SYS_CLK_RATE);
        _G_time--;
    }
    _G_time_taskid = NULL;

    _lian_msg ("  Game Over!  ", GUI_COLOR_GREEN);
}

/*==============================================================================
** FILE END
==============================================================================*/
