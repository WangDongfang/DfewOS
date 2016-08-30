/*==============================================================================
** semM.h -- the semaphore mutex module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __SEMM_H__
#define __SEMM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  the semM struct
======================================================================*/
typedef struct sem_mutex {
    DL_NODE  semM_list_node;

    OS_TCB  *owner_tcb;
    uint32   take_count;

    /* 
     * wait for this sem's highest priority,
     * owner task's current priority maybe
     * higher than this(because the owner
     * maybe take other semM or 
     * the owner task's origin_pri)
     */
    uint32   wait_pri;

    DL_LIST  wait_list;
} SEM_MUX;

/*======================================================================
  semM module support routines for other modules
======================================================================*/
SEM_MUX  *semM_init (SEM_MUX *pNewSemB);
OS_STATUS semM_take (SEM_MUX *pSemB, uint32 timeout);
OS_STATUS semM_give (SEM_MUX *pSemB);
void      semM_decrease ();
void      semM_free (OS_TCB *pTcb);
OS_STATUS semM_remove (SEM_MUX *pSemM, OS_TCB *pTcb);
void      semM_show (EACH_FUNC_PTR show_routine);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SEMM_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

