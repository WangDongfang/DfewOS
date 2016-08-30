/****************************************Copyright (c)*****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**                                      
**                                 http://www.embedtools.com
**
**--------------File Info----------------------------------------------------------------------------------
** File name:           s3c6410x_lcd.h
** Last modified Date:  2011-06-13
** Last Version:        1.0
** Descriptions:        the lcd controller driver's head
**
**---------------------------------------------------------------------------------------------------------
** Created by:          WangDongfang
** Created date:        2011-06-13
** Version:             1.0
** Descriptions:        the lcd controller driver's head
**
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/

#ifndef __s3c2440LCD_H_
#define __s3c2440LCD_H_

/**********************************************************************************************************
  LCD Panel
**********************************************************************************************************/
#define     PANEL_INNOLUX_AT080TN53

/**********************************************************************************************************
  Register Config Value
**********************************************************************************************************/
#ifdef  PANEL_INNOLUX_AT080TN53

#define U_LCD_XSIZE     800                                             /*  LCD x size                   */
#define U_LCD_YSIZE     600                                             /*  LCD y size                   */

#define HBP             230                                             /*  Horizontal back porch        */
#define HFP             22                                              /*  Horizontal front porch       */
#define HSW             62                                              /*  Horizontal pulse width       */

#define VBP             29                                              /*  Vertical back porch          */
#define VFP             3                                               /*  Vertical front porch         */
#define VSW             5                                               /*  Vertical pulse width         */
        
#elif defined(LCD_XXX)

#endif                                                                  /*  PANEL_INNOLUX_AT056TN53      */

#define CLK_VAL         2
#define HS_POL          0
#define VS_POL          0

/**********************************************************************************************************
  LCD Size
**********************************************************************************************************/
#define FRAME_BUFFER_NUM  3  /* have 3 frame buffer left -- middle -- right */

#define V_SCR_XSIZE    (U_LCD_XSIZE * FRAME_BUFFER_NUM)
#define V_SCR_YSIZE     U_LCD_YSIZE
#define BYTE_PER_PIXEL  2
#define V_SCR_STRIDE   (V_SCR_XSIZE * BYTE_PER_PIXEL)

#define FIRST_FB_ADDR  CONFIG_FRAME_BUFFER_START
#define SECND_FB_ADDR (FIRST_FB_ADDR + U_LCD_XSIZE * BYTE_PER_PIXEL)
#define THIRD_FB_ADDR (SECND_FB_ADDR + U_LCD_XSIZE * BYTE_PER_PIXEL)

/*********************************************************************************************************
  LCD¿ØÖÆÆ÷Ïà¹ØÌØÊâ¼Ä´æÆ÷¶¨Òå
*********************************************************************************************************/
#define rSPCON          (*(volatile unsigned int *)0x7F0081A0)          /*  ÌØÊâ¶Ë¿ÚÅäÖÃ¼Ä´æÆ÷          */
#define rMIFPCON        (*(volatile unsigned int *)0x7410800C)          /*  MODEM ½Ó¿Ú¿ØÖÆ¼Ä´æÆ÷        */

#define rGPICON         (*(volatile unsigned int *)0x7f008100)          /*  ÅäÖÃ¼Ä´æÆ÷                  */
#define rGPIPUD         (*(volatile unsigned int *)0x7f008108)          /*  ÉÏÀ­ÅäÖÃ¼Ä´æÆ÷              */
#define rGPJCON         (*(volatile unsigned int *)0x7f008120)          /*  ÅäÖÃ¼Ä´æÆ÷                  */
#define rGPJPUD         (*(volatile unsigned int *)0x7f008108)          /*  ÉÏÀ­ÅäÖÃ¼Ä´æÆ÷              */

/*********************************************************************************************************
  LCD¿ØÖÆÆ÷¼Ä´æÆ÷¶¨Òå
*********************************************************************************************************/
#define rVIDCON0        (*(volatile unsigned int *)(0x77100000 + 0x000))/*  LCDÅäÖÃ¼Ä´æÆ÷0              */
#define rVIDCON1        (*(volatile unsigned int *)(0x77100000 + 0x004))/*  LCDÅäÖÃ¼Ä´æÆ÷1              */
#define rVIDCON2        (*(volatile unsigned int *)(0x77100000 + 0x008))/*  LCDÅäÖÃ¼Ä´æÆ÷2              */

