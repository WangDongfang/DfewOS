/*==============================================================================
** media_cmd.c -- media commands to Dfew OS
**
** MODIFY HISTORY:
**
** 2011-08-17 wdf Create.
==============================================================================*/
#include "../config.h"
#include "../types.h"
#include "../cmd.h"
#include "../string.h"
#include "../serial.h"
#include "ffplay.h"
#include "jpeg.h"
int sscanf(const char *str, char const *fmt, ...);

/*======================================================================
  Function Declare Forward
======================================================================*/
int C_ffplay(int argc, char **argv);
int C_ffstop(int argc, char **argv);
int C_jpeg(int argc, char **argv);

/*======================================================================
  media commands information struct
======================================================================*/
static CMD_INF _G_media_cmds[] = {
    {"ffplay", "play a video", C_ffplay},
    {"ffstop", "stop a video", C_ffstop},
    {"jpeg", "show a picture", C_jpeg}
};

/*==============================================================================
 * - C_ffplay()
 *
 * - play a video
 */
int C_ffplay(int argc, char **argv)
{
    char media_name[60] = "/yaffs2/n1/media/";

    if (argc == 1) {
        strcat (media_name, "safe_and_sound.avi");
    } else {
        strcat (media_name, argv[1]);
    }
    if (strcmp (media_name + strlen (media_name) - 4 ,".avi") != 0) {
        strcat (media_name, ".avi");
    }

    media_play (media_name);

    return CMD_OK;
}

/*==============================================================================
 * - C_ffstop()
 *
 * - stop a video
 */
int C_ffstop(int argc, char **argv)
{
    media_stop ();

    return CMD_OK;
}

/*==============================================================================
 * - C_jpeg()
 *
 * - show a picture
 */
int C_jpeg (int argc, char **argv)
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    if (argc <= 1) {
        serial_printf ("usage: -> jpeg love.jpg [10] [20]");
        return CMD_ERROR;
    }

    if (argc >= 3) {
        sscanf (argv[2], "%d", &x);
    }
    if (argc >= 4) {
        sscanf (argv[3], "%d", &y);
    }
    if (argc >= 5) {
        sscanf (argv[4], "%d", &w);
    }
    if (argc >= 6) {
        sscanf (argv[5], "%d", &h);
    }

    jpeg_show_file (argv[1], x, y, w, h);
    return CMD_OK;
}

/*==============================================================================
 * - media_cmd_init()
 *
 * - intstall media commands
 */
void media_cmd_init()
{
    int i;
    int cmd_num;
    cmd_num = N_ELEMENTS(_G_media_cmds);

    for (i = 0; i < cmd_num; i++) {
        cmd_add(&_G_media_cmds[i]);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

