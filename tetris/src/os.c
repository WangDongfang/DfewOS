/*==============================================================================
** os.c -- tetris os interface. now support VXWORKS & DFEWOS.
**
** MODIFY HISTORY:
**
** 2011-09-29 wdf Create.
==============================================================================*/

#include "os.h"

#ifdef OS_VXWORKS                                    /* VXWORKS START */
/*======================================================================
  VxWorks TASK
======================================================================*/
OS_TASK_ID  os_task_create (char   		 *name,
                            unsigned int  stack_size,
                            unsigned char task_priority,
                            void         *task_entry,
                            int arg1, int arg2)
{
    return taskSpawn(name, task_priority, 0, stack_size,
                     (FUNCPTR)task_entry,
                     arg1, arg2, 3, 4, 5, 6, 7, 8, 9, 0);
}

void os_task_delay (int ms)
{
    int ticks = sysClkRateGet() * ms / 1000;
    taskDelay(ticks);
}

void os_task_delete (OS_TASK_ID task_id)
{
    taskDelete(task_id);
}

/*======================================================================
  VxWorks MSG_Q
======================================================================*/
OS_MSG_Q_ID os_msgQ_create (int max_num, int max_length)
{
    return msgQCreate(max_num, max_length, MSG_Q_FIFO);
}

int os_msgQ_send (OS_MSG_Q_ID msgQ_id, char *msg_buf, int msg_len)
{
    int  status = 0;

    if (msgQSend (msgQ_id, msg_buf, msg_len, NO_WAIT, MSG_PRI_NORMAL) == ERROR ) {
        if (errno == S_objLib_OBJ_UNAVAILABLE || errno == S_objLib_OBJ_TIMEOUT) {
            status = -1;
        } else {
            status = -2;
        }
    }

    return status;
}

void os_msgQ_receive (OS_MSG_Q_ID msgQ_id, char *buf, int buf_len)
{
    msgQReceive (msgQ_id, buf, buf_len, WAIT_FOREVER);
}

void os_msgQ_delete(OS_MSG_Q_ID msgQ_id)
{
    msgQDelete(msgQ_id);
}

void os_msgQ_flush(OS_MSG_Q_ID msgQ_id)
{
}

/*======================================================================
  VxWorks TICK
======================================================================*/
int os_tick_get()
{
    return tickGet();
}
/*****************************************************//* VXWORKS END */


#elif defined(OS_DFEWOS)                             /* DEFEWOS START */
/*======================================================================
  DfewOS TASK
======================================================================*/
OS_TASK_ID  os_task_create(char         *name,
                           unsigned int  stack_size,
                           unsigned char task_priority,
                           void         *task_entry,
                           int arg1, int arg2)
{
    return task_create(name, stack_size, 
                task_priority, 
                (FUNC_PTR)task_entry,
                arg1, arg2);
}

void os_task_delay (int ms)
{
    int ticks = SYS_CLK_RATE * ms / 1000;
    delayQ_delay(ticks);
}

void os_task_delete (OS_TASK_ID task_id)
{
    /*delayQ_delay(SYS_CLK_RATE / 10);*/
	task_delete (task_id);
}

/*======================================================================
  DfewOS MSG_Q
======================================================================*/
OS_MSG_Q_ID os_msgQ_create (int max_num, int max_length)
{
    return msgQ_init(NULL, max_num, max_length);
}

int os_msgQ_send (OS_MSG_Q_ID msgQ_id, char *msg_buf, int msg_len)
{
    if (msgQ_id == NULL) {
        return -2;
    }

    msgQ_send (msgQ_id, msg_buf, msg_len, NO_WAIT);

    return 0;
}

void os_msgQ_receive (OS_MSG_Q_ID msgQ_id, char *buf, int buf_len)
{
    msgQ_receive (msgQ_id, buf, buf_len, WAIT_FOREVER);
}

void os_msgQ_delete(OS_MSG_Q_ID msgQ_id)
{
    msgQ_delete (msgQ_id);
}

void os_msgQ_flush(OS_MSG_Q_ID msgQ_id)
{
    while (msgQ_receive (msgQ_id, NULL, 0, NO_WAIT) == OS_STATUS_OK);
}

/*======================================================================
  DfewOS TICK
======================================================================*/
int os_tick_get()
{
    return tick_get();
}
/*****************************************************//* DEFEWOS END */


#elif defined(OS_PPOS)                                  /* PPOS START */
#error This OS Need Yaozhan Complete it!
/********************************************************//* PPOS END */

#endif                                                      /* END OS */

/*==============================================================================
** FILE END
==============================================================================*/

