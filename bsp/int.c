/*==============================================================================
** int.c -- interrupt controller driver.
**
** MODIFY HISTORY:
**
** 2012-03-03 wdf Create.
==============================================================================*/
#include "int.h"

/*======================================================================
  S3C6410X Interrupt controller attribute
======================================================================*/
#define INT_NUMBER_MAX             (INT_NUMBER_ADC)

#define VIC0_BASE                   0x71200000
#define VIC1_BASE                   0x71300000
#define rVIC0ADDRESS                *(volatile unsigned int *)0x71200F00
#define rVIC1ADDRESS                *(volatile unsigned int *)0x71300F00

#define VIC_MASK_ALL                0xFFFFFFFF

/*======================================================================
  S3C6410X VECTORED INTERRUPT CONTROLLER Register Structure
======================================================================*/
typedef struct vic_reg {
    volatile unsigned int VICxIRQSTATUS;                    /* IRQ Status Register          */
    volatile unsigned int VICxFIQSTATUS;                    /* FIQ Status Register          */
    volatile unsigned int VICxRAWINTR;                      /* Raw Interrupt Status Register*/
    volatile unsigned int VICxINTSELECT;                    /* Interrupt Select Register    */
    volatile unsigned int VICxINTENABLE;                    /* Interrupt Enable Register    */
    volatile unsigned int VICxINTENCLEAR;                   /* Interrupt Enable Clear       */
    volatile unsigned int VICxSOFTINT;                      /* Software Interrupt Register  */
    volatile unsigned int VICxSOFTINTCLEAR;                 /* Software Interrupt Clear     */
    volatile unsigned int VICxPROTECTION;                   /* Protection Enable Register   */
    volatile unsigned int VICxSWPRIORITYMASK;               /* Software Priority Mask       */
    volatile unsigned int VICxPRIORITYDAISY;                /* Vector Priority of Daisy Chain */
    volatile unsigned int RESERVE_0[53];                    /* RESERVE 0x2C ~ 0xFC  */
    volatile unsigned int VICxVECTADDR0;                    /* Vector Address 0 Register    */
    volatile unsigned int VICxVECTADDR1;                    /* Vector Address 1 Register    */
    volatile unsigned int VICxVECTADDR2;                    /* Vector Address 2 Register    */
    volatile unsigned int VICxVECTADDR3;                    /* Vector Address 3 Register    */
    volatile unsigned int VICxVECTADDR4;                    /* Vector Address 4 Register    */
    volatile unsigned int VICxVECTADDR5;                    /* Vector Address 5 Register    */
    volatile unsigned int VICxVECTADDR6;                    /* Vector Address 6 Register    */
    volatile unsigned int VICxVECTADDR7;                    /* Vector Address 7 Register    */
    volatile unsigned int VICxVECTADDR8;                    /* Vector Address 8 Register    */
    volatile unsigned int VICxVECTADDR9;                    /* Vector Address 9 Register    */
    volatile unsigned int VICxVECTADDR10;                   /* Vector Address 10 Register   */
    volatile unsigned int VICxVECTADDR11;                   /* Vector Address 11 Register   */
    volatile unsigned int VICxVECTADDR12;                   /* Vector Address 12 Register   */
    volatile unsigned int VICxVECTADDR13;                   /* Vector Address 13 Register   */
    volatile unsigned int VICxVECTADDR14;                   /* Vector Address 14 Register   */
    volatile unsigned int VICxVECTADDR15;                   /* Vector Address 15 Register   */
    volatile unsigned int VICxVECTADDR16;                   /* Vector Address 16 Register   */
    volatile unsigned int VICxVECTADDR17;                   /* Vector Address 17 Register   */
    volatile unsigned int VICxVECTADDR18;                   /* Vector Address 18 Register   */
    volatile unsigned int VICxVECTADDR19;                   /* Vector Address 19 Register   */
    volatile unsigned int VICxVECTADDR20;                   /* Vector Address 20 Register   */
    volatile unsigned int VICxVECTADDR21;                   /* Vector Address 21 Register   */
    volatile unsigned int VICxVECTADDR22;                   /* Vector Address 22 Register   */
    volatile unsigned int VICxVECTADDR23;                   /* Vector Address 23 Register   */
    volatile unsigned int VICxVECTADDR24;                   /* Vector Address 24 Register   */
    volatile unsigned int VICxVECTADDR25;                   /* Vector Address 25 Register   */
    volatile unsigned int VICxVECTADDR26;                   /* Vector Address 26 Register   */
    volatile unsigned int VICxVECTADDR27;                   /* Vector Address 27 Register   */
    volatile unsigned int VICxVECTADDR28;                   /* Vector Address 28 Register   */
    volatile unsigned int VICxVECTADDR29;                   /* Vector Address 29 Register   */
    volatile unsigned int VICxVECTADDR30;                   /* Vector Address 30 Register   */
    volatile unsigned int VICxVECTADDR31;                   /* Vector Address 31 Register   */
    volatile unsigned int RESERVE_1[32];                    /*  RESERVE_1 0x180 ~ 0x1FC     */
    volatile unsigned int VICxVECTPRIORITY0;                /* Vector Priority 0 Register   */
    volatile unsigned int VICxVECTPRIORITY1;                /* Vector Priority 1 Register   */
    volatile unsigned int VICxVECTPRIORITY2;                /* Vector Priority 2 Register   */
    volatile unsigned int VICxVECTPRIORITY3;                /* Vector Priority 3 Register   */
    volatile unsigned int VICxVECTPRIORITY4;                /* Vector Priority 4 Register   */
    volatile unsigned int VICxVECTPRIORITY5;                /* Vector Priority 5 Register   */
    volatile unsigned int VICxVECTPRIORITY6;                /* Vector Priority 6 Register   */
    volatile unsigned int VICxVECTPRIORITY7;                /* Vector Priority 7 Register   */
    volatile unsigned int VICxVECTPRIORITY8;                /* Vector Priority 8 Register   */
    volatile unsigned int VICxVECTPRIORITY9;                /* Vector Priority 9 Register   */
    volatile unsigned int VICxVECTPRIORITY10;               /* Vector Priority 10 Register  */
    volatile unsigned int VICxVECTPRIORITY11;               /* Vector Priority 11 Register  */
    volatile unsigned int VICxVECTPRIORITY12;               /* Vector Priority 12 Register  */
    volatile unsigned int VICxVECTPRIORITY13;               /* Vector Priority 13 Register  */
    volatile unsigned int VICxVECTPRIORITY14;               /* Vector Priority 14 Register  */
    volatile unsigned int VICxVECTPRIORITY15;               /* Vector Priority 15 Register  */
    volatile unsigned int VICxVECTPRIORITY16;               /* Vector Priority 16 Register  */
    volatile unsigned int VICxVECTPRIORITY17;               /* Vector Priority 17 Register  */
    volatile unsigned int VICxVECTPRIORITY18;               /* Vector Priority 18 Register  */
    volatile unsigned int VICxVECTPRIORITY19;               /* Vector Priority 19 Register  */
    volatile unsigned int VICxVECTPRIORITY20;               /* Vector Priority 20 Register  */
    volatile unsigned int VICxVECTPRIORITY21;               /* Vector Priority 21 Register  */
    volatile unsigned int VICxVECTPRIORITY22;               /* Vector Priority 22 Register  */
    volatile unsigned int VICxVECTPRIORITY23;               /* Vector Priority 23 Register  */
    volatile unsigned int VICxVECTPRIORITY24;               /* Vector Priority 24 Register  */
    volatile unsigned int VICxVECTPRIORITY25;               /* Vector Priority 25 Register  */
    volatile unsigned int VICxVECTPRIORITY26;               /* Vector Priority 26 Register  */
    volatile unsigned int VICxVECTPRIORITY27;               /* Vector Priority 27 Register  */
    volatile unsigned int VICxVECTPRIORITY28;               /* Vector Priority 28 Register  */
    volatile unsigned int VICxVECTPRIORITY29;               /* Vector Priority 29 Register  */
    volatile unsigned int VICxVECTPRIORITY30;               /* Vector Priority 30 Register  */
    volatile unsigned int VICxVECTPRIORITY31;               /* Vector Priority 31 Register  */
} VIC_REG;

