/*==============================================================================
** msg_box.c -- message box.
**
** MODIFY HISTORY:
**
** 2012-03-20 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../gui.h"
#include "../../string.h"
#include "../LibJPEG/jpeg_util.h"

/*======================================================================
  Configs
======================================================================*/
#define MSG_BOX_MAGIC		80000
#define MSG_BOX_CBI_FILE_NAME "/n1/gui/msg_box.jpg"
#define MSG_BOX_WIDTH       200
#define MSG_BOX_HEIGHT      200
#define MSG_BOX_MARGIN      16
#define MSG_BOX_START_X     ((gra_scr_w() - MSG_BOX_WIDTH) / 2)
#define MSG_BOX_START_Y     ((gra_scr_h() - MSG_BOX_HEIGHT) / 2)

/*======================================================================
  Forward Function Declare
======================================================================*/
static OS_STATUS _msg_box_cb (GUI_CBI *pCBI_msg_box, GUI_COOR *pCoor);
#ifdef MSG_BOX_MAGIC
static void _show_frame_layer (const GUI_COOR *p_start, const GUI_SIZE *p_size,
                               int layer_no, const uint16 *p_data);
static void _delay (volatile int n);
#endif /* MSG_BOX_MAGIC */

/*==============================================================================
 * - txt_get_line()
 *
 * - get a line char num
 */
int txt_get_line (const char *text, int width)
{
    int text_width = 0;;
    int char_width;
    int char_num = 0;

    while (*text != '\0') {

        if (*text == '\n') {
            char_num++;
            break;
        }

        if ((*text &  0x80) == 0) { /* ascii char */
            char_width = GUI_FONT_WIDTH;
            text++;
        } else {                     /* han zi */
            char_width = GUI_FONT_WIDTH * 2;
            text += 2;
        }

        if (text_width + char_width > width) {
            break;
        }
        text_width += char_width;

        char_num += char_width / GUI_FONT_WIDTH;
    }

    return char_num;
}

/*==============================================================================
 * - msg_box_create()
 *
 * - create a message box for show some string
 */
