/*==============================================================================
** album.c -- album application. look up your photos.
**
** MODIFY HISTORY:
**
** 2011-11-15 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include <yaffs_guts.h>
#include "../gui.h"
#include "../driver/lcd.h"

/*======================================================================
  configs
======================================================================*/
#define ALBUM_PIC_PATH        "/n1/album/"
#define ALBUM_BASE_X          gra_scr_w()

/*======================================================================
  Global variables
======================================================================*/
typedef struct pic_name {
    DL_NODE  pic_list_node;
    char     name[PATH_LEN_MAX];
} PIC_NAME_NODE;
static DL_LIST _G_pic_list = {NULL, NULL};
static PIC_NAME_NODE *_G_left_pic  = NULL;
static PIC_NAME_NODE *_G_right_pic = NULL;

/*======================================================================
  other moudle support function
======================================================================*/
extern OS_STATUS jpeg_dump_pic (const char *pic_file_name,
                                const GUI_COOR *p_left_up,
                                GUI_SIZE       *p_expect_size);

/*======================================================================
  Function foreward declare
======================================================================*/
static int _album_add_file (const char *path, const char *ext);
static OS_STATUS _album_cb_change (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);
static OS_STATUS _album_cb_home (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor);

/*==============================================================================
 * - app_album()
 *
 * - album application entry
 */
OS_STATUS app_album (GUI_CBI *pCBI_album, GUI_COOR *coor)
{
    GUI_COOR pic_left_up = {gra_scr_w(), 0};
    GUI_SIZE album_size = {gra_scr_w(), gra_scr_h()};

    GUI_CBI *pCBI = malloc (sizeof (GUI_CBI));
    if (pCBI == NULL) {
        return OS_STATUS_ERROR;
    }
    strcpy (pCBI->name, "album cbi");

    pCBI->left_up.x = 0;
    pCBI->left_up.y = 0;
    pCBI->right_down.x = gra_scr_w() - 1;
    pCBI->right_down.y = gra_scr_h() - 1;

    pCBI->func_press   = cbf_noop;
    pCBI->func_leave   = cbf_noop;
    pCBI->func_release = _album_cb_change;
    pCBI->func_drag    = cbf_hw_drag;

    pCBI->data = NULL;

    cbi_delete_all ();   /* delete all other cbis */

    _album_add_file (ALBUM_PIC_PATH, ".jpg");
    _album_add_file (ALBUM_PIC_PATH, ".JPG");

    _G_right_pic = (PIC_NAME_NODE *)DL_FIRST(&_G_pic_list);
    _G_left_pic  = (PIC_NAME_NODE *)DL_LAST(&_G_pic_list);
    GUI_ASSERT(_G_right_pic != NULL);

    jpeg_dump_pic (_G_right_pic->name, &pic_left_up, &album_size);

    gra_set_show_fb (1); /* show frame buffer 1 */

    font_printf (&pic_left_up, GUI_COLOR_GREEN,
                 "There are %d picture(s) in \""ALBUM_PIC_PATH"\" directory!",
                 dlist_count (&_G_pic_list));

    _G_right_pic = (PIC_NAME_NODE *)DL_NEXT(_G_right_pic);

    /* dump first and last picture in frame buffer */
    pic_left_up.x = 0;
    jpeg_dump_pic (_G_left_pic->name, &pic_left_up, &album_size);
    pic_left_up.x = gra_scr_w() * 2;
    jpeg_dump_pic (_G_right_pic->name, &pic_left_up, &album_size);

    cbi_register (pCBI);

    /* home cbi */
    {
        GUI_CBI *pCBI_home = NULL;
        GUI_COOR left_up = {gra_scr_w() - ICON_SIZE, 0};
        GUI_SIZE size = {0, 0};
        pCBI_home = cbi_create_default ("/n1/gui/cbi_home.jpg", &left_up, &size, FALSE);
        pCBI_home->func_release = _album_cb_home;

        cbi_register (pCBI_home);
    }


    return OS_STATUS_OK;
}

/*==============================================================================
 * - _album_add_file()
 *
 * - search all file in <path> dir, and add extern with <ext> to list
 */
