/*==============================================================================
** semM.c -- the semaphore mutex module.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "serial.h"
#include "dlist.h"
#include "task.h"
#include "readyQ.h"
#include "semM.h"

#include "memH.h"

/*======================================================================
  extern function
======================================================================*/
extern OS_STATUS wait_to_ready (OS_TCB *pTcb, DL_LIST *pWait_list);

/*

_G_semM_list
+----------+      +---------+       +---------+         +---------+
| DL_LIST  | ==== | DL_NODE | ===== | DL_NODE | ======= | DL_NODE |     +-------+
+----------+      -----------       -----------         -----------     |  the  |
              +-- |owner_tcb|       |owner_tcb| -> NULL |owner_tcb| --> | owner |
 +-------+    |   -----------       -----------         -----------     |  TCB  |
 |  the  | <--+   |wait_pri |       |wait_pri |         |wait_pri |     +-------+
 | owner |        -----------       -----------         -----------
 |  TCB  |        |wait_list| ===   |wait_list| ===     |wait_list| ===
 +-------+        +---------+   #   +---------+   #     +---------+   #
                                #                 #                   #
                            +--------+        ----------          +--------+
                            |the wait|          ------            |the wait|
                            | task's |           ----             | task's |
                            |  TCB   |            --              |  TCB   |
                            +--------+                            +--------+
                                #                                     #
                            +--------+                            +--------+
                            |the wait|                            |the wait|
                            | task's |                            | task's |
                            |  TCB   |                            |  TCB   |
                            +--------+                            +--------+
                                #                                     #
                            +--------+                            +--------+
                            |the wait|                            |the wait|
                            | task's |                            | task's |
                            |  TCB   |                            |  TCB   |
                            +--------+                            +--------+

*/

/*======================================================================
  this list link all semM struct
======================================================================*/
static DL_LIST _G_semM_list = {NULL, NULL};

/*======================================================================
  forward functions declare
======================================================================*/
static void _up_owner_pri(OS_TCB *pTcb_wait, SEM_MUX *pSemM);
static void _sort_pri_put(DL_LIST *pTcb_list, OS_TCB *pTcb);
static int  _get_highest_wait_pri();
static void _update_wait_pri (SEM_MUX *pSemM);

/*==============================================================================
 * - _semM_alloc()
 *
 * - alloc a semM struct
 */
static SEM_MUX *_semM_alloc()
{
    return (SEM_MUX *)malloc(sizeof(SEM_MUX));
}

/*==============================================================================
 * - semM_init()
 *
 * - init a semM struct, add it to semM_list
 */
SEM_MUX *semM_init(SEM_MUX *pNewSemM)
{
    int cpsr_c;

    if (pNewSemM == NULL) {
        pNewSemM = _semM_alloc();
    }

    if (pNewSemM != NULL) {
        pNewSemM->owner_tcb = NULL;
        pNewSemM->take_count = 0;
        pNewSemM->wait_pri = 255;
        dlist_init(&pNewSemM->wait_list);

        cpsr_c = CPU_LOCK();
        dlist_add (&_G_semM_list, (DL_NODE *)pNewSemM);
        CPU_UNLOCK(cpsr_c);
    }

    return pNewSemM;
}

/*==============================================================================
 * - semM_take()
 *
 * - take a semM. this function maybe block the call task
 */
OS_STATUS semM_take (SEM_MUX *pSemM, uint32 timeout)
{
    int cpsr_c;

    OS_STATUS status = OS_STATUS_ERROR;

    cpsr_c = CPU_LOCK();
    if (pSemM->owner_tcb == NULL) {
        pSemM->owner_tcb = G_p_current_tcb;                             /*  assign owner                 */
        pSemM->take_count = 1;
        status = OS_STATUS_OK;
    } else {
        
        if (pSemM->owner_tcb == G_p_current_tcb) {
            pSemM->take_count++;
        } else {

            if (timeout != 0) {

                /*
                 * rise up the owner task priority(if necessary)
                 */
                _up_owner_pri(G_p_current_tcb, pSemM);

                /* 
                 * move the taker from readyQ to this sem's wait_list
                 * sortly by the tasker's priority, the higher the header
                 */
                readyQ_remove (G_p_current_tcb);
                G_p_current_tcb->delay_ticks = timeout;
                G_p_current_tcb->status = TASK_STATUS_PEND_SEM_M;
                G_p_current_tcb->pend_obj = pSemM;
                _sort_pri_put(&pSemM->wait_list, G_p_current_tcb); /* add to wait_list sort by pri */

                CONTEXT_SWITCH();

                /* 
                 * someone move the taker from semM's wait_list to readyQ
                 * semM_decrease():      delay_ticks == 0
                 * other task semM_give: delay_ticks != 0
                 */
                if (G_p_current_tcb->delay_ticks != 0) {
                    G_p_current_tcb->delay_ticks  = 0;

                    /* 
                     * because semM's owner is assigned when semM_give(),
                     * Here not assigned again.
                     */
                    /* pSemM->owner_tcb = G_p_current_tcb; */    /*  assign owner                 */

                    status = OS_STATUS_OK;
                }
            } /* if (timeout != 0) */
        } /* if (pSemM->owner_tcb == G_p_current_tcb) */
    }
    CPU_UNLOCK(cpsr_c);

    return status;
}

