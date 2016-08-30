/****************************************Copyright (c)*****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info----------------------------------------------------------------------------------
** File name:           s3c6410x_lcd.c
** Last modified Date:  2011-06-13
** Last Version:        1.0
** Descriptions:        the lcd controller driver
**
**---------------------------------------------------------------------------------------------------------
** Created by:          WangDongfang
** Created date:        2011-06-13
** Version:             1.0
** Descriptions:        the lcd controller driver
**
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
#include <dfewos.h>
#include "../../string.h"
#include "s3c6410x_lcd.h"

/**********************************************************************************************************
  The Globe Variables
**********************************************************************************************************/
static uint16 *_G_fb_addr[FRAME_BUFFER_NUM] =
{
    (uint16 *)FIRST_FB_ADDR,   /* Frame Buffer left */
    (uint16 *)SECND_FB_ADDR,   /* Frame Buffer middle */
    (uint16 *)THIRD_FB_ADDR,   /* Frame Buffer right */
};
static uint16 *_G_show_addr = (uint16 *)FIRST_FB_ADDR;

/**********************************************************************************************************
  Function forward declare
**********************************************************************************************************/
static void _lcd_update_fb_addr ();

/**********************************************************************************************************
** Function name:           lcd_enable
** Descriptions:            Enable lcd power. Called by common/lcd.c lcd_init()
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2011-04-14
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
void lcd_enable(void)
{
    rVIDCON0           |=  ((1    << 1)                                 /*  0   -   �������Ƶ�ź�------*/
                                                                        /*  1   -   �����Ƶ�ź�        */
                        |  (1    << 0));                                /*  0   -   ֡β��Ƶ�źű�ʹ��--*/
                                                                        /*  1   -   ֡β��Ƶ�źű���ֹ  */
}