/*********************************************************************************************************
  Video Time Control
*********************************************************************************************************/
#define rVIDTCON0       (*(volatile unsigned int *)(0x77100000 + 0x010))/*  LCDÊ±Ðò¿ØÖÆ¼Ä´æÆ÷0          */
#define rVIDTCON1       (*(volatile unsigned int *)(0x77100000 + 0x014))/*  LCDÊ±Ðò¿ØÖÆ¼Ä´æÆ÷1          */
#define rVIDTCON2       (*(volatile unsigned int *)(0x77100000 + 0x018))/*  LCDÊ±Ðò¿ØÖÆ¼Ä´æÆ÷2          */
#define rVIDTCON3       (*(volatile unsigned int *)(0x77100000 + 0x01c))/*  LCDÊ±Ðò¿ØÖÆ¼Ä´æÆ÷3          */

/*********************************************************************************************************
  Window Control
*********************************************************************************************************/
#define rWINCON0        (*(volatile unsigned int *)(0x77100000 + 0x020))
#define rWINCON1        (*(volatile unsigned int *)(0x77100000 + 0x024))
#define rWINCON2        (*(volatile unsigned int *)(0x77100000 + 0x028))
#define rWINCON3        (*(volatile unsigned int *)(0x77100000 + 0x02C))
#define rWINCON4        (*(volatile unsigned int *)(0x77100000 + 0x030))

/*********************************************************************************************************
  Video Window Position Control
*********************************************************************************************************/
#define rVIDOSD0A       (*(volatile unsigned int *)(0x77100000 + 0x040))
#define rVIDOSD0B       (*(volatile unsigned int *)(0x77100000 + 0x044))
#define rVIDOSD0C       (*(volatile unsigned int *)(0x77100000 + 0x048))
#define rVIDOSD1A       (*(volatile unsigned int *)(0x77100000 + 0x050))
#define rVIDOSD1B       (*(volatile unsigned int *)(0x77100000 + 0x054))
#define rVIDOSD1C       (*(volatile unsigned int *)(0x77100000 + 0x058))
#define rVIDOSD1D       (*(volatile unsigned int *)(0x77100000 + 0x05C))
#define rVIDOSD2A       (*(volatile unsigned int *)(0x77100000 + 0x060))
#define rVIDOSD2B       (*(volatile unsigned int *)(0x77100000 + 0x064))
#define rVIDOSD2C       (*(volatile unsigned int *)(0x77100000 + 0x068))
#define rVIDOSD2D       (*(volatile unsigned int *)(0x77100000 + 0x06c))
#define rVIDOSD3A       (*(volatile unsigned int *)(0x77100000 + 0x070))
#define rVIDOSD3B       (*(volatile unsigned int *)(0x77100000 + 0x074))
#define rVIDOSD3C       (*(volatile unsigned int *)(0x77100000 + 0x078))
#define rVIDOSD4A       (*(volatile unsigned int *)(0x77100000 + 0x080))
#define rVIDOSD4B       (*(volatile unsigned int *)(0x77100000 + 0x084))
#define rVIDOSD4C       (*(volatile unsigned int *)(0x77100000 + 0x088))

/*********************************************************************************************************
  Window Buffer Start Address
*********************************************************************************************************/
#define rVIDW00ADD0B0   (*(volatile unsigned int *)(0x77100000 + 0x0A0))
#define rVIDW00ADD0B1   (*(volatile unsigned int *)(0x77100000 + 0x0A4))
#define rVIDW01ADD0B0   (*(volatile unsigned int *)(0x77100000 + 0x0A8))
#define rVIDW01ADD0B1   (*(volatile unsigned int *)(0x77100000 + 0x0AC))
#define rVIDW02ADD0     (*(volatile unsigned int *)(0x77100000 + 0x0B0))
#define rVIDW03ADD0     (*(volatile unsigned int *)(0x77100000 + 0x0B8))
#define rVIDW04ADD0     (*(volatile unsigned int *)(0x77100000 + 0x0C0))

