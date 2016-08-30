/*==============================================================================
** delayQ.c -- the delay queue module.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "dlist.h"
#include "task.h"
#include "readyQ.h"

/*======================================================================
  this list link all delayed task tcb
======================================================================*/
static DL_LIST _G_delayQ = {NULL, NULL};

/*======================================================================
  forward functions declare
======================================================================*/
static OS_STATUS _delay_to_ready (OS_TCB *pTcb, int dec);

/*==============================================================================
 * - delayQ_decrease()
 *
 * - for each task in delayQ, decrease it's delay_ticks, when to 0, ready it.
 *   call in tick announce
 */
void delayQ_decrease ()
{
    dlist_each (&_G_delayQ, (EACH_FUNC_PTR)_delay_to_ready, 1);
}

/*==============================================================================
 * - delayQ_delay()
 *
 * - delay the caller task for some ticks
 */
void delayQ_delay (uint32 ticks)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();

    if (ticks == NO_WAIT) {
        readyQ_remove(G_p_current_tcb);
        readyQ_put(G_p_current_tcb);
    } else {
        readyQ_remove(G_p_current_tcb);
        G_p_current_tcb->delay_ticks = ticks;
        G_p_current_tcb->status = TASK_STATUS_DELAY;
        G_p_current_tcb->pend_obj = NULL;
        dlist_add (&_G_delayQ, (DL_NODE *)G_p_current_tcb);
    }
    CONTEXT_SWITCH();

    CPU_UNLOCK(cpsr_c);
}

/*==============================================================================
 * - delayQ_remove()
 *
 * - when delete a task that in delayQ, call me
 */
OS_STATUS delayQ_remove (OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();
    dlist_remove (&_G_delayQ, (DL_NODE *)pTcb);
    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _delay_to_ready()
 *
 * - decrease the delayed task's delay_tick, if it's 0, move it to readyQ
 */
static OS_STATUS _delay_to_ready (OS_TCB *pTcb, int dec)
{
    pTcb->delay_ticks--;

    if (pTcb->delay_ticks == 0) {
        dlist_remove (&_G_delayQ, (DL_NODE *)pTcb);
        readyQ_put (pTcb);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - delayQ_show()
 *
 * - show the delayed task information
 */
void delayQ_show (EACH_FUNC_PTR show_routine)
{
    dlist_each (&_G_delayQ, show_routine, 0);
}

/*==============================================================================
** FILE END
==============================================================================*/