static int _album_add_file (const char *path, const char *ext)
{
    yaffs_DIR     *d;
    yaffs_dirent  *de;

    int            i;
    PIC_NAME_NODE *pNewPic = NULL;
    int            file_name_len;

    d = yaffs_opendir(path);

    for(i = 0; (de = yaffs_readdir(d)) != NULL; i++) {

        file_name_len = strlen (de->d_name);

        if (strlen(path) +  file_name_len < PATH_LEN_MAX && /* file name not too long */
            strcmp (de->d_name + file_name_len - strlen(ext), ext) == 0) {

            pNewPic = malloc (sizeof (PIC_NAME_NODE));
            if (pNewPic != NULL) {
                strcpy(pNewPic->name, path);
                strcat(pNewPic->name, de->d_name);

                dlist_add (&_G_pic_list, (DL_NODE *)pNewPic);
            }
        }
    }

    yaffs_closedir(d);

    return i;
}

/*==============================================================================
 * - _album_cb_home()
 *
 * - quit album app, delete picture file name nodes memory
 */
static OS_STATUS _album_cb_home (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    PIC_NAME_NODE *temp_pic_node  = (PIC_NAME_NODE *)dlist_get (&_G_pic_list);

    while (temp_pic_node != NULL) {
        free (temp_pic_node);
        temp_pic_node  = (PIC_NAME_NODE *)dlist_get (&_G_pic_list);
    }

    gra_clear (GUI_BG_COLOR);
    cbf_go_home (pCBI, p_cbi_coor);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _album_cb_change()
 *
 * - when user release the touch screen call this
 */
static OS_STATUS _album_cb_change (GUI_CBI *pCBI, GUI_COOR *p_cbi_coor)
{
    int i;
    GUI_SIZE album_size = {gra_scr_w(), gra_scr_h()};
    int cur_x = lcd_get_start_x (); /* the first show pixel in all framebuffer' position */

#define _MIN_OFFSET  gra_scr_w()/4 /* to switch a picture, user drag this offset at least */

    if (cur_x < ALBUM_BASE_X - _MIN_OFFSET) { /* user drag to right */
        GUI_COOR pic_left_up = {0, 0};
        
        /* scroll to left screen */
        lcd_fb_move (cur_x - ALBUM_BASE_X + gra_scr_w(), 0);

        /* copy data */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (lcd_get_addr (i, ALBUM_BASE_X), lcd_get_addr (i, 0),
                    gra_scr_w() * sizeof (GUI_COLOR) * 2); 
        }

        /* show base screen */
        gra_set_show_fb (1); /* show frame buffer 1 */

        _G_right_pic = (PIC_NAME_NODE *)DL_PREVIOUS(_G_right_pic);
        if (_G_right_pic == NULL) {
            _G_right_pic = (PIC_NAME_NODE *)DL_LAST(&_G_pic_list);
        }
        /* dump previous picture at left */
        _G_left_pic = (PIC_NAME_NODE *)DL_PREVIOUS(_G_left_pic);
        if (_G_left_pic == NULL) {
            _G_left_pic = (PIC_NAME_NODE *)DL_LAST(&_G_pic_list);
        }

        jpeg_dump_pic (_G_left_pic->name, &pic_left_up, &album_size);

    } else if (cur_x > ALBUM_BASE_X + _MIN_OFFSET) { /* user drag to left */
        GUI_COOR pic_left_up = {gra_scr_w() * 2, 0};

        /* scroll screen */
        lcd_fb_move (cur_x - ALBUM_BASE_X - gra_scr_w(), 0);

        /* copy data */
        for (i = 0; i < gra_scr_h(); i++) {
            memcpy (lcd_get_addr (i, 0), lcd_get_addr (i, ALBUM_BASE_X),
                    gra_scr_w() * sizeof (GUI_COLOR) * 2); 
        }

        /* show base screen */
        gra_set_show_fb (1); /* show frame buffer 1 */

        _G_left_pic = (PIC_NAME_NODE *)DL_NEXT(_G_left_pic);
        if (_G_left_pic == NULL) {
            _G_left_pic = (PIC_NAME_NODE *)DL_FIRST(&_G_pic_list);
        }
        /* dump next picture at right */
        _G_right_pic = (PIC_NAME_NODE *)DL_NEXT(_G_right_pic);
        if (_G_right_pic == NULL) {
            _G_right_pic = (PIC_NAME_NODE *)DL_FIRST(&_G_pic_list);
        }

        jpeg_dump_pic (_G_right_pic->name, &pic_left_up, &album_size);
        han_draw_string_t (&pic_left_up, GUI_COLOR_RED,
                     (uint8 *)_G_right_pic->name, strlen (_G_right_pic->name));
    } else {
        /* back */
        lcd_fb_move (cur_x - ALBUM_BASE_X, 0);
    }

#undef _MIN_OFFSET

    return OS_STATUS_OK;
}

/*==============================================================================
** FILE END
==============================================================================*/

