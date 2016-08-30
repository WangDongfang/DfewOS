/*==============================================================================
** readyQ.h -- the ready queue module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __READYQ_H__
#define __READYQ_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  readyQ module support routines for other modules
======================================================================*/
void readyQ_init();
void readyQ_put(OS_TCB *pTcb);
void readyQ_remove(OS_TCB *pTcb);
void readyQ_show(EACH_FUNC_PTR show_routine);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __READYQ_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

