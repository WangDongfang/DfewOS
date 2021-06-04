/*==============================================================================
** memH.c -- memory heap manage module.
**
** MODIFY HISTORY:
**
** 2011-09-02 wdf Create.(First Fit Algorithm).
** 2011-09-03 luo Fix a big bug in malloc().
** 2011-11-09 clean up free().
==============================================================================*/
#include "config.h"
#include "types.h"
#include "dlist.h"
#include "task.h"
#include "semM.h"
#include "serial.h"
#include "string.h"
int sscanf(const char *str, char const *fmt, ...);

/*======================================================================
  memory module config
======================================================================*/
#define CONFIG_HEAP_START       CONFIG_MEM_HEAP_LOW
#define CONFIG_HEAP_SIZE        CONFIG_MEM_HEAP_SIZE

/*======================================================================
  heap info struct
======================================================================*/
typedef struct heap_info {
    uint32  heap_start;
    uint32  heap_size;

    uint32  block_num;
    uint32  alloced_num;
    uint32  alloced_size;
    uint32  free_num;
    uint32  free_size;
    uint32  max_free_block_size;
} HEAP_INFO;

/*======================================================================
  block head struct
======================================================================*/
typedef struct _block_head {
    struct _block_head  *prev;
    struct _block_head  *next;

    uint32    size;
    uint32    is_alloced;

    OS_TCB   *owner_tcb;
} _BLOCK_HEAD;

/*======================================================================
  global variables
======================================================================*/
static HEAP_INFO _G_sys_heap;

#ifdef USE_SEM_LOCK
static SEM_MUX   _G_mem_semM;
#define MEM_MANAGE_LOCK()    semM_take(&_G_mem_semM, WAIT_FOREVER)
#define MEM_MANAGE_UNLOCK()  semM_give(&_G_mem_semM)
#else
static int _G_cpsr_c;
#define MEM_MANAGE_LOCK()    _G_cpsr_c = CPU_LOCK()
#define MEM_MANAGE_UNLOCK()  CPU_UNLOCK(_G_cpsr_c)
#endif

/*==============================================================================
 * - heap_init()
 *
 * - init the only one block and heap info
 */
void heap_init ()
{
    _BLOCK_HEAD *p_block_head = NULL;

    /*
     * init heap info struct
     */
    _G_sys_heap.heap_start = CONFIG_HEAP_START;
    _G_sys_heap.heap_size  = CONFIG_HEAP_SIZE;

    _G_sys_heap.block_num    = 1;
    _G_sys_heap.alloced_num  = 0;
    _G_sys_heap.alloced_size = 0;
    _G_sys_heap.free_num     = 1;
    _G_sys_heap.free_size    = CONFIG_HEAP_SIZE - sizeof (_BLOCK_HEAD);
    _G_sys_heap.max_free_block_size = CONFIG_HEAP_SIZE - sizeof (_BLOCK_HEAD);

    /*
     * init the only one block's head
     */
    p_block_head = (_BLOCK_HEAD *)_G_sys_heap.heap_start;

    p_block_head->prev = NULL;
    p_block_head->next = NULL;
    p_block_head->is_alloced = 0;
    p_block_head->owner_tcb  = NULL;
    p_block_head->size = CONFIG_HEAP_SIZE - sizeof (_BLOCK_HEAD);

#ifdef USE_SEM_LOCK
    /*
     * init mutex semaphore
     */
    semM_init(&_G_mem_semM);
#endif
}

/*==============================================================================
 * - malloc()
 *
 * - malloc. alloc <nbytes> or <nbytes> + sizeof(_BLOCK_HEAD) memory
 */
