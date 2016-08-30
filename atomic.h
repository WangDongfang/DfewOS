/*==============================================================================
** atomic.h -- atomic library interface.
**
** MODIFY HISTORY:
**
** 2012-03-21 wdf Create.
==============================================================================*/

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  atomic type
======================================================================*/
typedef int atomic_t;

/*======================================================================
  atomic module support routines for other modules
======================================================================*/
atomic_t atomic_add (atomic_t *target, atomic_t value);
atomic_t atomic_sub (atomic_t *target, atomic_t value);
atomic_t atomic_inc (atomic_t *target);
atomic_t atomic_dec (atomic_t *target);
atomic_t atomic_set (atomic_t *target, atomic_t value);
atomic_t atomic_get (atomic_t *target);
atomic_t atomic_clear (atomic_t *target);
BOOL atomic_cas (atomic_t *target, atomic_t old_value, atomic_t new_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ATOMIC_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

