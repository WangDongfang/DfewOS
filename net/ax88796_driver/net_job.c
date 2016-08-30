/*==============================================================================
** net_job.c -- process ax88796 interrupt job.
**
** MODIFY HISTORY:
**
** 2011-08-24 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "ethif_ax88796b.h"

/*======================================================================
  net job config
======================================================================*/
#define JOB_TASK_STACK_SIZE     100 * 1024
#define JOB_TASK_PRIORITY       8
#define JOB_QUEUE_NUM           50

/*======================================================================
  the net job task receive message type
======================================================================*/
typedef struct execut_message {
    EXE_FUNC_PTR exe_func;
    uint32 arg1;
    uint32 arg2;
} EXE_MSG;

/*======================================================================
  global variables
======================================================================*/
static int      _G_msg_lost = 0;
static MSG_QUE  _G_msg_queue;

/*======================================================================
  forward function declare
======================================================================*/
static void _net_job_Thread();

/*==============================================================================
 * - net_job_init()
 *
 * - init msgQ & create a process thread
 */
OS_STATUS net_job_init()
{
    if (msgQ_init(&_G_msg_queue, JOB_QUEUE_NUM, sizeof(EXE_MSG)) == NULL) {
        return OS_STATUS_ERROR;
    }

    if (task_create("tNetJob", JOB_TASK_STACK_SIZE, JOB_TASK_PRIORITY,
                        _net_job_Thread, 0, 0) == NULL) {
    	//msgQ_delete();
    	return OS_STATUS_ERROR;
    } else {
    	return OS_STATUS_OK;
    }
}

/*==============================================================================
 * - net_job_add()
 *
 * - send a msg. called by ax88796 driver
 */
OS_STATUS net_job_add(EXE_FUNC_PTR usr_func, uint32 arg1, uint32 arg2)
{
    EXE_MSG msg = {usr_func, arg1, arg2};

    /* must be "NO_WAIT", avoid context switch */
    if (msgQ_send(&_G_msg_queue, &msg, sizeof(msg), NO_WAIT)
            == OS_STATUS_ERROR) {
        serial_printf("ax88796 dirver lost message!\n");
        _G_msg_lost++;
        return OS_STATUS_ERROR;
    }

    return OS_STATUS_OK;
}

/*==============================================================================
 * - net_job_lost()
 *
 * - get lost job message number
 */
int net_job_lost()
{
    return _G_msg_lost;
}

/*==============================================================================
 * - _net_job_Thread()
 *
 * - the process thread
 */
static void _net_job_Thread()
{
    EXE_MSG msg;

    FOREVER {
        msgQ_receive(&_G_msg_queue, &msg, sizeof(msg), WAIT_FOREVER);
        (msg.exe_func)(msg.arg1, msg.arg2);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