/*********************************************************************************************************
  Window Buffer End Address
*********************************************************************************************************/
#define rVIDW00ADD1B0   (*(volatile unsigned int *)(0x77100000 + 0x0D0))
#define rVIDW00ADD1B1   (*(volatile unsigned int *)(0x77100000 + 0x0D4))
#define rVIDW01ADD1B0   (*(volatile unsigned int *)(0x77100000 + 0x0D8))
#define rVIDW01ADD1B1   (*(volatile unsigned int *)(0x77100000 + 0x0DC))
#define rVIDW02ADD1     (*(volatile unsigned int *)(0x77100000 + 0x0E0))
#define rVIDW03ADD1     (*(volatile unsigned int *)(0x77100000 + 0x0E8))
#define rVIDW04ADD1     (*(volatile unsigned int *)(0x77100000 + 0x0F0))

/*********************************************************************************************************
  Window Buffer Size
*********************************************************************************************************/
#define rVIDW00ADD2     (*(volatile unsigned int *)(0x77100000 + 0x100))
#define rVIDW01ADD2     (*(volatile unsigned int *)(0x77100000 + 0x104))
#define rVIDW02ADD2     (*(volatile unsigned int *)(0x77100000 + 0x108))
#define rVIDW03ADD2     (*(volatile unsigned int *)(0x77100000 + 0x10C))
#define rVIDW04ADD2     (*(volatile unsigned int *)(0x77100000 + 0x110))

/*********************************************************************************************************
  Indicate the Video Interrupt Control
*********************************************************************************************************/
#define rVIDINTCON0     (*(volatile unsigned int *)(0x77100000 + 0x130))

/*********************************************************************************************************
  Video Interrupt Pending
*********************************************************************************************************/
#define rVIDINTCON1     (*(volatile unsigned int *)(0x77100000 + 0x134))

/*********************************************************************************************************
  Color Key Control/Value
*********************************************************************************************************/
#define rW1KEYCON0      (*(volatile unsigned int *)(0x77100000 + 0x140))
#define rW1KEYCON1      (*(volatile unsigned int *)(0x77100000 + 0x144))
#define rW2KEYCON0      (*(volatile unsigned int *)(0x77100000 + 0x148))
#define rW2KEYCON1      (*(volatile unsigned int *)(0x77100000 + 0x14C))
#define rW3KEYCON0      (*(volatile unsigned int *)(0x77100000 + 0x150))
#define rW3KEYCON1      (*(volatile unsigned int *)(0x77100000 + 0x154))
#define rW4KEYCON0      (*(volatile unsigned int *)(0x77100000 + 0x158))
#define rW4KEYCON1      (*(volatile unsigned int *)(0x77100000 + 0x15C))

/*********************************************************************************************************
  DithMode
*********************************************************************************************************/
#define rDITHMODE       (*(volatile unsigned int *)(0x77100000 + 0x170))

/*********************************************************************************************************
  Window Control
*********************************************************************************************************/
#define rWIN0MAP        (*(volatile unsigned int *)(0x77100000 + 0x180))
#define rWIN1MAP        (*(volatile unsigned int *)(0x77100000 + 0x184))
#define rWIN2MAP        (*(volatile unsigned int *)(0x77100000 + 0x188))
#define rWIN3MAP        (*(volatile unsigned int *)(0x77100000 + 0x18C))
#define rWIN4MAP        (*(volatile unsigned int *)(0x77100000 + 0x190))

/*********************************************************************************************************
    Window Palette Control
*********************************************************************************************************/
#define rWPALCON        (*(volatile unsigned int *)(0x77100000 + 0x1A0))

/*********************************************************************************************************
    I80/RGB Trigger Control
*********************************************************************************************************/
#define rTRIGCON        (*(volatile unsigned int *)(0x77100000 + 0x1A4))
#define rITUIFCON0      (*(volatile unsigned int *)(0x77100000 + 0x1A8))


/*********************************************************************************************************
    I80 Interface Control for Main/Sub LDI
*********************************************************************************************************/
#define rI80IFCONA0     (*(volatile unsigned int *)(0x77100000 + 0x1B0))
#define rI80IFCONA1     (*(volatile unsigned int *)(0x77100000 + 0x1B4))
#define rI80IFCONB0     (*(volatile unsigned int *)(0x77100000 + 0x1B8))
#define rI80IFCONB1     (*(volatile unsigned int *)(0x77100000 + 0x1BC))

/*********************************************************************************************************
    I80 Interface LDI Command Cotrol
*********************************************************************************************************/
#define rLDI_CMDCON0    (*(volatile unsigned int *)(0x77100000 + 0x1D0))
#define rLDI_CMDCON1    (*(volatile unsigned int *)(0x77100000 + 0x1D4))