void *malloc (size_t nbytes)
{
    _BLOCK_HEAD *p = (_BLOCK_HEAD *)_G_sys_heap.heap_start;
    _BLOCK_HEAD *new_p = NULL;
    int  need_size;  /* usr_bytes + align_bytes + block_head */

    nbytes = (nbytes + (DEFAULT_ALIGN_BYTE - 1)) & (~(DEFAULT_ALIGN_BYTE - 1)); /* align */

    MEM_MANAGE_LOCK();

    /*
     * find a free block which size big or equal <nbytes>
     */
    while ((p->is_alloced) || (p->size < nbytes)) {
        p = p->next;
        if (p == NULL) {
            MEM_MANAGE_UNLOCK();
            FOREVER {}
            return NULL;
        }
    }

    need_size = nbytes + sizeof (_BLOCK_HEAD);

    /*
     * change block chain and <_G_sys_heap> info
     */
    if ((int)(p->size - need_size) >= DEFAULT_ALIGN_BYTE) {       /* create a new block */
        /*
         * create a block and fill it's head
         */
        new_p = (_BLOCK_HEAD *)((uint32)p + need_size);
        new_p->prev = p;
        new_p->next = p->next;
        new_p->size = p->size - need_size;
        new_p->is_alloced = 0;
        new_p->owner_tcb  = NULL;

        /*
         * update the head of block next to new_p
         */
        if (new_p->next != NULL) {
            new_p->next->prev = new_p;
        }

        /*
         * update old block's head
         */
        p->next = new_p;
        p->size = nbytes;

        /*
         * update heap info
         */
        _G_sys_heap.block_num++;
        _G_sys_heap.free_size -= need_size;
    } else {
        _G_sys_heap.free_num--;
        _G_sys_heap.free_size -= p->size;
    }
    p->is_alloced = 1;
    p->owner_tcb  = G_p_current_tcb;
    _G_sys_heap.alloced_num++;
    _G_sys_heap.alloced_size += p->size;

    MEM_MANAGE_UNLOCK();

    return (void *)(p + 1);
}

/*==============================================================================
 * - calloc()
 *
 * - malloc <elem_num> * <elem_size> memory and initial to zero
 */
void *calloc (size_t elem_num, size_t elem_size)
{
    void  *p;
    size_t nbytes = elem_num * elem_size;

    p = malloc (nbytes);

    if (p != NULL) {
        memset (p, 0, nbytes);
    }

    return p;
}

#define CAN_MERGE(p)  (((p != NULL) && (!p->is_alloced)) ? 1 : 0)
/*==============================================================================
 * - free()
 *
 * - free a block. if <usr_p> is't from malloc, do nothing
 */
void free (void *usr_p)
{
    _BLOCK_HEAD *p = (_BLOCK_HEAD *)_G_sys_heap.heap_start;

    /*
     * check whether <usr_p> in heap
     */
    if (usr_p < (void *)_G_sys_heap.heap_start ||
        (usr_p >= (void *)(_G_sys_heap.heap_start + _G_sys_heap.heap_size))) {
        return;
    }

    MEM_MANAGE_LOCK();
    /*
     * find free which block
     */
    while (p != usr_p - sizeof (_BLOCK_HEAD)) {
        p = p->next;
        if (p == NULL) {
            MEM_MANAGE_UNLOCK();
            return; /* the usr_p point memory is not in heap list */
        }
    }

    _G_sys_heap.alloced_size -= p->size;
    _G_sys_heap.alloced_num--;
    _G_sys_heap.free_size += p->size; /* we may change it follow */

    /*
     * merge blocks if need
     */
    if (!CAN_MERGE(p->prev) && !CAN_MERGE(p->next)) {      /* prev alloced, next alloced : no_merge */
        p->is_alloced = 0;
        p->owner_tcb  = NULL;

        _G_sys_heap.free_num++;
    } else if (CAN_MERGE(p->prev) && !CAN_MERGE(p->next)) { /* prev free, next alloced : merge_prev */
        p->prev->next = p->next;
        p->prev->size += p->size + sizeof (_BLOCK_HEAD);

        if (p->next != NULL)
            p->next->prev = p->prev;

        _G_sys_heap.block_num--;
        _G_sys_heap.free_size += sizeof (_BLOCK_HEAD);
    } else if (!CAN_MERGE(p->prev) && CAN_MERGE(p->next)) { /* prev alloced, next free : merge_next */
        p->size += p->next->size + sizeof (_BLOCK_HEAD);
        p->next = p->next->next;
        p->is_alloced = 0;
        p->owner_tcb  = NULL;

        if (p->next != NULL)
            p->next->prev = p;

        _G_sys_heap.block_num--;
        _G_sys_heap.free_size += sizeof (_BLOCK_HEAD);
    } else {                                              /* prev free, next free : merge_prev_next */
        p->prev->next = p->next->next;
        p->prev->size += p->size + p->next->size + (2 * sizeof (_BLOCK_HEAD));

        if (p->next->next != NULL)
            p->next->next->prev = p->prev;

        _G_sys_heap.free_num--;
        _G_sys_heap.block_num -= 2;
        _G_sys_heap.free_size += 2 * sizeof (_BLOCK_HEAD);
    }

    MEM_MANAGE_UNLOCK();

    return;
}

