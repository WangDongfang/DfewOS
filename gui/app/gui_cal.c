/*==============================================================================
** gui_cal.c -- calculator in GUI.
*
                                   +--------------+     +--------------+
-----> Press           Release --> |  rec_segment | --> |  rec_strock  | --> out
   A     |                A     |  +--------------+     +--------------+  |
   |     |                |     |                                         |
   |     V       Drag     | 2nd strock                                    |
   |     +----------------+     |    +----------------+                   |
   |     | Get directions |     +--> | Simple analyse | -------------------> out
   |     +----------------+          +----------------+                   |
   |                                                                      |
   |      After recognise first strock, we know we shoud wait second.     |
   +----------------------------------------------------------------------+

*
**
** MODIFY HISTORY:
**
** 2011-11-23 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "../gui.h"
#include "../../calculator/calculator.h"
extern int sprintf(char *buf, const char *, ...);

/*======================================================================
  const values
======================================================================*/
#define MAX_COOR_NUM        100
#define MAX_EXPR_LEN        50
#define TAN_225             0.4142  /* tan(22.5) */
#define TAN_675             2.4142  /* tan(67.5) */

/*======================================================================
  configs
======================================================================*/
/* expression panel */
#define _BLANK_W            8
#define _BLANK_H            4
#define _EXPR_LINE_NUM      8
#define _EXPR_START_Y       (_BLANK_H + GUI_FONT_HEIGHT)
#define _EXPR_END_Y         (_EXPR_START_Y + GUI_FONT_HEIGHT * _EXPR_LINE_NUM)

/* dir offset */
#define _MIN_OFFSET         8

/* un-recognise char */
#define _UNKOWN_CHAR        ' '

/*======================================================================
  segment types
======================================================================*/
typedef enum segment_type {
    SEGMENT_NONE,
    SEGMENT_LINE,
    SEGMENT_CLOCKWISE,
    SEGMENT_ANTI_CLOCKWISE
} SEGMENT_TYPE;

/*======================================================================
  Global variable
======================================================================*/
static GUI_COOR _G_coors[MAX_COOR_NUM];
static int      _G_coor_num = 0;

static char     _G_dirs[MAX_COOR_NUM];
static int      _G_dir_num = 0;

static char     _G_expr[MAX_EXPR_LEN];
static int      _G_expr_len = 0;

static GUI_COOR _G_expr_coor = {_BLANK_W, _EXPR_START_Y};

static int  _G_igno_second_strock = 0;
static int  _G_want_second_strock = 0;
static int  _G_no_dot = 0;