OS_STATUS msg_box_create (const char *msg)
{
    GUI_CBI *pCBI = NULL;
    GUI_COOR left_up = {MSG_BOX_START_X, MSG_BOX_START_Y};
    GUI_SIZE size = {MSG_BOX_WIDTH, MSG_BOX_HEIGHT};
    GUI_COLOR  *p = gra_get_block (&left_up, &size); /* save curr pic */
    int char_num;
    int show_char_num;

    cbi_cover_all ();

    /*
     * create message cbi
     */
    pCBI = cbi_create_default (MSG_BOX_CBI_FILE_NAME, &left_up, &size, FALSE);
    pCBI->func_leave   = _msg_box_cb;
    pCBI->func_release = _msg_box_cb;
    cbi_register (pCBI);

    pCBI->data = p;

    /*
     * draw message string on message cbi
     */
    left_up.y = MSG_BOX_START_Y + MSG_BOX_MARGIN / 2;
    char_num = txt_get_line (msg, MSG_BOX_WIDTH - MSG_BOX_MARGIN);
    while (char_num != 0) {

        show_char_num = char_num;
        if (msg[char_num - 1] == '\n') {
            show_char_num--;
        }
        left_up.x = MSG_BOX_START_X + (MSG_BOX_WIDTH - GUI_FONT_WIDTH * show_char_num) / 2;
        han_draw_string (&left_up, GUI_COLOR_RED, (uint8 *)msg, show_char_num);

        msg += char_num;
        char_num = txt_get_line (msg, MSG_BOX_WIDTH - MSG_BOX_MARGIN);

        left_up.y += GUI_FONT_HEIGHT;
        if (left_up.y + GUI_FONT_HEIGHT >
            MSG_BOX_START_Y + MSG_BOX_HEIGHT - MSG_BOX_MARGIN / 2) {
            break;
        }
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - msg_box_image()
 *
 * - create a message box for show a image
 */
OS_STATUS msg_box_image (const char *image_name)
{
	GUI_CBI *pCBI = NULL;
    GUI_COOR left_up;
    GUI_COOR right_down;
    GUI_SIZE size;
    GUI_COLOR *image_pixels = NULL;

    cbi_cover_all ();

    image_pixels = get_pic_pixel (image_name, &size.w, &size.h);
    if (image_pixels == NULL) {
        cbi_uncover ();
        return OS_STATUS_ERROR;
    }

    left_up.x = (gra_scr_w() - size.w) / 2;
    left_up.y = (gra_scr_h() - size.h) / 2;
    right_down.x = left_up.x + size.w - 1;
    right_down.y = left_up.y + size.h - 1;

    pCBI = cbi_create_blank(&left_up, &right_down, cbf_default_press);
    pCBI->func_leave   = _msg_box_cb;
    pCBI->func_release = _msg_box_cb;
    pCBI->data = gra_get_block (&left_up, &size); /* save curr pic */

    gra_block (&left_up, &size, image_pixels);

    free (image_pixels);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _msg_box_cb()
 *
 * - message box callback function. In this function,
 *   reverse previous pixels data, and close message box
 */
static OS_STATUS _msg_box_cb (GUI_CBI *pCBI_msg_box, GUI_COOR *pCoor)
{
    GUI_COOR s = pCBI_msg_box->left_up;
    GUI_SIZE sz;

    sz.w = pCBI_msg_box->right_down.x - pCBI_msg_box->left_up.x + 1;
    sz.h = pCBI_msg_box->right_down.y - pCBI_msg_box->left_up.y + 1;

    /*
     * paint prev pic
     */
#ifdef MSG_BOX_MAGIC
    int i;
    int frame_layer_num = (MAX(sz.w, sz.h) + 1) / 2;
    for (i = frame_layer_num; i >= 1; i--) { /* show [1, frame_layer_num] layer */
        _show_frame_layer (&s, &sz, i, pCBI_msg_box->data);
        _delay (MSG_BOX_MAGIC);
    }
#else /* MSG_BOX_MAGIC */
    gra_block (&s, &sz, pCBI_msg_box->data);
#endif /* MSG_BOX_MAGIC */

    free (pCBI_msg_box->data);
    pCBI_msg_box->data = NULL; /* zero data */
    cbi_unregister (pCBI_msg_box);

    cbi_uncover ();

    return OS_STATUS_OK;
}

#ifdef MSG_BOX_MAGIC
/*==============================================================================
 * - _show_frame_layer()
 *
 * - show a frame line
 */
static void _show_frame_layer (const GUI_COOR *p_start, const GUI_SIZE *p_size,
                               int layer_no, const uint16 *p_data)
{
    int width  = MIN((2 * layer_no - p_size->w % 2), p_size->w);
    int height = MIN((2 * layer_no - p_size->h % 2), p_size->h);
    GUI_COOR s;
    int offset_x, offset_y;

    /* draw two horizontal lines */
    if ((p_size->h + 1) / 2 >= layer_no) {
        GUI_SIZE sz = {width, 1};

        offset_x = (p_size->w - width) / 2;
        offset_y = (p_size->h - height) / 2;
        s.x = p_start->x + offset_x;
        s.y = p_start->y + offset_y;
        gra_block (&s, &sz, p_data + offset_y * p_size->w + offset_x);
        offset_y += height - 1;
        s.y = p_start->y + offset_y;
        gra_block (&s, &sz, p_data + offset_y * p_size->w + offset_x);
    }

    /* draw two vertical lines */
    if ((p_size->w + 1) / 2 >= layer_no) {
        int i;
        int offset_x1;
        GUI_COOR s1;

        offset_x = (p_size->w - width) / 2;
        offset_x1 = (p_size->w - width) / 2 + width - 1;
        offset_y = (p_size->h - height) / 2;

        s.x = p_start->x + offset_x;
        s1.x = p_start->x + offset_x1;
        for (i = 0; i < height; i++) {
            s.y = p_start->y + offset_y;
            gra_set_pixel (&s, *(p_data + offset_y * p_size->w + offset_x));
            s1.y = p_start->y + offset_y;
            gra_set_pixel (&s1, *(p_data + offset_y * p_size->w + offset_x1));
            offset_y++;
        }
    }
}

/*==============================================================================
 * - _delay()
 *
 * - mini delay
 */
static void _delay (volatile int n)
{
    while (n--) {
        ;
    }
}

#endif /* MSG_BOX_MAGIC */

/*==============================================================================
** FILE END
==============================================================================*/

