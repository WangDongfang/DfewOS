/*==============================================================================
** msgQ.c -- the message queue module.
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
#include "msgQ.h"
#include "string.h"

#include "memH.h"

#include "serial.h"

/*======================================================================
  extern function
======================================================================*/
extern OS_STATUS wait_to_ready (OS_TCB *pTcb, DL_LIST *pWait_list);

/*======================================================================
  this list link all user init MSG_QUE struct
======================================================================*/
static DL_LIST _G_msgQ_list = {NULL, NULL};

/*==============================================================================
 * - msgQ_init()
 *
 * - init MSG_QUE struct, and add it to msgQ_list
 */
MSG_QUE *msgQ_init (MSG_QUE *pNewMsgQ,
                    uint32   max_num, 
                    uint32   max_len)
{
    int cpsr_c;

    if (max_num == 0 || max_len == 0) {
        return NULL;
    }

    if (pNewMsgQ == NULL) {
        pNewMsgQ = malloc(sizeof(MSG_QUE));
    }

    if (pNewMsgQ != NULL) {
        pNewMsgQ->max_num = max_num;
        pNewMsgQ->max_len = max_len;
        pNewMsgQ->cur_num = 0;
        dlist_init(&pNewMsgQ->msg_list);
        dlist_init(&pNewMsgQ->wait_send_list);
        dlist_init(&pNewMsgQ->wait_recv_list);

        cpsr_c = CPU_LOCK();
        dlist_add(&_G_msgQ_list, (DL_NODE *)pNewMsgQ);
        CPU_UNLOCK(cpsr_c);
    }

    return pNewMsgQ;
}

/*==============================================================================
 * - msgQ_send()
 *
 * - try to send a message to a msgQ, this maybe block the call task 
 */
