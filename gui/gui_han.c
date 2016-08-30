/*==============================================================================
** gui_han.c -- 打印汉字到LCD屏, 支持16×16的字库.
**
** 修改历史:
**
** 2011-11-29 王东方 创建.
===============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include <stdarg.h>
#include "gui.h"
#include "driver/lcd.h"

#include "yaffs_guts.h"

/*======================================================================
  配置参数
======================================================================*/
#define HAN_FILE_NAME   "/n1/gb2312_16.hzk"
#define WEIS_PER_QU     94
#define BYTES_PER_HAN   32
#define HAN_HEIGHT      GUI_FONT_HEIGHT
#define HAN_WIDTH       (GUI_FONT_WIDTH * 2)

/*======================================================================
  全局变量
======================================================================*/
static uint8 *_G_p_han_lib = NULL;

/*==============================================================================
 * - han_init()
 *
 * - 读取汉字点阵到内存
 */
void han_init ()
{
    int fd;
    int read_byte;

    if (_G_p_han_lib != NULL) {
        return ;
    }

    /*
     * 打开汉字字体点阵文件
     */
    fd = yaffs_open(HAN_FILE_NAME, O_RDONLY, 0);
    if (fd ==  -1) {
        serial_printf("汉字：打开点阵文件失败 : %s\n", HAN_FILE_NAME);
        return;
    }

    /*
     * 为汉字点阵申请内存
     */
    _G_p_han_lib = malloc (512 * KB);
    if (_G_p_han_lib == NULL) {
        serial_printf("汉字：申请内存失败\n");
        return;
    }

    /*
     * 读取汉字字体点阵文件
     */
    read_byte = yaffs_read(fd, _G_p_han_lib, 512 * KB);
    if (read_byte <= 0) {
        serial_printf("汉字：读取点阵文件失败\n");
    }

    yaffs_close(fd);
}

/*==============================================================================
 * - han_draw_char()
 *
 * - 在 <s> 处用颜色 <color> 写一个汉字
 */
void han_draw_char (const GUI_COOR *s, uint16 color,
                    const uint8 *string, int have_bg)
{
    int row;
    int col;

    if (*(string + 1) & 0x80) { /* 判断第二个字节是否是位码 */
        uint8 qu  = (*string - 0xa0);
        uint8 wei = (*(string + 1) - 0xa0);
        int   num =  (qu - 1) * WEIS_PER_QU + wei -1;
        uint8 *p_pixel = _G_p_han_lib + num * BYTES_PER_HAN;

        uint16 bits;
        uint8 l;
        uint8 r;

        for (row = 0; row < HAN_HEIGHT; row++) {
            l = (*p_pixel++);
            r = (*p_pixel++);
            bits = (l << 8) | r;

            for (col = 0; col < HAN_WIDTH; col++) {
                if (have_bg) {
                lcd_set_pixel (s->y + row,   /* 液晶屏的第几行 */
                               s->x + col,   /* 液晶屏的第几列 */
                               ((bits & 0x8000) ? color : GUI_BG_COLOR));
                } else if (bits & 0x8000){
                lcd_set_pixel (s->y + row,   /* 液晶屏的第几行 */
                               s->x + col,   /* 液晶屏的第几列 */
                               color);
                }
                
                bits <<= 1;
            }
        }
    }
}

/*==============================================================================
 * - han_draw_string()
 *
 * - 打印一个字符串，可以是中英文混杂的字符串
 */
void han_draw_string (const GUI_COOR *s, uint16 color,
                      const uint8 *string, int count)
{
    GUI_COOR cur_coor = *s;

    while (count > 0) {
        if ((*string & 0x80) == 0) { /* 当前要显示的字符是英文字符 */
            font_draw_string (&cur_coor, color, string, 1);

            string++;
            cur_coor.x += GUI_FONT_WIDTH;
            count--;
        } else {                     /* 第一个字节是是区码 */
            han_draw_char (&cur_coor, color, string, 1);

            string += 2;
            cur_coor.x += HAN_WIDTH;
            count -= 2;
        }
    }
}

/*==============================================================================
 * - han_draw_string()
 *
 * - 打印一个字符串，可以是中英文混杂的字符串，不绘背景
 */
void han_draw_string_t (const GUI_COOR *s, uint16 color,
                        const uint8 *string, int count)
{
    GUI_COOR cur_coor = *s;

    while (count > 0) {
        if ((*string & 0x80) == 0) { /* 当前要显示的字符是英文字符 */
            font_draw_string_t (&cur_coor, color, string, 1);

            string++;
            cur_coor.x += GUI_FONT_WIDTH;
            count--;
        } else {                     /* 第一个字节是是区码 */
            han_draw_char (&cur_coor, color, string, 0);

            string += 2;
            cur_coor.x += HAN_WIDTH;
            count -= 2;
        }
    }
}


/*==============================================================================
 * - han_draw_char_v()
 *
 * - 在 <s> 处用颜色 <color> 竖直着从下到上写一个汉字
 */
void han_draw_char_v (const GUI_COOR *s, uint16 color,
                      const uint8 *string)
{
    int row;
    int col;

    if (*(string + 1) & 0x80) { /* 判断第二个字节是否是位码 */
        uint8 qu  = (*string - 0xa0);
        uint8 wei = (*(string + 1) - 0xa0);
        int   num =  (qu - 1) * WEIS_PER_QU + wei -1;
        uint8 *p_pixel = _G_p_han_lib + num * BYTES_PER_HAN;

        uint16 bits;
        uint8 l;
        uint8 r;

        for (row = 0; row < HAN_HEIGHT; row++) {
            l = (*p_pixel++);
            r = (*p_pixel++);
            bits = (l << 8) | r;

            for (col = 0; col < HAN_WIDTH; col++) {
                lcd_set_pixel (s->y - col,   /* 液晶屏的第几行 */
                               s->x + row,   /* 液晶屏的第几列 */
                               ((bits & 0x8000) ? color : GUI_BG_COLOR));
                
                bits <<= 1;
            }
        }
    }
}

/*==============================================================================
 * - han_draw_string_v()
 *
 * - 竖直着打印一个字符串，可以是中英文混杂的字符串
 */
void han_draw_string_v (const GUI_COOR *s, uint16 color,
                        const uint8 *string, int count)
{
    GUI_COOR cur_coor = *s;

    while (count > 0) {
        if ((*string & 0x80) == 0) { /* 当前要显示的字符是英文字符 */
            font_draw_string_v (&cur_coor, color, string, 1);

            string++;
            cur_coor.y -= GUI_FONT_WIDTH;
            count--;
        } else {                     /* 第一个字节是区码 */
            han_draw_char_v (&cur_coor, color, string);

            string += 2;
            cur_coor.y -= HAN_WIDTH;
            count -= 2;
        }
    }
}

/*==============================================================================
** 文件结束
==============================================================================*/