/**********************************************************************************************************
** Function name:           lcd_ctrl_init
** Descriptions:            initial lcd controller's register. Called by common/lcd.c lcd_init
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2011-04-14
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
void lcd_init()
{
    /*
     *  ���ö˿�ΪLCD������
     */
    rMIFPCON           &= ~(1   << 3);                                  /*  bypass ģʽ����             */
    rSPCON              = (rSPCON & ~(3UL)) | 1UL;                      /*  ѡ��LCD������ΪRGBģʽ      */

    rGPICON             = 0xaaaaaaaa;                                   /*  ʹ�����Ź�����LCD������     */
    rGPIPUD             = 0;                                            /*  ��ֹ������������            */
    rGPJCON             = 0xaaaaaa;                                     /*  ����GPJ���Ź���ΪLCD������  */
    rGPJPUD             = 0;                                            /*  ��ֹ������������            */




    rVIDCON0            = (0    << 29)                                  /*  0   -   ����ģʽ------------*/
                                                                        /*  1   -   ����ģʽ            */

                        | (0    << 26)                                  /*  00  -   RGB ģʽ------------*/
                                                                        /*  01  -   TVģʽ              */
                                                                        /*  02  -   LDI0 180 IF         */
                                                                        /*  03  -   LDI1 180 IF         */

                        | (0    << 23)                                  /*  XXX -   LDI1 180 IF���ݷ�ʽ-*/

                        | (0    << 20)                                  /*  XXX -   LDI1 180 IF���ݷ�ʽ-*/

                        | (0    << 17)                                  /*  00  -   RGB���� RGB---------*/
                                                                        /*  01  -   RGB���� BGR         */
                                                                        /*  10  -   ����RGB R-G-B       */
                                                                        /*  10  -   ����RGB B-G-R       */

                        | (0    << 16)                                  /*  0   -   CLKVAL_Fһֱ��Ч----*/
                                                                        /*  1-  -   ֡��ʼʱһֻ֡��һ��*/

                        | (CLK_VAL << 6)                                /*  0-255   LCDʱ�ӷ�Ƶ��-------*/
                                                                        /*          VCLK = clk/������+1)*/
                                                                        /*          VCLK����ܳ���66M */

                        | (0    << 5)                                   /*  0   -   ����ģʽ/��ENVID����*/
                                                                        /*  1   -   ��������            */

                        | (1    << 4)                                   /*  0   -   ֱ��ʹ��ѡ��ʱ��----*/
                                                                        /*  1   -   ʹ�÷�Ƶ���ʱ��    */

                        | (0    << 2)                                   /*  00  -   clk ʱ��Դ =HCLK--  */
                                                                        /*  01  -   clk ʱ��Դ =��Ƶʱ��*/
                                                                        /*  10  -   RESV                */
                                                                        /*  11  -   clk ʱ��Դ=27M EXCLK*/

                        | (0    << 1)                                   /*  0   -   �������Ƶ�ź�------*/
                                                                        /*  1   -   �����Ƶ�ź�        */

                        | (0    << 0);                                  /*  0   -   ֡β��Ƶ�źű�ʹ��--*/
                                                                        /*  1   -   ֡β��Ƶ�źű���ֹ  */


    rVIDCON1            = (0      << 7)                                 /*  0   -   VCLK�½�����Ч------*/
                                                                        /*  1   -   VCLK��������Ч      */

                        | (HS_POL << 6)                                 /*  0   -   Hsync������Ч-------*/
                                                                        /*  1   -   Hsync����           */

                        | (VS_POL << 5)                                 /*  0   -   Vsync������Ч-------*/
                                                                        /*  1   -   Vsync����           */

                        | (0      << 4)                                 /*  0   -   VDEN������Ч--------*/
                                                                        /*  1   -   VDEN������Ч        */

                        | (0    << 0);                                  /*  ����                        */


    rVIDCON2            = (0    << 23)                                  /*  0   -   ITU601�����--------*/
                                                                        /*  1   -   ITU601��ʹ��        */

                        | (0    << 14)                                  /*  0   -   YUV���ݸ�ʽӲ��ѡ��-*/
                                                                        /*  1   -   YUV���ݸ�ʽ���ѡ�� */

                        | (0    << 12)                                  /*  00  -   YUV���RGB��ʽ------*/
                                                                        /*  01  -   YUV 422�����ʽ     */
                                                                        /*  1x  -   YUV 444�����ʽ     */

                        | (0    << 8)                                   /*  0   -   Y-cb-cr���ݸ�ʽ-----*/
                                                                        /*  1   -   cb-cr-Y���ݸ�ʽ     */

                        | (0    << 7)                                   /*  0   -   cb-cr���ݸ�ʽ-------*/
                                                                        /*  1   -   cr-cb���ݸ�ʽ       */

                        | (0    << 0);                                  /*  ����                        */


    /*
     *      |VFPD  |VSPW |VBPD |-------------��Ч��-----------------------------|
     *      |VFPDE |     |VBPDE|                                                |
     *   ֡��ʼ    |     |     |                                            ֡����
     *      V      V     V     V                                                V
     *      |------|     |------------------------------------------------------|
     *      |      |     |     ^                                                ^
     *      |      ------      |                                                |
     *      |                  -------------------------------------------------|
     *      |                  ^                                                ^
     *      |                line0                                            line?
     */
    rVIDTCON0           = (0            << 24)                          /*  VBPDE-----------------------*/
                                                                        /*  0-255  ֡��ʼ����ͬ������� */
                                                                        /*  �ж�����Ч��,(ֻ���YUV�˿�)*/

                        | (VBP   << 16)                                 /*  VBPD------------------------*/
                                                                        /*  0-255  ֡��ʼ����ͬ������� */
                                                                        /*  �ж�����Ч��                */

                        | (VFP   << 8)                                  /*  VFPD------------------------*/
                                                                        /*  0-255   -  ֡������,��ͬ����*/
                                                                        /*  ��ǰ�ж��ٸ���Ч��          */

                        | (VSW   << 0);                                 /*  VSPW------------------------*/
                                                                        /*  0-255 ֡ͬ�������ж�����Ч��*/


    /*
     *      |HFPD  |HSPW |HBPD |-------------��Ч����---------------------------|
     *   ����ʼ    |     |     |                                            �н���
     *      V      V     V     V                                                V
     *      |------|     |------------------------------------------------------|
     *      |      |     |     ^                                                ^
     *      |      ------      |                                                |
     *      |                  -------------------------------------------------|
     *      |                  ^                                                ^
     *      |                pixel0                                            pixel?
     */
    rVIDTCON1           = (0    << 24)                                  /*  VFPDE-----------------------*/
                                                                        /*  0-255 ֡�ھ���ͬ������ǰ,�� */
                                                                        /*  ������Ч��,(ֻ���YUV�˿�)  */

                        | (HBP   << 16)                                 /*  HBPD------------------------*/
                                                                        /*  0-255  ������ͬ�������½��� */
                                                                        /*  ��,���׸���Ч����ǰ,�ж���  */
                                                                        /*  ��Ч����                    */

                        | (HFP   << 8)                                  /*  HFPD------------------------*/
                                                                        /*  0-255�н�����,����ͬ��ʱ����*/
                                                                        /*  ����ǰ�ж��ٸ���Ч����      */

                        | (HSW   << 0);                                 /*  HSPW------------------------*/
                                                                        /*  0-255 ��ͬ���ж�����Чʱ��  */

    rVIDTCON2           = ((U_LCD_YSIZE - 1)  << 11)                    /*  0~2048  ��Ч����            */
                        | ((U_LCD_XSIZE - 1)  << 0);                    /*  0~2048  ��Ч����            */


    rWINCON0            = (0    << 22)                                  /*  0   -   ר��DMA�������ݷ��� */
                                                                        /*  1   *   �������            */

                        | (0    << 20)                                  /*  0~1     �����ĸ�������      */

                        | (0    << 19)                                  /*  0   -   ˫����̶�----------*/
                                                                        /*  0   -   ͨ��H/W�ź��Զ��ı� */

                        | (0    << 18)                                  /*  0   -   λ������ֹ----------*/
                                                                        /*  0   -   λ����ʹ��          */

                        | (0    << 17)                                  /*  0   -   �ֽڽ�����ֹ--------*/
                                                                        /*  1   -   �ֽڽ���ʹ��        */

                        | (1    << 16)                                  /*  0   -   ���ֽ�����ֹ--------*/
                                                                        /*  1   -   ���ֽ���ʹ��        */

                        | (0    << 13)                                  /*  0   -   RGB��ɫ�ռ�---------*/
                                                                        /*  1   -   YCbCr��ɫ�ռ�       */

                        | (0    << 9)                                   /*  0   -   ⧷�����16��--------*/
                                                                        /*  1   -   ⧷�����8��         */
                                                                        /*  2   -   ⧷�����4��         */

                        | (5    << 2)                                   /*  0000-   1BPP----------------*/
                                                                        /*  0001-   2BPP                */
                                                                        /*  0010-   4BPP                */
                                                                        /*  0011-   8BPP  ��ɫ��        */
                                                                        /*  0101-   16BPP 565           */
                                                                        /*  0111-   16BPP 555           */
                                                                        /*  1000-   16BPP �����        */
                                                                        /*  1011-   24BPP �����        */

                        | (0    << 0);                                  /*  0   -   ��ֹ����0���       */
                                                                        /*  1   -   ʹ�ܴ���0���-------*/


    rVIDOSD0A           = (0    << 11)                                  /*  �������Ͻ�X����             */

                        | (0    << 0);                                  /*  �������Ͻ�Y����             */


    rVIDOSD0B           = ((U_LCD_XSIZE - 1)    << 11)                   /*  �������½�X����             */
                        | ((U_LCD_YSIZE - 1)    << 0);                   /*  �������½�Y����             */


    rVIDOSD0C           = (U_LCD_XSIZE * U_LCD_YSIZE);


    rDITHMODE           = (0    << 5)                                   /*  ��ɫ����λ����--------------*/
                                                                        /*  00  -   8λ����             */
                                                                        /*  01  -   6λ����             */
                                                                        /*  10  -   5λ����             */

                        | (0    << 5)                                   /*  ��ɫ����λ����--------------*/
                                                                        /*  00  -   8λ����             */
                                                                        /*  01  -   6λ����             */
                                                                        /*  10  -   5λ����             */

                        | (0    << 5)                                   /*  ��ɫ����λ����--------------*/
                                                                        /*  00  -   8λ����             */
                                                                        /*  01  -   6λ����             */
                                                                        /*  10  -   5λ����             */

                        | (0    << 0);                                  /*  0   -   ������ֹ            */
                                                                        /*  1   -   ����ʹ��------------*/

    /*
     * Setup Frame buffer address in s3c6410 LCD controller
     */
    rVIDW00ADD0B0       = (FIRST_FB_ADDR);                              /* Frame buffer start address   */
    rVIDW00ADD1B0       = (FIRST_FB_ADDR & 0xffffff) + (V_SCR_XSIZE * U_LCD_YSIZE * BYTE_PER_PIXEL);
                                                                        /* Frame buffer end address     */

    rVIDW00ADD0B1       = (0);                                          /* Frame buffer start address   */
    rVIDW00ADD1B1       = (V_SCR_XSIZE * U_LCD_YSIZE * BYTE_PER_PIXEL); /* Frame buffer end address     */

    rVIDW00ADD2         = (((V_SCR_XSIZE - U_LCD_XSIZE) * BYTE_PER_PIXEL) << 13) |
                          (U_LCD_XSIZE * BYTE_PER_PIXEL) << 0;          /* Virtual screen controls    */


    rVIDINTCON0         = (0    << 0);                                  /*  0   -  ��ֹLCD�����������ж�*/


    rVIDINTCON1         = (0    << 0);                                  /*  0   -  ��ֹLCD�����������ж�*/


    /*
     *  �򿪴���0�����LCD���������
     */
    rWINCON0           |= (1    << 0);                                  /*  0   -   ��ֹ����0���       */
                                                                        /*  1   -   ʹ�ܴ���0���----- -*/


    rVIDCON0           |= (1    << 1)                                   /*  0   -   �������Ƶ�ź�------*/
                                                                        /*  1   -   �����Ƶ�ź�        */

                        | (1    << 0);                                  /*  0   -   ֡β��Ƶ�źű�ʹ��--*/
                                                                        /*  1   -   ֡β��Ƶ�źű���ֹ  */

    lcd_clear (0x001f); /* set lcd screen as blue */

    lcd_enable();

    _G_show_addr = (uint16 *)FIRST_FB_ADDR;
}