/*==============================================================================
 * - realloc()
 *
 * - reallocate a block of memory (ANSI)
 *   <pBlock> block to reallocate
 *   <newSize> new block size
 */
void *realloc (void *pBlock, size_t newSize)
{
    void *pNewBlock;
    
    if (newSize == 0 ) {
        free (pBlock);
        return (NULL);
    } else {
        pNewBlock = malloc (newSize);
    
        if (pBlock != NULL && pNewBlock != NULL) {
            bcopy((const char *) pBlock, (char *) pNewBlock, newSize);
            free (pBlock);
        }
    }

    return pNewBlock;
}

/*==============================================================================
 * - memH_free()
 *
 * - when delete a task, free this task alloc memory in heap
 */
int memH_free (OS_TCB *pTcb)
{
    _BLOCK_HEAD *p = (_BLOCK_HEAD *)_G_sys_heap.heap_start;
    int still_have = 1;

    while (still_have) {

        /* 
         * assume this loop can't find
         * a block that alloc by <pTcb>
         */
        still_have = 0;  

        MEM_MANAGE_LOCK();
        p = (_BLOCK_HEAD *)_G_sys_heap.heap_start;
        while (p != NULL) {

            /* find one */
            if (p->owner_tcb == pTcb) {
                still_have = 1;
                free (p + 1);
                break;
            }

            p = p->next;
        }
        MEM_MANAGE_UNLOCK();
    }

    return 0;

}

/*==============================================================================
 * - h()
 *
 * - the "h" command operat function. show sys heap infomation
 *   -> h                show memory briefly
 *   -> h -l             show all task alloced memory detail
 *   -> h -l 0x30010000  show task 0x30010000 alloced memory detail
 */
int h (int argc, char **argv)
{
    _BLOCK_HEAD *p = (_BLOCK_HEAD *)_G_sys_heap.heap_start;
    OS_TCB   *pTcb = NULL;

    serial_printf("  block_num : %3d\n"
                  "alloced_num : %3d  allock_size : %u\n"
                  "   free_num : %3d    free_size : %u\n",
                  _G_sys_heap.block_num,
                  _G_sys_heap.alloced_num,
                  _G_sys_heap.alloced_size,
                  _G_sys_heap.free_num,
                  _G_sys_heap.free_size);

    if ((argc >= 2) && (strcmp(argv[1], "-l") == 0)) {
        serial_printf("-----------  -------  ---------  ----------\n"
                      "   START     ALLOCED     TCB        SIZE\n"
                      "-----------  -------  ---------  ----------\n");
        if (argc >= 3) {
           sscanf (argv[2], "0x%X", (unsigned int *)&pTcb);
        }
        while (p != NULL) {
            if (pTcb == NULL || p->owner_tcb == pTcb) {
                serial_printf("0x%8X      %d     %9X   %9u\n",
                        p, p->is_alloced, p->owner_tcb, p->size);
            }
            p = p->next;
        }
    }

    return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

