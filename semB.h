/*==============================================================================
** semB.h -- the semaphore binary module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __SEMB_H__
#define __SEMB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  the semB struct
======================================================================*/
typedef struct sem_binary {
    DL_NODE  semB_list_node;

    OS_TCB  *owner_tcb;
    DL_LIST  wait_list;
} SEM_BIN;

/*======================================================================
  semB module support routines for other modules
======================================================================*/
SEM_BIN  *semB_init (SEM_BIN *pNewSemB);
OS_STATUS semB_take (SEM_BIN *pSemB, uint32 timeout);
OS_STATUS semB_give (SEM_BIN *pSemB);
OS_STATUS semB_flush (SEM_BIN *pSemB);
OS_STATUS semB_delete (SEM_BIN *pSemB);
OS_STATUS semB_remove (SEM_BIN *pSemB, OS_TCB *pTcb);
void      semB_decrease ();
void      semB_show (EACH_FUNC_PTR show_routine);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SEMB_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

