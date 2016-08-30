/*==============================================================================
** gui_wuzi.c -- Wuzi Qi Game.
**
** MODIFY HISTORY:
**
** 2012-04-18 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include <string.h>
#include "msg_box.h"
#include "../gui.h"
#include "../LibJPEG/jpeg_util.h"
#include "../../wuzi/wuzi.h"

/*======================================================================
  Configs
======================================================================*/
#define WHITE_POT_PIC           "/n1/gui/white_pot.jpg"
#define BLACK_POT_PIC           "/n1/gui/black_pot.jpg"
#define WHITE_TURN_PIC          "/n1/gui/white_hand.jpg"
#define BLACK_TURN_PIC          "/n1/gui/black_hand.jpg"
#define WHITE_MAN_PIC           "/n1/gui/white_chessman.jpg"
#define BLACK_MAN_PIC           "/n1/gui/black_chessman.jpg"
#define CHESSBOARD_START_X      70
#define OUT_MARGIN              20
#define LINE_WIDTH              2
#define BLANK_HIGHT             ((gra_scr_h() - WUZI_ROW_NUM * LINE_WIDTH) / (WUZI_ROW_NUM + 1))
#define BLANK_WIDTH             BLANK_HIGHT
#define CHESSMAN_SIZE           32
#define CBI_OFFSET              (CHESSMAN_SIZE - LINE_WIDTH) / 2
#define CHESSBOARD_END_X        (CHESSBOARD_START_X + WUZI_COL_NUM * LINE_WIDTH + (WUZI_COL_NUM - 1) * BLANK_WIDTH)

/*======================================================================
  Function Forward Declare
======================================================================*/
static OS_STATUS _wuzi_cb_home (GUI_CBI *pCBI_home, GUI_COOR *pCoor);
static OS_STATUS _wuzi_cb_p2p (GUI_CBI *pCBI_p2p, GUI_COOR *coor);
static OS_STATUS _wuzi_cb_p2c (GUI_CBI *pCBI_p2c, GUI_COOR *coor);
static OS_STATUS _wuzi_cb_restart (GUI_CBI *pCBI_restart, GUI_COOR *coor);
static OS_STATUS _wuzi_cb_action (GUI_CBI *pCBI_chessman, GUI_COOR *coor);
static void _get_row_col (const GUI_COOR *p_coor, int *p_row, int *p_col);
static void _get_cbi_coor (int row, int col, GUI_COOR *p_coor);
static void _draw_chessboard ();
static void _reg_chessman_cbis ();
static void _show_turn ();

/*======================================================================
  Global Varibales
======================================================================*/
static int _G_game_over = 0;
static int _G_in_p2p_mode = 0;
static int _G_white_black_turn = 0;
static GUI_CBI _G_last_chessman;

/*==============================================================================
 * - app_wuzi()
 *
 * - wuzi application entry, called by home app
 */
OS_STATUS app_wuzi (GUI_CBI *pCBI_wuzi, GUI_COOR *coor)
{
    int i;
    GUI_CBI *pCBI;
    GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, 0};
    GUI_SIZE size = {0, 0};

    ICON_CBI ic_right[] = {
        {"/n1/gui/cbi_home.jpg",     _wuzi_cb_home},
        {"/n1/gui/cbi_people.jpg",   _wuzi_cb_p2p},
        {"/n1/gui/cbi_computer.jpg", _wuzi_cb_p2c},
        {"/n1/gui/cbi_fresh.jpg",    _wuzi_cb_restart}
    };

    /*
     * clear home app bequest
     */
    cbi_delete_all (); /* delete all other cbis */
    gra_clear (GUI_BG_COLOR); /* clear screen */
    gra_set_show_fb (0);

    /* 
     * register 'home' cbi 
     */
    pCBI = cbi_create_default (ic_right[0].name, &left_up, &size, FALSE);
    pCBI->func_release = ic_right[0].func; /* home */
    cbi_register (pCBI);

    /* register function cbis */
    for (i = 1; i < N_ELEMENTS(ic_right); i++) {
        left_up.y = gra_scr_h() - (N_ELEMENTS(ic_right) - i) * ICON_SIZE;

        pCBI = cbi_create_default (ic_right[i].name, &left_up, &size, FALSE);
        pCBI->func_press = ic_right[i].func; /* reply function */

        cbi_register (pCBI);
    }

    /* show chessman pots */
    {
    GUI_COOR white_pot_coor = {0, gra_scr_h() * 2 / 3};
    GUI_COOR black_pot_coor = {CHESSBOARD_END_X + OUT_MARGIN, gra_scr_h() * 2 / 3};
    GUI_SIZE pot_size = {0, 0};
    jpeg_dump_pic (WHITE_POT_PIC, &white_pot_coor, &pot_size);
    jpeg_dump_pic (BLACK_POT_PIC, &black_pot_coor, &pot_size);
    }

    _wuzi_cb_restart (NULL, NULL);

    _reg_chessman_cbis ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _wuzi_cb_home()
 *
 * - quit wuzi application
 */
