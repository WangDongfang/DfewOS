/*==============================================================================
** bri_tetris.c -- a tetris game in gui.
**
** MODIFY HISTORY:
**
** 2011-10-26 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../string.h"
#include "../gui.h"
#include "../LibJPEG/jpeg_util.h"
int sprintf(char *buf, const char *, ...);

#define TETRIS_NET_OFF
/*======================================================================
  configs
======================================================================*/
#define BLOCK_TYPE_NUM   7
#define TET_PIC_PATH     "/n1/tetris/"

#define ICON_START_X     (gra_scr_w() - ICON_SIZE)
#define ICON_NUM          5

/*======================================================================
  globals
======================================================================*/
uint16 *_G_block_pixel[BLOCK_TYPE_NUM + 1];
static int     _G_block_width;
static int     _G_block_height;

/*======================================================================
  Bridge module support APIs declare
======================================================================*/
void bri_Russian_main (void *Display_ID,
      int fd,
      int left_top_x, int left_top_y,
      int right_bottom_x, int right_bottom_y,
      int node_width,
      int (*dsply_show_routine)(void *Display_ID, int x0, int y0, int x1, int y1, int show));
void bri_bridge_free ();
int  bri_send_msg (int type);

#ifdef TETRIS_NET_ON
/*======================================================================
  Tetris Net module support APIs declare
======================================================================*/
void tet_net_init (uint16 **p_blocks, int block_num, int block_w, int block_h);
int  tet_net_send_show (int x, int y, int show);
void tet_net_free ();
#endif /* TETRIS_NET_ON */

/*======================================================================
  forward functions declare
======================================================================*/
static void _tet_init ();
#ifdef TETRIS_NET_ON
static OS_STATUS _tet_cb_net (GUI_CBI *pCBI_right, GUI_COOR *coor);
#endif /* TETRIS_NET_ON */
static OS_STATUS _tet_cb_hold (GUI_CBI *pCBI_hold, GUI_COOR *coor);
static OS_STATUS _tet_cb_home (GUI_CBI *pCBI_home, GUI_COOR *coor);
static OS_STATUS _tet_cb_auto (GUI_CBI *pCBI_auto, GUI_COOR *coor);
static OS_STATUS _tet_cb_right (GUI_CBI *pCBI_right, GUI_COOR *coor);
static OS_STATUS _tet_cb_turn (GUI_CBI *pCBI_turn, GUI_COOR *coor);
static OS_STATUS _tet_cb_bottom (GUI_CBI *pCBI_bottom, GUI_COOR *coor);
static OS_STATUS _tet_cb_down (GUI_CBI *pCBI_down, GUI_COOR *coor);
static OS_STATUS _tet_cb_left (GUI_CBI *pCBI_left, GUI_COOR *coor);


/*==============================================================================
 * - app_tetris()
 *
 * - tetris application entry. called by home app
 */
OS_STATUS app_tetris (GUI_CBI *pCBI_tetris, GUI_COOR *coor)
{
	int i;

    ICON_CBI ic_top[3] = {
        {"/n1/gui/cbi_hold_t.jpg",  _tet_cb_hold},
        {"/n1/gui/cbi_home_t.jpg",  _tet_cb_home},
        {"/n1/gui/cbi_auto_t.jpg",  _tet_cb_auto}
    };

    ICON_CBI ic_bottom[ICON_NUM] = {
        {"/n1/gui/cbi_up.jpg",      _tet_cb_right},
        {"/n1/gui/cbi_turn_t.jpg",  _tet_cb_turn},
        {"/n1/gui/cbi_fast_down_t.jpg", _tet_cb_bottom},
        {"/n1/gui/cbi_right.jpg",   _tet_cb_down},
        {"/n1/gui/cbi_down.jpg",    _tet_cb_left}
    };
    
    GUI_SIZE size = {0, 0};
    GUI_CBI *pCBI;

    /* clear screen */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR);
    gra_set_show_fb (0);

    /* register 3 cbis */
    for (i = 0; i < 3; i++) {
        GUI_COOR left_up = {0, i * ICON_SIZE * 2};

        pCBI = cbi_create_default (ic_top[i].name, &left_up, &size, FALSE);

        if (ic_top[i].func == _tet_cb_home) {
            pCBI->func_release = ic_top[i].func; 
        } else {
            pCBI->func_press = ic_top[i].func; 
        }

        cbi_register (pCBI);
    }

    /* register 5 cbis */
    for (i = 0; i < ICON_NUM; i++) {
        GUI_COOR left_up = {ICON_START_X, i * ICON_SIZE};

        pCBI = cbi_create_default (ic_bottom[i].name, &left_up, &size, FALSE);

        if (pCBI == NULL) {
        	FOREVER {}
        }

        pCBI->func_press = ic_bottom[i].func;

        /* except 'turn' and 'bottom' cbi, 
         * if you don't release, we will coutinue send message */
        if ((i != 1) && (i != 2)) { 
            pCBI->func_drag    = ic_bottom[i].func;
        }

        cbi_register (pCBI);
    }

