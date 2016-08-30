/*==============================================================================
** gui_util.c -- draw something
**
** MODIFY HISTORY:
**
** 2011-11-01 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../string.h"
#include "gui.h"
#include "driver/lcd.h"

/*==============================================================================
 * - gra_get_block()
 *
 * - get a screen blcok pixels data to a continue memory
 */
GUI_COLOR *gra_get_block (const GUI_COOR *p_start, const GUI_SIZE *p_size)
{
    int i;
    GUI_COLOR  *p = malloc (p_size->w * p_size->h * sizeof (GUI_COLOR));

    if (p == NULL) {
        return NULL;
    }

    for (i = 0; i < p_size->h; i++) {
        memcpy (p + i * p_size->w,
                lcd_get_addr (p_start->y + i, p_start->x),
                p_size->w * sizeof (GUI_COLOR));
    }

    return p;
}

/*==============================================================================
 * - gra_block()
 *
 * - draw a picture, which pixel color data in <p_data>
 *   if <p_data> is NULL, use background color cover the size.
 */
void gra_block (const GUI_COOR *p_start, const GUI_SIZE *p_size, const uint16 *p_data)
{
    int i;
    int start_row = p_start->y;
    int end_row   = p_start->y + p_size->h;

    if (p_data == NULL) {
        gra_rect (p_start, p_size, GUI_BG_COLOR);
    } else {
        for (i = start_row; i < end_row; i++) {
            lcd_set_block (i, p_start->x,
                           p_data + (i - start_row) * p_size->w,
                           p_size->w);
        }
    }
}

/*==============================================================================
 * - gra_block_t()
 *
 * - draw a picture, which around GUI_BG_COLOR not draw
 *   if <p_data> is NULL, no draw
 */
void gra_block_t (const GUI_COOR *p_start, const GUI_SIZE *p_size, const uint16 *p_data)
{
    int i, j;
    GUI_COLOR color;
    int start_row = p_start->y;
    int end_row   = p_start->y + p_size->h;
    int turn_on;

    if (p_data != NULL) {
        int middle_col = p_start->x + p_size->w / 2;
        for (i = start_row; i < end_row; i++) {
            turn_on = 0;
            for (j = p_start->x; j <= middle_col; j++) {
                color = *(p_data + (i - start_row) * p_size->w + j - p_start->x);
                if ((color & 0xE79C) != GUI_BG_COLOR || turn_on) {
                    lcd_set_pixel (i, j, color);
                    turn_on = 1;
                }
            }
            turn_on = 0;
            for (j = p_start->x + p_size->w - 1; j > middle_col; j--) {
                color = *(p_data + (i - start_row) * p_size->w + j - p_start->x);
                if ((color & 0xE79C) != GUI_BG_COLOR || turn_on) {
                    lcd_set_pixel (i, j, color);
                    turn_on = 1;
                }
            }

        }
    }
}


/*==============================================================================
 * - gra_rect()
 *
 * - draw a rectangle
 */
void gra_rect (const GUI_COOR *p_start, const GUI_SIZE *p_size, uint16 color)
{
    int i;
    int start_row = p_start->y;
    int end_row   = p_start->y + p_size->h;

    for (i = start_row; i < end_row; i++) {
        lcd_set_color (i, p_start->x, color, p_size->w);
    }
}

/*==============================================================================
 * - gra_line()
 *
 * - draw a line (only vertical or horizon line is support)
 */
