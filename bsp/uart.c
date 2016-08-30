/*==============================================================================
** uart.c -- s3c6410x uart driver.
**
** MODIFY HISTORY:
**
** 2012-03-02 wdf Create.
==============================================================================*/
#include "bsp_config.h"
#include "types.h"
#include "dlist.h"
#include "task.h"
#include "semB.h"

#include "int.h"

/*======================================================================
  UART config
======================================================================*/
#define USE_COM0_AS_TTY                   /* COM0 as TTY              */
#define TX_POLLING_MODE                   /* Transmit in Polling mode */

/*********************************************************************************************************
  UART clock and gpio config bits
*********************************************************************************************************/
#define __COM0_CLKBIT       (1 << 1)                                    /*  COM0 �� PCLK_GATE �е�λ��  */
#define __COM1_CLKBIT       (1 << 2)                                    /*  COM1 �� PCLK_GATE �е�λ��  */
#define __COM2_CLKBIT       (1 << 3)                                    /*  COM2 �� PCLK_GATE �е�λ��  */
#define __COM3_CLKBIT       (1 << 4)                                    /*  COM3 �� PCLK_GATE �е�λ��  */

#define __COM0_GPIO         ((0x3 << 0) | (0x3 << 2))                   /*  COM0 �� IO �����е�λ��     */
#define __COM1_GPIO         ((0x3 << 8) | (0x3 << 10))                  /*  COM1 �� IO �����е�λ��     */
#define __COM2_GPIO         ((0x3 << 0) | (0x3 << 2))                   /*  COM2 �� IO �����е�λ��     */
#define __COM3_GPIO         ((0x3 << 4) | (0x3 << 6))                   /*  COM3 �� IO �����е�λ��     */

#define __COM0_GPACON       ((0x2 <<  0) | (0x2 <<  4))                 /*  COM0 �� GPACON �е�����     */
#define __COM1_GPACON       ((0x2 << 16) | (0x2 << 20))                 /*  COM1 �� GPACON �е�����     */
#define __COM2_GPBCON       ((0x2 <<  0) | (0x2 <<  4))                 /*  COM2 �� GPBCON �е�����     */
#define __COM3_GPBCON       ((0x2 <<  8) | (0x2 << 12))                 /*  COM3 �� GPBCON �е�����     */

#define __COM0_MASK         ((0xF <<  0) | (0xF <<  4))                 /*  COM0 �� GPACON �е�����     */
#define __COM1_MASK         ((0xF << 16) | (0xF << 20))                 /*  COM1 �� GPACON �е�����     */
#define __COM2_MASK         ((0xF <<  0) | (0xF <<  4))                 /*  COM2 �� GPBCON �е�����     */
#define __COM3_MASK         ((0xF <<  8) | (0xF << 12))                 /*  COM3 �� GPBCON �е�����     */

/*********************************************************************************************************
  PLL & CLOCK configuration registers
*********************************************************************************************************/
#define PLL_BASE                0x7E00F000
#define PCLK_GATE_OFFSET        0x00000034                              /* PCLK clock gating            */

/*********************************************************************************************************
  GPIO configuration registers
*********************************************************************************************************/
#define rGPACON         (*(volatile unsigned int *)0x7F008000)
#define rGPADAT         (*(volatile unsigned int *)0x7F008004)
#define rGPAPUD         (*(volatile unsigned int *)0x7F008008)
#define rGPACONSLP      (*(volatile unsigned int *)0x7F00800C)
#define rGPAPUDSLP      (*(volatile unsigned int *)0x7F008010)

/*********************************************************************************************************
  UART Registers
*********************************************************************************************************/
#ifdef USE_COM0_AS_TTY
#define rULCON             (*(volatile unsigned *)0x7F005000)          /*  UART 0 Line control         */
#define rUCON              (*(volatile unsigned *)0x7F005004)          /*  UART 0 Control              */
#define rUFCON             (*(volatile unsigned *)0x7F005008)          /*  UART 0 FIFO control         */
#define rUMCON             (*(volatile unsigned *)0x7F00500c)          /*  UART 0 Modem control        */
#define rUTRSTAT           (*(volatile unsigned *)0x7F005010)          /*  UART 0 Tx/Rx status         */
#define rUERSTAT           (*(volatile unsigned *)0x7F005014)          /*  UART 0 Rx error status      */
#define rUFSTAT            (*(volatile unsigned *)0x7F005018)          /*  UART 0 FIFO status          */
#define rUMSTAT            (*(volatile unsigned *)0x7F00501c)          /*  UART 0 Modem status         */
#define rUBRDIV            (*(volatile unsigned *)0x7F005028)          /*  UART 0 Baud rate divisor    */
#define rUDIVSLOT          (*(volatile unsigned *)0x7F00502c)          /*  UART 0 Dividing slot reg    */
#define rUINTP             (*(volatile unsigned *)0x7F005030)          /*  UART 0 Interrupt Pending Reg*/
#define rUINTSP            (*(volatile unsigned *)0x7F005034)   /*  UART 0 Interrupt source Pending Reg*/
#define rUINTM             (*(volatile unsigned *)0x7F005038)          /*  UART 0 Interrupt Mask Reg   */
#define WrUTXH(ch)         (*(volatile unsigned char *)0x7F005020) = (unsigned char)(ch)
#define RdURXH()           (*(volatile unsigned char *)0x7F005024)
#endif /* USE_COM0_AS_TTY */