#ifdef TETRIS_NET_ON
    /* create net cbi */
    {
        GUI_COOR left_up = {ICON_SIZE, 0};
        GUI_COOR right_bottom = {ICON_START_X - 1, gra_scr_h() - 1};
        cbi_create_blank (&left_up, &right_bottom, _tet_cb_net);
    }
#endif /* TETRIS_NET_ON */

    _tet_init ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _tet_gui_show()
 *
 * - show or erase a block on lcd
 */
static int _tet_gui_show (void *Display_ID, int x0, int y0, int x1, int y1, int show)
{
    GUI_COOR start;
    GUI_SIZE size = {_G_block_width, _G_block_width};

    start.x = y0, 
    start.y = gra_scr_h() - 1 - x1;

    if (show & 0x80000000) {  /* show score */
        char score[40];
        start.x = (ICON_SIZE - GUI_FONT_HEIGHT) / 2;
        start.y = ICON_SIZE * 4 - 12;
        /* font_printf (&start, GUI_COLOR_RED, "%d", show & 0x7FFFFFFF); */
        sprintf(score, "Score: %d", show & 0x7FFFFFFF);
        font_draw_string_v (&start, GUI_COLOR_RED, (uint8 *)score, strlen(score));
        return show;
    }

    GUI_ASSERT(((x1 - x0 + 1) == _G_block_width));

    /* show block */
    gra_block (&start, &size, _G_block_pixel[show]);

#ifdef TETRIS_NET_ON
    /* net send */
    tet_net_send_show (x0, y0, show);
#endif /* TETRIS_NET_ON */

    return show;
}

/*==============================================================================
 * - _tet_init()
 *
 * - load picture data, and start tetris core
 */
static void _tet_init ()
{
    int i;
    int temp_width;
    char pic_file_name[40] = {TET_PIC_PATH"1.jpg"};

    _G_block_pixel[0] = NULL;

    for (i = 1; i <= BLOCK_TYPE_NUM; i++) {
        _G_block_pixel[i] = get_pic_pixel (pic_file_name,
                                           &_G_block_width,
                                           &_G_block_height);

        if (_G_block_pixel[i] == NULL) {
            while (--i >= 0) {
                free (_G_block_pixel[i]);
            }
            return;
        }

        if (i != 1) {
            GUI_ASSERT((_G_block_width == temp_width));
        }
        temp_width = _G_block_width;

        GUI_ASSERT((_G_block_width == _G_block_height));

        pic_file_name[strlen (TET_PIC_PATH)]++;
    }

    /* draw tetris bottom bound */
    {
        #define BOUND_WIDTH  ((ICON_START_X - ICON_SIZE) % _G_block_width)
        GUI_COOR start = {ICON_START_X - BOUND_WIDTH, 0};
        GUI_COOR end   = {ICON_START_X - BOUND_WIDTH, gra_scr_h()};
        gra_line (&start, &end, GUI_COLOR_YELLOW, BOUND_WIDTH);
    }

    /* start tetris */
    bri_Russian_main (NULL, 0,
                      0,       
                      ICON_SIZE, 
                      gra_scr_h(),      /* for tetris x: 0 - 480 */
                      ICON_START_X,     /* y:  ICON_SIZE --- (800 - ICON_SIZE) */
                      _G_block_width,
                      _tet_gui_show);

}

