/*==============================================================================
** fifo.c -- looped queue, using array implement it.
**
** ----------------------
** Empty: in == out
** ----------------------
** out in
**   |/
**   V
** |  |  |  |  |  |  |  |
** ----------------------
** 
** ----------------------
** Full: (in+1)%NUM == out
** ----------------------
**     in out
**      |  |
**      V  V
** |aa|  |aa|aa|aa|aa|aa|
** ----------------------
** 
** ----------------------
** Normal:
** ----------------------
**     out         in
**      |           |
**      V           V
** |  |aa|aa|aa|aa|  |  |
** ----------------------
** MODIFY HISTORY:
**
** 2011-10-20 wdf Create.
==============================================================================*/
#include "config.h"
#include "types.h"
#include "string.h"
#include "fifo.h"

#include "memH.h"

/*==============================================================================
 * - fifo_create()
 *
 * - create a FIFO which can max hold <max_num> elemnets
 */
FIFO* fifo_create (uint32 max_num, uint32 ele_size)
{
	FIFO *pFifo = NULL;

    if (max_num == 0 || ele_size == 0) {
        return NULL;
    }

    pFifo = (FIFO *)malloc (sizeof(FIFO) + ((max_num+1) * ele_size));

    if (pFifo != NULL) {
        pFifo->max_num  = max_num;
        pFifo->ele_size = ele_size;
        pFifo->out      = 0;
        pFifo->in       = 0;
        pFifo->data     = pFifo + 1;
    }

    return pFifo;
}

/*==============================================================================
 * - fifo_is_empty()
 *
 * - check whether the FIFO is empty
 */
BOOL fifo_is_empty (const FIFO *pFifo)
{
    return (pFifo->in == pFifo->out);
}

/*==============================================================================
 * - fifo_is_full()
 *
 * - check whether the FIFO is full
 */
BOOL fifo_is_full (const FIFO *pFifo)
{
    return ((pFifo->in + 1) % (pFifo->ele_size) == pFifo->out);
}

/*==============================================================================
 * - fifo_put()
 *
 * - try to put a element into the FIFO
 */
OS_STATUS fifo_put (FIFO *pFifo, const void *pElement)
{
    int cpsr_c;
    OS_STATUS status = OS_STATUS_ERROR;

    cpsr_c = CPU_LOCK();
    if (!fifo_is_full (pFifo)) { /* not full */
        memcpy (pFifo->data + (pFifo->in * pFifo->ele_size),
                pElement,
                pFifo->ele_size);
        pFifo->in = (pFifo->in + 1) % (pFifo->ele_size);

        status = OS_STATUS_OK;
    }
    CPU_UNLOCK(cpsr_c);

    return status;
}

/*==============================================================================
 * - fifo_get()
 *
 * - try to get and delete a element from the FIFO
 */
OS_STATUS fifo_get (FIFO *pFifo, void *pElement)
{
    int cpsr_c;
    OS_STATUS status = OS_STATUS_ERROR;

    cpsr_c = CPU_LOCK();
    if (!fifo_is_empty (pFifo)) { /* not empty */
        memcpy (pElement,
                pFifo->data + (pFifo->out * pFifo->ele_size),
                pFifo->ele_size);
        pFifo->out = (pFifo->out + 1) % (pFifo->ele_size);

        status = OS_STATUS_OK;
    }
    CPU_UNLOCK(cpsr_c);

    return status;
}

/*==============================================================================
 * - fifo_head()
 *
 * - try to read the first element in the FIFO
 */
OS_STATUS fifo_head (FIFO *pFifo, void *pElement)
{
    int cpsr_c;
    OS_STATUS status = OS_STATUS_ERROR;

    cpsr_c = CPU_LOCK();
    if (!fifo_is_empty (pFifo)) { /* not empty */
        memcpy (pElement,
                pFifo->data + (pFifo->out * pFifo->ele_size),
                pFifo->ele_size);

        status = OS_STATUS_OK;
    }
    CPU_UNLOCK(cpsr_c);

    return status;
}

/*==============================================================================
 * - fifo_delete()
 *
 * - delete a FIFO
 */
void fifo_delete (FIFO *pFifo)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();
    free (pFifo);
    CPU_UNLOCK(cpsr_c);
}

/*==============================================================================
** FILE END
==============================================================================*/

