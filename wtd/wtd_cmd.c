/*==============================================================================
** wtd_cmd.c -- wtd command to Dfew OS
**
** MODIFY HISTORY:
**
** 2011-10-21 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../cmd.h"
#include "wtd.h"
int sscanf(const char *str, char const *fmt, ...);

/*==============================================================================
 * - C_wtd()
 *
 * - 'wtd' command
 */
int C_wtd (int argc, char **argv)
{
    int count = 0xFFFF;
    int delay = 1;
    int times = 10;

    if (argc < 4) {
        serial_printf ("useage: wtd count delay times");
        return CMD_OK;
    }
    if (argc >= 2) {
        sscanf (argv[1], "%d", (unsigned int *)&count);
    }
    if (argc >= 3) {
        sscanf (argv[2], "%d", (unsigned int *)&delay);
    }
    if (argc >= 4) {
        sscanf (argv[3], "%d", (unsigned int *)&times);
    }

    wtdStart ((unsigned short)count);

    if (argc >= 5) {
        *(volatile unsigned *)0x7E004000 = 0xFF31;
    }

    while (times--) {
        delayQ_delay (delay * SYS_CLK_RATE);
        wtdClear ();
        serial_printf ("watch dog have feed!\n");
    }

    wtdStop ();

    return CMD_OK;
}

/*======================================================================
  "wtd" command information struct
======================================================================*/
static CMD_INF _G_wtd_cmd = {"wtd", "set and show wtd", C_wtd};

/*==============================================================================
 * - wtd_cmd_init()
 *
 * - intstall "wtd" command
 */
void wtd_cmd_init()
{
    cmd_add(&_G_wtd_cmd);
}

/*==============================================================================
** FILE END
==============================================================================*/

