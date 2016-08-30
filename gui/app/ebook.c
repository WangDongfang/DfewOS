/*==============================================================================
** ebook.c -- ebook reader.
**
** MODIFY HISTORY:
**
** 2012-04-21 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include <yaffs_guts.h>
#include "../gui.h"
#include "msg_box.h"
#include "file_mgr.h"
extern int sprintf(char *buf, const char *, ...);

/*======================================================================
  Configs
======================================================================*/
#define EBOOK_BG_PIC            "/n1/album/ebook_bg.jpg"
#define EBOOK_START_X           gra_scr_w()
#define LEFT_RIGHT_MARGIN       40
#define TOP_BOTTOM_MARGIN       40
#define LINE_STRIP              (GUI_FONT_HEIGHT + 4)
#define MAX_PAGE_NUM            4096

/*======================================================================
  Constant
======================================================================*/
#define PAGE_PREV       (-1)
#define PAGE_NEXT       (-2)
#define PAGE_HOME       0

/*======================================================================
  Global Variables
======================================================================*/
static GUI_CBI *_G_pCBI_ebook = NULL;
static char    *_G_text = NULL;
static const char    *_G_pages[MAX_PAGE_NUM];
static char _G_book_name[40];

/*==============================================================================
 * - _read_a_page()
 *
 * - show text on screen
 */
static const char *_read_a_page (const char *text)
{
    int line_char_num;
    int show_char_num;
    GUI_COOR line_start = {EBOOK_START_X + LEFT_RIGHT_MARGIN, TOP_BOTTOM_MARGIN};

    line_char_num = txt_get_line (text, gra_scr_w() - 2 * LEFT_RIGHT_MARGIN);
    while (line_char_num != 0) {

        /* show current line in transparent */
        show_char_num = line_char_num;
        if (text[line_char_num - 1] == '\n') {
            show_char_num--;
        }
        if (line_char_num >= 2 && text[line_char_num - 2] == '\r') {
            show_char_num--;
        }
        han_draw_string_t (&line_start, GUI_COLOR_BLACK, (uint8 *)text, show_char_num);

        /* <text> go a head */
        text += line_char_num;

        /* update next line coordinate */
        line_start.y += LINE_STRIP;
        if (line_start.y + GUI_FONT_HEIGHT > gra_scr_h() - TOP_BOTTOM_MARGIN) {
            break;
        }

        /* get next line chars */
        line_char_num = txt_get_line (text, gra_scr_w() - 2 * LEFT_RIGHT_MARGIN);
    }

    return text;
}

/*==============================================================================
 * - _dump_background()
 *
 * - dump background on screen
 */
static void _dump_background ()
{
    GUI_COOR bg_pic_coor = {EBOOK_START_X, 0};
    GUI_SIZE bg_pic_size = {gra_scr_w(), gra_scr_h()};

    gra_block (&bg_pic_coor, &bg_pic_size, _G_pCBI_ebook->data);
}

/*==============================================================================
 * - _open_file()
 *
 * - open a text file, and read it's all context to memory
 */