/*********************************************************************************************************
  UART sub interrupt bits
*********************************************************************************************************/
#define UINTP_RXD_BIT       (1 << 0)
#define UINTP_ERROR_BIT     (1 << 1)
#define UINTP_TXD_BIT       (1 << 2)
#define UINTP_MODEM_BIT     (1 << 3)

/*======================================================================
  enable & disable transmit intterupt macro
======================================================================*/
#define DISABLE_TX_INT()    rUINTM |= UINTP_TXD_BIT;

/*======================================================================
  Global variables used in this file
======================================================================*/
static SEM_BIN *_G_p_tx_semB = NULL;
static SEM_BIN *_G_p_rx_semB = NULL;

/*********************************************************************************************************
 UARTʱ��Ƶ��΢��ֵ
*********************************************************************************************************/
static unsigned int _G_uiUDIVSLOT_Tab[16] = { 0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x4924, 0x4A52,
                              0x54AA, 0x5555, 0xD555, 0xD5D5, 0xDDD5, 0xDDDD, 0xDFDD, 0xDFDF, 0xFFDF};

/*======================================================================
  Function Forward Declare
======================================================================*/
static void _uart_isr (void *unused);

/*==============================================================================
 * - uart_init()
 *
 * - init UART hardware
 */
int uart_init()
{
    unsigned int iBaud = 115200;
    unsigned int uiUDIVSLOTn;                                   /*  ������΢��ֵ                */
    unsigned int band_reg =(int)(PCLK / (16 * iBaud) - 0.5);

    rGPACON    &=  ~(__COM0_MASK);                              /*  GPA0 GPA1                   */
    rGPACON    |=  __COM0_GPACON;
    rGPAPUD     =  (rGPAPUD & ~(__COM0_GPIO)) | 0xa;            /*  ʹ����������                */
    rGPACONSLP |=  __COM0_GPIO;                                 /*  ˯��ģʽ����״̬            */
    rGPAPUDSLP  =  (rGPAPUDSLP & ~(__COM0_GPIO)) | 0xa;         /*  ˯��ģʽʹ����������        */

    /*
     * UARTʱ�����ú͹ҽ� * ʹ��PCLK 66.5Mhz
     */
    *(unsigned *)(PLL_BASE + PCLK_GATE_OFFSET)  |=  __COM0_CLKBIT;


    rUCON  = 0;

    rULCON =  (0 << 6) |                            /*  ��ʹ�ú���                   */
              (0 << 3) |                            /*  ��У��λ                     */
              (0 << 2) |                            /*  1 λֹͣλ                   */
              (3 << 0);                             /*  8 λ����λ                   */
    rUMCON =  0;
    rUFCON =  (3 << 6) |                            /*  Tx FIFO 48-byte              */
              (3 << 4) |                            /*  Rx FIFO 32-byte              */
              (1 << 2) |                            /*  Tx FIFO reset                */
              (1 << 1) |                            /*  Rx FIFO reset                */
              (1 << 0);                             /*  FIFO ʹ��                    */

    rUCON  =  (0 << 10) |                           /*  PCLK selected                */
              (0 << 9) |                            /*  Tx Interrupt Type PULSE      */
              (0 << 8) |                            /*  Rx Interrupt Type PULSE      */
              (1 << 7) |                            /*  Rx Time Out Enable           */
              (1 << 6) |

              (0 << 5) |
              (0 << 4) |

              (1 << 2) |                            /*  Transmit & Receive Mode :    */
              (1 << 0);                             /*  Interrupt request or polling */

    rUBRDIV = band_reg;

    /*
     * 6410���мĴ���
     * uiUDIVSLOTn ��Ҫ���С�����ֵ�ֵ����16����С������
     * ���Ƽ���΢��ֵ
     */
    if(PCLK < 256000000) {                          /* PCLKһ�㲻����256Mh          */
        uiUDIVSLOTn    =    ((PCLK << 4) / (iBaud << 4) - 16) & 0xf;
    } else {
        uiUDIVSLOTn    =    0;
    }

    /*
     * ��������������uiUDIVSLOTnֵΪ������_G_uiUDIVSLOT_Tab��
     * �õ�ʵ�ʵ�UDIVSLOTֵ
     */
    rUDIVSLOT = _G_uiUDIVSLOT_Tab[uiUDIVSLOTn];

    /*
     * init transmit and receive semaphores
     */
    _G_p_tx_semB = semB_init(NULL);
    _G_p_rx_semB = semB_init(NULL);

    /*
     * enable uart sub interrupt
     */
    rUINTSP = (UINTP_RXD_BIT | UINTP_TXD_BIT);
    rUINTP = (UINTP_RXD_BIT | UINTP_TXD_BIT);          /*  ������ͽ����ж�         */
    rUINTM &=  ~(UINTP_RXD_BIT | UINTP_TXD_BIT);       /*  �򿪷��ͽ������ж�       */

#ifdef  TX_POLLING_MODE
    DISABLE_TX_INT();  /* when transmit, I use poll method, because int method lose data sometimes */
#endif

    /*
     * register uart isr and enable uart interrupt
     */
    int_connect (INT_NUMBER_UART0, _uart_isr, 0);
    int_enable (INT_NUMBER_UART0);

    return 0;
}

