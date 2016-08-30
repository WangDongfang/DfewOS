/*==============================================================================
** calendar.c -- calendar application.
**
** MODIFY HISTORY:
**
** 2011-12-21 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include <yaffs_guts.h>
#include "../../date/timeLib.h"
#include "../../date/nongli.h"
#include "../../string.h"
#include "../driver/lcd.h"
#include "../gui.h"
#include "msg_box.h"
extern int sprintf(char *buf, const char *, ...);

/*======================================================================
  CONSTANT
======================================================================*/
#define DAYS_PER_WEEK       7
#define CAL_ROW_NUM         6                   /* calendar has 6 row */
#define CAL_COL_NUM         DAYS_PER_WEEK       /* calendar has 7 col */

/*======================================================================
  Location & Size Configs (LCD SIZE: 800 X 600)
======================================================================*/
#define CAL_FRAME_START_X     116
#define CAL_FRAME_START_Y     68
#define CAL_FRAME_BORDER      4
#define CAL_FRAME_WEEK_HEIGHT 40

#define CAL_BLOCK_START_X    (CAL_FRAME_START_X + CAL_FRAME_BORDER)
#define CAL_BLOCK_START_Y    (CAL_FRAME_START_Y + CAL_FRAME_BORDER + CAL_FRAME_WEEK_HEIGHT)
#define CAL_BLOCK_HEIGHT      70                    /* day block height */
#define CAL_BLOCK_WIDTH       80                    /* day block width  */
#define CAL_BLOCK_BORDER      1               /* day block border width */

#define CAL_FRAME_WIDTH  (CAL_BLOCK_WIDTH * CAL_COL_NUM + 2 * CAL_FRAME_BORDER) 
#define CAL_FRAME_HEIGHT (CAL_FRAME_WEEK_HEIGHT + CAL_BLOCK_HEIGHT * CAL_ROW_NUM + 2 * CAL_FRAME_BORDER) 

#define BLKS_SCR_WIDTH   (CAL_BLOCK_WIDTH * CAL_COL_NUM)
#define BLKS_SCR_HEIGHT  (CAL_BLOCK_HEIGHT * CAL_ROW_NUM)

/*======================================================================
  Behavors
======================================================================*/
#define CAL_SLIDE_DELAY     30000
#define CAL_A_SPEED         4
#define CAL_NOTE_MAX_LEN    300
#define CAL_NOTE_PATH       "/n1/calendar/"

/*======================================================================
  Color config
======================================================================*/
#define GUI_COLOR_GRAY          ((0xF << 11) | (0x1F << 5) | (0xF << 0))
#define GUI_COLOR_LIGHT_BLACK   ((5<<11)|(10<<5)|(5<<0))
#define GUI_COLOR_NO_COLOR      ((1<<11)|(2<<5)|(1<<0))

#define CAL_STRING_COLOR        GUI_COLOR_RED
#define CAL_BLOCK_BORDER_COLOR  GUI_COLOR_GRAY
#define CAL_WEEK_STR_COLOR      GUI_COLOR_YELLOW
#define CAL_DAY_COLOR           GUI_COLOR_WHITE
#define CAL_NO_COLOR            GUI_COLOR_NO_COLOR
#define CAL_BG_COLOR            GUI_BG_COLOR
#define CAL_TODAY_BG_COLOR      GUI_COLOR_RED
#define CAL_TODAY_FG_COLOR      GUI_COLOR_YELLOW
#define CAL_WEEKEND_BG_COLOR    GUI_COLOR_LIGHT_BLACK
#define CAL_WEEKEND_FG_COLOR    GUI_COLOR_MAGENTA
#define CAL_FRAME_BORDER_COLOR  GUI_COLOR_CYAN

#define CAL_SET_CBI_X    (CAL_FRAME_START_X + CAL_FRAME_WIDTH + 10)
#define CAL_SET_CBI_Y     CAL_FRAME_START_Y
#define SET_CBI_FILE_NAME "/n1/gui/cbi_up.jpg"
#define SLIDE_FILE_NAME   "/n1/gui/tai_ji.jpg"
#define CUR_SET_CHAR_X    (CAL_SET_CBI_X + (ICON_SIZE -  GUI_FONT_WIDTH) / 2)
#define CUR_SET_CHAR_Y    (CAL_SET_CBI_Y - GUI_FONT_HEIGHT)
#define SET_STR_LEN        19

/*======================================================================
  Weekday strings
======================================================================*/
static const char *_G_str_weeks[DAYS_PER_WEEK] = {"Sunday", "Monday",
            "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *_G_short_weeks[DAYS_PER_WEEK] = {"SUN", "MON", "TUE",
                                            "WED", "THU", "FRI", "SAT"};
static GUI_CBI *_G_pCBI_days[32];
static int      _G_quit = 0;
static int      _G_set_mod = 0;
static int      _G_set_pos = 0;
static int      _G_set_max = 0;
static int      _G_set_drag_len = 0;

typedef struct blocks_screen {
    GUI_COLOR *prev_month_scr;
    GUI_COLOR *cur_month_scr;
    GUI_COLOR *next_month_scr;

    int prev_year;
    int cur_year;
    int next_year;

    int prev_month;
    int cur_month;
    int next_month;

    int prev_first_week_day;
    int cur_first_week_day;
    int next_first_week_day;
} BLOCKS_SCREEN;

static BLOCKS_SCREEN _G_blks_scr;
static int _G_next_month_pos = BLKS_SCR_WIDTH;
static int _G_prev_month_pos = -1;

/*======================================================================
  Function foreward declare
======================================================================*/
static OS_STATUS _calendar_cb_now (GUI_CBI *pCBI_now, GUI_COOR *p_cbi_coor);
static OS_STATUS _calendar_cb_prev (GUI_CBI *pCBI_prev, GUI_COOR *p_cbi_coor);
static OS_STATUS _calendar_cb_next (GUI_CBI *pCBI_next, GUI_COOR *p_cbi_coor);
static OS_STATUS _calendar_cb_set (GUI_CBI *pCBI_set, GUI_COOR *p_cbi_coor);
static OS_STATUS _calendar_cb_home (GUI_CBI *pCBI_home, GUI_COOR *p_cbi_coor);
static OS_STATUS _calendar_cb_note (GUI_CBI *pCBI_day, GUI_COOR *p_cbi_coor);

static void _draw_current_year_month ();
static void _calendar_draw_date_str (const TIME_SOCKET *pTime);
static void _calendar_start_from_this ();
static void _calendar_reg_set_cbis ();
static void _calendar_draw_frame ();
static void _calendar_draw_a_day_blk (const GUI_COOR *p_blk_coor, int day,
                                      GUI_COLOR str_color,
                                      GUI_COLOR bg_color,
                                      const char *nong_str);