/*======================================================================
  Function forward declare
======================================================================*/
static OS_STATUS _cal_cb_press (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
static OS_STATUS _cal_cb_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor);
static OS_STATUS _cal_cb_release (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
static OS_STATUS _cal_cb_home (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
static OS_STATUS _cal_cb_clear (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
static OS_STATUS _cal_cb_back (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);

static void _PUT2 (char ch, GUI_COLOR color);
static void _draw_big_point (GUI_COOR coor, GUI_COLOR color);

/*==============================================================================
 * - app_cal()
 *
 * - calculator application entry, called by home app
 */
OS_STATUS app_cal (GUI_CBI *pCBI, GUI_COOR *coor)
{
    GUI_CBI *pCBI_cal = malloc (sizeof (GUI_CBI));

    if (pCBI_cal == NULL) {
        return OS_STATUS_OK;
    }

    /* clear screen */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR);

    /* accelerate touch screen dirver speed */
    extern int G_message_delay; /* implement in touch dirver */
    G_message_delay = 1;

    /* init cbi struct */
    strcpy (pCBI_cal->name, "cal cbi");

    pCBI_cal->left_up.x = 0;
    pCBI_cal->left_up.y = 0;
    pCBI_cal->right_down.x = gra_scr_w() - ICON_SIZE - 1;
    pCBI_cal->right_down.y = gra_scr_h() - 1;

    pCBI_cal->func_press   = _cal_cb_press;
    pCBI_cal->func_leave   = cbf_default_leave;
    pCBI_cal->func_release = _cal_cb_release;
    pCBI_cal->func_drag    = _cal_cb_drag;

    pCBI_cal->data = NULL;

    cbi_register (pCBI_cal);

    /* register operate cbis */
    {
        int i;

        ICON_CBI ic_right[] = {
            {"/n1/gui/cbi_home.jpg",   _cal_cb_home},
            {"/n1/gui/cbi_clear.jpg",  _cal_cb_clear},
            {"/n1/gui/cbi_back.jpg",   _cal_cb_back}
        };
        GUI_CBI *pCBI;
        GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, 0};
        GUI_SIZE size = {0, 0};

        for (i = 0; i < N_ELEMENTS(ic_right); i++) {

            left_up.y = i * ICON_SIZE * 2; /* calculate start y coor */
            pCBI = cbi_create_default (ic_right[i].name, &left_up, &size, FALSE);

            if (ic_right[i].func == _cal_cb_home) {
                pCBI->func_release = ic_right[i].func; /* home */
            } else {
                pCBI->func_press = ic_right[i].func;   /*  */
            }

            cbi_register (pCBI);
        }
    }

    _G_expr_len = 0;
    _G_igno_second_strock = 0;
    _G_want_second_strock = 0;
    _G_no_dot = 0;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - rec_segment()
 *
 * - recognise a segment type form <dir_str>
 *   the end point is store in <*p_dir_str>
 */
SEGMENT_TYPE rec_segment (const char *dir_str, int *p_max_turn, const char **p_dir_str)
{
    SEGMENT_TYPE seg_rec = SEGMENT_NONE;
    int turn_degree = 0;
    int dir_diff;

    /* empty direction string */
    if (strlen (dir_str) <= 2) {
        goto _recognised;
    }
    *p_max_turn = 0;

    /* read the dirs couple */
    dir_str+=2;

    while (*dir_str) {

        dir_diff = (*dir_str - *(dir_str-1) + 8) % 8;

        if (dir_diff == 0) {       /* 直线   */

        } else if (dir_diff < 4) { /* 顺时针 */
            if ( (turn_degree == 0) && (dir_diff >= 2) ) { /* 以前是 line, 这次拐的角度较大*/
                break; /* 停止识别 返回 line */
            }
            if ( (turn_degree < -1) && (dir_diff >= 2) ) { /* 以前是 anti-clockwise */
                break; /* 停止识别 返回 anti-clockwise */
            }

            /* add clockwise degree */
            turn_degree += dir_diff;
            if (GUI_ABS(turn_degree) > *p_max_turn) {
                *p_max_turn = GUI_ABS(turn_degree);
            }
        } else {                   /* 逆时针 */
            if ( (turn_degree == 0) && (dir_diff <= 6) ) { /* 以前是 line, 这次拐的角度较大*/
                break;
            }
            if ( (turn_degree > 1) && (dir_diff <= 6) ) {  /* 以前是 clockwise */
                break;
            }

            turn_degree += dir_diff - 8;
            if (GUI_ABS(turn_degree) > *p_max_turn) {
                *p_max_turn = GUI_ABS(turn_degree);
            }
        }

        dir_str++;
    }

    /* extern line area */
    if (GUI_ABS(turn_degree) <= 1) {
        turn_degree = 0;
    }

    /* recognise segment type */
    if (turn_degree == 0) {
        seg_rec = SEGMENT_LINE;
    } else if (turn_degree > 0) {
        seg_rec = SEGMENT_CLOCKWISE;
    } else if (turn_degree < 0) {
        seg_rec = SEGMENT_ANTI_CLOCKWISE;
    }

_recognised:
    *p_dir_str = dir_str;

    return seg_rec;
}

/*==============================================================================
 * - rec_strock()
 *
 * - recognise first strock
 */
char rec_strock (const char *dir_str)
{
    int i = 0;
    int strock_max_turn = 0;
    const char *save_p  = dir_str;
    char start_dir = dir_str[1];
    char end_dir   = dir_str[strlen (dir_str) - 2];
    SEGMENT_TYPE segs[40];
    SEGMENT_TYPE cur_seg;

    char rec_ch = _UNKOWN_CHAR;

    /* recognise segments */
    cur_seg = rec_segment (dir_str, &strock_max_turn, &dir_str);
    while (cur_seg != SEGMENT_NONE) {
        /* _PUT2 ('A' + cur_seg - 1, GUI_COLOR_RED); */
        segs[i++] = cur_seg;
        cur_seg = rec_segment (dir_str, &strock_max_turn, &dir_str);
    }
    /* _PUT2 ('>', GUI_COLOR_CYAN); */

    /* return result, according as segments */
    if (i == 1) {
        switch (segs[0]) {
            case SEGMENT_LINE:
                if (strock_max_turn <= 1) {
                    rec_ch = '1'; /* A */
                } else if (strock_max_turn <= 2) {
                    rec_ch = '('; /* A */
                } else {
                    rec_ch = '8'; /* A */
                }
                break;
            case SEGMENT_CLOCKWISE:
                if (strock_max_turn <= 6) {
                    int i = 2;
                    int diff;
                    int turn = 0;
                    while (save_p[i] != '\0') {
                        diff = (save_p[i] - save_p[i-1] + 8) % 8;
                        if (diff == 0) {
                            if (turn != 0) {
                                turn--;
                            }
                        } else {
                            turn += 2 * diff;
                        }

                        if (turn > 2) {
                            break;
                        }
                        i++;
                    }

                    if (save_p[i] != '\0') {
                        rec_ch = '7'; /* B */
                    } else {
                        rec_ch = ')'; /* B */
                    }
                } else if (strock_max_turn <= 9) {
                    rec_ch = '0'; /* B */
                } else {
                    rec_ch = '3'; /* B */
                }
                break;
            case SEGMENT_ANTI_CLOCKWISE:
                if (strock_max_turn <= 4) {
                    int i = 2;
                    int diff;
                    int turn;
                    while (save_p[i] != '\0') {
                        diff = (save_p[i] - save_p[i-1] + 8) % 8;
                        if (diff == 0) {
                            turn = 0;
                        } else {
                            turn += 8 - diff;
                        }

                        if (turn == 2) {
                            break;
                        }
                        i++;
                    }

                    if (save_p[i] != '\0') {
                        rec_ch = '4'; /* C */
                    } else {
                        rec_ch = '('; /* C */
                    }
                } else if (strock_max_turn <= 9) {
                    if (GUI_ABS(_G_coors[_G_coor_num-1].x - _G_coors[0].x) < 30 &&
                        GUI_ABS(_G_coors[_G_coor_num-1].y - _G_coors[0].y) < 30) {
                        rec_ch = '0'; /* C */
                    } else {
                        int last = strlen (save_p) - 1;
                        while (save_p[last] == save_p[last-1]) {
                            last--;
                        }
                        if (last <= strlen(save_p) - 4) {
                            rec_ch = '9'; /* C */
                        } else {
                            rec_ch = '6'; /* C */
                        }
                    }
                } else {
                    rec_ch = '9'; /* C */
                }
                break;
            default:
                break;
        }
    } else if (i == 2) {
        switch (segs[0]) {
            case SEGMENT_LINE:
                if (segs[1] == SEGMENT_LINE) {
                    if (start_dir > end_dir) {
                        rec_ch = '4'; /* AA */
                    } else {
                        rec_ch = '7'; /* AA */
                    }
                } else if (segs[1] == SEGMENT_CLOCKWISE) {
                    rec_ch = '5'; /* AB */
                }
                break;
            case SEGMENT_CLOCKWISE:
                if (segs[1] == SEGMENT_LINE) {
                    rec_ch = '2'; /* BA */
                } else if (segs[1] == SEGMENT_CLOCKWISE) {
                    rec_ch = '3'; /* BB */
                }
                break;
            case SEGMENT_ANTI_CLOCKWISE:
                if (segs[1] == SEGMENT_LINE) {
                    rec_ch = '9'; /* CA */
                } else if (segs[1] == SEGMENT_CLOCKWISE) {
                    rec_ch = '8'; /* CB */
                }
                break;
            default:
                break;
        }
    } else {
        rec_ch = '9'; /* ACA */
    }

    return rec_ch;
}

/*==============================================================================
 * - _PUT2()
 *
 * - draw recongised char on lcd
 */
static void _PUT2 (char ch, GUI_COLOR color)
{
    /* igno space */
    if (ch == ' ') {
        return ;
    }

    /* back */
    if ((ch == '<')) {
        if (_G_expr_len > 0) {
            _G_expr_len--;
            _G_expr_coor.x -= GUI_FONT_WIDTH;
            font_draw_string (&_G_expr_coor, color, (const uint8 *)&ch, 1);
        }
        return ;
    }

    /* new line */
    if (ch =='\n') {
        _G_expr_coor.x = gra_scr_w();
        return ;
    }

    /* print char */
    if (_G_expr_coor.x >= gra_scr_w() - ICON_SIZE - _BLANK_W) {
        _G_expr_coor.x = _BLANK_W;
        _G_expr_coor.y += GUI_FONT_HEIGHT;
        if (_G_expr_coor.y >= _EXPR_END_Y) {
            _G_expr_coor.y = _EXPR_START_Y;

            /* clear the expression panel */
            GUI_SIZE size = {gra_scr_w() - 2*_BLANK_W - ICON_SIZE, /* width */
                             _EXPR_LINE_NUM * GUI_FONT_HEIGHT};  /* height */
            gra_rect (&_G_expr_coor, &size, GUI_BG_COLOR);
        }

    }
    font_draw_string (&_G_expr_coor, color, (const uint8 *)&ch, 1);
    _G_expr_coor.x += GUI_FONT_WIDTH;

    /* store char or calculate the expression */
    if (ch != '=') {
        _G_expr[_G_expr_len++] = ch;
    } else {
        int  i = 0;
        CAL_TYPE  value;
        char val_str[20];

        _G_expr[_G_expr_len] = '\0';

        /* calculte and show */
        value = calculate (_G_expr);

        /* draw result value */
#ifdef CAL_TYPE_INT
        sprintf(val_str, "%d\n", value);
#else
        sprintf(val_str, "%f\n", value);
#endif
        while (val_str[i] != '\0') {
            _PUT2 (val_str[i], GUI_COLOR_MAGENTA);
            i++;
        }

        _G_expr_len = 0;
    }
}

/*==============================================================================
 * - _PUT()
 *
 * - draw a dir character on lcd,
 *   when user release touch screen, start recognise process
 */
static void _PUT (char dir)
{
#if 0
    static GUI_COOR ch_coor = {_BLANK_W, _BLANK_H};

    /* draw dir on the lcd */
    if (ch_coor.x >= gra_scr_w() - ICON_SIZE - _BLANK_W) {
        GUI_SIZE size = {gra_scr_w() - ICON_SIZE - 2 * _BLANK_W, GUI_FONT_HEIGHT};
        ch_coor.x = _BLANK_W;
        gra_rect (&ch_coor, &size, GUI_BG_COLOR);
    }
    font_draw_string (&ch_coor, GUI_COLOR_WHITE, (const uint8 *)&dir, 1);
    ch_coor.x += GUI_FONT_WIDTH;
#endif

    /* store this dir */
    _G_dirs[_G_dir_num++] = dir;
}

/*==============================================================================
 * - _cal_cb_press()
 *
 * - first press record the screen coordinate
 */
static OS_STATUS _cal_cb_press (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    /* start record touch coordinate */
    _G_coor_num = 0;

    /* start record direction */
    _G_dir_num = 0; 

    /* store the first coordinate */
    _G_coors[_G_coor_num++] = *p_cbi_coor;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _cal_cb_drag()
 *
 * - update screen coodinate and draw link line
 */
static OS_STATUS _cal_cb_drag (GUI_CBI *pCBI, GUI_COOR *p_offset_coor)
{
    static GUI_COOR sum_offset = {0, 0};

    /* check coor number */
    if (_G_coor_num >= MAX_COOR_NUM) {
        return OS_STATUS_ERROR;
    }

    /* store current coor */
    _G_coors[_G_coor_num].x = _G_coors[_G_coor_num - 1].x + p_offset_coor->x;
    _G_coors[_G_coor_num].y = _G_coors[_G_coor_num - 1].y + p_offset_coor->y;

    /* draw point and line */
    _draw_big_point (_G_coors[_G_coor_num], GUI_COLOR_YELLOW);
    gra_line_u (&_G_coors[_G_coor_num - 1], &_G_coors[_G_coor_num], GUI_COLOR_GREEN, 2);

    /* first drag, so we clear <sum_offset> */
    if (_G_coor_num == 1) {
        sum_offset.x = 0;
        sum_offset.y = 0;
    }

    /* adding offset */
    sum_offset.x += p_offset_coor->x;
    sum_offset.y += p_offset_coor->y;

    /* check the sum offset */
    if (GUI_ABS(sum_offset.x) + GUI_ABS(sum_offset.y) > _MIN_OFFSET) { 
        p_offset_coor->x = sum_offset.x;
        p_offset_coor->y = sum_offset.y;
    }

    /* according as offset, to get the direction */
    if (GUI_ABS(p_offset_coor->x) + GUI_ABS(p_offset_coor->y) > _MIN_OFFSET) {
        double dx = p_offset_coor->x;
        double dy = p_offset_coor->y;
        double dtan = GUI_ABS(dy) / GUI_ABS(dx);

        /* reverse the y axis */
        p_offset_coor->y = -p_offset_coor->y;

        /* in y axis */
        if (p_offset_coor->x == 0) {
            _PUT ((p_offset_coor->y > 0) ? 'a' : 'e');
        }

        /* check tan value */
        if ((p_offset_coor->x > 0) && (p_offset_coor->y >= 0)) {        /* first quadrant */
            if (dtan <= TAN_225) {
                _PUT('c');
            } else if ((dtan > TAN_225) && ((dtan < TAN_675))) {
                _PUT('b');
            } else if ((dtan >= TAN_675)) {
                _PUT('a');
            }
        } else if ((p_offset_coor->x > 0) && (p_offset_coor->y < 0)) {  /* fourth quadrant */
            if (dtan <= TAN_225) {
                _PUT('c');
            } else if ((dtan > TAN_225) && ((dtan < TAN_675))) {
                _PUT('d');
            } else if ((dtan >= TAN_675)) {
                _PUT('e');
            }
        } else if ((p_offset_coor->x < 0) && (p_offset_coor->y >= 0)) { /* second quadrant */
            if (dtan <= TAN_225) {
                _PUT('g');
            } else if ((dtan > TAN_225) && ((dtan < TAN_675))) {
                _PUT('h');
            } else if ((dtan >= TAN_675)) {
                _PUT('a');
            }
        } else if ((p_offset_coor->x < 0) && (p_offset_coor->y < 0)) {  /* third quadrant */
            if (dtan <= TAN_225) {
                _PUT('g');
            } else if ((dtan > TAN_225) && ((dtan < TAN_675))) {
                _PUT('f');
            } else if ((dtan >= TAN_675)) {
                _PUT('e');
            }
        }

        sum_offset.x = 0;
        sum_offset.y = 0;
    }

    /* inc <_G_coor_num> */
    _G_coor_num++;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _cal_cb_release()
 *
 * - try to recognise a character
 */
static OS_STATUS _cal_cb_release (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    static int  last_x;
    char rec_ch = ' ';

    /* end the direction sequence */
    _PUT ('\0');

    /*
     * start recognise
     */

    /* process second strock */
    if (_G_igno_second_strock) { /* igno */
        _PUT2 ((char)_G_igno_second_strock, GUI_COLOR_WHITE);
        _G_igno_second_strock = 0;
        goto _finish;
    }

    if (_G_want_second_strock == 1) { /* check */
        int i;
        int e_num = 0;
        int c_num = 0;
        for (i = 0; i < _G_dir_num; i++) {
            if (_G_dirs[i] == 'e') {
                e_num++;
            } else if (_G_dirs[i] == 'c') {
                c_num++;
            }
        }

        if (_G_coors[0].x > last_x) { /* this is a new char */
            rec_ch = '-';
        } else if (_G_dir_num <= 3) { /* this strock is little */
            rec_ch = '/'; _G_no_dot = 1;
        } else if (e_num >= _G_dir_num / 2) { /* this strock almost all 'e' dir */
            rec_ch = '+';
        } else if (c_num >= _G_dir_num / 2) { /* this strock almost all 'c' dir */
            rec_ch = '=';
        }

        _PUT2 ('<', GUI_BG_COLOR);
        _PUT2 (rec_ch, GUI_COLOR_WHITE);

        _G_want_second_strock = 0;

        if (rec_ch != '-') {
            goto _finish;
        }
    }

    /* process first strock */
    if (_G_dir_num >= 3) {
        rec_ch = rec_strock (_G_dirs);

        switch (rec_ch) {
            case '1':
                {
                    int i;
                    int e_num = 0;
                    int c_num = 0;
                    int f_num = 0;

                    for (i = 0; i < _G_dir_num; i++) {
                        if (_G_dirs[i] == 'e') {
                            e_num++;
                        } else if (_G_dirs[i] == 'c') {
                            c_num++;
                        } else if (_G_dirs[i] == 'f') {
                            f_num++;
                        }
                    }

                    if (e_num >= _G_dir_num / 2) {
                    } else if (c_num >= _G_dir_num / 2) {
                        last_x = _G_coors[_G_coor_num-1].x;
                        _PUT2 ('-', GUI_COLOR_WHITE);
                        _G_want_second_strock = 1;
                    } else if (f_num >= _G_dir_num / 2) {
                        _G_igno_second_strock = '*';
                    } else {
                        //GUI_ASSERT(0);
                    }
                    break;
                }
            case '4':
            case '5':
                _G_igno_second_strock = rec_ch;
                break;
            default:
                break;
        }

        if (_G_want_second_strock == 0 &&
            _G_igno_second_strock == 0) {
            _PUT2 (rec_ch, GUI_COLOR_WHITE);
        }
    } else {
        if (_G_no_dot) {
            _G_no_dot = 0;
        } else {
            _PUT2 ('.', GUI_COLOR_WHITE);
        }
    }

_finish:
    cbf_default_release (pCBI, p_cbi_coor);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _cal_cb_home()
 *
 * - clear screen and go to home
 */
static OS_STATUS _cal_cb_home (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    /* un-accelerate touch screen dirver speed */
    extern int G_message_delay; /* implement in touch dirver */
    G_message_delay = 10;

    /* clear screen */
    gra_clear(GUI_BG_COLOR);

    /* go home */
    cbf_go_home (pCBI, p_cbi_coor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _cal_cb_clear()
 *
 * - clear user input panel, and clear expr.
 */
static OS_STATUS _cal_cb_clear (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    GUI_COOR coor = {0, _EXPR_END_Y};
    GUI_SIZE size = {gra_scr_w() - ICON_SIZE, gra_scr_h() - _EXPR_END_Y};

    /* clear user input panel */
    gra_rect (&coor, &size, GUI_BG_COLOR);

    _G_expr_len = 0;

    _PUT2 ('\n', GUI_BG_COLOR);

    _G_igno_second_strock = 0;
    _G_want_second_strock = 0;
    _G_no_dot = 0;

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _cal_cb_back()
 *
 * - delete a expression character
 */
static OS_STATUS _cal_cb_back (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    int i;
    for (i = 1; i < _G_coor_num; i++) {
        _draw_big_point (_G_coors[i], GUI_BG_COLOR);
        gra_line_u (&_G_coors[i - 1], &_G_coors[i], GUI_BG_COLOR, 2);
    }

    _G_coor_num = 0;

    _PUT2 ('<', GUI_BG_COLOR);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _draw_big_point()
 *
 * - show a 4*4 pixels point on <coor>
 */
static void _draw_big_point (GUI_COOR coor, GUI_COLOR color)
{
#define _POINT_WIDTH  5

    static uint8 point[_POINT_WIDTH][_POINT_WIDTH] = {
        {0, 1, 1, 1, 0},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {0, 1, 1, 1, 0}
    };
    int i, j;
    GUI_COOR t_coor;

    coor.x -= _POINT_WIDTH / 2;
    coor.y -= _POINT_WIDTH / 2;

    for (i = 0; i < _POINT_WIDTH; i++) {
        t_coor.y = coor.y + i;
        for (j = 0; j < _POINT_WIDTH; j++) {
            if (point[i][j]) {
                t_coor.x = coor.x + j;
                gra_set_pixel (&t_coor, color);
            }
        }
    }

#undef _POINT_WIDTH
}

/*==============================================================================
** FILE END
==============================================================================*/

