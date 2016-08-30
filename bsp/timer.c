/*==============================================================================
** timer.c -- the ticks timer.
**
** MODIFY HISTORY:
**
** 2012-03-02 wdf Create.
==============================================================================*/
#include "bsp/bsp_config.h"
#include "int.h"

/*======================================================================
  TIMER interrupt handler function declare
======================================================================*/
extern void timer_isr();

/*======================================================================
  TIMER config
======================================================================*/
#define TICK_TIMER              4

/*******************************************************************************
  定时器配置参数
*******************************************************************************/
#define TIMER_ENABLE            (1 << 0)      /*  启动定时器                  */
#define TIMER_DISABLE           (0 << 0)      /*  停止定时器                  */
#define TIMER_MANUAL_UPDATE     (1 << 1)      /*  定时器手动重载              */
#define TIMER_UNMANUAL_UPDATE   (0 << 1)      /*  定时器自动重载              */
#define TIMER_INVERTER          (1 << 2)      /*  定时器输出翻转              */
#define TIMER_UNINVERTER        (0 << 2)      /*  定时器输出不翻转            */
#define TIMER_RELOAD            (1 << 3)      /*  循环计数                    */
#define TIMER_UNRELOAD          (0 << 3)      /*  单次计数                    */
#define TIMER_DEADZONE          (1 << 4)      /*  定时器0使用死区             */
#define TIMER_UNDEAD_ZONE       (0 << 4)      /*  定时器0不使用死区           */
#define TIMER_DEVIDE_1          0x0           /*  1倍分频（不分频）           */
#define TIMER_DEVIDE_2          0x1           /*  2倍分频（1/2）              */
#define TIMER_DEVIDE_4          0x2           /*  2倍分频（1/4）              */
#define TIMER_DEVIDE_8          0x3           /*  2倍分频（1/8）              */
#define TIMER_DEVIDE_16         0x4           /*  2倍分频（1/16）             */

#define TIMER_PRESCALE          99            /*  预分频值                    */

/*******************************************************************************
  PWM Timer configuration registers
*******************************************************************************/
#define rTCFG0      *(volatile unsigned *)0x7F006000
#define rTCFG1      *(volatile unsigned *)0x7F006004
#define rTCON       *(volatile unsigned *)0x7F006008
#define rTCNTB0     *(volatile unsigned *)0x7F00600C
#define rTCMPB0     *(volatile unsigned *)0x7F006010
#define rTCNTO0     *(volatile unsigned *)0x7F006014
#define rTCNTB1     *(volatile unsigned *)0x7F006018
#define rTCMPB1     *(volatile unsigned *)0x7F00601c
#define rTCNTO1     *(volatile unsigned *)0x7F006020
#define rTCNTB2     *(volatile unsigned *)0x7F006024
#define rTCNTO2     *(volatile unsigned *)0x7F00602c
#define rTCNTB3     *(volatile unsigned *)0x7F006030
#define rTCNTO3     *(volatile unsigned *)0x7F006038
#define rTCNTB4     *(volatile unsigned *)0x7F00603c
#define rTCNTO4     *(volatile unsigned *)0x7F006040
#define rTINT_CSTAT *(volatile unsigned *)0x7F006044

#define rVIC0VECTADDR_28  *(volatile unsigned int *)0x71200170 /* Interrupt Controller ADDR */
#define rVIC0ADDRESS      *(volatile unsigned int *)0x71200F00
#define rVIC1ADDRESS      *(volatile unsigned int *)0x71300F00

/*==============================================================================
 * - timer_isr_c()
 *
 * - TIMER interrupt service routine, called by <timer_isr>
 */
void timer_isr_c()
{
    while (rTINT_CSTAT & (1 << (TICK_TIMER + 5))) {
        rTINT_CSTAT |= (1 << (TICK_TIMER + 5));
    }

    rVIC0ADDRESS = 0;
    rVIC1ADDRESS = 0;
}

/*==============================================================================
 * - timer_init()
 *
 * - init tick timer, and start tick timer interrupt
 */
int timer_init()
{
    unsigned short usSlice = (unsigned short)((PCLK/(800))/SYS_CLK_RATE);
    unsigned char  ucOption;

    rTCFG0 &= 0xFFFF00FF;
    rTCFG0 |= (unsigned int)(TIMER_PRESCALE << 8);

    rTCFG1 &= 0xFFF0FFFF;
    rTCFG1 |= (unsigned int)(TIMER_DEVIDE_8 << 16);

    rTCNTB4 = usSlice;

    rTCON &= 0xFF0FFFFF;
    rTCON |= (unsigned int)(TIMER_MANUAL_UPDATE << 20);

    ucOption = TIMER_ENABLE | TIMER_RELOAD | TIMER_UNMANUAL_UPDATE;
    rTCON &= 0xFF0FFFFF;
    if (ucOption &  TIMER_RELOAD) {           /*  timer4 没有 INVERTER 位     */
        ucOption &= 0x07;
        ucOption |= TIMER_INVERTER;           /*  INVERTER 位 决定 Reload     */
    }
    rTCON |= (unsigned int)(ucOption << 20);

    /*
     * enable timer4 sub interrupt
     */
    rTINT_CSTAT |=  (1 << TICK_TIMER);

    /*
     * register uart isr and enable uart interrupt
     */
#if 0
    int_connect (INT_NUMBER_TIMER4, timer_isr_c, 0);
#else
    rVIC0VECTADDR_28 =  (unsigned int)timer_isr;
#endif
    int_enable (INT_NUMBER_TIMER4);

    return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

