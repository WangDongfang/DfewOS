/*==============================================================================
** home.c -- home application. show some cbi on desktop for user click.
**
** MODIFY HISTORY:
**
** 2011-11-07 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../gui.h"

/*======================================================================
  configs
======================================================================*/
#define IMG_SIZE            100
#define IMGS_PER_LAYER      3

/*======================================================================
  other file support function
======================================================================*/
extern OS_STATUS app_tetris (GUI_CBI *pCBI_tetris, GUI_COOR *coor);
extern OS_STATUS app_lian (GUI_CBI *pCBI_lian, GUI_COOR *coor);
extern OS_STATUS app_album (GUI_CBI *pCBI_album, GUI_COOR *coor);
extern OS_STATUS app_cal (GUI_CBI *pCBI_cal, GUI_COOR *coor);
extern OS_STATUS app_reader (GUI_CBI *pCBI_reader, GUI_COOR *coor);
extern OS_STATUS app_calendar (GUI_CBI *pCBI_calendar, GUI_COOR *coor);
extern OS_STATUS app_media (GUI_CBI *pCBI_media, GUI_COOR *coor);
extern OS_STATUS app_wuzi (GUI_CBI *pCBI_wuzi, GUI_COOR *coor);
extern OS_STATUS app_clock (GUI_CBI *pCBI_clock, GUI_COOR *coor);

/*==============================================================================
 * - app_home()
 *
 * - home application entry.
 */
void app_home ()
{
    int i;
    ICON_CBI ics[] = {
        {"/n1/gui/tetris.jpg",  app_tetris},
        {"/n1/gui/lian.jpg",    app_lian},
        {"/n1/gui/album.jpg",   app_album},
        {"/n1/gui/cal.jpg",     app_cal},
        {"/n1/gui/reader.jpg",  app_reader},
        {"/n1/gui/calendar.jpg",app_calendar},
        {"/n1/gui/media.jpg",   app_media},
        {"/n1/gui/wuzi.jpg",    app_wuzi},
        {"/n1/gui/clock.jpg",   app_clock}
    };
    int img_total_num = N_ELEMENTS(ics);

    GUI_CBI *pCBI = NULL;
    GUI_COOR left_up;
    GUI_SIZE size = {0, 0};
    int blank_width;
    int blank_height;
    int img_layer;          /* the layer number */
    GUI_SIZE scr_size;

    /* clear screen */
    gra_set_show_fb (0); /* show frame buffer 0 */
    cbi_delete_all (); /* delete all other cbis */

    /* calculate the cbis' left up coordinate */
    gra_get_scr_size (&scr_size);
    img_layer = (img_total_num + IMGS_PER_LAYER - 1) / IMGS_PER_LAYER;
    blank_width  = (scr_size.w - IMGS_PER_LAYER * IMG_SIZE) / (IMGS_PER_LAYER + 1);
    blank_height = (scr_size.h - img_layer * IMG_SIZE) / (img_layer + 1);
    
    /* register cbis */
    for (i = 0; i < img_total_num; i++) {
        int layer = i / IMGS_PER_LAYER;
        int order = i % IMGS_PER_LAYER;
        left_up.x = (order + 1) * blank_width + order * IMG_SIZE;
        left_up.y = (layer + 1) * blank_height + layer * IMG_SIZE;

        pCBI = cbi_create_default (ics[i].name, &left_up, &size, TRUE);
        GUI_ASSERT(pCBI != NULL);

        pCBI->func_drag    = cbf_do_drag;
        pCBI->func_release = ics[i].func;

        cbi_register (pCBI);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