/*======================================================================
  Interrupt Service Routine Infomation
======================================================================*/
typedef struct isr_info {
    FUNC_ISR  isr;
    void     *arg;
} ISR_INFO;

/*======================================================================
  Global Variables
======================================================================*/
static VIC_REG *_G_pvic0RegGrp = (VIC_REG *)VIC0_BASE;
static VIC_REG *_G_pvic1RegGrp = (VIC_REG *)VIC1_BASE;

static ISR_INFO _G_isr_table[INT_NUMBER_MAX + 1];

/*======================================================================
  Interrupt Handler Function Declare, This fucntion must be in ASM file
======================================================================*/
extern void irq_handler ();

/*==============================================================================
 * - _vicControlEnable()
 *
 * - enable s3c6410x vic controller
 */
static void _vicControlEnable()
{
    __asm (
       " mrc     p15,0,r0,c1,c0,0 \n"
       " orr     r0,r0,#(1<<24)   \n"
       " mcr     p15,0,r0,c1,c0,0 \n"
    );
}

/*==============================================================================
 * - _vicControlDisable()
 *
 * - disable s3c6410x vic controller
 */
static void _vicControlDisable()
{
    __asm (
       " mrc     p15,0,r0,c1,c0,0 \n"
       " bic     r0,r0,#(1<<24)   \n"
       " mcr     p15,0,r0,c1,c0,0 \n"
    );
}

/*==============================================================================
 * - int_init()
 *
 * - init interrupt controller
 */
