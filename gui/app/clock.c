/*==============================================================================
** clock.c -- clock application.
**
** MODIFY HISTORY:
**
** 2012-04-20 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include <math.h>
#include "../gui.h"
#include "../../date/timeLib.h"

/*======================================================================
  Configs
======================================================================*/
#define HUR_HAND_LENGTH    160
#define MIN_HAND_LENGTH    220
#define SEC_HAND_LENGTH    230
#define HUR_HAND_WIDTH     5
#define MIN_HAND_WIDTH     3
#define SEC_HAND_WIDTH     1

/*======================================================================
  Global Variables
======================================================================*/
static OS_TCB *_G_fresh_task_id;

/*======================================================================
  Function foreward declare
======================================================================*/
static OS_STATUS _clock_cb_home (GUI_CBI *pCBI_home, GUI_COOR *pCoor);
static void _T_fresh_clock (uint32 unused, uint32 unused2);
static void _draw_clock_bg ();
static void _draw_clock_hands ();
static void _draw_hand (int length, int degress, GUI_COLOR color, int width);
static void _draw_big_point (GUI_COOR coor, GUI_COLOR color, int width_i);

/*==============================================================================
 * - app_clock()
 *
 * - clock application entry, called by home app
 */
OS_STATUS app_clock (GUI_CBI *pCBI_clock, GUI_COOR *coor)
{
    GUI_CBI *pCBI = NULL;
    GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, 0};
    GUI_COOR right_down = {gra_scr_w() - 1, ICON_SIZE - 1};

    /*
     * clear home app bequest
     */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR); /* clear screen */
    gra_set_show_fb (0);

    /*
     * register 'home' cbi
     */
    pCBI = cbi_create_blank (&left_up, &right_down, cbf_default_press);
    pCBI->func_release = _clock_cb_home;

    /* draw clock background & hands */
    _draw_clock_bg ();
    _draw_clock_hands ();

    /* create a task to fresh clock hands */
    _G_fresh_task_id = task_create ("tClock", 4 * KB, 150, _T_fresh_clock, 0, 0);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _clock_cb_home()
 *
 * - quit clock application
 */
static OS_STATUS _clock_cb_home (GUI_CBI *pCBI_home, GUI_COOR *pCoor)
{
    /* delete fresh clock task */
    task_delete (_G_fresh_task_id);

    /* clear lcd screen */
    gra_clear (GUI_BG_COLOR);

    /* go to home */
    cbf_go_home (pCBI_home, pCoor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _T_fresh_clock()
 *
 * - task to fresh clock hands
 */
static void _T_fresh_clock (uint32 unused, uint32 unused2)
{
    FOREVER {
        _draw_clock_hands ();
        delayQ_delay (SYS_CLK_RATE);
    }
}

/*==============================================================================
 * - _get_scr_coor()
 *
 * - convert clock coordinate to screen coordinate
 */
static void _get_scr_coor (const GUI_COOR *clock_coor, GUI_COOR *scr_coor)
{
    GUI_COOR origin = {gra_scr_w() / 2, gra_scr_h() / 2};

    scr_coor->x = clock_coor->x + origin.x;
    scr_coor->y = origin.y - clock_coor->y;
}

/*==============================================================================
 * - _draw_clock_bg()
 *
 * - draw clock pointers
 */
static void _draw_clock_bg ()
{
    int i;
    const GUI_COOR points[] = {{0,    240}, {120,  208}, /* 0, 1 */
                               {208,  120}, {240,    0}, /* 2, 3 */
                               {208, -120}, {120, -208}, /* 4, 5 */
                               {0,   -240}, {-120,-208}, /* 6, 7 */
                               {-208,-120}, {-240,   0}, /* 8, 9 */
                               {-208, 120}, {-120, 208}, /* 10, 11 */
    };
    GUI_COOR scr_coor;

    for (i = 0; i < 12; i++) {
        _get_scr_coor (&points[i], &scr_coor);
        if (i % 3 == 0) {
            _draw_big_point (scr_coor, GUI_COLOR_RED, 1);
        } else {
            _draw_big_point (scr_coor, GUI_COLOR_GREEN, 0);
        }

    }
}

/*==============================================================================
 * - _draw_clock_hands()
 *
 * - draw clock three hands
 */
static void _draw_clock_hands ()
{
    TIME_SOCKET tsTimeNow;
    static int hour, minute, second;

    tsTimeNow = timeGet();

    /* erase old hands */
    _draw_hand (HUR_HAND_LENGTH, hour * 30 + minute / 2, GUI_BG_COLOR, HUR_HAND_WIDTH);
    _draw_hand (MIN_HAND_LENGTH, minute * 6, GUI_BG_COLOR, MIN_HAND_WIDTH);
    _draw_hand (SEC_HAND_LENGTH, second * 6, GUI_BG_COLOR, SEC_HAND_WIDTH);

    /* update time */
    hour = tsTimeNow.iHour;
    minute = tsTimeNow.iMin;
    second = tsTimeNow.iSec;

    /* draw new hands */
    _draw_hand (HUR_HAND_LENGTH, hour * 30 + minute / 2, GUI_COLOR_RED, HUR_HAND_WIDTH);
    _draw_hand (MIN_HAND_LENGTH, minute * 6, GUI_COLOR_WHITE, MIN_HAND_WIDTH);
    _draw_hand (SEC_HAND_LENGTH, second * 6, GUI_COLOR_BLUE, SEC_HAND_WIDTH);

}

/*==============================================================================
 * - _draw_hand()
 *
 * - draw one clock hand
 */
static void _draw_hand (int length, int degress, GUI_COLOR color, int width)
{
    double radians = degress * 0.0174533;
    GUI_COOR line_start;
    GUI_COOR line_end;

    line_start.x = (int)(length * sin (radians));
    line_start.y = (int)(length * cos (radians));
    line_end.x = - (line_start.x / 5);
    line_end.y = - (line_start.y / 5);

    _get_scr_coor (&line_start, &line_start);
    _get_scr_coor (&line_end, &line_end);
    gra_line_u (&line_start, &line_end, color, width);
}

/*==============================================================================
 * - _draw_big_point()
 *
 * - show a 5*5 pixels point on <coor>
 */
static void _draw_big_point (GUI_COOR coor, GUI_COLOR color, int width_i)
{
    const int point_width[2] = {5, 9};

    static uint8 point5[5][5] = {
        {0, 1, 1, 1, 0},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {0, 1, 1, 1, 0}
    };
    static uint8 point9[9][9] = {
        {0, 0, 0, 1, 1, 1, 0, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 0},
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 1, 1, 1, 0, 0, 0},
    };

    int i, j;
    GUI_COOR t_coor;
    int width = point_width[width_i];

    coor.x -= width / 2;
    coor.y -= width / 2;

    for (i = 0; i < width; i++) {
        t_coor.y = coor.y + i;
        for (j = 0; j < width; j++) {
            if (width_i == 0) {
                if (point5[i][j]) {
                    t_coor.x = coor.x + j;
                    gra_set_pixel (&t_coor, color);
                }
            } else {
                if (point9[i][j]) {
                    t_coor.x = coor.x + j;
                    gra_set_pixel (&t_coor, color);
                }
            }
        }
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

