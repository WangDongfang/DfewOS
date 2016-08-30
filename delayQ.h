/*==============================================================================
** delayQ.h -- the delay queue module header.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __DELAYQ_H__
#define __DELAYQ_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  delayQ module supports routines for other modules
======================================================================*/
void delayQ_decrease();
void delayQ_delay(uint32 ticks);
void delayQ_show(EACH_FUNC_PTR show_routine);
OS_STATUS delayQ_remove (OS_TCB *pTcb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DELAYQ_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

