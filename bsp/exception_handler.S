/*==============================================================================
** exception_handler.S -- exception handlers.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

/**********************************************************************************************************
  The exception handlers.
**********************************************************************************************************/
.globl undefined_instruction
undefined_instruction:
    b undefined_instruction
.globl software_interrupt
software_interrupt:
    b software_interrupt
.globl prefetch_abort
prefetch_abort:
    b prefetch_abort
.globl data_abort
data_abort:
    b data_abort
.globl not_used
not_used:
    b not_used
.globl irq
irq:
    b not_used
.globl fiq
fiq:
    b fiq

/*==============================================================================
** FILE END
==============================================================================*/