/*==============================================================================
 * - semM_give()
 *
 * - give a semM. this function maybe block the call task
 */
OS_STATUS semM_give (SEM_MUX *pSemM)
{
    int cpsr_c;
    int highest_wait_pri;   /* other semMs' highest wait_pri */
    int task_next_pri;      /* this task's next priority */
    OS_TCB *pNextOwner = NULL;

    cpsr_c = CPU_LOCK();
    
    if (--pSemM->take_count != 0) {  /* current task still own this semM */
        CPU_UNLOCK(cpsr_c);
        return OS_STATUS_OK;
    }
    
    pNextOwner = (OS_TCB *)dlist_get(&pSemM->wait_list);
    if (pNextOwner != NULL) {
        pSemM->owner_tcb = pNextOwner;                     /*  assign owner                 */
        readyQ_put(pNextOwner);
        _update_wait_pri (pSemM);
    } else {
        pSemM->owner_tcb = NULL;
    }

    /*
     * find the higest pri in this task ownered semM(s)
     * this pri maybe this task's next priority
     */
    highest_wait_pri = _get_highest_wait_pri();

    task_next_pri = MIN (highest_wait_pri, G_p_current_tcb->origin_pri);

    /*
     * the next priority is lower than current priority,
     * low myself, and switch task
     */
    if (task_next_pri > G_p_current_tcb->task_priority) {
        readyQ_remove (G_p_current_tcb);
        G_p_current_tcb->task_priority = task_next_pri;
        readyQ_put (G_p_current_tcb);

        CONTEXT_SWITCH(); /* go to the higher task */
    }

    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _update_wait_pri()
 *
 * - make the wait_pri is the highest prioriry of <pSemM>'s wait_list
 */
static void _update_wait_pri (SEM_MUX *pSemM)
{
    /*
     * set the wait_pri to highest pri of left wait tasks
     */
    if (DL_EMPTY (&pSemM->wait_list)) {
        pSemM->wait_pri = 255;
    } else {
        pSemM->wait_pri = ((OS_TCB *)DL_FIRST (&pSemM->wait_list))->task_priority;
    }
}

/*==============================================================================
 * - _free_semM()
 *
 * - check if the <pTcb> task own this semM. if do, give it 
 */
static OS_STATUS _free_semM (SEM_MUX *pSemM, OS_TCB *pTcb)
{
    OS_TCB *pNextOwner = NULL;
    if (pSemM->owner_tcb == pTcb) {
        pSemM->take_count = 0;

        pNextOwner = (OS_TCB *)dlist_get(&pSemM->wait_list);
        if (pNextOwner != NULL) {
            readyQ_put(pNextOwner);
            _update_wait_pri (pSemM);
        } else {
            pSemM->owner_tcb = NULL;
        }
    }
    return OS_STATUS_OK;
}

/*==============================================================================
 * - semM_free()
 *
 * - when task delete, free all the semMs the task taked
 */
void semM_free (OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();
    dlist_each (&_G_semM_list, (EACH_FUNC_PTR)_free_semM, (int)pTcb);
    CPU_UNLOCK(cpsr_c);
}

/*==============================================================================
 * - semM_remove()
 *
 * - when task delete, remove the task tcb from a semM's wait list
 */
OS_STATUS semM_remove (SEM_MUX *pSemM, OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();
    dlist_remove (&pSemM->wait_list, (DL_NODE *)pTcb);
    _update_wait_pri (pSemM);
    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _check_owner()
 *
 * - check if current task own this semM. if do, fresh highest_wait_pri
 */
static OS_STATUS _check_owner (SEM_MUX *pSemM, int *p_highest_wait_pri)
{
    /*
     * current task own this semM
     */
    if (pSemM->owner_tcb == G_p_current_tcb) {
        if (*p_highest_wait_pri > pSemM->wait_pri) {
            *p_highest_wait_pri = pSemM->wait_pri;
        }
    }
    return OS_STATUS_OK;
}

/*==============================================================================
 * - _get_highest_wait_pri()
 *
 * - check current task owned semM(s), get the higest pri from semMs' wait_pri
 */
static int _get_highest_wait_pri()
{
    int highest_wait_pri = 255;
    dlist_each (&_G_semM_list, (EACH_FUNC_PTR)_check_owner, (int)&highest_wait_pri);

    return highest_wait_pri;
}

/*==============================================================================
 * - _sort_pri_put()
 *
 * - insert the tcb to a list.
 *   the tcb's priority is key word like this: [BEGIN] 1-->3-->8-...->255 [END]
 */
static void _sort_pri_put(DL_LIST *pTcb_list, OS_TCB *pTcb)
{

    if ( DL_EMPTY(pTcb_list) ||    /* list is empty */
         /* or the inserted tcb pri lower than list' last node */
         ((OS_TCB *)DL_LAST(pTcb_list))->task_priority < pTcb->task_priority) {

        /* insert end */
        dlist_add (pTcb_list, (DL_NODE *)pTcb);
    } else {                            /* list have node(s) & last node big or equ insert node pri */
        DL_NODE *pPrev = DL_FIRST (pTcb_list);

        /*
         * find the node that priority big or equal
         * the insert node
         */
        while (pPrev != NULL &&
               ((OS_TCB *)pPrev)->task_priority < pTcb->task_priority
              ) {
            pPrev = DL_NEXT(pPrev);
        }

        pPrev = DL_PREVIOUS(pPrev);
        dlist_insert (pTcb_list, pPrev, (DL_NODE *)pTcb);
    }
}

/*==============================================================================
 * - _up_owner_pri()
 *
 * - when can't take semM immediately, rise the semM's owner pri
 */
static void _up_owner_pri(OS_TCB *pTcb_wait, SEM_MUX *pSemM)
{
    OS_TCB *pTcb_owner = pSemM->owner_tcb;

    /*
     * the waiter priority is higher than the semM owner task
     */
    if (pTcb_wait->task_priority < pTcb_owner->task_priority) {
        if (pTcb_owner->status == TASK_STATUS_READY) {             /* owner is ready */
            readyQ_remove (pTcb_owner);
            pTcb_owner->task_priority = pTcb_wait->task_priority;
            readyQ_put (pTcb_owner);
        } else if (pTcb_owner->status == TASK_STATUS_PEND_SEM_M) { /* owner is pend on other semM */
            pTcb_owner->task_priority = pTcb_wait->task_priority;
            _up_owner_pri(pTcb_owner, (SEM_MUX *)pTcb_owner->pend_obj);
        } else {                                                   /* DELAY, PEND_SEM_B PEND_MSG_S, PEND_MSG_R */
            pTcb_owner->task_priority = pTcb_wait->task_priority;
        }
    }

    /*
     * the waiter priority is higher than the semM wait_pri
     */
    if (pTcb_wait->task_priority < pSemM->wait_pri) {
        pSemM->wait_pri = pTcb_wait->task_priority;
    }
}

/*==============================================================================
 * - _each_wait_list()
 *
 * - for each semM's wait_list, call <show_routine>
 */
static OS_STATUS _each_wait_list (SEM_MUX *pSemM,  EACH_FUNC_PTR show_routine)
{
    dlist_each (&pSemM->wait_list, show_routine, 0);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - semM_show()
 *
 * - show the task brief which status: TASK_STATUS_PEND_SEM_M
 */
void semM_show (EACH_FUNC_PTR show_routine)
{
    dlist_each (&_G_semM_list, (EACH_FUNC_PTR)_each_wait_list, (int)show_routine);
}

/*==============================================================================
 * - _update_wait_list()
 *
 * - for each node in wait list, decrease it's wait time
 */
static OS_STATUS _update_wait_list (SEM_MUX *pSemM, int unused)
{
    dlist_each (&pSemM->wait_list, (EACH_FUNC_PTR)wait_to_ready, (int)&pSemM->wait_list);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - semM_decrease()
 *
 * - for each semM in semM_list, check it's wait_list
 */
void semM_decrease ()
{
    dlist_each (&_G_semM_list, (EACH_FUNC_PTR)_update_wait_list, 0);
}


/**********************************************************************************************************
  show the semM brief which in _G_semM_list
**********************************************************************************************************/
static OS_STATUS _show_semM_info (SEM_MUX *pSemM, int unused)
{
    serial_printf("ID: %8X  owner_tcb: 0x%8X  wait_pri: %3d  wait_task: %d\n",
                  pSemM,
                  pSemM->owner_tcb,
                  pSemM->wait_pri,
                  dlist_count (&pSemM->wait_list)
                 );
    return OS_STATUS_OK;
}
int m()
{
    dlist_each (&_G_semM_list, (EACH_FUNC_PTR)_show_semM_info, 0);
    return 0;
}
/*==============================================================================
** FILE END
==============================================================================*/