/*==============================================================================
 * - clear_16bit()
 *
 * - �ӵ� row �е� col �е����ؿ�ʼ������ pixels ������Ϊ <color> ��ɫ
 *
 * - <pixels> ����Ϊ 16 ��������
 */
void lcd_set_color (int row, int col, uint16 color, int pixels)
{
    int  i;
    int     stride;
    uint32 *pu32  = NULL;
    uint16 *pu16  = (uint16 *)(FIRST_FB_ADDR + (row * V_SCR_XSIZE + col) * BYTE_PER_PIXEL);
    uint32  color32 = color | (color << 16);

    if (pixels && (col & 0x1)) {
        *pu16++ = color;
        pixels--;
    }
    stride = pixels >> 4;
    pu32 = (uint32 *)pu16;

    for (i = 0; i < stride; i++) { /* one loop  32 bytes, 16 pixel */
        *pu32++ = color32; *pu32++ = color32; *pu32++ = color32; *pu32++ = color32;
        *pu32++ = color32; *pu32++ = color32; *pu32++ = color32; *pu32++ = color32;
    }

    pixels &= 0xF;
    pu16 = (uint16 *)pu32;
    while (pixels--) {
        *pu16++ = color;
    }
}

/*==============================================================================
 * - lcd_set_block()
 *
 * - fill frame buffer with user support color array
 */