#ifdef TETRIS_NET_ON
static OS_STATUS _tet_cb_net (GUI_CBI *pCBI_right, GUI_COOR *coor)
{
    /* we first hold the game */
    bri_send_msg (7);

    /* this operation will take some time */
    /* send whole screen to PC, send all Block pixels to PC */
    tet_net_init (_G_block_pixel, BLOCK_TYPE_NUM, _G_block_width, _G_block_height);

    /* un-hold the game */
    bri_send_msg (7);

    return OS_STATUS_OK;
}
#endif /* TETRIS_NET_ON */

/*==============================================================================
 * - _tet_cb_right()
 *
 * - send right message
 */
static OS_STATUS _tet_cb_right (GUI_CBI *pCBI_right, GUI_COOR *coor)
{
    bri_send_msg (3);

    return OS_STATUS_OK;
}
/*==============================================================================
 * - _tet_cb_turn()
 *
 * - send turn message
 */
static OS_STATUS _tet_cb_turn (GUI_CBI *pCBI_turn, GUI_COOR *coor)
{
    bri_send_msg (1);

    return OS_STATUS_OK;
}
/*==============================================================================
 * - _tet_cb_home()
 *
 * - sned quit message and wait tetris core is end, then free resource.
 *   finally start home appliction
 */
static OS_STATUS _tet_cb_home (GUI_CBI *pCBI_home, GUI_COOR *coor)
{    
	int i;

    /* 
     * wait tetris moudle is over.
     * to avoid it use Bridge module control struct after below free
     */
    do {
        i = bri_send_msg (0);
        delayQ_delay (SYS_CLK_RATE / 10);
    } while (i == 0);

    /* free block pic data */
    for (i = 1; i <= BLOCK_TYPE_NUM; i++) {
        free (_G_block_pixel[i]);
    }

    /* free bridge module resoure */
    bri_bridge_free ();

#ifdef TETRIS_NET_ON
    /* free net module resoure */
    tet_net_free ();
#endif /* TETRIS_NET_ON */

    /* clear screen */
    gra_clear(GUI_BG_COLOR);

    /* start home application */
    cbf_go_home (pCBI_home, coor);

    return OS_STATUS_OK;
}
/*==============================================================================
 * - _tet_cb_bottom()
 *
 * - send bottom message
 */
static OS_STATUS _tet_cb_bottom (GUI_CBI *pCBI_bottom, GUI_COOR *coor)
{
    bri_send_msg (6);

    return OS_STATUS_OK;
}
/*==============================================================================
 * - _tet_cb_down()
 *
 * - send down message
 */
static OS_STATUS _tet_cb_down (GUI_CBI *pCBI_down, GUI_COOR *coor)
{
    bri_send_msg (2);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _tet_cb_left()
 *
 * - send left message
 */
static OS_STATUS _tet_cb_left (GUI_CBI *pCBI_left, GUI_COOR *coor)
{
    bri_send_msg (4);

    return OS_STATUS_OK;
}
/*==============================================================================
 * - _tet_cb_auto()
 *
 * - send auto message
 */
static OS_STATUS _tet_cb_auto (GUI_CBI *pCBI_auto, GUI_COOR *coor)
{
    bri_send_msg (5);

    return OS_STATUS_OK;
}
/*==============================================================================
 * - _tet_cb_hold()
 *
 * - send hold message
 */
static OS_STATUS _tet_cb_hold (GUI_CBI *pCBI_hold, GUI_COOR *coor)
{
    bri_send_msg (7);

    return OS_STATUS_OK;
}

/*==============================================================================
** FILE END
==============================================================================*/

