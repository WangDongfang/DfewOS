/*==============================================================================
** gui_init.c -- init gui.
**
** MODIFY HISTORY:
**
** 2011-10-28 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include "gui.h"
#include "driver/lcd.h"
#include "driver/touch.h"
#include "LibJPEG/jpeg_util.h"

void gui_cmd_init();
void app_logo();

/*==============================================================================
 * - gui_init()
 *
 * - init gui
 */
int gui_init ()
{
    /* lcd init */
    lcd_init ();

    /* touch screen init */
    touch_screen_init ();

    /* add gui cmd */
    gui_cmd_init ();

    /* start gui job thread */
    gui_core_init ();

    /* init han zi */
    han_init();

    /* start logo app */
    app_logo ();

    return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