void lcd_set_block (int row, int col, const uint16 *data, int width)
{
    void *start  = (void *)(FIRST_FB_ADDR + (row * V_SCR_XSIZE + col) * BYTE_PER_PIXEL);

    memcpy (start, data, width * BYTE_PER_PIXEL);
}

/*==============================================================================
 * - lcd_clear()
 *
 * - set all frame buffer as black color
 */
void lcd_clear (uint16 color)
{
    lcd_set_color(0, 0, 0x0000, V_SCR_XSIZE * V_SCR_YSIZE);
}

/*==============================================================================
 * - lcd_reverse_pixel()
 *
 * - reverse one pixel color
 */
void lcd_reverse_pixel (int row, int col)
{
    uint16 *pixel = (uint16 *)FIRST_FB_ADDR;
    pixel += row * V_SCR_XSIZE + col;

    *pixel = ~(*pixel);
}

/*==============================================================================
 * - lcd_set_pixel()
 *
 * - set one pixel to <color>
 */
void lcd_set_pixel (int row, int col, uint16 color)
{
    uint16 *pixel = (uint16 *)FIRST_FB_ADDR;
    pixel += row * V_SCR_XSIZE + col;

    *pixel = color;
}

/*==============================================================================
 * - lcd_get_pixel()
 *
 * - get one pixel to <color>
 */