static OS_STATUS _wuzi_cb_home (GUI_CBI *pCBI_home, GUI_COOR *pCoor)
{
    /* clear lcd screen */
    gra_clear (GUI_BG_COLOR);

    /* go to home */
    cbf_go_home (pCBI_home, pCoor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _wuzi_cb_p2p()
 *
 * - people to people play
 */
static OS_STATUS _wuzi_cb_p2p (GUI_CBI *pCBI_p2p, GUI_COOR *coor)
{
    _G_in_p2p_mode = 1;
    _wuzi_cb_restart (NULL, NULL);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _wuzi_cb_p2c()
 *
 * - people to computer play
 */
static OS_STATUS _wuzi_cb_p2c (GUI_CBI *pCBI_p2c, GUI_COOR *coor)
{
    _G_in_p2p_mode = 0;
    _G_white_black_turn = 0;
    _wuzi_cb_restart (NULL, NULL);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _wuzi_cb_restart()
 *
 * - restart game. clear and redraw chessboard.
 */
static OS_STATUS _wuzi_cb_restart (GUI_CBI *pCBI_restart, GUI_COOR *coor)
{
    _G_game_over = 0;
    _G_last_chessman.left_up.x = 0;
    _show_turn ();
    _draw_chessboard ();

    wuzi_start ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _show_msg()
 *
 * - show message on chessboard top
 */
static void _show_msg (const char *msg, GUI_COLOR color)
{
    GUI_COOR msg_coor;
    int msg_len = strlen (msg);
    int chessboard_width = WUZI_COL_NUM * LINE_WIDTH + (WUZI_COL_NUM - 1) * BLANK_WIDTH;

    msg_coor.x = CHESSBOARD_START_X + (chessboard_width - GUI_FONT_WIDTH * msg_len) / 2;
    msg_coor.y = BLANK_HIGHT - GUI_FONT_HEIGHT - 2;

    han_draw_string (&msg_coor, color, (uint8 *)msg, msg_len);
}

/*==============================================================================
 * - _mark_computer_chessman()
 *
 * - marked computer last put chessman
 */
static void _mark_computer_chessman (GUI_COOR *pCoor_chessman)
{
    _G_last_chessman.left_up.x = pCoor_chessman->x;
    _G_last_chessman.left_up.y = pCoor_chessman->y;

    _G_last_chessman.right_down.x = pCoor_chessman->x + CHESSMAN_SIZE - 1;
    _G_last_chessman.right_down.y = pCoor_chessman->y + CHESSMAN_SIZE - 1;

    cbf_default_press (&_G_last_chessman, NULL);
}

/*==============================================================================
 * - _unmark_last_computer_chessman()
 *
 * - unmark computer last put chessman
 */
static void _unmark_last_computer_chessman ()
{
    if (_G_last_chessman.left_up.x != 0) {
        cbf_default_release (&_G_last_chessman, NULL);
    }
}

/*==============================================================================
 * - _wuzi_cb_action()
 *
 * - when player press on a chessman postion, call this
 */
static OS_STATUS _wuzi_cb_action (GUI_CBI *pCBI_chessman, GUI_COOR *coor)
{
    static GUI_COLOR *player_chessman_pic = NULL;
    static GUI_COLOR *comput_chessman_pic = NULL;
    static GUI_SIZE   pic_size = {0, 0};

    int player_row, player_col;
    int comput_row, comput_col;
    int chess_status;
    GUI_COOR player_coor = pCBI_chessman->left_up;
    GUI_COOR comput_coor;

    /* check whether game over */
    if (_G_game_over == 1) {
        cbf_default_press (pCBI_chessman, NULL);
        return OS_STATUS_OK;
    }

    /* get chessman picture pixels data */
    if (player_chessman_pic == NULL) {
        player_chessman_pic = get_pic_pixel (WHITE_MAN_PIC, &pic_size.w, &pic_size.h);
    }
    if (comput_chessman_pic == NULL) {
        comput_chessman_pic = get_pic_pixel (BLACK_MAN_PIC, &pic_size.w, &pic_size.h);
    }

    /* get player press which chessman */
    _get_row_col (&player_coor, &player_row, &player_col);
    
    if (_G_in_p2p_mode) {
        GUI_COLOR *cur_pic = NULL;
        if (_G_white_black_turn == 0) {
            chess_status = wuzi_white_do (player_row, player_col);
            cur_pic = player_chessman_pic;
        } else {
            chess_status = wuzi_black_do (player_row, player_col);
            cur_pic = comput_chessman_pic;
        }
        /* error */
        if (chess_status == WUZI_NONLEGAL) {
            _show_msg ("Please change a position.", GUI_COLOR_RED);
            return OS_STATUS_ERROR;
        }
        /* show player chessman */
        gra_block (&player_coor, &pic_size, cur_pic);
        switch (chess_status) {
            case WUZI_GO_AHEAD:
                _G_white_black_turn = 1 - _G_white_black_turn;
                _show_turn ();
                break;
            case WUZI_WHITE_WIN:
                _G_game_over = 1;
                _show_msg ("white winner!", GUI_COLOR_GREEN);
                break;
            case WUZI_BLACK_WIN:
                _G_game_over = 1;
                _show_msg ("black winner!", GUI_COLOR_GREEN);
                break;
            case WUZI_DOGFALL:
                _G_game_over = 1;
                _show_msg ("dogfall!", GUI_COLOR_GREEN);
                break;
            default:
                break;
        }
        return OS_STATUS_OK;
    }

    /* do a player & computer round */
    chess_status = wuzi_round_do (player_row, player_col, &comput_row, &comput_col);

    /* error */
    if (chess_status == WUZI_NONLEGAL) {
        _show_msg ("Please change a position.", GUI_COLOR_RED);
        return OS_STATUS_ERROR;
    }

    /* get computer replay chessman coor */
    _get_cbi_coor (comput_row, comput_col, &comput_coor);

    /* show player chessman */
    gra_block (&player_coor, &pic_size, player_chessman_pic);

    /* unmark last computer chessman */
    _unmark_last_computer_chessman();

    /* it's computer turn now */
    _G_white_black_turn = 1 - _G_white_black_turn;
    _show_turn ();

    /* deal the action return value */
    switch (chess_status) {
        case WUZI_GO_AHEAD:
            /* show computer chessman */
            gra_block (&comput_coor, &pic_size, comput_chessman_pic);
            _mark_computer_chessman(&comput_coor);
            break;
        case WUZI_PLAYER_WIN:
            _G_game_over = 1;
            _show_msg ("player winner!", GUI_COLOR_GREEN);
            break;
        case WUZI_COMPUT_WIN:
            /* show computer chessman */
            gra_block (&comput_coor, &pic_size, comput_chessman_pic);
            _mark_computer_chessman(&comput_coor);
            _G_game_over = 1;
            _show_msg ("computer winner!", GUI_COLOR_GREEN);
            break;
        case WUZI_DOGFALL:
            _G_game_over = 1;
            _show_msg ("dogfall!", GUI_COLOR_GREEN);
            break;
        default:
            break;
    }

    /* it's player turn now */
    _G_white_black_turn = 1 - _G_white_black_turn;
    _show_turn ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _get_row_col()
 *
 * - convert chessman cbi coordinate to (row, col)
 */
static void _get_row_col (const GUI_COOR *p_coor, int *p_row, int *p_col)
{
    *p_col = (p_coor->x + CBI_OFFSET - CHESSBOARD_START_X) / (LINE_WIDTH + BLANK_WIDTH);
    *p_row = (p_coor->y + CBI_OFFSET - BLANK_HIGHT) / (LINE_WIDTH + BLANK_HIGHT);
}

/*==============================================================================
 * - _get_cbi_coor()
 *
 * - convert (row, col) to chessman cbi coordinate
 */
static void _get_cbi_coor (int row, int col, GUI_COOR *p_coor)
{
    p_coor->x = CHESSBOARD_START_X + col * (LINE_WIDTH + BLANK_WIDTH) - CBI_OFFSET;
    p_coor->y = (row + 1) * BLANK_HIGHT + row * LINE_WIDTH - CBI_OFFSET;
}

/*==============================================================================
 * - _draw_chessboard()
 *
 * - draw a chessboard
 */
static void _draw_chessboard ()
{
    int i;
    GUI_COOR line_start;
    GUI_COOR line_end;
    GUI_COOR bg_coor;
    GUI_SIZE bg_size;

    /* draw backgroud */
    bg_coor.x = CHESSBOARD_START_X - OUT_MARGIN;
    bg_coor.y = BLANK_HIGHT - OUT_MARGIN;
    bg_size.w = WUZI_COL_NUM * LINE_WIDTH + (WUZI_COL_NUM - 1) * BLANK_WIDTH + 2 * OUT_MARGIN;
    bg_size.h = WUZI_ROW_NUM * LINE_WIDTH + (WUZI_ROW_NUM - 1) * BLANK_HIGHT + 2 * OUT_MARGIN;
    gra_rect (&bg_coor, &bg_size, GUI_COLOR_BLACK);

    /* draw horizal lines */
    line_start.x = CHESSBOARD_START_X;
    line_end.x = CHESSBOARD_END_X - 1;
    for (i = 0; i < WUZI_ROW_NUM; i++) {
        line_start.y = (i + 1) * BLANK_HIGHT + i * LINE_WIDTH;
        line_end.y = line_start.y;
        gra_line (&line_start, &line_end, GUI_COLOR_BLUE, LINE_WIDTH);
    }

    /* draw vertical lines */
    line_start.y = BLANK_HIGHT;
    line_end.y = WUZI_ROW_NUM * (BLANK_HIGHT + LINE_WIDTH) - 1;
    for (i = 0; i < WUZI_COL_NUM; i++) {
        line_start.x = CHESSBOARD_START_X + i * (LINE_WIDTH + BLANK_WIDTH);
        line_end.x = line_start.x;
        gra_line (&line_start, &line_end, GUI_COLOR_BLUE, LINE_WIDTH);
    }
}

/*==============================================================================
 * - _reg_chessman_cbis()
 *
 * - register all chessman postions as cbis
 */
static void _reg_chessman_cbis ()
{
    int row, col;
    GUI_CBI *pCBI = NULL;
    GUI_COOR chessman_cbi_start;
    GUI_COOR chessman_cbi_end;

    for (row = 0; row < WUZI_ROW_NUM; row++) {
        for (col = 0; col < WUZI_COL_NUM; col++) {
            _get_cbi_coor (row, col, &chessman_cbi_start);
            chessman_cbi_end.x = chessman_cbi_start.x + CHESSMAN_SIZE - 1;
            chessman_cbi_end.y = chessman_cbi_start.y + CHESSMAN_SIZE - 1;
            pCBI = cbi_create_blank (&chessman_cbi_start, &chessman_cbi_end,
                                      _wuzi_cb_action); /* press callback function */
            pCBI->func_release = cbf_noop; /* release callback function */
        }
    }
}

/*==============================================================================
 * - _show_turn()
 *
 * - show which turn is it now
 */
static void _show_turn ()
{
    static GUI_COLOR *white_turn_pic = NULL;
    static GUI_COLOR *black_turn_pic = NULL;
    static GUI_SIZE   pic_size = {0, 0};
    GUI_COOR white_coor = {10, gra_scr_h() * 2 / 3 - 52 };
    GUI_COOR black_coor = {CHESSBOARD_END_X + OUT_MARGIN + 10,
                           gra_scr_h() * 2 / 3 - 52};

    /* get chessman picture pixels data */
    if (white_turn_pic == NULL) {
        white_turn_pic = get_pic_pixel (WHITE_TURN_PIC, &pic_size.w, &pic_size.h);
    }
    if (black_turn_pic == NULL) {
        black_turn_pic = get_pic_pixel (BLACK_TURN_PIC, &pic_size.w, &pic_size.h);
    }

    if (_G_white_black_turn == 0) {
        gra_rect  (&black_coor, &pic_size, GUI_BG_COLOR);
        gra_block (&white_coor, &pic_size, white_turn_pic);
    } else {
        gra_rect  (&white_coor, &pic_size, GUI_BG_COLOR);
        gra_block (&black_coor, &pic_size, black_turn_pic);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

