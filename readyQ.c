/*==============================================================================
** readyQ.c -- the ready queue module.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "dlist.h"
#include "task.h"

#define PRIORITY_NUM_MAX  256

/*

when remove a task, we should find the highest priority tcb, 
In this algorithm, We use BIT map to achieve it:
We let 32 priorities is a GROUP,
A GROUP use a bMap element MAP which priority have node(s),
Fianlly, use metaBMap MAP which bMap element is not 0, the correspond bit is set 1.
 
The follow graph assume the PRIORITY_NUM_MAX is 256.

metaBMap [ (0)   (1)   (2)   (3)   (4)   (5)   (6)   (7) .. (31)]             --- map 8 level
          [0|1] [0|1] [0|1] [0|1] [0|1] [0|1] [0|1] [0|1]
             \___ \______________________   ....       \__________
                |                       |      ....              |
bMap           [0]                     [1]          ....        [7]           --- map 32 level
     [(0) (1) (2) ... (31)]   [(0) (1) (2) ... (31)]   [(0) (1) (2) ... (31)]
       |   |   |______..|____   |    ....        |       |    ....        |
       |   \_____    | ..   |    \   ....       /       /   ....          |
       -----    |    | ..   |     |  ...  +----+       +  ....  +---------+
           |    |    | ..   |     |  ..   |            |  ....  |
           V    V    V      V     V       V            V        V
listArray [0]  [1]  [2] .. [31]  [32] .. [63] ...... [224] .. [255]           --- TCB lists
           #                #                                   #
          TCB              TCB                                 TCB
           #
          TCB
           #
          TCB

*/
/*======================================================================
  the ready queue struct, only used in this file
======================================================================*/
typedef struct ready_queue {
    OS_TCB  *highest_tcb;

    uint32   metaBMap;
    uint32   bMap[PRIORITY_NUM_MAX / 32];
    DL_LIST  listArray[PRIORITY_NUM_MAX];
} READY_QUEUE;


/*======================================================================
  extern function
======================================================================*/
extern int LSB (uint32 bit_map);

/*======================================================================
  forward functions declare
======================================================================*/
static int _readQ_highest_pri();

/*======================================================================
  the only ready queue, used by this file and scheduler.S
======================================================================*/
READY_QUEUE G_readyQ;

/*==============================================================================
 * - readyQ_init()
 *
 * - init the readyQ struct. called by system init routine
 */
void readyQ_init ()
{
    int i = 0;
    for (; i < PRIORITY_NUM_MAX; i++) {
        dlist_init (&G_readyQ.listArray[i]);
    }
}

/*==============================================================================
 * - readyQ_put()
 *
 * - put a tcb in the correspond pri list, fresh "highest_tcb","metaBMap","bMap"
 */
void readyQ_put (OS_TCB *pTcb)
{
    int priority = pTcb->task_priority;

    /*
     * update highest task
     */
    if ((G_readyQ.highest_tcb == NULL) ||
    	(priority < G_readyQ.highest_tcb->task_priority)) {
        G_readyQ.highest_tcb = pTcb;
    }

    G_readyQ.metaBMap            |= (1 << (priority >> 5));
    G_readyQ.bMap[priority >> 5] |= (1 << (priority & 0x1f));
    
    pTcb->status = TASK_STATUS_READY;
    pTcb->pend_obj = NULL;
    dlist_add (&G_readyQ.listArray[priority], (DL_NODE *)pTcb);
}

/*==============================================================================
 * - readyQ_remove()
 *
 * - del a tcb in the correspond pri list, fresh "highest_tcb","metaBMap","bMap"
 */
void readyQ_remove (OS_TCB *pTcb)
{
    int priority = pTcb->task_priority;
    DL_LIST *pList = &G_readyQ.listArray[priority];

    dlist_remove(pList, (DL_NODE *)pTcb);

    if (DL_EMPTY(pList)) {  /* this priority level list is empty */

        /*
         * clear this priority maped bit
         */
        G_readyQ.bMap [priority >> 5] &= ~(1 << (priority & 0x1f));

        if (G_readyQ.bMap[priority >> 5] == 0) {
            G_readyQ.metaBMap &= ~(1 << (priority >> 5));
        }

        if (pTcb == G_readyQ.highest_tcb) {
            G_readyQ.highest_tcb = (OS_TCB *)DL_FIRST(&G_readyQ.listArray[_readQ_highest_pri()]);
        }

    } else if (pTcb == G_readyQ.highest_tcb) { /* this priority have another one */
        G_readyQ.highest_tcb = (OS_TCB *)DL_FIRST(pList);
    }
}

/*==============================================================================
 * - _readQ_highest_pri()
 *
 * - get the highest priority:[0, PRIORITY_NUM_MAX-1], called by readyQ_remove()
 */
static int _readQ_highest_pri()
{
    uint32 highBits = LSB(G_readyQ.metaBMap);
    uint32 lowBits;

    lowBits = LSB(G_readyQ.bMap[highBits]);

    return ((highBits << 5) | lowBits);
}

/*==============================================================================
 * - readyQ_show()
 *
 * - show the task brief which status: TASK_STATUS_READY
 */
void readyQ_show(EACH_FUNC_PTR show_routine)
{
    int i = 0;
    for (; i < PRIORITY_NUM_MAX; i++) {
        dlist_each (&G_readyQ.listArray[i], show_routine, 0);
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

