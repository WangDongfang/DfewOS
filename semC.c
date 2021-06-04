/*==============================================================================
** semC.c -- the semaphore count module.
**
** MODIFY HISTORY:
**
** 2011-08-23 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "dlist.h"
#include "task.h"
#include "readyQ.h"
#include "semC.h"

#include "memH.h"

#include "serial.h"

/*======================================================================
  this list link all semC struct
======================================================================*/
static DL_LIST _G_semC_list = {NULL, NULL};

/*==============================================================================
 * - semC_init()
 *
 * - init a semC struct, add it to semC_list
 */
SEM_CNT *semC_init(SEM_CNT *pNewSemC, uint32 init_count, uint32 max_count)
{
    int cpsr_c;

    if (max_count == 0) {
        return NULL;
    }

    if (pNewSemC == NULL) {
        pNewSemC = malloc(sizeof(SEM_CNT));
    }

    if (pNewSemC != NULL) {
        pNewSemC->count = init_count;
        pNewSemC->max_count = max_count;
        dlist_init(&pNewSemC->wait_list);

        cpsr_c = CPU_LOCK();
        dlist_add (&_G_semC_list, (DL_NODE *)pNewSemC);
        CPU_UNLOCK(cpsr_c);
    }

    return pNewSemC;
}

/*==============================================================================
 * - semC_take()
 *
 * - take a semC. this function maybe block the call task
 */
OS_STATUS semC_take (SEM_CNT *pSemC, uint32 timeout)
{
    int cpsr_c;

    OS_STATUS status = OS_STATUS_ERROR;

    cpsr_c = CPU_LOCK();
again:
    if (pSemC->count > 0) {
        pSemC->count--;
        G_p_current_tcb->delay_ticks = 0;
        status = OS_STATUS_OK;
    } else {

        if (timeout != 0) {
            readyQ_remove (G_p_current_tcb);
            G_p_current_tcb->delay_ticks = timeout;
            G_p_current_tcb->status = TASK_STATUS_PEND_SEM_C;
            G_p_current_tcb->pend_obj = pSemC;
            dlist_add (&pSemC->wait_list, (DL_NODE *)G_p_current_tcb);

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
 * - semC_give()
 *
 * - give a semC. one give inc count
 */
OS_STATUS semC_give (SEM_CNT *pSemC)
{
    int cpsr_c;
    OS_STATUS status = OS_STATUS_OK;
    OS_TCB *pOutTcb = NULL;

    cpsr_c = CPU_LOCK();

    if (pSemC->count < pSemC->max_count) {    /* have space */
        pSemC->count++;

        pOutTcb = (OS_TCB *)dlist_get(&pSemC->wait_list);
        if (pOutTcb != NULL) {
            readyQ_put(pOutTcb);
        }
    } else {                                   /* no space */
        status = OS_STATUS_ERROR;
    }

    CPU_UNLOCK(cpsr_c);

    return status;
}

/*==============================================================================
 * - semC_flush()
 *
 * - pop all wait task, and set semC's count is 0
 */
OS_STATUS semC_flush (SEM_CNT *pSemC)
{
    int cpsr_c;
    OS_TCB *pOutTcb = NULL;

    cpsr_c = CPU_LOCK();

    pOutTcb = (OS_TCB *)dlist_get(&pSemC->wait_list);
    while (pOutTcb != NULL) {

        /* when this task resume, semC_take() return ERROR immediately */
        pOutTcb->delay_ticks = 0;
        readyQ_put(pOutTcb);
        pOutTcb = (OS_TCB *)dlist_get(&pSemC->wait_list);
    }
    pSemC->count = 0;

    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - semC_delete()
 *
 * - flush and free. only free malloc semC(caller careful)
 */
OS_STATUS semC_delete (SEM_CNT *pSemC)
{
    int cpsr_c;

    semC_flush (pSemC);

    cpsr_c = CPU_LOCK();
    dlist_remove (&_G_semC_list, (DL_NODE *)pSemC);
    CPU_UNLOCK(cpsr_c);

    if ((uint32)pSemC >= CONFIG_MEM_HEAP_LOW && (uint32)pSemC <= CONFIG_MEM_HEAP_HIGH) {
        free (pSemC);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - semC_remove()
 *
 * - when task delete, remove the task tcb from a semC's wait list
 */
OS_STATUS semC_remove (SEM_CNT *pSemC, OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();
    dlist_remove (&pSemC->wait_list, (DL_NODE *)pTcb);
    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/**********************************************************************************************************
  in every tick, check the task which in semC's wait_list
 **********************************************************************************************************/
extern OS_STATUS wait_to_ready (OS_TCB *pTcb, DL_LIST *pWait_list);

static OS_STATUS _update_wait_list (SEM_CNT *pSemC, int unused)
{
    dlist_each (&pSemC->wait_list, (EACH_FUNC_PTR)wait_to_ready, (int)&pSemC->wait_list);

    return OS_STATUS_OK;
}

void semC_decrease ()
{
    dlist_each (&_G_semC_list, (EACH_FUNC_PTR)_update_wait_list, 0);
}

/**********************************************************************************************************
  show the task brief which in semC's wait_list
**********************************************************************************************************/
static OS_STATUS _each_wait_list (SEM_CNT *pSemC,  EACH_FUNC_PTR show_routine)
{
    dlist_each (&pSemC->wait_list, show_routine, 0);
    return OS_STATUS_OK;
}
void semC_show(EACH_FUNC_PTR show_routine)
{
    dlist_each (&_G_semC_list, (EACH_FUNC_PTR)_each_wait_list, (int)show_routine);
}

/**********************************************************************************************************
  show the semC brief which in _G_semC_list
**********************************************************************************************************/
static OS_STATUS _show_semC_info (SEM_CNT *pSemC, int unused)
{
    serial_printf("ID: %8X  cur_cnt: %d  max_cnt: %3d  wait_task: %d\n",
                  pSemC,
                  pSemC->count,
                  pSemC->max_count,
                  dlist_count (&pSemC->wait_list)
                 );
    return OS_STATUS_OK;
}
int c()
{
    dlist_each (&_G_semC_list, (EACH_FUNC_PTR)_show_semC_info, 0);
    return 0;
}
/*==============================================================================
** FILE END
==============================================================================*/

