/*==============================================================================
** os.h -- tetris os interface header file.
**
** MODIFY HISTORY:
**
** 2011-09-29 wdf Create.
==============================================================================*/

#ifndef __TETRIS_OS_H__
#define __TETRIS_OS_H__

/*======================================================================
  Select OS
======================================================================*/
#define OS_DFEWOS           1    /* this is for DFEWOS */

#ifdef OS_VXWORKS                           /* VXWORKS */

#include <vxWorks.h>
#include <taskLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <msgQLib.h>
#include <memLib.h>
#define OS_TASK_ID          int
#define OS_TASK_ID_ERROR    ERROR
#define OS_MSG_Q_ID         MSG_Q_ID

#elif defined(OS_DFEWOS)                    /* DFEWOS */

#include <dfewos.h>
#include "../../string.h"
#define OS_TASK_ID          OS_TCB*
#define OS_TASK_ID_ERROR    NULL
#define OS_MSG_Q_ID         MSG_QUE*

#elif defined(OS_PPOS)                      /* PPOS */

#error This OS Need Yaozhan Complete it!

#endif                                      /* END OS */

/*======================================================================
  os interface function declare
======================================================================*/
OS_TASK_ID  os_task_create(char         *name,
                           unsigned int  stack_size,
                           unsigned char task_priority,
                           void         *task_entry,
                           int arg1, int arg2);
void        os_task_delay(int ms);
void        os_task_delete(OS_TASK_ID task_id);

OS_MSG_Q_ID os_msgQ_create (int max_num, int max_length);

/* return -2, when <msgQ_id> is a not a legal msgQ */
/* return -1, when there is no space to send the message */
/* return  0, when send the message successfully */
int         os_msgQ_send (OS_MSG_Q_ID msgQ_id, char *msg_buf, int msg_len);

void        os_msgQ_receive (OS_MSG_Q_ID msgQ_id, char *buf, int buf_len);
void        os_msgQ_delete (OS_MSG_Q_ID msgQ_id);
void        os_msgQ_flush (OS_MSG_Q_ID msgQ_id);

int         os_tick_get ();

#endif /* __TETRIS_OS_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

