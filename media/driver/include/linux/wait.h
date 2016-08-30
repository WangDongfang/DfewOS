#include <dfewos.h>

#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H

#include <stddef.h>

typedef SEM_BIN*                                    wait_queue_head_t; /*  等待队列的首部              */
#define interruptible_sleep_on_timeout(q, timeout)  (semB_take(*(q), timeout) == OS_STATUS_OK ? (timeout) : 0)
#define wake_up_interruptible(q)                    {if ((q) != NULL) semB_give(*(q));}
#define init_waitqueue_head(q)                      {*(q) = semB_init(NULL); semB_take(*(q), 0);};

#define DECLARE_WAIT_QUEUE_HEAD(name)               SEM_BIN  *name

#endif                                                                  /*  _LINUX_WAIT_H               */