static OS_STATUS _open_file (const char *file_name)
{
    struct yaffs_stat statFile;
    int fd;
    int read_cnt;

    /* alloc memory to store file text */
    if (yaffs_stat (file_name, &statFile) < 0) { /* not exist */
        return OS_STATUS_ERROR;
    }
	_G_text = calloc (1, statFile.st_size + 1);

    /* open file */
    fd = yaffs_open (file_name, O_RDONLY, 0666);
	if (fd < 0) {
        free (_G_text);
        return OS_STATUS_ERROR;
	}

    /* read text */
	read_cnt = yaffs_read (fd, _G_text, statFile.st_size);
	if (read_cnt < 0) {
        free (_G_text);
        yaffs_close (fd);
        return OS_STATUS_ERROR;
	}

    /* close file */
	yaffs_close (fd);

    /* make book name */
    {
        char *ext = NULL;

        strcpy (_G_book_name, strrchr (file_name, '/') + 1);
        if ((ext = strrchr (_G_book_name, '.')) != NULL) {
            *ext = '\0';
        }
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _show_page()
 *
 * - show previous page or next page
 */
static void _show_page (int page_index)
{
    static int cur_page = 0;
    char page_header_str[40];
    char page_footer_str[20];
    GUI_COOR page_header_coor;
    GUI_COOR page_footer_coor;
    int draw_len;

    switch (page_index) {
        case PAGE_HOME:
            _G_pages[0] = _G_text;
            cur_page = 0;
            _G_pages[1] = _read_a_page (_G_pages[0]);
            break;
        case PAGE_PREV:
            if (cur_page > 0) {
                _dump_background ();
                cur_page--;
                _read_a_page (_G_pages[cur_page]);
            }
            break;
        case PAGE_NEXT:
            if (*(_G_pages[cur_page + 1]) != '\0') {
                _dump_background ();
                cur_page++;
                _G_pages[cur_page + 1] = _read_a_page (_G_pages[cur_page]);
            }
            break;
        default:
            break;
    }

    /* draw page header & footer */
    sprintf (page_header_str, "<<%s>>", _G_book_name);
    draw_len = strlen (page_header_str);
    page_header_coor.x = EBOOK_START_X + (gra_scr_w() - draw_len * GUI_FONT_WIDTH) / 2;
    page_header_coor.y = (TOP_BOTTOM_MARGIN - GUI_FONT_HEIGHT) / 2;
    han_draw_string_t (&page_header_coor, GUI_COLOR_MAGENTA, (uint8 *)page_header_str, draw_len);

    sprintf (page_footer_str, "ตฺ %d าณ", cur_page);
    draw_len = strlen (page_footer_str);
    page_footer_coor.x = EBOOK_START_X + (gra_scr_w() - draw_len * GUI_FONT_WIDTH) / 2;
    page_footer_coor.y = gra_scr_h() - (TOP_BOTTOM_MARGIN + GUI_FONT_HEIGHT) / 2;
    han_draw_string_t (&page_footer_coor, GUI_COLOR_MAGENTA, (uint8 *)page_footer_str, draw_len);
}

/*==============================================================================
 * - _close_file()
 *
 * - close ebook return file list
 */
static void _close_file ()
{
    /* show file list screen */
    gra_set_show_fb (0);

    free (_G_text);

    /* unregister 'ebook' cbi */
    cbi_unregister (_G_pCBI_ebook);
}

/*==============================================================================
 * - _ebook_cb_page()
 *
 * - when user release 'ebook' cbi, call this
 */
static OS_STATUS _ebook_cb_page (GUI_CBI *pCBI_ebook, GUI_COOR *pCoor)
{
    if (pCoor->x < gra_scr_w() / 2) {
        _show_page (PAGE_PREV);
    } else {
        if (pCoor->y >= ICON_SIZE || pCoor->x < (gra_scr_w () - ICON_SIZE)) {
            _show_page (PAGE_NEXT);
        } else {
            _close_file ();
        }
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _ebook_cb_read()
 *
 * - start read a file. when user [release] a [*.txt] file icon, call this
 */
static OS_STATUS _ebook_cb_read (GUI_CBI *pCBI_file, GUI_COOR *pCoor)
{
    GUI_COOR left_up = {EBOOK_START_X, 0};
    GUI_SIZE size = {0, 0};

    /* restore the frame */
    cbf_default_release(pCBI_file, pCoor);

    /* open file */
    if (_open_file (pCBI_file->name) == OS_STATUS_ERROR) {
        msg_box_create ("Open file failed!");
        return OS_STATUS_ERROR;
    }
    
    /* show middle fb */
    gra_set_show_fb (1);

    /* create & register 'ebook' cbi */
    _G_pCBI_ebook = cbi_create_default (EBOOK_BG_PIC, &left_up, &size, TRUE);
    _G_pCBI_ebook->left_up.x    -= EBOOK_START_X;
    _G_pCBI_ebook->right_down.x -= EBOOK_START_X;
    _G_pCBI_ebook->func_release = _ebook_cb_page;
    cbi_register (_G_pCBI_ebook);

    /* show page 0 */
    _show_page (PAGE_HOME);

    return OS_STATUS_OK;
}

void ebook_register ()
{
    file_mgr_register ("*.txt|*.sh|*.c|*.h|*.cal", _ebook_cb_read);
}

/*==============================================================================
** FILE END
==============================================================================*/