/*********************************************************************************************************
    I80 Interface System Command Control
*********************************************************************************************************/
#define rSIFCCON0       (*(volatile unsigned int *)(0x77100000 + 0x1E0))
#define rSIFCCON1       (*(volatile unsigned int *)(0x77100000 + 0x1E4))
#define rSIFCCON2       (*(volatile unsigned int *)(0x77100000 + 0x1E8))

/*********************************************************************************************************
    I80 Interface LDI Command
*********************************************************************************************************/
#define rLDI_CMD0       (*(volatile unsigned int *)(0x77100000 + 0x280))
#define rLDI_CMD1       (*(volatile unsigned int *)(0x77100000 + 0x284))
#define rLDI_CMD2       (*(volatile unsigned int *)(0x77100000 + 0x288))
#define rLDI_CMD3       (*(volatile unsigned int *)(0x77100000 + 0x28C))
#define rLDI_CMD4       (*(volatile unsigned int *)(0x77100000 + 0x290))
#define rLDI_CMD5       (*(volatile unsigned int *)(0x77100000 + 0x294))
#define rLDI_CMD6       (*(volatile unsigned int *)(0x77100000 + 0x298))
#define rLDI_CMD7       (*(volatile unsigned int *)(0x77100000 + 0x29C))
#define rLDI_CMD8       (*(volatile unsigned int *)(0x77100000 + 0x2A0))
#define rLDI_CMD9       (*(volatile unsigned int *)(0x77100000 + 0x2A4))
#define rLDI_CMD10      (*(volatile unsigned int *)(0x77100000 + 0x2A8))
#define rLDI_CMD11      (*(volatile unsigned int *)(0x77100000 + 0x2AC))

/*********************************************************************************************************
    Window Palette Data
*********************************************************************************************************/
#define rW2PDATA01      (*(volatile unsigned int *)(0x77100000 + 0x300))
#define rW2PDATA23      (*(volatile unsigned int *)(0x77100000 + 0x304))
#define rW2PDATA45      (*(volatile unsigned int *)(0x77100000 + 0x308))
#define rW2PDATA67      (*(volatile unsigned int *)(0x77100000 + 0x30C))
#define rW2PDATA89      (*(volatile unsigned int *)(0x77100000 + 0x310))
#define rW2PDATAAB      (*(volatile unsigned int *)(0x77100000 + 0x314))
#define rW2PDATACD      (*(volatile unsigned int *)(0x77100000 + 0x318))
#define rW2PDATAEF      (*(volatile unsigned int *)(0x77100000 + 0x31C))
#define rW3PDATA01      (*(volatile unsigned int *)(0x77100000 + 0x320))
#define rW3PDATA23      (*(volatile unsigned int *)(0x77100000 + 0x324))
#define rW3PDATA45      (*(volatile unsigned int *)(0x77100000 + 0x328))
#define rW3PDATA67      (*(volatile unsigned int *)(0x77100000 + 0x32C))
#define rW3PDATA89      (*(volatile unsigned int *)(0x77100000 + 0x330))
#define rW3PDATAAB      (*(volatile unsigned int *)(0x77100000 + 0x334))
#define rW3PDATACD      (*(volatile unsigned int *)(0x77100000 + 0x338))
#define rW3PDATAEF      (*(volatile unsigned int *)(0x77100000 + 0x33C))
#define rW4PDATA01      (*(volatile unsigned int *)(0x77100000 + 0x340))
#define rW4PDATA23      (*(volatile unsigned int *)(0x77100000 + 0x344))


/*======================================================================
  lcd driver support APIs for upper layer
======================================================================*/
void lcd_enable(void);
void lcd_init();
void lcd_set_color (int row, int low, uint16 color, int pixels);
void lcd_set_block (int row, int col, const uint16 *data, int width);
void lcd_clear (uint16 color);
void lcd_reverse_pixel (int row, int col);
void lcd_set_pixel (int row, int col, uint16 color);
uint16 lcd_get_pixel (int row, int col);
uint16 *lcd_get_addr (int row, int col);
int  lcd_get_start_x ();
void *lcd_get_show_fb ();
int lcd_set_show_fb_base_x (int x);
void *lcd_set_show_fb (int index);
void lcd_fb_move (int offset_x, int offset_y);
void lcd_delay (volatile int n);

#endif                                                                  /*  __s3c2440Lcd_H_              */

/**********************************************************************************************************
  END FILE
**********************************************************************************************************/

