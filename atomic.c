/*==============================================================================
** atomic.c -- atomic operations library.
**
** MODIFY HISTORY:
**
** 2012-03-21 wdf Create.
==============================================================================*/

#include "config.h"
#include "types.h"
#include "atomic.h"

/*==============================================================================
 * - atomic_add()
 *
 * - atomically add a value to a atomic variable
 */
atomic_t atomic_add (atomic_t *target, atomic_t value)
{
    atomic_t old_value;
    int      cpsr_c;

    cpsr_c = CPU_LOCK();
    old_value = *(volatile atomic_t *)target;
    *(volatile atomic_t *)target = old_value + value;
    CPU_UNLOCK(cpsr_c);

    return (old_value);
}

/*==============================================================================
 * - atomic_sub()
 *
 * - atomically subtract a value from a atomic variable
 */
atomic_t atomic_sub (atomic_t *target, atomic_t value)
{
    atomic_t old_value;
    int      cpsr_c;

    cpsr_c = CPU_LOCK();
    old_value = *(volatile atomic_t *)target;
    *(volatile atomic_t *)target = old_value - value;
    CPU_UNLOCK(cpsr_c);

    return (old_value);
}

/*==============================================================================
 * - atomic_inc()
 *
 * - atomically increment a atomic variable
 */
atomic_t atomic_inc (atomic_t *target)
{
    return (atomic_add (target, 1));
}

/*==============================================================================
 * - atomic_dec()
 *
 * - atomically decrement a atomic variable
 */
atomic_t atomic_dec (atomic_t *target)
{
    return (atomic_sub (target, 1));
}

/*==============================================================================
 * - atomic_set()
 *
 * - atomically set a atomic variable
 */
atomic_t atomic_set (atomic_t *target, atomic_t value)
{
    atomic_t oldValue;
    int      cpsr_c;

    cpsr_c = CPU_LOCK();
    oldValue = *(volatile atomic_t *)target;
    *(volatile atomic_t *)target = value;
    CPU_UNLOCK(cpsr_c);

    return (oldValue);
}

/*==============================================================================
 * - atomic_get()
 *
 * - atomically get a atomic variable
 */
atomic_t atomic_get (atomic_t *target)
{
    return *(volatile atomic_t *)target;
}

/*==============================================================================
 * - atomic_clear()
 *
 * - atomically set a atomic variable to 0
 */
atomic_t atomic_clear (atomic_t *target)
{
    return atomic_set (target, 0);
}

/*==============================================================================
 * - atomic_cas()
 *
 * - atomically compare-and-swap the contents of a atomic variable
 *   Testing that <*target> contains <old_value>, and if it does,
 *   setting the value of <*target> to <new_value>
 *
 * - <old_value>: compare to this value
 *   <new_value>: swap with this value
 */
BOOL atomic_cas (atomic_t *target, atomic_t old_value, atomic_t new_value)
{
    BOOL swapped = FALSE;
    int  cpsr_c;

    cpsr_c = CPU_LOCK();
    if (*(volatile atomic_t *)target == old_value) {
        *(volatile atomic_t *)target = new_value;
        swapped = TRUE;
    }
    CPU_UNLOCK(cpsr_c);

    return (swapped);
}

/*==============================================================================
** FILE END
==============================================================================*/

