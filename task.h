/*==============================================================================
** task.h -- task module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __TCB_H__
#define __TCB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  config macro
======================================================================*/
#define NAME_LEN_MAX	32
#define TASK_NUM_MAX    256

/*======================================================================
  The TCB structure
======================================================================*/
typedef struct os_tcb {
    DL_NODE  dlist_node;

    uint32 	 sp;

	char     task_name[NAME_LEN_MAX];
	FUNC_PTR task_entry;
    uint32   task_priority;
	uint32   stack_start;
	uint32   stack_size;
    uint32   arg1;
    uint32   arg2;

    uint32   delay_ticks;

    uint32   origin_pri;        /* Ѫͳ */
    uint32   status;
    void    *pend_obj;          /* point to the pend obj */
                                /* this tcb in the obj's wait_list */
                                /* the obj maybe semB, semM, msgQ */

} OS_TCB;

/*======================================================================
  this module global variables declare
======================================================================*/
extern OS_TCB *G_p_current_tcb;
extern uint32  G_Task_Num;

/*======================================================================
  assemble language functions declare
======================================================================*/
extern void    CONTEXT_SWITCH();
extern void    LOAD_HIGHEST_TASK();

/*======================================================================
  task library support apis
======================================================================*/
void    task_init();
OS_TCB *task_create(const char *task_name,
                    uint32      stack_size,
                    uint32      task_priority,
                    FUNC_PTR    task_entry,
                    uint32      arg1,
                    uint32      arg2);
OS_STATUS task_delete (OS_TCB *pTcb);
void task_show ();
OS_STATUS tcb_shw (OS_TCB *pTcb, int unused);


/*======================================================================
  task status
======================================================================*/
#define TASK_STATUS_READY           0    /* task tcb in readyQ           */
#define TASK_STATUS_DELAY           1    /* task tcb in delayQ           */
#define TASK_STATUS_PEND_SEM_M      2    /* task tcb in semM's wait_list */
#define TASK_STATUS_PEND_SEM_B      3    /* task tcb in semB's wait_list */
#define TASK_STATUS_PEND_SEM_C      4    /* task tcb in semC's wait_list */
#define TASK_STATUS_PEND_MSG_S      5    /* task tcb in msgQ's wait_list */
#define TASK_STATUS_PEND_MSG_R      6    /* task tcb in msgQ's wait_list */


/*======================================================================
  wait ticks macro
======================================================================*/
#define NO_WAIT         0x0
#define WAIT_FOREVER    0xffffffff

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TCB_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

