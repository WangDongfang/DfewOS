/*==============================================================================
** scheduler.S -- task scheduler routines.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

/********************************************************************
*
*  after push the context frame in stack, the SP store in TCB
*
*           |        |              [LOW ADDRESS]
*           |        |
*           |        |
*           ----------
*  SP --->  |  CPSR  |  \
*           ----------   |
*           |   R0   |   |
*           ----------   |
*           |   ..   |   |
*           |   ..   |   |
*           |   ..   |   |---> a context frame in task stack
*           ----------   |
*           |   R12  |   |
*           ----------   |
*           |   LR   |   |
*           ----------   |
*           |   PC   |  /
*-----------------------------------
*  SP --->  |  ....  |  before push the context in stack
*           |  ....  |
*           |  ....  |              [HIGH ADDRESS]
*
*********************************************************************/

;/*==============================================================================
; * - CONTEXT_SWITCH()
; *
; * - in svc mode, store current task context and load the highest task context
; */
.globl CONTEXT_SWITCH
CONTEXT_SWITCH:
	push {lr}                   /* pc          */
    push {r0-r12, lr}           /* LR R12 - R0 */
    mrs  r0, cpsr
    push {r0}                   /* cpsr        */

    ldr  r0, _G_p_current_tcb   /* store sp to current TCB */
    ldr  r0, [r0]
    str  sp, [r0, #8]

/****** load the highest task ********/
    ldr r0, _G_readyQ           /* get next TCB */
    ldr r0, [r0]

    ldr r1, _G_p_current_tcb    /* change current tcb ptr */
    str r0, [r1]

    ldr r13, [r0, #8]

    pop  {r0}
    msr  cpsr, r0
    pop  {r0 - r12, lr, pc}
    

;/*==============================================================================
; * - LOAD_HIGHEST_TASK()
; *
; * - load the highest task context
; */
.globl LOAD_HIGHEST_TASK
LOAD_HIGHEST_TASK:
    ldr r0, _G_readyQ    /* r0中是G_readyQ的地址 */
    ldr r0, [r0]         /* 将G_readyQ处得第一个字装载 */
    					 /* r0中是highest_tcb的值，是TCB的地址 */

    /*
     *  store highest_tcb to G_p_current_tcb
     */
    ldr r1, _G_p_current_tcb        /* r1 = &G_p_current_tcb */
    str r0, [r1]                    /* *r1 = highest_tcb     */

    ldr r13, [r0, #8]    /* 将TCB中的第8个字节偏移处的数据装载 */
                         /* r13 = highest_tcb->sp              */

    pop  {r0}
    msr  cpsr, r0
    pop  {r0 - r12, lr, pc}

/**********************************************************************************************************
  C roution Global Variables
**********************************************************************************************************/
_G_readyQ:
	.word G_readyQ          /* in readyQ.c */

_G_p_current_tcb:
    .word G_p_current_tcb   /* int task.c */

/*==============================================================================
** FILE END
==============================================================================*/

