/*==============================================================================
** gui_font.c -- lcd font.
**
** MODIFY HISTORY:
**
** 2011-11-03 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include <stdio.h>
#include <stdarg.h>
#include "gui.h"
#include "gui_font.h"
#include "driver/lcd.h"

/*======================================================================
  config
======================================================================*/
#define PRINT_BUF_SIZE      1024

/*==============================================================================
 * - font_draw_string()
 *
 * - print a string on lcd.
 */
void font_draw_string (const GUI_COOR *s, uint16 color,
                       const uint8 *string, int count)
{
    int f_row, f_col;
    uint8 bits;
    int i;
    const uint8 *str;
    
    for (f_row = 0; f_row < GUI_FONT_HEIGHT; f_row++) {
        str = string;
        for (i = 0; i < count; i++) {

            bits = _G_font_data[*str++][f_row]; /* the row of char <c> */

            for (f_col = 0; f_col < GUI_FONT_WIDTH; f_col++) {

                /* set a pixel color */
                lcd_set_pixel (s->y + f_row,                        /* lcd row */
                               s->x + i * GUI_FONT_WIDTH + f_col,   /* lcd col */
                               ((bits & 0x80) ? color : GUI_BG_COLOR));
                bits <<= 1;
            }
        }
    }
}

/*==============================================================================
 * - font_draw_string_t()
 *
 * - print a string on lcd. no background color
 */
void font_draw_string_t (const GUI_COOR *s, uint16 color,
                         const uint8 *string, int count)
{
    int f_row, f_col;
    uint8 bits;
    int i;
    const uint8 *str;
    
    for (f_row = 0; f_row < GUI_FONT_HEIGHT; f_row++) {
        str = string;
        for (i = 0; i < count; i++) {

            bits = _G_font_data[*str++][f_row]; /* the row of char <c> */

            for (f_col = 0; f_col < GUI_FONT_WIDTH; f_col++) {

                /* set a pixel color */
                if (bits & 0x80) {
                    lcd_set_pixel (s->y + f_row,                        /* lcd row */
                                   s->x + i * GUI_FONT_WIDTH + f_col,   /* lcd col */
                                   color);
                }
                bits <<= 1;
            }
        }
    }
}


/*==============================================================================
 * - font_draw_string_v()
 *
 * - print a string on lcd verticaly
 */
void font_draw_string_v (const GUI_COOR *s, uint16 color,
                         const uint8 *string, int count)
{
    int f_row, f_col;
    uint8 bits;
    int i;
    const uint8 *str;
    
    for (f_row = 0; f_row < GUI_FONT_HEIGHT; f_row++) {
        str = string;
        for (i = 0; i < count; i++) {

            bits = _G_font_data[*str++][f_row]; /* the row of char <c> */

            for (f_col = 0; f_col < GUI_FONT_WIDTH; f_col++) {

                /* set a pixel color */
                lcd_set_pixel (s->y - i * GUI_FONT_WIDTH - f_col,   /* lcd row */
                               s->x + f_row,                        /* lcd col */
                               ((bits & 0x80) ? color : GUI_BG_COLOR));
                bits <<= 1;
            }
        }
    }
}

/*==============================================================================
 * - font_printf()
 *
 * - format a string then print it on lcd.
 */
void font_printf (const GUI_COOR *start, uint16 color,
                  const char *fmt, ...)
{
    va_list args;
	char buf[PRINT_BUF_SIZE];

	va_start(args, fmt);
	vsnprintf(buf, PRINT_BUF_SIZE, fmt, args);
	va_end(args);

	font_draw_string (start, color,(uint8 *)buf, strlen (buf));
}

/*==============================================================================
** FILE END
==============================================================================*/

