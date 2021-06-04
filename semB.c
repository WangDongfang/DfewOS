/*==============================================================================
** semB.c -- the semaphore binary module.
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
#include "semB.h"

#include "memH.h"

#include "serial.h"

/*======================================================================
  this list link all semB struct
======================================================================*/
static DL_LIST _G_semB_list = {NULL, NULL};

/*==============================================================================
 * - semB_init()
 *
 * - init a semB struct, add it to semB_list
 */
SEM_BIN *semB_init(SEM_BIN *pNewSemB)
{
    int cpsr_c;

    if (pNewSemB == NULL) {
        pNewSemB = malloc(sizeof(SEM_BIN));
    }

    if (pNewSemB != NULL) {
        pNewSemB->owner_tcb = NULL;
        dlist_init(&pNewSemB->wait_list);

        cpsr_c = CPU_LOCK();
        dlist_add (&_G_semB_list, (DL_NODE *)pNewSemB);
        CPU_UNLOCK(cpsr_c);
    }

    return pNewSemB;
}

/*==============================================================================
 * - semB_take()
 *
 * - take a semB. this function maybe block the call task
 */
OS_STATUS semB_take (SEM_BIN *pSemB, uint32 timeout)
{
    int cpsr_c;

    OS_STATUS status = OS_STATUS_ERROR;

    cpsr_c = CPU_LOCK();
again:
    if (pSemB->owner_tcb == NULL) {
        pSemB->owner_tcb = G_p_current_tcb;            /*  assign owner  */
        G_p_current_tcb->delay_ticks = 0;
        status = OS_STATUS_OK;
    } else {

        if (timeout != 0) {
            readyQ_remove (G_p_current_tcb);
            G_p_current_tcb->delay_ticks = timeout;
            G_p_current_tcb->status = TASK_STATUS_PEND_SEM_B;
            G_p_current_tcb->pend_obj = pSemB;
            dlist_add (&pSemB->wait_list, (DL_NODE *)G_p_current_tcb);

            CONTEXT_SWITCH();

            if (G_p_current_tcb->delay_ticks != 0) {
                timeout = G_p_current_tcb->delay_ticks;  /* recalculate timeout value */
                goto again;
            }
        }
    }
    CPU_UNLOCK(cpsr_c);

    return status;
}

/*==============================================================================
 * - semB_give()
 *
 * - give a semB. one give one pop
 */
OS_STATUS semB_give (SEM_BIN *pSemB)
{
    int cpsr_c;
    OS_TCB *pOutTcb = NULL;

    cpsr_c = CPU_LOCK();

    pSemB->owner_tcb = NULL;
    pOutTcb = (OS_TCB *)dlist_get(&pSemB->wait_list);
    if (pOutTcb != NULL) {
        readyQ_put(pOutTcb);
    }

    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - semB_flush()
 *
 * - pop all wait task, and set semB's owner is NULL
 */
OS_STATUS semB_flush (SEM_BIN *pSemB)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();

    pSemB->owner_tcb = (OS_TCB *)dlist_get(&pSemB->wait_list);
    while (pSemB->owner_tcb != NULL) {

        /* when this task resume, semB_take() return ERROR immediately */
        pSemB->owner_tcb->delay_ticks = 0;
        readyQ_put(pSemB->owner_tcb);
        pSemB->owner_tcb = (OS_TCB *)dlist_get(&pSemB->wait_list);
    }

    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - semB_delete()
 *
 * - flush and free. only free malloc semB(caller careful)
 */
OS_STATUS semB_delete (SEM_BIN *pSemB)
{
    int cpsr_c;

    semB_flush (pSemB);

    cpsr_c = CPU_LOCK();
    dlist_remove (&_G_semB_list, (DL_NODE *)pSemB);
    CPU_UNLOCK(cpsr_c);

    if ((uint32)pSemB >= CONFIG_MEM_HEAP_LOW && (uint32)pSemB <= CONFIG_MEM_HEAP_HIGH) {
        free (pSemB);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - semB_remove()
 *
 * - when task delete, remove the task tcb from a semB's wait list
 */
OS_STATUS semB_remove (SEM_BIN *pSemB, OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();
    dlist_remove (&pSemB->wait_list, (DL_NODE *)pTcb);
    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/**********************************************************************************************************
  in every tick, check the task which in semB's wait_list
**********************************************************************************************************/
OS_STATUS wait_to_ready (OS_TCB *pTcb, DL_LIST *pWait_list)
{
    if (pTcb->delay_ticks != WAIT_FOREVER) {
        pTcb->delay_ticks--;
    }

    if (pTcb->delay_ticks == 0) {
        dlist_remove (pWait_list, (DL_NODE *)pTcb);

        readyQ_put (pTcb);
    }

    return OS_STATUS_OK;
}

static OS_STATUS _update_wait_list (SEM_BIN *pSemB, int unused)
{
    dlist_each (&pSemB->wait_list, (EACH_FUNC_PTR)wait_to_ready, (int)&pSemB->wait_list);

    return OS_STATUS_OK;
}

void semB_decrease ()
{
    dlist_each (&_G_semB_list, (EACH_FUNC_PTR)_update_wait_list, 0);
}

/**********************************************************************************************************
  show the task brief which in semB's wait_list
**********************************************************************************************************/
static OS_STATUS _each_wait_list (SEM_BIN *pSemB,  EACH_FUNC_PTR show_routine)
{
    dlist_each (&pSemB->wait_list, show_routine, 0);
    return OS_STATUS_OK;
}
void semB_show(EACH_FUNC_PTR show_routine)
{
    dlist_each (&_G_semB_list, (EACH_FUNC_PTR)_each_wait_list, (int)show_routine);
}

/**********************************************************************************************************
  show the semB brief which in _G_semB_list
**********************************************************************************************************/
static OS_STATUS _show_semB_info (SEM_BIN *pSemB, int unused)
{
    serial_printf("ID: %8X  owner_tcb: 0x%8X  wait_task: %d\n",
                  pSemB,
                  pSemB->owner_tcb,
                  dlist_count (&pSemB->wait_list)
                 );
    return OS_STATUS_OK;
}
int b()
{
    dlist_each (&_G_semB_list, (EACH_FUNC_PTR)_show_semB_info, 0);
    return 0;
}
/*==============================================================================
** FILE END
==============================================================================*/

