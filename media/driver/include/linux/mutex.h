#include <dfewos.h>

#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

struct mutex {
    SEM_MUX  semId;
};

#define mutex_unlock(h_mutex)             semM_give((SEM_MUX *)h_mutex)
#define mutex_lock(h_mutex)               semM_take((SEM_MUX *)h_mutex, WAIT_FOREVER)
#define mutex_init(h_mutex)              (h_mutex = (struct mutex*)semM_init((SEM_MUX *)h_mutex))
#define mutex_destroy(h_mutex)            /* DfewOS not support */;

#endif                                                                  /*  __LINUX_MUTEX_H             */