OS_STATUS gra_line (const GUI_COOR *p_start, const GUI_COOR *p_end, uint16 color, int width)
{
    if ((p_start->x != p_end->x) && (p_start->y != p_end->y)) {
        return OS_STATUS_ERROR;
    }

    GUI_COOR s = {MIN(p_start->x, p_end->x), MIN(p_start->y, p_end->y)};
    GUI_COOR e = {MAX(p_start->x, p_end->x), MAX(p_start->y, p_end->y)};
    GUI_SIZE sz = {(e.x - s.x) ? (e.x - s.x + 1) : width,
                   (e.y - s.y) ? (e.y - s.y + 1) : width};

    gra_rect (&s, &sz, color);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - gra_line_u()
 *
 * - draw a line (universally)
 */
void gra_line_u (const GUI_COOR *p_start, const GUI_COOR *p_end, GUI_COLOR color, int width)
{
    int   dx;      /* 直线x轴差值变量 */
    int   dy;      /* 直线y轴差值变量 */
    int   dx_sym;  /* x轴增长方向，为-1时减值方向，为1时增值方向 */
    int   dy_sym;  /* y轴增长方向，为-1时减值方向，为1时增值方向 */
    int   dx_x2;   /* dx*2值变量，用于加快运算速度 */
    int   dy_x2;   /* dy*2值变量，用于加快运算速度 */
    int   di;      /* 决策变量 */

    int   wx, wy;  /* 线宽变量 */

    int x0 = p_start->x;
    int y0 = p_start->y;
    int x1 = p_end->x;
    int y1 = p_end->y;

    GUI_COOR start;
    GUI_COOR end;

    /* try to draw horizon or vertical line */
    if (gra_line (p_start, p_end, color, width) == OS_STATUS_OK) {
        return ;
    }

    dx = x1 - x0;  /* 求取两点之间的差值 */
    dy = y1 - y0;

    wx = width / 2;
    wy = width - wx - 1;

    /* 判断增长方向 */
    dx_sym = (dx > 0) ? 1 : -1;
    dy_sym = (dy > 0) ? 1 : -1;

    /* 将dx、dy取绝对值 */
    dx = dx_sym * dx;
    dy = dy_sym * dy;

    /* 计算2倍的dx及dy值 */
    dx_x2 = dx * 2;
    dy_x2 = dy * 2;

#define _LINE(a, b, c, d)   start.x = a; \
                            start.y = b; \
                            end.x   = c; \
                            end.y   = d; \
                            gra_line (&start, &end, color, 1);

    /* 使用 Bresenham 法进行画直线 */
    if (dx >= dy) {                       /* 对于dx>=dy，则使用x轴为基准 */

        di = dy_x2 - dx;

        while (x0 != x1) {  /* x轴向增长，则宽度在y方向，即画垂直线 */

            _LINE(x0, y0 - wx, x0, y0 + wy);
            
            x0 += dx_sym;                

            if (di < 0) {
                di += dy_x2; /* 计算出下一步的决策值 */
            } else {
                di += dy_x2 - dx_x2;
                y0 += dy_sym;
            }
        }

        _LINE(x0, y0 - wx, x0, y0 + wy);

    } else {                              /* 对于dx < dy，则使用y轴为基准 */

        di = dx_x2 - dy;

        while (y0 != y1) {  /* y轴向增长，则宽度在x方向，即画水平线 */

            _LINE(x0 - wx, y0, x0 + wy, y0);

            y0 += dy_sym;

            if (di < 0) {
                di += dx_x2;
            } else {
                di += dx_x2 - dy_x2;
                x0 += dx_sym;
            }
        }
        _LINE(x0 - wx, y0, x0 + wy, y0);
    } 
#undef _LINE
}

/*==============================================================================
 * - gra_clear()
 *
 * - set all pixel as <color>
 */
void gra_clear (GUI_COLOR color)
{
    lcd_clear (color);
}

/*==============================================================================
 * - gra_get_scr_size()
 *
 * - get lcd screen size
 */
void gra_get_scr_size (GUI_SIZE *p_size)
{
    p_size->w = U_LCD_XSIZE;
    p_size->h = U_LCD_YSIZE;
}

/*==============================================================================
 * - gra_scr_w()
 *
 * - return lcd screen width
 */
int gra_scr_w ()
{
    return U_LCD_XSIZE;
}

/*==============================================================================
 * - gra_scr_h()
 *
 * - return lcd screen height
 */
int gra_scr_h ()
{
    return U_LCD_YSIZE;
}

/*==============================================================================
 * - gra_set_show_fb()
 *
 * - tell lcd controller show which buffer
 */
void *gra_set_show_fb (int index)
{
    return lcd_set_show_fb (index);
}

/*==============================================================================
 * - gra_set_pixel()
 *
 * - set a pixel to <color>
 */
void gra_set_pixel (const GUI_COOR *p_coor, GUI_COLOR color)
{
    return lcd_set_pixel (p_coor->y, p_coor->x, color);
}

/*==============================================================================
 * - gra_get_pixel()
 *
 * - get a pixel's color
 */
GUI_COLOR gra_get_pixel (const GUI_COOR *p_coor)
{
    return lcd_get_pixel (p_coor->y, p_coor->x);
}


/*==============================================================================
 ** FILE END
==============================================================================*/

