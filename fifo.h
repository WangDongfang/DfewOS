/*==============================================================================
** fifo.h -- fifo header.
**
** MODIFY HISTORY:
**
** 2011-10-20 wdf Create.
==============================================================================*/

#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================================
  The FIFO struct
======================================================================*/
typedef struct fifo {
    uint32 out; /* point to the place can fetch a old element */
    uint32 in;  /* point to the place can store a new element */

    uint32 max_num;
    uint32 ele_size;
    void  *data;
} FIFO;

/*======================================================================
  FIFO module support routines for other modules
======================================================================*/
FIFO*     fifo_create (uint32 max_num, uint32 ele_size);
BOOL      fifo_is_empty (const FIFO *pFifo);
BOOL      fifo_is_full (const FIFO *pFifo);
OS_STATUS fifo_put (FIFO *pFifo, const void *pElement);
OS_STATUS fifo_get (FIFO *pFifo, void *pElement);
OS_STATUS fifo_head (FIFO *pFifo, void *pElement);
void      fifo_delete (FIFO *pFifo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIFO_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