int int_init ()
{
    volatile unsigned int *vic_addr0 = 0;
    volatile unsigned int *vic_addr1 = 0;
    int i;

    _G_pvic0RegGrp->VICxINTENCLEAR   = VIC_MASK_ALL;       /* 关闭所有中断    */
    _G_pvic0RegGrp->VICxINTSELECT    = 0x0;                /* 所有中断为IRQ   */
    _G_pvic0RegGrp->VICxSOFTINTCLEAR = VIC_MASK_ALL;       /* 关闭所有软中断  */

    _G_pvic1RegGrp->VICxINTENCLEAR   = VIC_MASK_ALL;       /* 关闭所有中断    */
    _G_pvic1RegGrp->VICxINTSELECT    = 0x0;                /* 所有中断为IRQ   */
    _G_pvic1RegGrp->VICxSOFTINTCLEAR = VIC_MASK_ALL;       /* 关闭所有软中断  */

    rVIC0ADDRESS = 0;
    rVIC1ADDRESS = 0;

    vic_addr0  =  &_G_pvic0RegGrp->VICxVECTADDR0;    /* 向量地址寄存器首址    */
    vic_addr1  =  &_G_pvic1RegGrp->VICxVECTADDR0;    /* 向量地址寄存器首址    */
    for(i = 0; i < 32; i++) {
        *vic_addr0++ =  (unsigned int)irq_handler; /* 中断服务函数地址写入VIC0寄存器 */
        *vic_addr1++ =  (unsigned int)irq_handler; /* 中断服务函数地址写入VIC1寄存器 */
    }

    _vicControlEnable();

    return 0;
}

/*==============================================================================
 * - int_uninit()
 *
 * - uninit interrupt controller
 */
int int_uninit ()
{
    _G_pvic0RegGrp->VICxINTENCLEAR   = VIC_MASK_ALL;       /* 关闭所有中断    */
    _G_pvic0RegGrp->VICxINTSELECT    = 0x0;                /* 所有中断为IRQ   */
    _G_pvic0RegGrp->VICxSOFTINTCLEAR = VIC_MASK_ALL;       /* 关闭所有软中断  */

    _G_pvic1RegGrp->VICxINTENCLEAR   = VIC_MASK_ALL;       /* 关闭所有中断    */
    _G_pvic1RegGrp->VICxINTSELECT    = 0x0;                /* 所有中断为IRQ   */
    _G_pvic1RegGrp->VICxSOFTINTCLEAR = VIC_MASK_ALL;       /* 关闭所有软中断  */

    rVIC0ADDRESS = 0;
    rVIC1ADDRESS = 0;

	_vicControlDisable();

	return 0;
}

/*==============================================================================
 * - int_connect()
 *
 * - register a isr for special interrupt number
 */
int int_connect (unsigned int int_num, FUNC_ISR isr, void *arg)
{
    if(int_num > INT_NUMBER_MAX) {
        return -1;
    }

    _G_isr_table[int_num].isr = isr;
    _G_isr_table[int_num].arg = arg;

    return  0;
}

/*==============================================================================
 * - int_disconnect()
 *
 * - unregister a isr for special interrupt number
 */
int int_disconnect (unsigned int int_num)
{
    if(int_num > INT_NUMBER_MAX) {
        return -1;
    }

    _G_isr_table[int_num].isr = 0;
    _G_isr_table[int_num].arg = 0;

    return 0;
}

/*==============================================================================
 * - int_enable()
 *
 * - enable a intterrupt number
 */
int int_enable (unsigned int int_num)
{
    if(int_num > INT_NUMBER_MAX) {
        return -1;
    }

    if(int_num <= 31) {
        _G_pvic0RegGrp->VICxINTENABLE   =  (1  <<  int_num);
    } else {
        _G_pvic1RegGrp->VICxINTENABLE   =  (1  <<  (int_num-32) );
    }

    return 0;
}

/*==============================================================================
 * - int_disable()
 *
 * - disable a interrupt number
 */
int int_disable (unsigned int int_num)
{
    if(int_num > INT_NUMBER_MAX) {
        return -1;
    }

    if(int_num <= 31) {
        _G_pvic0RegGrp->VICxINTENCLEAR   =  (1  <<  int_num);
    } else {
        _G_pvic1RegGrp->VICxINTENCLEAR   =  (1  <<  (int_num-32));
    }

    return 0;
}

/*==============================================================================
 * - _int_handler_c()
 *
 * - interrupt handler C routine, called by <irq_handler>
 */
void _int_handler_c ()
{
    int i;
    unsigned int int_status;

    int_status = _G_pvic0RegGrp->VICxIRQSTATUS;
    for (i = 0; (int_status && i < 32); i++) {
        if (int_status & 0x1) {
            if (_G_isr_table[i].isr != 0) {
                (_G_isr_table[i].isr)(_G_isr_table[i].arg);
            }
        }
        int_status >>= 1;
    }

    int_status = _G_pvic1RegGrp->VICxIRQSTATUS;
    for (i = 32; (int_status && i < 64); i++) {
        if (int_status & 0x1) {
            if (_G_isr_table[i].isr != 0) {
                (_G_isr_table[i].isr)(_G_isr_table[i].arg);
            }
        }
        int_status >>= 1;
    }

    rVIC0ADDRESS = 0;
    rVIC1ADDRESS = 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