static void _calendar_clear_day_blks ();

static void _calendar_reg_day_cbis ();
static void _calendar_unreg_day_cbis ();

static OS_STATUS _calendar_cb_set_drag (GUI_CBI *pCBI_set, GUI_COOR *p_offset_coor);
static OS_STATUS _calendar_cb_set_release (GUI_CBI *pCBI_set, GUI_COOR *p_coor);

static OS_STATUS _calendar_cb_slide_drag (GUI_CBI *pCBI_slide, GUI_COOR *p_offset_coor);
static OS_STATUS _calendar_cb_slide_release (GUI_CBI *pCBI_slide, GUI_COOR *p_offset_coor);

static void _set_str_left_to (int pos);
static void _set_str_clear ();

static void _prev_month_to (int pos, int delay);
static void _next_month_to (int pos, int delay);
static GUI_COLOR *_calendar_get_blks_scr (int year, int month,
                                          int first_week_day);

static int _month_days(int y, int m);
static void _T_time_fresh ();

/*==============================================================================
 * - app_calendar()
 *
 * - calendar application entry
 */
OS_STATUS app_calendar (GUI_CBI *pCBI_calendar, GUI_COOR *coor)
{
    /* clear screen */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR);
    gra_set_show_fb (0);

    /* register operate cbis */
    {
        int i;
        ICON_CBI ic_left[] = {
            {"/n1/gui/cbi_home.jpg",   _calendar_cb_home},
            {"/n1/gui/cbi_android.jpg",_calendar_cb_now},
            {"/n1/gui/cbi_apple.jpg",  _calendar_cb_set},
            {"/n1/gui/cbi_right.jpg",  _calendar_cb_prev},
            {"/n1/gui/cbi_left.jpg",   _calendar_cb_next}
        };
        GUI_CBI *pCBI;
        GUI_COOR left_up = {0, gra_scr_h() - ICON_SIZE - 60};
        GUI_SIZE size = {0, 0};

        for (i = 0; i < N_ELEMENTS(ic_left); i++) {

            pCBI = cbi_create_default (ic_left[i].name, &left_up, &size, FALSE);

            if (ic_left[i].func == _calendar_cb_home) {
                pCBI->func_release = ic_left[i].func; /* home */
            } else {
                pCBI->func_press = ic_left[i].func;   /*  */
            }

            cbi_register (pCBI);

            left_up.y -= ICON_SIZE; /* calculate start y coor */
        }
    }

    /* draw frame */
    _calendar_draw_frame ();

    /* register DAY cbis */
    _calendar_start_from_this ();

    /* register SET cbis */
    _calendar_reg_set_cbis ();

    task_create ("tDateFresh", 4096, 200, _T_time_fresh, 0, 0);

    _G_quit = 0;
    _G_set_mod = 0;
    _G_set_pos = 0;
    _G_set_max = 0;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _T_time()
 *
 * - fresh date string task
 */
static void _T_time_fresh ()
{
    while (_G_quit == 0) {
        _calendar_draw_date_str (NULL);
        delayQ_delay (SYS_CLK_RATE);
    }
    _G_quit = 2;
}

/*==============================================================================
 * - _calendar_cb_now()
 *
 * - redraw calendar swicth to today now
 */
