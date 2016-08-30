/*==============================================================================
** gui_media.c -- media application.
**
** MODIFY HISTORY:
**
** 2012-04-02 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "../gui.h"
#include "../driver/lcd.h"
#include "../LibJPEG/jpeg_util.h"
#include "msg_box.h"
#include "file_mgr.h"
#include <media/ffplay.h>

/*======================================================================
  configs
======================================================================*/
#define MEDIA_FILE_PATH         "/n1/media/"
#define PAUSE_PIC_NAME          "/n1/gui/cbi_pause.jpg"
#define WAIT_PIC_NAME           "/n1/album/landscape.jpg"

#define SLIDE_DELAY             30000 /* slide the control bar delay */
#define CONTROL_CBI_NUM         6

/*======================================================================
  Control Slide Struct
======================================================================*/
typedef struct control_slide {
    int        slide_pos;       /* [-1, gra_scr_h()-1] last row pos */
    GUI_COLOR *slide_bakup;
    GUI_COLOR *slide_icons;
} CTRL_SLIDE;

/*======================================================================
  Global Variables
======================================================================*/
static CTRL_SLIDE   _G_control_slide; /* control bar slide */
static GUI_CBI     *_G_pCBI_play; /* the play cbi*/
static GUI_CBI     *_G_pCBI_ctrls[CONTROL_CBI_NUM + 1]; /* ctrl cbis */
static int          _G_in_pause = 0;
static int          _G_first_drag = 0;
#define _Gc         _G_control_slide /* just for short */

/*======================================================================
  Function foreward declare
======================================================================*/
static OS_STATUS _media_cb_play (GUI_CBI *pCBI_file, GUI_COOR *pCoor);
static OS_STATUS _media_cb_home (GUI_CBI *pCBI_home, GUI_COOR *pCoor);
static OS_STATUS _media_cb_fresh (GUI_CBI *pCBI_fresh, GUI_COOR *pCoor);
static OS_STATUS _media_cb_updir (GUI_CBI *pCBI_updir, GUI_COOR *pCoor);
static void _media_reg_play_cbi ();
static void _media_dump_control_icons ();
static void _media_reg_control_cbis ();
static void _media_unreg_control_cbis ();
static OS_STATUS _media_cb_ctrl_close (GUI_CBI *pCBI_ctrl_close, GUI_COOR *coor);
static OS_STATUS _media_cb_ctrl_li_fo (GUI_CBI *pCBI_ctrl_li_fo, GUI_COOR *coor);
static OS_STATUS _media_cb_ctrl_li_ba (GUI_CBI *pCBI_ctrl_li_ba, GUI_COOR *coor);
static OS_STATUS _media_cb_ctrl_bi_fo (GUI_CBI *pCBI_ctrl_bi_fo, GUI_COOR *coor);
static OS_STATUS _media_cb_ctrl_bi_ba (GUI_CBI *pCBI_ctrl_bi_ba, GUI_COOR *coor);
static OS_STATUS _media_cb_ctrl_hide (GUI_CBI *pCBI_ctrl_hide, GUI_COOR *coor);
static OS_STATUS _media_cb_play_drag (GUI_CBI *pCBI_play, GUI_COOR *p_offset_coor);
static OS_STATUS _media_cb_play_release (GUI_CBI *pCBI_play, GUI_COOR *p_offset_coor);
static void _media_slide_to (int pos, int delay);

/*==============================================================================
 * - app_media()
 *
 * - media application entry, called by home app
 */
