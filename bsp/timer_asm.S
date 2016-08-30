/*==============================================================================
** timer_asm.S -- tick interrupt service routine.
**
** MODIFY HISTORY:
**
** 2012-03-04 wdf Create.
==============================================================================*/
#include "bsp_config.h"

;/*==============================================================================
; * - timer_isr()
; *
; * - when tick interrupt occured, this is the first code
; */
.globl timer_isr
timer_isr:
	sub lr, lr, #4
    ldr r13, =CONFIG_IRQ_STACK
    STMFD r13!, {r0 - r3, r12, lr}

    bl timer_isr_c

    LDMFD r13!, {r0 - r3, r12, lr}

/*=======================================SWITCH TASK=====================================================*/
    ldr r13, =IRQ_LR_R0_ADDR
    str lr, [r13], #4
    str r0, [r13]
    
    mrs r13, spsr
    orr r13, #0x80           /* disable irq  */
    msr cpsr, r13                                                       /*  in no irq svc mode           */

    ldr   r0, =IRQ_LR_R0_ADDR
    ldr   r0, [r0]
    push  {r0}               /* pc           */
    ldr   r0, =IRQ_LR_R0_ADDR
    ldr   r0, [r0, #4]
    push  {r0 - r12, lr}     /* lr, r12 - r0 */
    mrs   r0, cpsr
    bic   r0, r0, #0x80
    push  {r0}               /* cpsr         */

    ldr  r0, _G_p_current_tcb   /* store sp to current TCB */
    ldr  r0, [r0]
    str  sp, [r0, #8]

    bl tick_announce

    b  LOAD_HIGHEST_TASK                                                /*  load new context             */
/*=======================================SWITCH TASK=====================================================*/

/**********************************************************************************************************
  C roution Global Variables in task.c
**********************************************************************************************************/
_G_p_current_tcb:
    .word G_p_current_tcb

/*==============================================================================
** FILE END
==============================================================================*/

