/*==============================================================================
** gui_han.c -- ��ӡ���ֵ�LCD��, ֧��16��16���ֿ�.
**
** �޸���ʷ:
**
** 2011-11-29 ������ ����.
===============================================================================*/
#include <dfewos.h>
#include "../string.h"
#include <stdarg.h>
#include "gui.h"
#include "driver/lcd.h"

#include "yaffs_guts.h"

/*======================================================================
  ���ò���
======================================================================*/
#define HAN_FILE_NAME   "/n1/gb2312_16.hzk"
#define WEIS_PER_QU     94
#define BYTES_PER_HAN   32
#define HAN_HEIGHT      GUI_FONT_HEIGHT
#define HAN_WIDTH       (GUI_FONT_WIDTH * 2)

/*======================================================================
  ȫ�ֱ���
======================================================================*/
static uint8 *_G_p_han_lib = NULL;

/*==============================================================================
 * - han_init()
 *
 * - ��ȡ���ֵ����ڴ�
 */
void han_init ()
{
    int fd;
    int read_byte;

    if (_G_p_han_lib != NULL) {
        return ;
    }

    /*
     * �򿪺�����������ļ�
     */
    fd = yaffs_open(HAN_FILE_NAME, O_RDONLY, 0);
    if (fd ==  -1) {
        serial_printf("���֣��򿪵����ļ�ʧ�� : %s\n", HAN_FILE_NAME);
        return;
    }

    /*
     * Ϊ���ֵ��������ڴ�
     */
    _G_p_han_lib = malloc (512 * KB);
    if (_G_p_han_lib == NULL) {
        serial_printf("���֣������ڴ�ʧ��\n");
        return;
    }

    /*
     * ��ȡ������������ļ�
     */
    read_byte = yaffs_read(fd, _G_p_han_lib, 512 * KB);
    if (read_byte <= 0) {
        serial_printf("���֣���ȡ�����ļ�ʧ��\n");
    }

    yaffs_close(fd);
}

/*==============================================================================
 * - han_draw_char()
 *
 * - �� <s> ������ɫ <color> дһ������
 */
void han_draw_char (const GUI_COOR *s, uint16 color,
                    const uint8 *string, int have_bg)
{
    int row;
    int col;

    if (*(string + 1) & 0x80) { /* �жϵڶ����ֽ��Ƿ���λ�� */
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
                lcd_set_pixel (s->y + row,   /* Һ�����ĵڼ��� */
                               s->x + col,   /* Һ�����ĵڼ��� */
                               ((bits & 0x8000) ? color : GUI_BG_COLOR));
                } else if (bits & 0x8000){
                lcd_set_pixel (s->y + row,   /* Һ�����ĵڼ��� */
                               s->x + col,   /* Һ�����ĵڼ��� */
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
 * - ��ӡһ���ַ�������������Ӣ�Ļ��ӵ��ַ���
 */
void han_draw_string (const GUI_COOR *s, uint16 color,
                      const uint8 *string, int count)
{
    GUI_COOR cur_coor = *s;

    while (count > 0) {
        if ((*string & 0x80) == 0) { /* ��ǰҪ��ʾ���ַ���Ӣ���ַ� */
            font_draw_string (&cur_coor, color, string, 1);

            string++;
            cur_coor.x += GUI_FONT_WIDTH;
            count--;
        } else {                     /* ��һ���ֽ��������� */
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
 * - ��ӡһ���ַ�������������Ӣ�Ļ��ӵ��ַ��������汳��
 */
void han_draw_string_t (const GUI_COOR *s, uint16 color,
                        const uint8 *string, int count)
{
    GUI_COOR cur_coor = *s;

    while (count > 0) {
        if ((*string & 0x80) == 0) { /* ��ǰҪ��ʾ���ַ���Ӣ���ַ� */
            font_draw_string_t (&cur_coor, color, string, 1);

            string++;
            cur_coor.x += GUI_FONT_WIDTH;
            count--;
        } else {                     /* ��һ���ֽ��������� */
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
 * - �� <s> ������ɫ <color> ��ֱ�Ŵ��µ���дһ������
 */
void han_draw_char_v (const GUI_COOR *s, uint16 color,
                      const uint8 *string)
{
    int row;
    int col;

    if (*(string + 1) & 0x80) { /* �жϵڶ����ֽ��Ƿ���λ�� */
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
                lcd_set_pixel (s->y - col,   /* Һ�����ĵڼ��� */
                               s->x + row,   /* Һ�����ĵڼ��� */
                               ((bits & 0x8000) ? color : GUI_BG_COLOR));
                
                bits <<= 1;
            }
        }
    }
}

/*==============================================================================
 * - han_draw_string_v()
 *
 * - ��ֱ�Ŵ�ӡһ���ַ�������������Ӣ�Ļ��ӵ��ַ���
 */
void han_draw_string_v (const GUI_COOR *s, uint16 color,
                        const uint8 *string, int count)
{
    GUI_COOR cur_coor = *s;

    while (count > 0) {
        if ((*string & 0x80) == 0) { /* ��ǰҪ��ʾ���ַ���Ӣ���ַ� */
            font_draw_string_v (&cur_coor, color, string, 1);

            string++;
            cur_coor.y -= GUI_FONT_WIDTH;
            count--;
        } else {                     /* ��һ���ֽ������� */
            han_draw_char_v (&cur_coor, color, string);

            string += 2;
            cur_coor.y -= HAN_WIDTH;
            count -= 2;
        }
    }
}

/*==============================================================================
** �ļ�����
==============================================================================*/

