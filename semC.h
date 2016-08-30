/*==============================================================================
** semC.h -- the semaphore count module header.
**
** MODIFY HISTORY:
**
** 2011-08-23 wdf Create.
==============================================================================*/

#ifndef __SEMC_H__
#define __SEMC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  the semC struct
======================================================================*/
typedef struct sem_count {
    DL_NODE  semC_list_node;

    uint32   count;
    uint32   max_count;
    DL_LIST  wait_list;
} SEM_CNT;

/*======================================================================
  semC module support routines for other modules
======================================================================*/
SEM_CNT  *semC_init (SEM_CNT *pNewSemC, uint32 init_count, uint32 max_count);
OS_STATUS semC_take (SEM_CNT *pSemC, uint32 timeout);
OS_STATUS semC_give (SEM_CNT *pSemC);
OS_STATUS semC_flush (SEM_CNT *pSemC);
OS_STATUS semC_delete (SEM_CNT *pSemC);
OS_STATUS semC_remove (SEM_CNT *pSemC, OS_TCB *pTcb);
void      semC_decrease ();
void      semC_show (EACH_FUNC_PTR show_routine);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SEMC_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

