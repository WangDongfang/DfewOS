/*==============================================================================
** int_asm.S -- interrupt service rouine.
**
** MODIFY HISTORY:
**
** 2012-03-04 wdf Create.
==============================================================================*/
#include "bsp_config.h"

;/*==============================================================================
; * - irq_handler()
; *
; * - when irq(except tick interrupt) occured, this is the first code
; */
.globl irq_handler
irq_handler:
	sub lr, lr, #4
    ldr r13, =CONFIG_IRQ_STACK
    STMFD r13!, {r0 - r3, r12, lr}

    bl _int_handler_c    /* jump to interrupt controller dirver */

    LDMFD r13!, {r0 - r3, r12, pc}^

/*==============================================================================
** FILE END
==============================================================================*/