static OS_STATUS _calendar_cb_now (GUI_CBI *pCBI_now, GUI_COOR *p_cbi_coor)
{
    _calendar_start_from_this ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _calendar_cb_prev()
 *
 * - change to prev month blocks screen
 */
static OS_STATUS _calendar_cb_prev (GUI_CBI *pCBI_prev, GUI_COOR *p_cbi_coor)
{
    _prev_month_to (BLKS_SCR_WIDTH - 1, CAL_SLIDE_DELAY);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _calendar_cb_next()
 *
 * - change to next month blocks screen
 */
static OS_STATUS _calendar_cb_next (GUI_CBI *pCBI_next, GUI_COOR *p_cbi_coor)
{
#if 0
    GUI_COOR s = {CAL_BLOCK_START_X, CAL_BLOCK_START_Y};
    GUI_SIZE sz = {BLKS_SCR_WIDTH, BLKS_SCR_HEIGHT};
    gra_block (&s, &sz, _G_blks_scr.next_month_scr);
#endif

    _next_month_to (0, CAL_SLIDE_DELAY);


    return OS_STATUS_OK;
}

/*==============================================================================
 * - _calendar_cb_set()
 *
 * - start set date
 */
static OS_STATUS _calendar_cb_set (GUI_CBI *pCBI_set, GUI_COOR *p_cbi_coor)
{
    GUI_COOR set_str_coor;
    TIME_SOCKET tsTimeNow;

    if (_G_set_mod == 1) {
        _set_str_clear ();
        return OS_STATUS_OK;
    }

    /* get now time */
    tsTimeNow = timeGet();

    /* calculator coordinate */
    set_str_coor.x = CUR_SET_CHAR_X;
    set_str_coor.y = CUR_SET_CHAR_Y;

    /* draw set string */
    font_printf (&set_str_coor, CAL_STRING_COLOR,
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 tsTimeNow.iYear,
                 tsTimeNow.iMon,
                 tsTimeNow.iDay,
                 tsTimeNow.iHour,
                 tsTimeNow.iMin,
                 tsTimeNow.iSec);

    _G_set_mod = 1;
    _G_set_pos = 0;
    _G_set_max = 3;
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _calendar_cb_home()
 *
 * - quit calendar application, go to home
 */
static OS_STATUS _calendar_cb_home (GUI_CBI *pCBI_home, GUI_COOR *p_cbi_coor)
{
    _G_quit = 1;

    while (_G_quit != 2) {
        delayQ_delay (1);
    }
    
    /* 
     * free blocks screen memory if need
     */
    if (_G_blks_scr.prev_month_scr != NULL) {
        free (_G_blks_scr.prev_month_scr);
        _G_blks_scr.prev_month_scr = NULL;
    }
    if (_G_blks_scr.cur_month_scr != NULL) {
        free (_G_blks_scr.cur_month_scr);
        _G_blks_scr.cur_month_scr = NULL;
    }
    if (_G_blks_scr.next_month_scr != NULL) {
        free (_G_blks_scr.next_month_scr);
        _G_blks_scr.next_month_scr = NULL;
    }

    _calendar_unreg_day_cbis ();


    gra_clear (GUI_BG_COLOR);
    cbf_go_home (pCBI_home, p_cbi_coor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _month_days()
 *
 * - return how many days of the month <m> in year <y> 
 */
static int _month_days(int y, int m) {
    switch (m) { case 1: case 3: case 5: case 7: case 8: case 10: case 12: return (31);
    case 4: case 6: case 9: case 11: return (30); case 2: return (((y % 400 == 0) ||
    ((y % 4 == 0) && (y % 100 != 0))) ? 29: 28); default: return 0; }
}

/*==============================================================================
 * - _calendar_draw_date_str()
 *
 * - draw date string
 */
static void _calendar_draw_date_str (const TIME_SOCKET *pTime)
{
    GUI_COOR date_string_coor = {CAL_FRAME_START_X + 8,
                                 CAL_FRAME_START_Y - 3 * GUI_FONT_HEIGHT - 8};
    TIME_SOCKET tsTimeNow;
    NONGLI_DATE nong_date;
    char nong_str[200];
    int  ret;

    /* get now time */
    if (pTime != NULL) {
        tsTimeNow = *pTime;
    } else {
        tsTimeNow = timeGet();
    }

    /* draw string date and time */
    font_printf (&date_string_coor, CAL_STRING_COLOR,
                 "Date : %d-%02d-%02d Today : %s Time : %02d:%02d:%02d   ",
                  tsTimeNow.iYear,
                  tsTimeNow.iMon,
                  tsTimeNow.iDay,
                  _G_str_weeks[tsTimeNow.iWday],
                  tsTimeNow.iHour,
                  tsTimeNow.iMin,
                  tsTimeNow.iSec);

    date_string_coor.y += GUI_FONT_HEIGHT;

    /*
     * nong li
     */
    ret = nongli_get_date (tsTimeNow.iYear, tsTimeNow.iMon, tsTimeNow.iDay,
                     	   &nong_date);
    if (ret != -1) {

        /*
         * print ri qi
         */
        sprintf (nong_str, "农历 : %s年 %s 【%s】",
                 nong_date.tian_di, nong_date.ri_qi, nong_date.sheng_xiao);
        han_draw_string (&date_string_coor, CAL_STRING_COLOR,
                         (uint8 *)nong_str, strlen (nong_str));

        /*
         * erase jie qi
         */
        date_string_coor.y += GUI_FONT_HEIGHT;
        sprintf (nong_str, "                            ");
        han_draw_string (&date_string_coor, CAL_STRING_COLOR,
                         (uint8 *)nong_str, strlen (nong_str));

        /*
         * print jie qi
         */
        sprintf (nong_str, "     : %s%s", nong_date.jie_qi, nong_date.bei_zhu); 
        han_draw_string (&date_string_coor, CAL_STRING_COLOR,
                         (uint8 *)nong_str, strlen (nong_str));
    } else {
        sprintf (nong_str, "不能将此阳历日期转化为农历");
        han_draw_string (&date_string_coor, CAL_STRING_COLOR,
                         (uint8 *)nong_str, strlen (nong_str));

        date_string_coor.y += GUI_FONT_HEIGHT;
        sprintf (nong_str, "                            ");
        han_draw_string (&date_string_coor, CAL_STRING_COLOR,
                         (uint8 *)nong_str, strlen (nong_str));
    }
}

/*==============================================================================
 * - _calendar_start_from_this()
 *
 * - in this function we will fill the day blocks screen area with current date,
 *   and register day cbis
 */
static void _calendar_start_from_this ()
{
    int i;
    int first_week_day;
    int days_of_month;
    int i_week_day;
    GUI_COOR day_blk_coor;
    GUI_COLOR day_blk_color;
    GUI_COLOR day_str_color;

    TIME_SOCKET tsTimeNow;
    NONGLI_DATE nong_date;
    char *nong_str = "无";
    int ret;

    /* get time */
    tsTimeNow = timeGet();
    
    /* 
     * 根据今天是星期几,
     * 计算今天所在月份的一号是星期几 [0, 6]
     */
    first_week_day = (7 - (tsTimeNow.iDay - 1) % 7 + tsTimeNow.iWday) % 7;

    /* 计算当前时间, 所在月份一共有几天 */
    days_of_month = _month_days(tsTimeNow.iYear, tsTimeNow.iMon);

    /* draw date string */
    _calendar_draw_date_str (&tsTimeNow);

    /* clear day blocks screen area */
    _calendar_clear_day_blks ();

    /* draw and register day cbis */
    i_week_day = first_week_day;
    for (i = 1; i <= days_of_month; i++) {                         /*  The days 1# ... 31#  START   */

        /* calculate the day block start coordinate */
        day_blk_coor.y = CAL_BLOCK_START_Y + ((first_week_day + i - 1) / 7) * CAL_BLOCK_HEIGHT;
        day_blk_coor.x = CAL_BLOCK_START_X + i_week_day * CAL_BLOCK_WIDTH;

        /* set color */
        if (i == tsTimeNow.iDay) { /* today */
            day_str_color = CAL_TODAY_FG_COLOR;
            day_blk_color = CAL_TODAY_BG_COLOR;
        } else if (i_week_day == 0 || i_week_day == 6) { /* weekend */
            day_str_color = CAL_WEEKEND_FG_COLOR;
            day_blk_color = CAL_WEEKEND_BG_COLOR;
        } else { /* normal day */
            day_str_color = CAL_DAY_COLOR;
            day_blk_color = CAL_BG_COLOR;
        }
        
        ret = nongli_get_date (tsTimeNow.iYear, tsTimeNow.iMon, i, &nong_date);
        if (ret != -1) {
            if (nong_date.jie_qi[0] != '\0') {
                nong_str = nong_date.jie_qi;
            } else if (nong_date.ri == 1) {
                nong_str = &nong_date.ri_qi[0];
                nong_date.ri_qi[4] = '\0';
            } else {
                nong_str = &nong_date.ri_qi[4];
            }
        }
        /* draw */
        _calendar_draw_a_day_blk (&day_blk_coor, i, day_str_color, day_blk_color, nong_str);

        i_week_day = (i_week_day + 1) % 7;
    }                                                              /*  The days 1# ... 31#  END     */

    /*
     * update <_G_blks_scr>
     */
    {
    int prev_days_of_month;
    _G_blks_scr.cur_year = tsTimeNow.iYear;
    _G_blks_scr.cur_month = tsTimeNow.iMon;
    _G_blks_scr.prev_year = tsTimeNow.iYear;
    _G_blks_scr.prev_month = tsTimeNow.iMon - 1;
    _G_blks_scr.next_year = tsTimeNow.iYear;
    _G_blks_scr.next_month = tsTimeNow.iMon + 1;

    if (_G_blks_scr.prev_month == 0) {
        _G_blks_scr.prev_month = 12;
        _G_blks_scr.prev_year--;
    }

    if (_G_blks_scr.next_month == 13) {
        _G_blks_scr.next_month = 1;
        _G_blks_scr.next_year++;
    }

    prev_days_of_month = _month_days(_G_blks_scr.prev_year, _G_blks_scr.prev_month);

    _G_blks_scr.prev_first_week_day = (first_week_day - prev_days_of_month + 35) % 7;
    _G_blks_scr.cur_first_week_day = first_week_day;
    _G_blks_scr.next_first_week_day = (first_week_day + days_of_month) % 7;

    /* 
     * free blocks screen memory if need
     */
    if (_G_blks_scr.prev_month_scr != NULL) {
        free (_G_blks_scr.prev_month_scr);
        _G_blks_scr.prev_month_scr = NULL;
    }
    if (_G_blks_scr.cur_month_scr != NULL) {
        free (_G_blks_scr.cur_month_scr);
        _G_blks_scr.cur_month_scr = NULL;
    }
    if (_G_blks_scr.next_month_scr != NULL) {
        free (_G_blks_scr.next_month_scr);
        _G_blks_scr.next_month_scr = NULL;
    }

    /* 
     * free blocks screen memory
     */
    _G_blks_scr.prev_month_scr = _calendar_get_blks_scr (_G_blks_scr.prev_year, _G_blks_scr.prev_month,
             _G_blks_scr.prev_first_week_day);
    _G_blks_scr.cur_month_scr = _calendar_get_blks_scr (_G_blks_scr.cur_year, _G_blks_scr.cur_month,
             _G_blks_scr.cur_first_week_day);
    _G_blks_scr.next_month_scr = _calendar_get_blks_scr (_G_blks_scr.next_year, _G_blks_scr.next_month,
             _G_blks_scr.next_first_week_day);
    }

    _calendar_unreg_day_cbis ();
    _calendar_reg_day_cbis ();
    _draw_current_year_month ();
}

/*==============================================================================
 * - _calendar_reg_day_cbis()
 *
 * - register current day blocks cbis
 */
static void _calendar_reg_day_cbis ()
{
    int i;
    int days_of_month;
    int i_week_day;
    GUI_COOR day_blk_start, day_blk_end;

    days_of_month = _month_days(_G_blks_scr.cur_year, _G_blks_scr.cur_month);

    i_week_day = _G_blks_scr.cur_first_week_day;

    for (i = 1; i <= days_of_month; i++) {                         /*  The days 1# ... 31#  START   */
        /* calculate the day block start coordinate */
        day_blk_start.x = CAL_BLOCK_START_X + i_week_day * CAL_BLOCK_WIDTH;
        day_blk_start.y = CAL_BLOCK_START_Y + ((_G_blks_scr.cur_first_week_day + i - 1) / 7) * CAL_BLOCK_HEIGHT;
        day_blk_end.x = day_blk_start.x + CAL_BLOCK_WIDTH - 1;
        day_blk_end.y = day_blk_start.y + CAL_BLOCK_HEIGHT - 1;

        /* create and register a day cbi */
        _G_pCBI_days[i - 1] = cbi_create_blank (&day_blk_start, &day_blk_end,
                                                cbf_default_press); /* press callback function */
        _G_pCBI_days[i - 1]->func_release = _calendar_cb_note; /* release callback function */
        _G_pCBI_days[i - 1]->data = (GUI_COLOR *)i; /* store day */

        i_week_day = (i_week_day + 1) % 7;
    }                                                              /*  The days 1# ... 31#  START   */

    _G_pCBI_days[i - 1] = NULL; /* NULL terminal pCBI array */
}

/*==============================================================================
 * - _calendar_unreg_day_cbis()
 *
 * - unregister current day blocks cbis
 */
static void _calendar_unreg_day_cbis ()
{
    int i = 0;

    while (_G_pCBI_days[i] != NULL) {
        _G_pCBI_days[i]->data = NULL; /* zero data */

        cbi_unregister (_G_pCBI_days[i]);
        _G_pCBI_days[i] = NULL; /* end of day cbis */
        i++;
    }
}

/*==============================================================================
 * - _calendar_reg_set_cbis()
 *
 * - dump picture and register set date cbis
 */
static void _calendar_reg_set_cbis ()
{
    GUI_CBI *pCBI = NULL;
    GUI_COOR left_up = {CAL_SET_CBI_X, CAL_SET_CBI_Y};
    GUI_SIZE size = {0, 0};

    /* register the right slide cbi */
    pCBI = cbi_create_default (SET_CBI_FILE_NAME, &left_up, &size, TRUE);
    pCBI->func_drag    = _calendar_cb_set_drag;
    pCBI->func_leave   = _calendar_cb_set_release;
    pCBI->func_release = _calendar_cb_set_release;

    cbi_register (pCBI);

    /* register the bottom slide cbi */
    left_up.x = CAL_FRAME_START_X + CAL_FRAME_WIDTH - 64;
    left_up.y = CAL_FRAME_START_Y + CAL_FRAME_HEIGHT;
    size.w = 0; size.h = 0;
    pCBI = cbi_create_default (SLIDE_FILE_NAME, &left_up, &size, TRUE);
    pCBI->func_drag    = _calendar_cb_slide_drag;
    pCBI->func_leave   = _calendar_cb_slide_release;
    pCBI->func_release = _calendar_cb_slide_release;

    cbi_register (pCBI);
}

/*==============================================================================
 * - _calendar_cb_slide_drag()
 *
 * - drag slide left <--> right
 */
static OS_STATUS _calendar_cb_slide_drag (GUI_CBI *pCBI_slide, GUI_COOR *p_offset_coor)
{
    int end_pos;
    int old_pos = _G_next_month_pos;

    if (p_offset_coor->x < 0) { /* drag to left */
        end_pos = MAX(64 - CAL_FRAME_BORDER, _G_next_month_pos + p_offset_coor->x);
    } else if (p_offset_coor->x > 0) { /* drag to right */
        end_pos = MIN(BLKS_SCR_WIDTH, _G_next_month_pos + p_offset_coor->x);
    } else {
        return OS_STATUS_OK;
    }

    _next_month_to (end_pos, 0);

    p_offset_coor->x = end_pos - old_pos;
    p_offset_coor->y = 0;
    return cbf_do_drag (pCBI_slide, p_offset_coor);
}

/*==============================================================================
 * - _calendar_cb_slide_release()
 *
 * - release the slide cbi, show next month blocks screen or back to current,
 *   and make the cbi to right most
 */
static OS_STATUS _calendar_cb_slide_release (GUI_CBI *pCBI_slide, GUI_COOR *p_offset_coor)
{
    int end_pos;
    int total_mov_len = BLKS_SCR_WIDTH - _G_next_month_pos;

    if (_G_next_month_pos > BLKS_SCR_WIDTH * 2 / 3) {
        end_pos = BLKS_SCR_WIDTH;
    } else {
        end_pos = 0;
    }

    /*
     * slip the SLIDE cbi to right
     */
#if 1
    {
    int i = 1;
    int mov_len;
    GUI_COOR mov_coor = {0, 0};
    while (total_mov_len > 0) {
        mov_len = ((i << 1) - 1) * CAL_A_SPEED;
        mov_coor.x = MIN(mov_len, total_mov_len);

        cbf_do_drag (pCBI_slide, &mov_coor);
        
        total_mov_len -= mov_coor.x;
        delayQ_delay(1);
        i++;
    }
    }
#else
    GUI_COOR mov_coor = {total_mov_len, 0};
    cbf_do_drag (pCBI_slide, &mov_coor);
#endif

    /*
     * slip the blocks screen to frame border
     */
    _next_month_to (end_pos, CAL_SLIDE_DELAY);

    return cbf_default_release (pCBI_slide, NULL);
}

/*==============================================================================
 * - _calendar_cb_set_drag()
 *
 * - SET cbi drag callback function. we will calcaulate the set value
 */
static OS_STATUS _calendar_cb_set_drag (GUI_CBI *pCBI_set, GUI_COOR *p_offset_coor)
{
    _G_set_drag_len += p_offset_coor->y;

    /* if drag beyond top */
    if (_G_set_drag_len < 0) {
        p_offset_coor->y -= _G_set_drag_len;
        _G_set_drag_len = 0;
    }

    {
        int t;
        GUI_COOR mov_coor;
        int mov_len = 0;

        mov_len = (gra_scr_h() - CAL_SET_CBI_Y) / _G_set_max;
        t = _G_set_drag_len / mov_len;
        mov_coor.x = CUR_SET_CHAR_X;
        mov_coor.y = CUR_SET_CHAR_Y;
        font_printf (&mov_coor, GUI_COLOR_GREEN, "%d", t);
    }

    p_offset_coor->x = 0;
    return cbf_do_drag (pCBI_set, p_offset_coor);
}

/*==============================================================================
 * - _calendar_cb_set_release()
 *
 * - SET cbi release callback function. we will set the time or wait next call
 */
static OS_STATUS _calendar_cb_set_release (GUI_CBI *pCBI_set, GUI_COOR *p_coor)
{
    static TIME_SOCKET tsTimeSet;
    int i = 1;
    int t;
    int mov_len;
    GUI_COOR mov_coor = {0, 0};

    /* no drag */
    if (_G_set_drag_len == 0) {
        goto _release_end;
    }

    /*
     * get set value
     */
    mov_len = (gra_scr_h() - CAL_SET_CBI_Y) / _G_set_max;
    t = _G_set_drag_len / mov_len;

    /*
     * slip the SET cbi to top
     */
    while (_G_set_drag_len > 0) {
        mov_len = ((i << 1) - 1) * CAL_A_SPEED;
        mov_coor.y = -(MIN(mov_len, _G_set_drag_len));

        cbf_do_drag (pCBI_set, &mov_coor);
        
        _G_set_drag_len += mov_coor.y;
        delayQ_delay(2);
        i++;
    }

    /* if not in set mode, return directly */
    if (_G_set_mod == 0) {
        goto _release_end;
    }

    /*
     * continue set
     */
    switch (_G_set_pos) {
        case 0:                     /* Year */
            tsTimeSet.iYear = t * 1000;
            _set_str_left_to (1);
            _G_set_max = 10;
            break;
        case 1:
            tsTimeSet.iYear += t * 100;
            _set_str_left_to (2);
            _G_set_max = 10;
            break;
        case 2:
            tsTimeSet.iYear += t * 10;
            _set_str_left_to (3);
            _G_set_max = 10;
            break;
        case 3:
            tsTimeSet.iYear += t;
            _set_str_left_to (5);
            _G_set_max = 2;
            break;
        case 5:                     /* Month */
            tsTimeSet.iMon = t * 10;
            _set_str_left_to (6);
            _G_set_max = 10;
            break;
        case 6:
            tsTimeSet.iMon += t;
            _set_str_left_to (8);
            _G_set_max = 4;
            break;
        case 8:                     /* Day */
            tsTimeSet.iDay = t * 10;
            _set_str_left_to (9);
            _G_set_max = 10;
            break;
        case 9:
            tsTimeSet.iDay += t;
            _set_str_left_to (11);
            _G_set_max = 3;
            break;
        case 11:                    /* Hour */
            tsTimeSet.iHour = t * 10;
            _set_str_left_to (12);
            _G_set_max = 10;
            break;
        case 12:
            tsTimeSet.iHour += t;
            _set_str_left_to (14);
            _G_set_max = 6;
            break;
        case 14:                    /* Minute */
            tsTimeSet.iMin = t * 10;
            _set_str_left_to (15);
            _G_set_max = 10;
            break;
        case 15:
            tsTimeSet.iMin += t;
            _set_str_left_to (17);
            _G_set_max = 6;
            break;
        case 17:                    /* Second */
            tsTimeSet.iSec = t * 10;
            _set_str_left_to (18);
            _G_set_max = 10;
            break;
        case 18:
            tsTimeSet.iSec += t;
            _set_str_left_to (19);
            break;
        default:
            break;
    }

    /*
     * set date
     */
    if (_G_set_pos == SET_STR_LEN) {
        timeSet(tsTimeSet);
        
        _set_str_clear ();

        _calendar_start_from_this ();
    }

_release_end:
    cbf_default_release(pCBI_set, NULL);
    return OS_STATUS_OK;
}


/*==============================================================================
 * - _calendar_cb_note()
 *
 * - read note of the day
 */
static OS_STATUS _calendar_cb_note (GUI_CBI *pCBI_day, GUI_COOR *p_cbi_coor)
{
    int  fd;
    char file_name[PATH_LEN_MAX];
    char msg[CAL_NOTE_MAX_LEN + 100];
    char txt[CAL_NOTE_MAX_LEN] = {0};

    cbf_default_release (pCBI_day, NULL);

    sprintf (file_name, CAL_NOTE_PATH"%d-%d-%d.cal", _G_blks_scr.cur_year,
                _G_blks_scr.cur_month, (int)pCBI_day->data);
    fd = yaffs_open (file_name, O_RDONLY, 0666);
	if (fd < 0) {
        sprintf (msg, "%d年%d月%d日:\n\n今天无记事！", _G_blks_scr.cur_year,
                _G_blks_scr.cur_month, (int)pCBI_day->data);
    } else {
        yaffs_read (fd, txt, CAL_NOTE_MAX_LEN - 1);
        sprintf (msg, "%d年%d月%d日:\n\n%s", _G_blks_scr.cur_year,
                _G_blks_scr.cur_month, (int)pCBI_day->data, txt);
        yaffs_close (fd);
    }

    msg_box_create (msg);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _calendar_draw_frame()
 *
 * - draw calendar frame, like this:
 *   +------------------------------+
 *   | SUN | MON | ...        | SAT |
 *   +------------------------------+
 *   |                              |
 *   |                              |
 *   |                              |
 *   |                              |
 *   |                              |
 *   |                              |
 *   +------------------------------+
 */
static void _calendar_draw_frame ()
{
    int i;
    GUI_COOR week_str_coor;
    GUI_COOR line_coor1;
    GUI_COOR line_coor2;

    /* draw 4 frame borders */
    line_coor1.x = CAL_FRAME_START_X;
    line_coor1.y = CAL_FRAME_START_Y;
    /* top */
    line_coor2.x = CAL_FRAME_START_X + CAL_FRAME_WIDTH - 1;
    line_coor2.y = line_coor1.y;
    gra_line (&line_coor1, &line_coor2, CAL_FRAME_BORDER_COLOR, CAL_FRAME_BORDER);
    /* left */
    line_coor2.x = line_coor1.x;
    line_coor2.y += CAL_FRAME_HEIGHT - 1;
    gra_line (&line_coor1, &line_coor2, CAL_FRAME_BORDER_COLOR, CAL_FRAME_BORDER);
    line_coor2.x = line_coor1.x + CAL_FRAME_WIDTH - 1;
    line_coor2.y = line_coor1.y + CAL_FRAME_HEIGHT - CAL_FRAME_BORDER;
    /* bottom */
    line_coor1.y = line_coor2.y;
    gra_line (&line_coor1, &line_coor2, CAL_FRAME_BORDER_COLOR, CAL_FRAME_BORDER);
    /* right */
    line_coor2.x = CAL_FRAME_START_X + CAL_FRAME_WIDTH - CAL_FRAME_BORDER;
    line_coor1.x = line_coor2.x;
    line_coor1.y = CAL_FRAME_START_Y;
    gra_line (&line_coor1, &line_coor2, CAL_FRAME_BORDER_COLOR, CAL_FRAME_BORDER);

    /* date weekday name */
    week_str_coor.x = CAL_FRAME_START_X + CAL_FRAME_BORDER +
        (CAL_BLOCK_WIDTH - GUI_FONT_WIDTH * strlen(_G_short_weeks[0])) / 2;
    week_str_coor.y = CAL_FRAME_START_Y + CAL_FRAME_BORDER +
        (CAL_FRAME_WEEK_HEIGHT - GUI_FONT_HEIGHT) / 2;
    for (i = 0; i < DAYS_PER_WEEK; i++) {                          /*  SUN MON TUE ... SAT          */
        font_printf (&week_str_coor, CAL_WEEK_STR_COLOR, _G_short_weeks[i]);
        week_str_coor.x += CAL_BLOCK_WIDTH;
    }
}

/*==============================================================================
 * - _calendar_draw_a_day_blk()
 *
 * - draw a day block and it' border, like this:
 *   +----+
 *   | 12 |
 *   +----+
 */
static void _calendar_draw_a_day_blk (const GUI_COOR *p_blk_coor, int day,
                                      GUI_COLOR str_color,
                                      GUI_COLOR bg_color,
                                      const char *nong_str)
{
    int day_str_width;
    GUI_COOR day_str_coor;
    GUI_COOR line_coor1;
    GUI_COOR line_coor2;
    line_coor1 = line_coor2 = *p_blk_coor;

    /* draw day block 4 borders */
    /* top */
    line_coor2.x += CAL_BLOCK_WIDTH - 1;
    gra_line (&line_coor1, &line_coor2, CAL_BLOCK_BORDER_COLOR, CAL_BLOCK_BORDER);
    /* left */
    line_coor2.x = line_coor1.x;
    line_coor2.y += CAL_BLOCK_HEIGHT - 1;
    gra_line (&line_coor1, &line_coor2, CAL_BLOCK_BORDER_COLOR, CAL_BLOCK_BORDER);
    line_coor2.x = line_coor1.x + CAL_BLOCK_WIDTH - 1;
    line_coor2.y = line_coor1.y + CAL_BLOCK_HEIGHT - CAL_BLOCK_BORDER;
    /* bottom */
    line_coor1.y = line_coor2.y;
    gra_line (&line_coor1, &line_coor2, CAL_BLOCK_BORDER_COLOR, CAL_BLOCK_BORDER);
    /* right */
    line_coor2.x = p_blk_coor->x + CAL_BLOCK_WIDTH - CAL_BLOCK_BORDER;
    line_coor1.x = line_coor2.x;
    line_coor1.y = p_blk_coor->y;
    gra_line (&line_coor1, &line_coor2, CAL_BLOCK_BORDER_COLOR, CAL_BLOCK_BORDER);

    /* draw backgroud color */
    if (bg_color != CAL_NO_COLOR) {
        GUI_COOR rect_coor = {p_blk_coor->x + CAL_BLOCK_BORDER,
                              p_blk_coor->y + CAL_BLOCK_BORDER};
        GUI_SIZE day_blk_size = {CAL_BLOCK_WIDTH - 2 * CAL_BLOCK_BORDER,
                                 CAL_BLOCK_HEIGHT - 2 * CAL_BLOCK_BORDER};
        gra_rect (&rect_coor, &day_blk_size, bg_color);
    }

    /* calculate the string start coordinate */
    day_str_width  = GUI_FONT_WIDTH * (day<10 ? 1 : 2);
    day_str_coor.y = p_blk_coor->y + (CAL_BLOCK_HEIGHT - 2 * GUI_FONT_HEIGHT) / 2;
    day_str_coor.x = p_blk_coor->x + (CAL_BLOCK_WIDTH - day_str_width) / 2;

    /* draw day string */
    font_printf (&day_str_coor, str_color, "%d", day);

    day_str_coor.y += GUI_FONT_HEIGHT;
    day_str_coor.x = p_blk_coor->x + (CAL_BLOCK_WIDTH - GUI_FONT_WIDTH * strlen(nong_str)) / 2;
    han_draw_string (&day_str_coor, str_color, (uint8 *)nong_str, strlen(nong_str));
}

/*==============================================================================
 * - _calendar_clear_day_blks()
 *
 * - clear the day blocks area
 */
static void _calendar_clear_day_blks ()
{
    GUI_COOR rect_coor = {CAL_BLOCK_START_X, CAL_BLOCK_START_Y};
    GUI_SIZE day_blks_size = {CAL_BLOCK_WIDTH * CAL_COL_NUM,
                              CAL_BLOCK_HEIGHT * CAL_ROW_NUM};
    gra_rect (&rect_coor, &day_blks_size, CAL_BG_COLOR);
}

/*==============================================================================
 * - _set_str_left_to()
 *
 * - move the set string left to <pos>
 */
static void _set_str_left_to (int pos)
{
	int i;
    int len = 1;
    int src_x, dst_x;
    int start_y;

    if ((pos >= 5) && ((pos - 5) % 3 == 0)) {
        len = 2;
    }
    /* calculator coordinate */
    src_x = CUR_SET_CHAR_X - (pos - len) * GUI_FONT_WIDTH;
    dst_x = CUR_SET_CHAR_X - pos * GUI_FONT_WIDTH;
    start_y = CUR_SET_CHAR_Y;

    for (i = 0; i < GUI_FONT_HEIGHT; i++) {
        memcpy (lcd_get_addr (i + start_y, dst_x),
                lcd_get_addr (i + start_y, src_x),
                sizeof (GUI_COLOR) * GUI_FONT_WIDTH * (SET_STR_LEN + len)); 
    }

    _G_set_pos = pos;
}

/*==============================================================================
 * - _set_str_clear()
 *
 * - clear the set string
 */
static void _set_str_clear ()
{
    GUI_COOR set_str_coor;
    GUI_SIZE set_str_size = {GUI_FONT_WIDTH * SET_STR_LEN, GUI_FONT_HEIGHT};

    /* calculator coordinate */
    set_str_coor.x = CUR_SET_CHAR_X - _G_set_pos * GUI_FONT_WIDTH;
    set_str_coor.y = CUR_SET_CHAR_Y;

    gra_rect (&set_str_coor, &set_str_size, GUI_BG_COLOR);

    _G_set_mod = 0;
}

/*==============================================================================
 * - _draw_current_year_month()
 *
 * - show current blocks screen year and month string
 */
static void _draw_current_year_month ()
{
    int len;
    char cur_month_str[40];
    GUI_COOR left_up;

    sprintf (cur_month_str, "公元 %04d 年 %02d 月",  _G_blks_scr.cur_year,  _G_blks_scr.cur_month);
    len = strlen(cur_month_str);
    left_up.x = CAL_FRAME_START_X + (CAL_FRAME_WIDTH - GUI_FONT_WIDTH * len) / 2;
    left_up.y = CAL_FRAME_START_Y + CAL_FRAME_HEIGHT + 4;
    han_draw_string (&left_up, GUI_COLOR_BLUE, (uint8 *)cur_month_str, len);
}

/*==============================================================================
 * - _prev_month_to()
 *
 * - slide prev month screen to <pos> [-1, BLKS_SCR_WIDTH-1]
 *   when pos == 0, it means change to prev month screen
 *   when pos == CAL_BLOCK_WIDTH * CAL_COL_NUM,
 *               it means prev month screen hide totally
 */
static void _prev_month_to (int pos, int delay)
{
    int x = _G_prev_month_pos;
    int y;
    GUI_COOR pixel_coor;

    if (x < pos) { /* make slide to right */
        x++;
        pixel_coor.x = CAL_BLOCK_START_X + x;
        for (; x <= pos; x++) {
            pixel_coor.y = CAL_BLOCK_START_Y;
            for (y = 0; y < BLKS_SCR_HEIGHT; y++) {
                gra_set_pixel (&pixel_coor,
                    *(_G_blks_scr.prev_month_scr +
                    BLKS_SCR_WIDTH * y + x));
                pixel_coor.y++;
            }
            lcd_delay(delay);
            pixel_coor.x++;
        }
    } else if (x > pos) { /* make slide to left */
        pixel_coor.x = CAL_BLOCK_START_X + x;
        for (; x > pos; x--) {
            pixel_coor.y = CAL_BLOCK_START_Y;
            for (y = 0; y < BLKS_SCR_HEIGHT; y++) {
                gra_set_pixel (&pixel_coor,
                    *(_G_blks_scr.cur_month_scr +
                    BLKS_SCR_WIDTH * y + x));
                pixel_coor.y++;
            }
            lcd_delay(delay);
            pixel_coor.x--;
        }
    }

    _G_prev_month_pos = pos;

    /*
     * prepare for next press
     */
    if (_G_prev_month_pos == BLKS_SCR_WIDTH - 1) {
        int prev_days_of_month;
        _G_prev_month_pos = -1;

        _G_blks_scr.next_year  = _G_blks_scr.cur_year;
        _G_blks_scr.next_month = _G_blks_scr.cur_month;
        _G_blks_scr.cur_year   = _G_blks_scr.prev_year;
        _G_blks_scr.cur_month  = _G_blks_scr.prev_month;
        _G_blks_scr.prev_year  = _G_blks_scr.prev_year;
        _G_blks_scr.prev_month = _G_blks_scr.prev_month - 1;

        if (_G_blks_scr.prev_month == 0) {
            _G_blks_scr.prev_month = 12;
            _G_blks_scr.prev_year--;
        }

        prev_days_of_month = _month_days(_G_blks_scr.prev_year, _G_blks_scr.prev_month);

        _G_blks_scr.next_first_week_day = _G_blks_scr.cur_first_week_day;
        _G_blks_scr.cur_first_week_day  = _G_blks_scr.prev_first_week_day;
        _G_blks_scr.prev_first_week_day = (_G_blks_scr.cur_first_week_day - prev_days_of_month + 35) % 7;;

        free (_G_blks_scr.next_month_scr);
        _G_blks_scr.next_month_scr = _G_blks_scr.cur_month_scr;
        _G_blks_scr.cur_month_scr  = _G_blks_scr.prev_month_scr;
        _G_blks_scr.prev_month_scr = _calendar_get_blks_scr (_G_blks_scr.prev_year, _G_blks_scr.prev_month,
                 _G_blks_scr.prev_first_week_day);

        _calendar_unreg_day_cbis ();
        _calendar_reg_day_cbis ();

        _draw_current_year_month ();
    }
}

/*==============================================================================
 * - _next_month_to()
 *
 * - slide next month screen to <pos> [0, CAL_BLOCK_WIDTH * CAL_COL_NUM]
 *   when pos == 0, it means change to next month screen
 *   when pos == CAL_BLOCK_WIDTH * CAL_COL_NUM,
 *               it means next month screen hide totally
 */
static void _next_month_to (int pos, int delay)
{
    int x = _G_next_month_pos;
    int y;
    GUI_COOR pixel_coor;

    if (x < pos) { /* make slide to right */
        pixel_coor.x = CAL_BLOCK_START_X + x;
        for (; x < pos; x++) {
            pixel_coor.y = CAL_BLOCK_START_Y;
            for (y = 0; y < BLKS_SCR_HEIGHT; y++) {
                gra_set_pixel (&pixel_coor,
                    *(_G_blks_scr.cur_month_scr +
                    BLKS_SCR_WIDTH * y + x));
                pixel_coor.y++;
            }
            lcd_delay(delay);
            pixel_coor.x++;
        }
    } else if (x > pos) { /* make slide to left */
        x--;
        pixel_coor.x = CAL_BLOCK_START_X + x;
        for (; x >= pos; x--) {
            pixel_coor.y = CAL_BLOCK_START_Y;
            for (y = 0; y < BLKS_SCR_HEIGHT; y++) {
                gra_set_pixel (&pixel_coor,
                    *(_G_blks_scr.next_month_scr +
                    BLKS_SCR_WIDTH * y + x));
                pixel_coor.y++;
            }
            lcd_delay(delay);
            pixel_coor.x--;
        }
    }

    _G_next_month_pos = pos;

    /*
     * prepare for next press
     */
    if (_G_next_month_pos == 0) {
        int cur_days_of_month;
        _G_next_month_pos = BLKS_SCR_WIDTH;

        _G_blks_scr.prev_year  = _G_blks_scr.cur_year;
        _G_blks_scr.prev_month = _G_blks_scr.cur_month;
        _G_blks_scr.cur_year   = _G_blks_scr.next_year;
        _G_blks_scr.cur_month  = _G_blks_scr.next_month;
        _G_blks_scr.next_year  = _G_blks_scr.next_year;
        _G_blks_scr.next_month = _G_blks_scr.next_month + 1;

        if (_G_blks_scr.next_month == 13) {
            _G_blks_scr.next_month = 1;
            _G_blks_scr.next_year++;
        }

        cur_days_of_month = _month_days(_G_blks_scr.cur_year, _G_blks_scr.cur_month);

        _G_blks_scr.prev_first_week_day = _G_blks_scr.cur_first_week_day;
        _G_blks_scr.cur_first_week_day  = _G_blks_scr.next_first_week_day;
        _G_blks_scr.next_first_week_day = (_G_blks_scr.cur_first_week_day + cur_days_of_month) % 7;

        free (_G_blks_scr.prev_month_scr);
        _G_blks_scr.prev_month_scr = _G_blks_scr.cur_month_scr;
        _G_blks_scr.cur_month_scr  = _G_blks_scr.next_month_scr;
        _G_blks_scr.next_month_scr = _calendar_get_blks_scr (_G_blks_scr.next_year, _G_blks_scr.next_month,
                _G_blks_scr.next_first_week_day);

        _calendar_unreg_day_cbis ();
        _calendar_reg_day_cbis ();

        _draw_current_year_month ();
    }
}

/*==============================================================================
 * - _calendar_get_blks_scr()
 *
 * - get <year> <month> blocks screen pixel data
 */
static GUI_COLOR *_calendar_get_blks_scr (int year, int month,
                                          int first_week_day)
{
    int i;
    int days_of_month;
    int i_week_day;
    GUI_COOR day_blk_coor;
    GUI_COLOR day_blk_color;
    GUI_COLOR day_str_color;
    GUI_COLOR *p;

    NONGLI_DATE nong_date;
    char *nong_str = "无";
    int ret;

    int temp_blks_x = CAL_BLOCK_START_X + gra_scr_w();
    int temp_blks_y = CAL_BLOCK_START_Y;

    /* clear temp screen area */
    GUI_COOR rect_coor = {temp_blks_x, temp_blks_y};
    GUI_SIZE day_blks_size = {BLKS_SCR_WIDTH, BLKS_SCR_HEIGHT};
    gra_rect (&rect_coor, &day_blks_size, CAL_BG_COLOR);

    /* 计算当前时间, 所在月份一共有几天 */
    days_of_month = _month_days(year, month);

    /* draw and register day cbis */
    i_week_day = first_week_day;
    for (i = 1; i <= days_of_month; i++) {                         /*  The days 1# ... 31#  START   */

        /* calculate the day block start coordinate */
        day_blk_coor.y = temp_blks_y + ((first_week_day + i - 1) / 7) * CAL_BLOCK_HEIGHT;
        day_blk_coor.x = temp_blks_x + i_week_day * CAL_BLOCK_WIDTH;

        /* set color */
        if (i == 0) { /* today */
            day_str_color = CAL_TODAY_FG_COLOR;
            day_blk_color = CAL_TODAY_BG_COLOR;
        } else if (i_week_day == 0 || i_week_day == 6) { /* weekend */
            day_str_color = CAL_WEEKEND_FG_COLOR;
            day_blk_color = CAL_WEEKEND_BG_COLOR;
        } else { /* normal day */
            day_str_color = CAL_DAY_COLOR;
            day_blk_color = CAL_NO_COLOR;
        }

        ret = nongli_get_date (year, month, i, &nong_date);
        if (ret != -1) {
            if (nong_date.jie_qi[0] != '\0') {
                nong_str = nong_date.jie_qi;
            } else if (nong_date.ri == 1) {
                nong_str = &nong_date.ri_qi[0];
                nong_date.ri_qi[4] = '\0';
            } else {
                nong_str = &nong_date.ri_qi[4];
            }
        }
        /* draw */
        _calendar_draw_a_day_blk (&day_blk_coor, i, day_str_color, day_blk_color, nong_str);

        i_week_day = (i_week_day + 1) % 7;
    }                                                              /*  The days 1# ... 31#  END     */

    p = calloc (1, BLKS_SCR_WIDTH * BLKS_SCR_HEIGHT * sizeof (GUI_COLOR));
    GUI_ASSERT (p != NULL);
    for (i = 0; i < BLKS_SCR_HEIGHT; i++) {
        memcpy (p + i * BLKS_SCR_WIDTH, 
                lcd_get_addr (temp_blks_y + i, temp_blks_x),
                BLKS_SCR_WIDTH * sizeof (GUI_COLOR));
    }

    return p;
}

/*==============================================================================
** FILE END
==============================================================================*/