OS_STATUS msgQ_send (MSG_QUE     *pMsgQ, 
                     const void  *buffer,
                     uint32       buf_len,
                     uint32       timeout)
{
    int cpsr_c;
    OS_STATUS status = OS_STATUS_ERROR;

    DL_NODE *pMsgNode = NULL;
    OS_TCB  *pWaitTcb = NULL;

    cpsr_c = CPU_LOCK();

again:
    if (pMsgQ->cur_num < pMsgQ->max_num) {     /*  there is some space          */
        pMsgQ->cur_num++;
        G_p_current_tcb->delay_ticks = 0;

        /*
         * alloc a messege node and add it into messge list
         */
        pMsgNode = malloc(sizeof(DL_NODE) + pMsgQ->max_len);
        memcpy((void *)(pMsgNode + 1), buffer, MIN(buf_len, pMsgQ->max_len));
        dlist_add(&pMsgQ->msg_list, pMsgNode);

        /*
         * get a wait for Receive task and put it into readyQ
         */
        pWaitTcb = (OS_TCB *)dlist_get(&pMsgQ->wait_recv_list);
        if (pWaitTcb != NULL) {
            readyQ_put(pWaitTcb);
        }

        status =  OS_STATUS_OK;
    } else {                                   /*  there is no space            */
        if (timeout != 0) {
            readyQ_remove (G_p_current_tcb);
            G_p_current_tcb->delay_ticks = timeout;
            G_p_current_tcb->status = TASK_STATUS_PEND_MSG_S;  /* msg send pend */
            G_p_current_tcb->pend_obj = pMsgQ;
            dlist_add (&pMsgQ->wait_send_list, (DL_NODE *)G_p_current_tcb);

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
 * - msgQ_receive()
 *
 * - try to receive a message from a msgQ, this maybe block the call task 
 */
OS_STATUS msgQ_receive (MSG_QUE *pMsgQ,
                        void    *buffer,
                        uint32   buf_len,
                        uint32   timeout)
{
    int cpsr_c;
    OS_STATUS status = OS_STATUS_ERROR;

    DL_NODE *pMsgNode = NULL;
    OS_TCB  *pWaitTcb = NULL;

    cpsr_c = CPU_LOCK();

again:
    if (pMsgQ->cur_num > 0) {               /* have message(s) */
        pMsgQ->cur_num--;
        G_p_current_tcb->delay_ticks = 0;

        /*
         * copy message context to msg_list
         */
        pMsgNode = dlist_get(&pMsgQ->msg_list);
        memcpy(buffer, (void *)(pMsgNode + 1), MIN(buf_len, pMsgQ->max_len));
        free(pMsgNode);

        /*
         * if there are some task wait for send, alive one of them
         */
        pWaitTcb = (OS_TCB *)dlist_get(&pMsgQ->wait_send_list);
        if (pWaitTcb != NULL) {
            readyQ_put(pWaitTcb);
        }

        status =  OS_STATUS_OK;
    } else {                                /* no message */
        if (timeout != 0) {
            readyQ_remove (G_p_current_tcb);
            G_p_current_tcb->delay_ticks = timeout;
            G_p_current_tcb->status = TASK_STATUS_PEND_MSG_R; /* msg receive pend */
            G_p_current_tcb->pend_obj = pMsgQ;
            dlist_add (&pMsgQ->wait_recv_list, (DL_NODE *)G_p_current_tcb);

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
 * - msgQ_delete()
 *
 * - free all message, pop all wait task,
 * - remove from _G_msgQ_list,
 * - free msgQ if it's malloced
 */
OS_STATUS msgQ_delete (MSG_QUE *pMsgQ)
{
    int cpsr_c;
    DL_NODE *pMsgNode = NULL;
    OS_TCB  *pWaitTcb = NULL;

    cpsr_c = CPU_LOCK();

    /* free message */
    pMsgNode = dlist_get(&pMsgQ->msg_list);
    while (pMsgNode != NULL) {
        free(pMsgNode);
        pMsgNode = dlist_get(&pMsgQ->msg_list);
    }

    /* pop send wait task */
    pWaitTcb = (OS_TCB *)dlist_get(&pMsgQ->wait_send_list);
    if (pWaitTcb != NULL) {
        pWaitTcb->delay_ticks = 0;  /* when this task resume, msgQ_send() return ERROR */
        readyQ_put(pWaitTcb);
        pWaitTcb = (OS_TCB *)dlist_get(&pMsgQ->wait_send_list);
    }
    /* pop recv wait task */
    pWaitTcb = (OS_TCB *)dlist_get(&pMsgQ->wait_recv_list);
    if (pWaitTcb != NULL) {
        pWaitTcb->delay_ticks = 0;  /* when this task resume, msgQ_receive() return ERROR */
        readyQ_put(pWaitTcb);
        pWaitTcb = (OS_TCB *)dlist_get(&pMsgQ->wait_recv_list);
    }

    /* remove from <_G_msgQ_list> */
    dlist_remove (&_G_msgQ_list, (DL_NODE *)pMsgQ);

    CPU_UNLOCK(cpsr_c);

    if ((uint32)pMsgQ >= CONFIG_MEM_HEAP_LOW && (uint32)pMsgQ < CONFIG_MEM_HEAP_HIGH) {
        free (pMsgQ);
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - msgQ_remove()
 *
 * - when task delete, remove the task tcb from a msgQ's wait list
 */
OS_STATUS msgQ_remove (MSG_QUE *pMsgQ, OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();

    if (pTcb->status == TASK_STATUS_PEND_MSG_S) {
        dlist_remove (&pMsgQ->wait_send_list, (DL_NODE *)pTcb);
    } else if (pTcb->status == TASK_STATUS_PEND_MSG_R) {
        dlist_remove (&pMsgQ->wait_recv_list, (DL_NODE *)pTcb);
    }
    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - _update_wait_list()
 *
 * - for each node in wait list, decrease it's wait time
 */
static OS_STATUS _update_wait_list (MSG_QUE *pMsgQ, int unused)
{
    dlist_each (&pMsgQ->wait_send_list, (EACH_FUNC_PTR)wait_to_ready, (int)&pMsgQ->wait_send_list);
    dlist_each (&pMsgQ->wait_recv_list, (EACH_FUNC_PTR)wait_to_ready, (int)&pMsgQ->wait_recv_list);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - msgQ_decrease()
 *
 * - for each msgQ in msgQ_list, check it's wait_list
 */
void msgQ_decrease ()
{
    dlist_each (&_G_msgQ_list, (EACH_FUNC_PTR)_update_wait_list, 0);
}

/*==============================================================================
 * - _each_wait_list()
 *
 * - for each node in wait list, call <show_routine> show task brief
 */
static OS_STATUS _each_wait_list (MSG_QUE *pMsgQ,  EACH_FUNC_PTR show_routine)
{
    dlist_each (&pMsgQ->wait_send_list, show_routine, 0);
    dlist_each (&pMsgQ->wait_recv_list, show_routine, 0);
    return OS_STATUS_OK;
}

/*==============================================================================
 * - msgQ_show()
 *
 * - show the task brief which status: TASK_STATUS_PEND_MSG_S|R
 */
void msgQ_show (EACH_FUNC_PTR show_routine)
{
    dlist_each (&_G_msgQ_list, (EACH_FUNC_PTR)_each_wait_list, (int)show_routine);
}

static OS_STATUS _show_msgQ_info (MSG_QUE *pMsgQ, int unused)
{
    serial_printf("ID: %8X  cur_num: %3d  max_num: %3d  wait_send_task: %d wait_recv_task: %d\n",
                  pMsgQ,
                  pMsgQ->cur_num,
                  pMsgQ->max_num,
                  dlist_count (&pMsgQ->wait_send_list),
                  dlist_count (&pMsgQ->wait_recv_list)
                 );
    return OS_STATUS_OK;
}
int q()
{
    dlist_each (&_G_msgQ_list, (EACH_FUNC_PTR)_show_msgQ_info, 0);
    return 0;
}
/*==============================================================================
** FILE END
==============================================================================*/