OS_STATUS app_media (GUI_CBI *pCBI_media, GUI_COOR *coor)
{
    int i;
    GUI_CBI *pCBI;
    GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, 0};
    GUI_SIZE size = {0, 0};

    ICON_CBI ic_right[] = {
        {"/n1/gui/cbi_home.jpg",     _media_cb_home},
        {"/n1/gui/cbi_up.jpg",       _media_cb_updir},
        {"/n1/gui/cbi_fresh.jpg",    _media_cb_fresh}
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
    for (i = 1; i < N_ELEMENTS(ic_right); i++) {
        left_up.y = gra_scr_h() - (N_ELEMENTS(ic_right) - i) * ICON_SIZE;

        pCBI = cbi_create_default (ic_right[i].name, &left_up, &size, FALSE);
        pCBI->func_press = ic_right[i].func;   /* for extern */

        cbi_register (pCBI);
    }

    /*
     * show and all files those are in [MEDIA_FILE_PATH],
     * user click one of these cbis, start play it.
     */
    extern void ebook_register ();
    ebook_register ();
    extern void image_register ();
    image_register ();
    file_mgr_register ("*.avi|*.flv", _media_cb_play);
    file_mgr_show (MEDIA_FILE_PATH, "*");

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_cb_play()
 *
 * - play a media file
 */
static OS_STATUS _media_cb_play (GUI_CBI *pCBI_file, GUI_COOR *pCoor)
{
    static GUI_COLOR *pWait_pic = NULL;
    char media_file_name[PATH_LEN_MAX] = "/yaffs2";
    char msg_str[PATH_LEN_MAX + 20];
    GUI_COOR startup_pic_coor = {gra_scr_w(), 0};
    GUI_SIZE startup_pic_size = {gra_scr_w(), gra_scr_h()};
    OS_STATUS status;

    /* add '/yaffs2' prefix, because ffmpeg use ios */
    strcat (media_file_name, pCBI_file->name);

    /* dump a picture at 'play' screen */
    if (pWait_pic == NULL) {
        pWait_pic = get_pic_pixel (WAIT_PIC_NAME, NULL, NULL);
    }
    gra_block (&startup_pic_coor, &startup_pic_size, pWait_pic);

    /* restore the frame */
    cbf_default_release(pCBI_file, pCoor);

    /* show middle fb */
    gra_set_show_fb (1);

    /* draw open tip */
    strcpy (msg_str, media_file_name);
    strcat (msg_str, " is opening...");
    han_draw_string (&startup_pic_coor, GUI_COLOR_GREEN, (uint8 *)msg_str, strlen(msg_str));

    /* play the media file */
    status = media_play (media_file_name);
    if (status == OS_STATUS_ERROR) {
        strcpy (msg_str, "can't play\n");
        strcat (msg_str, media_file_name);
        msg_box_create (msg_str);

        /* show first fb */
        gra_set_show_fb (0);
        return status;
    }

    /* prapare for control bar */
    _media_dump_control_icons ();
    
    /* register play cbi */
    _media_reg_play_cbi ();

    /* start play one file, we are not in pause */
    _G_in_pause = 0;

    return status;
}

/*==============================================================================
 * - _media_cb_fresh()
 *
 * - fresh current directory files
 */
static OS_STATUS _media_cb_fresh (GUI_CBI *pCBI_fresh, GUI_COOR *pCoor)
{
    file_mgr_fresh ();
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_cb_updir()
 *
 * - show upper directory files
 */
static OS_STATUS _media_cb_updir (GUI_CBI *pCBI_updir, GUI_COOR *pCoor)
{
    file_mgr_updir ();
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_cb_home()
 *
 * - quit media application
 */
static OS_STATUS _media_cb_home (GUI_CBI *pCBI_home, GUI_COOR *pCoor)
{
    /* unregister file cbis */
    file_mgr_clear ();

    /* clear lcd screen */
    gra_clear (GUI_BG_COLOR);

    /* go to home */
    cbf_go_home (pCBI_home, pCoor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_reg_play_cbi()
 *
 * - register play page cbi, only one
 */
static void _media_reg_play_cbi ()
{
    _G_pCBI_play = malloc (sizeof (GUI_CBI));
    if (_G_pCBI_play == NULL) {
        msg_box_create ("Alloc memory failed!");
        return;
    }

    _G_pCBI_play->left_up.x = 0;
    _G_pCBI_play->left_up.y = 0;
    _G_pCBI_play->right_down.x = gra_scr_w() - 1;
    _G_pCBI_play->right_down.y = gra_scr_h() - 1;

    _G_pCBI_play->data = NULL;

    _G_pCBI_play->func_press   = cbf_noop;
    _G_pCBI_play->func_leave   = cbf_noop;
    _G_pCBI_play->func_release = _media_cb_play_release;
    _G_pCBI_play->func_drag    = _media_cb_play_drag;

    cbi_register (_G_pCBI_play);
}

/*==============================================================================
 * - _media_dump_control_icons()
 *
 * - dump control page cbi's icon to memory <_Gc.slide_icons>
 */
static void _media_dump_control_icons ()
{
    int i, j;
    GUI_COLOR *icon_pixels;
    int icon_width;
    int icon_height;
    int icon_start_y;
    int icon_gap;
    char *icon_file_name[CONTROL_CBI_NUM] = {
                                "/n1/gui/cbi_close.jpg",
                                "/n1/gui/cbi_right.jpg",
                                "/n1/gui/cbi_left.jpg",
                                "/n1/gui/cbi_fast_right.jpg",
                                "/n1/gui/cbi_fast_left.jpg",
                                "/n1/gui/cbi_up.jpg"
    };

    /* this memory to store old screen pixels data */
    _Gc.slide_bakup = malloc (ICON_SIZE * gra_scr_h() * sizeof (GUI_COLOR));
    /* this memory to store slide control bar icons pixels data */
    _Gc.slide_icons = calloc (1, ICON_SIZE * gra_scr_h() * sizeof (GUI_COLOR));

    icon_gap = (gra_scr_h() - ICON_SIZE * CONTROL_CBI_NUM) / (CONTROL_CBI_NUM - 1);
    for (i = 0; i < CONTROL_CBI_NUM; i++) {
        icon_start_y = i * (ICON_SIZE + icon_gap);
        icon_pixels = get_pic_pixel (icon_file_name[i], &icon_width, &icon_height);

        for (j = 0; j < ICON_SIZE; j++) {
            memcpy (_Gc.slide_icons + (icon_start_y + j) * ICON_SIZE,
                    icon_pixels + j * ICON_SIZE,
                    ICON_SIZE * sizeof (GUI_COLOR)); 
        }
        free (icon_pixels);
    }

    _Gc.slide_pos = -1; /* totally hide */
}

/*==============================================================================
 * - _media_reg_control_cbis()
 *
 * - cover 'play' & 'pause' cbis and register control bar cbis
 */
static void _media_reg_control_cbis ()
{
    int i;
    GUI_COOR left_up = {0, 0};
    GUI_COOR right_down = {gra_scr_w() - 1, gra_scr_h() - 1};
    int cbi_gap;
    CB_FUNC_PTR cbi_funcs[CONTROL_CBI_NUM] = {
                _media_cb_ctrl_close,
                _media_cb_ctrl_li_fo,
                _media_cb_ctrl_li_ba,
                _media_cb_ctrl_bi_fo,
                _media_cb_ctrl_bi_ba,
                _media_cb_ctrl_hide
    };

    /* register 'cover' cbi to cover 'play' & 'pause' cbis */
    _G_pCBI_ctrls[0] = cbi_create_blank(&left_up, &right_down, cbf_noop);
    _G_pCBI_ctrls[0]->func_release = cbf_noop;
    _G_pCBI_ctrls[0]->func_leave = cbf_noop;

    /* create 6 control cbis */
    cbi_gap = (gra_scr_h() - ICON_SIZE * CONTROL_CBI_NUM) / (CONTROL_CBI_NUM - 1);
    for (i = 0; i < CONTROL_CBI_NUM; i++) {
        left_up.x = gra_scr_w() - ICON_SIZE;
        left_up.y = i * (ICON_SIZE + cbi_gap);
        right_down.x = left_up.x + ICON_SIZE - 1;
        right_down.y = left_up.y + ICON_SIZE - 1;

        _G_pCBI_ctrls[i+1] = cbi_create_blank (&left_up, &right_down, cbf_default_press);
        _G_pCBI_ctrls[i+1]->func_release = cbi_funcs[i];
    }
}

/*==============================================================================
 * - _media_unreg_control_cbis()
 *
 * - delete control bar cbis and uncover 'play' & 'pause' cbis
 */
static void _media_unreg_control_cbis ()
{
    int i;
    for (i = 0; i < CONTROL_CBI_NUM + 1; i++) {
        cbi_unregister (_G_pCBI_ctrls[i]);
    }
}

static OS_STATUS _media_cb_ctrl_close (GUI_CBI *pCBI_ctrl_close, GUI_COOR *coor)
{
    /* show file list screen */
    gra_set_show_fb (0);

    /* clear control bar resource */
    free (_Gc.slide_bakup);
    free (_Gc.slide_icons);
    
    /* close video */
    media_stop ();

    /* unregister control bar & 'cover' cbis */
    _media_unreg_control_cbis ();

    /* unregister 'play' cbi */
    cbi_unregister (_G_pCBI_play);
    
    return OS_STATUS_OK;
}
static OS_STATUS _media_cb_ctrl_li_fo (GUI_CBI *pCBI_ctrl_li_fo, GUI_COOR *coor)
{
    return OS_STATUS_OK;
}
static OS_STATUS _media_cb_ctrl_li_ba (GUI_CBI *pCBI_ctrl_li_ba, GUI_COOR *coor)
{
    return OS_STATUS_OK;
}
static OS_STATUS _media_cb_ctrl_bi_fo (GUI_CBI *pCBI_ctrl_bi_fo, GUI_COOR *coor)
{
    return OS_STATUS_OK;
}
static OS_STATUS _media_cb_ctrl_bi_ba (GUI_CBI *pCBI_ctrl_bi_ba, GUI_COOR *coor)
{
    return OS_STATUS_OK;
}
static OS_STATUS _media_cb_ctrl_hide (GUI_CBI *pCBI_ctrl_hide, GUI_COOR *coor)
{
    /* hide the control bar */
    _media_slide_to (-1, SLIDE_DELAY);

    /* unregister control bar & 'cover' cbis */
    _media_unreg_control_cbis ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_cb_play_drag()
 *
 * - drag slide down <--> up
 */
static OS_STATUS _media_cb_play_drag (GUI_CBI *pCBI_play, GUI_COOR *p_offset_coor)
{
    int i;
    int end_pos = _Gc.slide_pos;
    static GUI_COLOR *pPause_pic = NULL;
    static GUI_SIZE pic_size = {0, 0};

    /* pause the video */
    if (_G_first_drag == 0 && _G_in_pause == 0) {
        GUI_COOR pic_coor = {gra_scr_w() + (gra_scr_w() - ICON_SIZE) / 2,
                             (gra_scr_h() - ICON_SIZE) / 2};

        _G_first_drag = 1;
        media_pause ();
        delayQ_delay (SYS_CLK_RATE / 20); /* wait for message is reached */
        if (pPause_pic == NULL) {
            pPause_pic = get_pic_pixel (PAUSE_PIC_NAME, &pic_size.w, &pic_size.h);
        }
        gra_block_t (&pic_coor, &pic_size, pPause_pic);
        cbf_default_press (pCBI_play, NULL);

        /* copy current screen pixels to <_Gc.slide_bakup> array */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (_Gc.slide_bakup + i * ICON_SIZE,                /* dst */
                    lcd_get_addr (i, gra_scr_w() * 2 - ICON_SIZE),  /* src */
                    ICON_SIZE * sizeof (GUI_COLOR));                /* size */
        }
    }

    if (p_offset_coor->y > 0) { /* drag to down */
        end_pos = MIN(gra_scr_h() - 1, _Gc.slide_pos + p_offset_coor->y);
    } else if (p_offset_coor->y < 0) { /* drag to up */
        end_pos = MAX(-1, _Gc.slide_pos + p_offset_coor->y);
    }
    _media_slide_to (end_pos, 0);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_cb_play_release()
 *
 * - determine go to 'control' page or stay 'play' page
 */
static OS_STATUS _media_cb_play_release (GUI_CBI *pCBI_play, GUI_COOR *p_offset_coor)
{
    int end_pos;

    if (_Gc.slide_pos >= (gra_scr_h() / 4)) {
        end_pos = gra_scr_h() - 1;
        _media_slide_to (end_pos, SLIDE_DELAY);
        _media_reg_control_cbis (); /* register control bar cbis */
        _G_in_pause = 1;
    } else {
        end_pos = -1;
        _media_slide_to (end_pos, SLIDE_DELAY);

        if (_G_in_pause == 1) {
            media_continue ();
        }
        _G_in_pause = 1 - _G_in_pause;
    }

    _G_first_drag = 0;
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _media_slide_to()
 *
 * - set slide last line at <pos>
 */
static void _media_slide_to (int pos, int delay)
{
    int i = _Gc.slide_pos;
    int x = gra_scr_w() * 2 - ICON_SIZE;

    if (i < pos) { /* make slide bigger */
        for (i++; i <= pos; i++) {
            memcpy ((void *)lcd_get_addr (i, x), _Gc.slide_icons + i * ICON_SIZE,
                    ICON_SIZE * sizeof (GUI_COLOR)); 
            lcd_delay(delay);
        }
    } else if (i > pos) { /* make slide smaller */
        for (; i > pos; i--) {
            memcpy ((void*)lcd_get_addr (i, x), _Gc.slide_bakup + i * ICON_SIZE,
                    ICON_SIZE * sizeof (GUI_COLOR)); 
            lcd_delay(delay);
        }
    }

    _Gc.slide_pos = pos;
}

/*==============================================================================
** FILE END
==============================================================================*/