/*==============================================================================
 * - uart_isr()
 *
 * - UART interrupt service C routine, called by <uart_ist>
 */
static void _uart_isr(void *unused)
{
    /*
     * UART Transmit Interrupt
     */
    if (rUINTP & UINTP_TXD_BIT) {                  /*  �����ж�               */
        if ((rUFSTAT & (1 << 14)) == 0) {          /*  Tx FIFO have space     */
            semB_give (_G_p_tx_semB);
        }
        rUINTP = UINTP_TXD_BIT;                    /*  ��������ж�           */
    }

    /*
     * UART Receive Interrupt
     */
    if (rUINTP & UINTP_RXD_BIT) {                  /*  �����ж�               */
        if ((rUFSTAT & (0x3f)) != 0) {             /*  Rx FIFO have data      */
            semB_give (_G_p_rx_semB);
        }
        rUINTP = UINTP_RXD_BIT;                    /*  ��������ж�           */
    }

    /*
     * UART Error Interrupt
     */
    if (rUINTP & UINTP_ERROR_BIT) {                /*  �����ж�               */
        rUINTP = UINTP_ERROR_BIT;              /*  ��������ж�           */
    }
}

/*==============================================================================
 * - uart_putc()
 *
 * - use UART send a char
 */
int uart_putc (const char c)
{
#ifdef TX_POLLING_MODE
    extern int telnet_stor(const char);
    
    /* if telnet is opened, we don't print to tty */
    if (telnet_stor(c) == 0) { /* telnet shell isn't open */
        while (rUFSTAT & (1 << 14))
            /* wait for room in the tx FIFO */ ;
        WrUTXH(c);
    }
#else
    static int iTake = 0;
    if (iTake == 0) {
        semB_take (_G_p_tx_semB, NO_WAIT);
        iTake = 1;
    }
again:
    if ((rUFSTAT & (1 << 14)) == 0) {     /*  Tx FIFO have space           */
        WrUTXH(c);
    } else {
        semB_take (_G_p_tx_semB, WAIT_FOREVER);
        goto again;
    }
#endif

    /* If \n, also do \r */
    if (c == '\n')
        uart_putc('\r');

    return 1;
}

/*==============================================================================
 * - uart_getc()
 *
 * - use UART receive a char
 */
int uart_getc ()
{
    int c;
    static int iTake = 0;
    if (iTake == 0) {
        semB_take (_G_p_rx_semB, NO_WAIT);
        iTake = 1;
    }

again:
    if ((rUFSTAT & (0x3f)) != 0) {    /*  Rx FIFO have data            */
        c = RdURXH();
    } else {
        semB_take (_G_p_rx_semB, WAIT_FOREVER);
        goto again;
    }
    return c;
}

/*==============================================================================
 * - uart_tstc()
 *
 * - check whether have received char in FIFO
 */
int uart_tstc(void)
{
    return (rUFSTAT & (0x3f));
}

/*==============================================================================
 * - uart_pend()
 *
 * - wait some ticks for receive data
 */
int uart_pend (int ticks)
{
    if (semB_take (_G_p_rx_semB, ticks) == OS_STATUS_OK) {
        return 1;
    } else {
        return 0;
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

