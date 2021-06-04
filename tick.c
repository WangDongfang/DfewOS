/*==============================================================================
** tick.c -- the tick module.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "dlist.h"
#include "task.h"
#include "readyQ.h"
#include "delayQ.h"
#include "msgQ.h"
#include "semB.h"
#include "semC.h"
#include "semM.h"

/*======================================================================
  the ticks from system start
======================================================================*/
static uint32  _G_ticks = 0;

/*==============================================================================
 * - tick_announce()
 *
 * - in every tick interrupt isr call this function
 */
void tick_announce()
{
	_G_ticks++;

    /*
     * put semaphore pend timeout task, 
     * delay awake task to readyQ
     */
    semB_decrease ();
    semC_decrease ();
    semM_decrease ();
    msgQ_decrease ();
    delayQ_decrease ();

    /*
     * rotate the same priority tasks
     * put now task to end
     */
    readyQ_remove(G_p_current_tcb);
    readyQ_put(G_p_current_tcb);
}

/*==============================================================================
 * - tick_get()
 *
 * - return the ticks from system start
 */
int tick_get()
{
	return _G_ticks;
}

/*==============================================================================
** FILE END
==============================================================================*/

