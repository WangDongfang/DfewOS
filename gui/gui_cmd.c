/*==============================================================================
** jpeg_cmd.c -- jpeg command to Dfew OS
**
** MODIFY HISTORY:
**
** 2011-10-24 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include "gui.h"
#include "driver/lcd.h"
#include "driver/rotator.h"
int sscanf(const char *str, char const *fmt, ...);

/*==============================================================================
 * - C_press()
 *
 * - send a touch message to gui core
 */
int C_press (int argc, char **argv)
{
    GUI_COOR scr_coor;
    int press_down = 1;

    /* check argument */
    if (argc < 3) {
        serial_printf ("usage: -> press 100 200 [down|up]");
        return CMD_ERROR;
    } else {
        sscanf (argv[1], "%d", (unsigned int *)&scr_coor.x);
        sscanf (argv[2], "%d", (unsigned int *)&scr_coor.y);

        if (argc >= 4) {
            if (strcmp (argv[3], "up") == 0) {
                press_down = 0;
            }
        }
    }

    /* send message */
    if (press_down) {
        gui_job_add (&scr_coor, GUI_MSG_TOUCH_DOWN);
    } else {
        gui_job_add (&scr_coor, GUI_MSG_TOUCH_UP);
    }
    return CMD_OK;
}

/*==============================================================================
 * - C_rotator()
 *
 * - rotate screen
 */
int C_rotator (int argc, char **argv)
{
    unsigned int src_x, src_y;
    unsigned int width, height;
    unsigned int dst_x, dst_y;
    unsigned int degree;

    /* check argument */
    if (argc < 8) {
        serial_printf ("usage: -> rotator 0 0 800 600 0 0 180");
        return CMD_ERROR;
    } else {
        sscanf (argv[1], "%d", &src_x);
        sscanf (argv[2], "%d", &src_y);
        sscanf (argv[3], "%d", &width);
        sscanf (argv[4], "%d", &height);
        sscanf (argv[5], "%d", &dst_x);
        sscanf (argv[6], "%d", &dst_y);
        sscanf (argv[7], "%d", &degree);
    }

    degree /= 90;

    /* rotate */
    rotator_scr (src_x, src_y, width, height, dst_x, dst_y,
                 (ROTATE_DEGREE)degree);

    return CMD_OK;
}

/*======================================================================
  gui command information struct
======================================================================*/
static CMD_INF _G_gui_cmds[] = {
    {"press",   "vitrual touch screen", C_press},
    {"rotator", "rotate lcd screen", C_rotator}
};

/*==============================================================================
 * - gui_cmd_init()
 *
 * - intstall gui command
 */
void gui_cmd_init()
{
    int i;
    int cmd_num;
    cmd_num = N_ELEMENTS(_G_gui_cmds);

    for (i = 0; i < cmd_num; i++) {
        cmd_add(&_G_gui_cmds[i]);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

