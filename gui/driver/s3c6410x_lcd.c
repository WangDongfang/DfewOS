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
    rVIDCON0           |=  ((1    << 1)                                 /*  0   -   不输出视频信号------*/
                                                                        /*  1   -   输出视频信号        */
                        |  (1    << 0));                                /*  0   -   帧尾视频信号被使能--*/
                                                                        /*  1   -   帧尾视频信号被禁止  */
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
     *  配置端口为LCD控制器
     */
    rMIFPCON           &= ~(1   << 3);                                  /*  bypass 模式禁能             */
    rSPCON              = (rSPCON & ~(3UL)) | 1UL;                      /*  选择LCD控制器为RGB模式      */

    rGPICON             = 0xaaaaaaaa;                                   /*  使能引脚功能是LCD控制器     */
    rGPIPUD             = 0;                                            /*  禁止引脚上拉功能            */
    rGPJCON             = 0xaaaaaa;                                     /*  设置GPJ引脚功能为LCD控制器  */
    rGPJPUD             = 0;                                            /*  禁止引脚上拉功能            */




    rVIDCON0            = (0    << 29)                                  /*  0   -   渐进模式------------*/
                                                                        /*  1   -   交错模式            */

                        | (0    << 26)                                  /*  00  -   RGB 模式------------*/
                                                                        /*  01  -   TV模式              */
                                                                        /*  02  -   LDI0 180 IF         */
                                                                        /*  03  -   LDI1 180 IF         */

                        | (0    << 23)                                  /*  XXX -   LDI1 180 IF数据方式-*/

                        | (0    << 20)                                  /*  XXX -   LDI1 180 IF数据方式-*/

                        | (0    << 17)                                  /*  00  -   RGB并行 RGB---------*/
                                                                        /*  01  -   RGB并行 BGR         */
                                                                        /*  10  -   串行RGB R-G-B       */
                                                                        /*  10  -   串行RGB B-G-R       */

                        | (0    << 16)                                  /*  0   -   CLKVAL_F一直有效----*/
                                                                        /*  1-  -   帧开始时一帧只有一次*/

                        | (CLK_VAL << 6)                                /*  0-255   LCD时钟分频器-------*/
                                                                        /*          VCLK = clk/本数据+1)*/
                                                                        /*          VCLK最大不能超过66M */

                        | (0    << 5)                                   /*  0   -   常规模式/由ENVID控制*/
                                                                        /*  1   -   自由运行            */

                        | (1    << 4)                                   /*  0   -   直接使用选择时钟----*/
                                                                        /*  1   -   使用分频后的时钟    */

                        | (0    << 2)                                   /*  00  -   clk 时钟源 =HCLK--  */
                                                                        /*  01  -   clk 时钟源 =视频时钟*/
                                                                        /*  10  -   RESV                */
                                                                        /*  11  -   clk 时钟源=27M EXCLK*/

                        | (0    << 1)                                   /*  0   -   不输出视频信号------*/
                                                                        /*  1   -   输出视频信号        */

                        | (0    << 0);                                  /*  0   -   帧尾视频信号被使能--*/
                                                                        /*  1   -   帧尾视频信号被禁止  */


    rVIDCON1            = (0      << 7)                                 /*  0   -   VCLK下降沿有效------*/
                                                                        /*  1   -   VCLK上升沿有效      */

                        | (HS_POL << 6)                                 /*  0   -   Hsync正常有效-------*/
                                                                        /*  1   -   Hsync反向           */

                        | (VS_POL << 5)                                 /*  0   -   Vsync正常有效-------*/
                                                                        /*  1   -   Vsync反向           */

                        | (0      << 4)                                 /*  0   -   VDEN正常有效--------*/
                                                                        /*  1   -   VDEN反向有效        */

                        | (0    << 0);                                  /*  保留                        */


    rVIDCON2            = (0    << 23)                                  /*  0   -   ITU601无输出--------*/
                                                                        /*  1   -   ITU601被使能        */

                        | (0    << 14)                                  /*  0   -   YUV数据格式硬件选择-*/
                                                                        /*  1   -   YUV数据格式软件选择 */

                        | (0    << 12)                                  /*  00  -   YUV输出RGB格式------*/
                                                                        /*  01  -   YUV 422输出格式     */
                                                                        /*  1x  -   YUV 444输出格式     */

                        | (0    << 8)                                   /*  0   -   Y-cb-cr数据格式-----*/
                                                                        /*  1   -   cb-cr-Y数据格式     */

                        | (0    << 7)                                   /*  0   -   cb-cr数据格式-------*/
                                                                        /*  1   -   cr-cb数据格式       */

                        | (0    << 0);                                  /*  保留                        */


    /*
     *      |VFPD  |VSPW |VBPD |-------------有效行-----------------------------|
     *      |VFPDE |     |VBPDE|                                                |
     *   帧起始    |     |     |                                            帧结束
     *      V      V     V     V                                                V
     *      |------|     |------------------------------------------------------|
     *      |      |     |     ^                                                ^
     *      |      ------      |                                                |
     *      |                  -------------------------------------------------|
     *      |                  ^                                                ^
     *      |                line0                                            line?
     */
    rVIDTCON0           = (0            << 24)                          /*  VBPDE-----------------------*/
                                                                        /*  0-255  帧起始经过同步脉冲后 */
                                                                        /*  有多少无效行,(只针对YUV端口)*/

                        | (VBP   << 16)                                 /*  VBPD------------------------*/
                                                                        /*  0-255  帧起始经过同步脉冲后 */
                                                                        /*  有多少无效行                */

                        | (VFP   << 8)                                  /*  VFPD------------------------*/
                                                                        /*  0-255   -  帧结束后,在同步脉*/
                                                                        /*  冲前有多少个无效行          */

                        | (VSW   << 0);                                 /*  VSPW------------------------*/
                                                                        /*  0-255 帧同步脉冲有多少无效行*/


    /*
     *      |HFPD  |HSPW |HBPD |-------------有效像素---------------------------|
     *   行起始    |     |     |                                            行结束
     *      V      V     V     V                                                V
     *      |------|     |------------------------------------------------------|
     *      |      |     |     ^                                                ^
     *      |      ------      |                                                |
     *      |                  -------------------------------------------------|
     *      |                  ^                                                ^
     *      |                pixel0                                            pixel?
     */
    rVIDTCON1           = (0    << 24)                                  /*  VFPDE-----------------------*/
                                                                        /*  0-255 帧在经过同步脉冲前,有 */
                                                                        /*  多少无效行,(只针对YUV端口)  */

                        | (HBP   << 16)                                 /*  HBPD------------------------*/
                                                                        /*  0-255  经过行同步脉冲下降沿 */
                                                                        /*  后,在首个有效数据前,有多少  */
                                                                        /*  无效像素                    */

                        | (HFP   << 8)                                  /*  HFPD------------------------*/
                                                                        /*  0-255行结束后,在行同步时钟上*/
                                                                        /*  升沿前有多少个无效像素      */

                        | (HSW   << 0);                                 /*  HSPW------------------------*/
                                                                        /*  0-255 行同步有多少无效时钟  */

    rVIDTCON2           = ((U_LCD_YSIZE - 1)  << 11)                    /*  0~2048  有效行数            */
                        | ((U_LCD_XSIZE - 1)  << 0);                    /*  0~2048  有效列数            */


    rWINCON0            = (0    << 22)                                  /*  0   -   专用DMA进行数据访问 */
                                                                        /*  1   *   程序控制            */

                        | (0    << 20)                                  /*  0~1     采用哪个缓冲区      */

                        | (0    << 19)                                  /*  0   -   双缓冲固定----------*/
                                                                        /*  0   -   通过H/W信号自动改变 */

                        | (0    << 18)                                  /*  0   -   位交换禁止----------*/
                                                                        /*  0   -   位交换使能          */

                        | (0    << 17)                                  /*  0   -   字节交换禁止--------*/
                                                                        /*  1   -   字节交换使能        */

                        | (1    << 16)                                  /*  0   -   半字交换禁止--------*/
                                                                        /*  1   -   半字交换使能        */

                        | (0    << 13)                                  /*  0   -   RGB颜色空间---------*/
                                                                        /*  1   -   YCbCr颜色空间       */

                        | (0    << 9)                                   /*  0   -   猝发传输16字--------*/
                                                                        /*  1   -   猝发传输8字         */
                                                                        /*  2   -   猝发传输4字         */

                        | (5    << 2)                                   /*  0000-   1BPP----------------*/
                                                                        /*  0001-   2BPP                */
                                                                        /*  0010-   4BPP                */
                                                                        /*  0011-   8BPP  调色板        */
                                                                        /*  0101-   16BPP 565           */
                                                                        /*  0111-   16BPP 555           */
                                                                        /*  1000-   16BPP 无填充        */
                                                                        /*  1011-   24BPP 无填充        */

                        | (0    << 0);                                  /*  0   -   禁止窗口0输出       */
                                                                        /*  1   -   使能窗口0输出-------*/


    rVIDOSD0A           = (0    << 11)                                  /*  窗口左上角X坐标             */

                        | (0    << 0);                                  /*  窗口左上角Y坐标             */


    rVIDOSD0B           = ((U_LCD_XSIZE - 1)    << 11)                   /*  窗口右下角X坐标             */
                        | ((U_LCD_YSIZE - 1)    << 0);                   /*  窗口右下角Y坐标             */


    rVIDOSD0C           = (U_LCD_XSIZE * U_LCD_YSIZE);


    rDITHMODE           = (0    << 5)                                   /*  红色抖动位控制--------------*/
                                                                        /*  00  -   8位抖动             */
                                                                        /*  01  -   6位抖动             */
                                                                        /*  10  -   5位抖动             */

                        | (0    << 5)                                   /*  绿色抖动位控制--------------*/
                                                                        /*  00  -   8位抖动             */
                                                                        /*  01  -   6位抖动             */
                                                                        /*  10  -   5位抖动             */

                        | (0    << 5)                                   /*  蓝色抖动位控制--------------*/
                                                                        /*  00  -   8位抖动             */
                                                                        /*  01  -   6位抖动             */
                                                                        /*  10  -   5位抖动             */

                        | (0    << 0);                                  /*  0   -   抖动禁止            */
                                                                        /*  1   -   抖动使能------------*/

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


    rVIDINTCON0         = (0    << 0);                                  /*  0   -  禁止LCD控制器所有中断*/


    rVIDINTCON1         = (0    << 0);                                  /*  0   -  禁止LCD控制器所有中断*/


    /*
     *  打开窗口0输出、LCD控制器输出
     */
    rWINCON0           |= (1    << 0);                                  /*  0   -   禁止窗口0输出       */
                                                                        /*  1   -   使能窗口0输出----- -*/


    rVIDCON0           |= (1    << 1)                                   /*  0   -   不输出视频信号------*/
                                                                        /*  1   -   输出视频信号        */

                        | (1    << 0);                                  /*  0   -   帧尾视频信号被使能--*/
                                                                        /*  1   -   帧尾视频信号被禁止  */

    lcd_clear (0x001f); /* set lcd screen as blue */

    lcd_enable();

    _G_show_addr = (uint16 *)FIRST_FB_ADDR;
}

/*==============================================================================
 * - clear_16bit()
 *
 * - 从第 row 行第 col 列的像素开始，设置 pixels 个像素为 <color> 颜色
 *
 * - <pixels> 不必为 16 的整数倍
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