uint16 lcd_get_pixel (int row, int col)
{
    uint16 *pixel = (uint16 *)FIRST_FB_ADDR;
    pixel += row * V_SCR_XSIZE + col;

    return *pixel;
}

/*==============================================================================
 * - lcd_get_addr()
 *
 * - get pixel address in frame buffer
 */
uint16 *lcd_get_addr (int row, int col)
{
    uint16 *pixel = (uint16 *)FIRST_FB_ADDR;
    pixel += row * V_SCR_XSIZE + col;

    return pixel;
}

/*==============================================================================
 * - lcd_get_start_x()
 *
 * - get current show left up x coodinate
 */
int lcd_get_start_x ()
{
    return _G_show_addr - (uint16 *)FIRST_FB_ADDR;
}

/*==============================================================================
 * - lcd_get_show_fb()
 *
 * - get current show left up addrress
 */
void *lcd_get_show_fb ()
{
    return _G_show_addr;
}

/*==============================================================================
 * - lcd_set_show_fb_base_x()
 *
 * - make show screen base on column <x> immediately
 */
int lcd_set_show_fb_base_x (int x)
{
    if ((x < 0) || (x > U_LCD_XSIZE * 2)) {
        return -1;
    }

    _G_show_addr = lcd_get_addr(0, x);

    _lcd_update_fb_addr ();

    return 0;
}

/*==============================================================================
 * - lcd_set_show_fb()
 *
 * - show frame buffer left | middle | right
 */
void *lcd_set_show_fb (int index)
{
    if (index >= FRAME_BUFFER_NUM) {
        return NULL;
    }

    /* change left up address */
    _G_show_addr = _G_fb_addr[index];
    _lcd_update_fb_addr ();

    return _G_show_addr;
}

/*==============================================================================
 * - lcd_fb_move()
 *
 * - if <offset_x> > 0 move fb start addr to left, or right
 */
void lcd_fb_move (int offset_x, int offset_y)
{
#define _MOVE_DELAY         5000
    if (offset_x > 0) {
        while ((offset_x--) && (_G_show_addr > (uint16 *)FIRST_FB_ADDR)) {
            _G_show_addr--;

            _lcd_update_fb_addr ();
            lcd_delay (_MOVE_DELAY);
        }
    } else if (offset_x < 0) {
        while ((offset_x++) && (_G_show_addr < (uint16 *)THIRD_FB_ADDR)) {
            _G_show_addr++;

            _lcd_update_fb_addr ();
            lcd_delay (_MOVE_DELAY);
        }
    }
#undef _MOVE_DELAY
}

/*==============================================================================
 * - lcd_delay()
 *
 * - delay
 */
void lcd_delay (volatile int n)
{
    while (n--) {
        ;
    }
}

/*==============================================================================
 * - _lcd_update_fb_addr()
 *
 * - set the frame buffer left up to <_G_show_addr>
 */
static void _lcd_update_fb_addr ()
{
    /*
     * Setup Frame buffer address in s3c6410 LCD controller
     */
    rVIDW00ADD0B0       = (unsigned int)(_G_show_addr);     /* Frame buffer start address   */
    rVIDW00ADD1B0       = ((unsigned int)(_G_show_addr) & 0xffffff)
                          + (V_SCR_XSIZE * U_LCD_YSIZE * BYTE_PER_PIXEL);
                                                            /* Frame buffer end address     */
}

/**********************************************************************************************************
  END FILE
**********************************************************************************************************/

