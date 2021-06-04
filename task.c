/*==============================================================================
** task.c -- task module.
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
#include "semB.h"
#include "semC.h"
#include "semM.h"
#include "msgQ.h"
#include "serial.h"
#include "string.h"

#include "memH.h"

/*======================================================================
  config
======================================================================*/
/* #define CONFIG_STACK_IN_HEAP */

/*======================================================================
  extern function
======================================================================*/
extern int memH_free (OS_TCB *pTcb);

/*======================================================================
  the global variables
======================================================================*/
uint32  G_Task_Num = 0;
OS_TCB *G_p_current_tcb = NULL;

/*======================================================================
  this file used global variables
======================================================================*/
static OS_TCB  _G_tcb[TASK_NUM_MAX];    /* TCB array block */
static uint32  _G_tcb_map[(TASK_NUM_MAX + 31) / 32]; /* 1: available, 0: not available */

/*======================================================================
  task stack memory manage
======================================================================*/
/*
                     8 uint32 maped
        128K                                 128K
  (0)   ...   (31)        ...          (0)   ...   (31)
  / \         /  \                     / \         /  \
 |4K |  ...  | 4K |      .....        |4K |  ...  | 4K |

                  1 MB task stack memory
*/
/* 1: available, 0: not available */
static uint32  _G_stack_map[(CONFIG_MEM_STACK_PAGE_NUM + 31) / 32];

/*======================================================================
  forward functions declare
======================================================================*/
static void _task_entry (OS_TCB *pTcb);

/*==============================================================================
 * - task_init()
 *
 * - set <_G_tcb_map> to 0xff, make the tcb resource available
 *   set <_G_stack_map> to 0xff, make the tab resource available
 */
void task_init ()
{
    int i;
    int idle_num = 0;

    /* make the tcb available */
    memset(_G_tcb_map, 0xff, sizeof (_G_tcb_map));

    idle_num = N_ELEMENTS(_G_tcb_map) * 32 - TASK_NUM_MAX;
    for (i = 0; i < idle_num; i++) {
        BIT_CLEAR(_G_tcb_map[(TASK_NUM_MAX + 31) / 32 - 1], (31 - i));
    }

    /* make the stack memory available */
    memset(_G_stack_map, 0xff, sizeof (_G_stack_map));
}

/*==============================================================================
 * - LSB()
 *
 * - find the first bit 1 position in <bit_map> binary bits
 *
 * - RETURN:
 *   [0 - 31]: the lowest 1 position
 *         -1: there is none 1
 */
int LSB (uint32 bit_map)
{
    int i = 0;

    if (bit_map == 0)
        return -1;

    while ((bit_map & 1) == 0) {
        bit_map >>= 1;
        i++;
    }

    return i;
}

/*==============================================================================
 * - _alloc_tcb()
 *
 * - inner TCB alloc
 */
static OS_TCB *_alloc_tcb ()
{
#ifdef CONFIG_TCB_IN_HEAP
    return malloc(sizeof (OS_TCB));
#else
    int cpsr_c;
    int index_tcb = 0;          /* the index for _G_tcb[] array */
    int index_tcb_map = 0;      /* the index for _G_tcb_map[] array */
    int lowest_available = -1;

    cpsr_c = CPU_LOCK();
    /* find the first available tcb */
    for (; index_tcb_map < (TASK_NUM_MAX + 31) / 32; index_tcb_map++) {
        lowest_available = LSB(_G_tcb_map[index_tcb_map]);
        if (lowest_available != -1) {
            index_tcb += lowest_available;
            break;
        }
        index_tcb += 32;
    }

    if (lowest_available == -1) {
        CPU_UNLOCK(cpsr_c);
        return NULL;
    } else {
        BIT_CLEAR(_G_tcb_map[index_tcb_map], lowest_available);

        CPU_UNLOCK(cpsr_c);
        return &_G_tcb[index_tcb];
    }
#endif
}

/*==============================================================================
 * - _release_tcb()
 *
 * - inner TCB release
 */
static void _release_tcb (OS_TCB *pTcb)
{
#ifdef CONFIG_TCB_IN_HEAP
    free (pTcb);
#else
    int cpsr_c;
    int index_tcb_map = (pTcb - (&_G_tcb[0])) >> 5;
    int index_map_bit = (pTcb - (&_G_tcb[0])) & 0x1f;

    cpsr_c = CPU_LOCK();
    BIT_SET(_G_tcb_map[index_tcb_map], index_map_bit);
    CPU_UNLOCK(cpsr_c);
#endif
}

/*==============================================================================
 * - _alloc_stack()
 *
 * - inner task stack alloc. return stack top
 */
static uint32 _alloc_stack (uint32 stack_size)
{
#ifdef CONFIG_STACK_IN_HEAP
    void *p = malloc(stack_size);
    if (p != NULL) {
        memset (p, 0xcc, stack_size);
        return (uint32)(p + stack_size);
    } else {
        return 0;
    }
#else
    uint32 p = 0;
    int cpsr_c;
    int need_stack_pages = (stack_size + CONFIG_MEM_STACK_PAGE_SIZE - 1) / 
                                                CONFIG_MEM_STACK_PAGE_SIZE;
    int current_page = 0;
    int matched_stack_pages = 0;

    cpsr_c = CPU_LOCK();
    while (current_page + need_stack_pages <= CONFIG_MEM_STACK_PAGE_NUM) {
        matched_stack_pages = 0;

        /* try to make matched_stack_pages == need_stack_pages */
        while (matched_stack_pages != need_stack_pages) {

            /* this bit is not available, start a new check */
            if (BIT_VALUE(_G_stack_map[current_page >> 5], current_page & 0x1F) == 0) { 
                current_page++;
                break;
            }

            /* find a free STACK_PAGE */
            matched_stack_pages++;

            /* this bit is available, prepare for check next bit */
            current_page++;
        }
        if (matched_stack_pages == need_stack_pages) {
            break;
        }
    } /* while () */

    if (matched_stack_pages == need_stack_pages) {  /* successful to alloc stack */

        /* set the memory to "0xcc" for stack uesed check */
        p = CONFIG_MEM_STACK_LOW + current_page * CONFIG_MEM_STACK_PAGE_SIZE;
        memset ((void *)(p - stack_size), 0xcc, stack_size);

        /* set to '0' make the page are not available */
        while (need_stack_pages-- > 0) {
            current_page--;
            BIT_CLEAR(_G_stack_map[current_page >> 5], current_page & 0x1F);
        }

    }
    CPU_UNLOCK(cpsr_c);

    return (p);
#endif
}

/*==============================================================================
 * - _release_stack()
 *
 * - inner task stack release
 */
static void _release_stack (OS_TCB *pTcb)
{
#ifdef CONFIG_STACK_IN_HEAP
    free((void *)(pTcb->stack_start - pTcb->stack_size));
#else
    int cpsr_c;
    int need_stack_pages = (pTcb->stack_size + CONFIG_MEM_STACK_PAGE_SIZE - 1) / 
                                                CONFIG_MEM_STACK_PAGE_SIZE;
    int top_page = (pTcb->stack_start - CONFIG_MEM_STACK_LOW) / CONFIG_MEM_STACK_PAGE_SIZE;
    
    /* set to '1' make the page are available */
    cpsr_c = CPU_LOCK();
    while (need_stack_pages-- > 0) {
        top_page--;
        BIT_SET(_G_stack_map[top_page >> 5], top_page & 0x1F);
    }
    CPU_UNLOCK(cpsr_c);
#endif
}

/*==============================================================================
 * - _stack_used_percent()
 *
 * - calculate the percent of task stack have used
 *   return value: [1, 101]
 */
static int _stack_used_percent (OS_TCB *pTcb)
{
#if 1
    /* Binary Search */
    uint32 *low = (uint32 *)(pTcb->stack_start - pTcb->stack_size);
    uint32 *high = (uint32 *)(pTcb->stack_start - 4);
    uint32 *mid = low + (high - low) / 2;
    while (high > low) {
        if (*mid == 0xcccccccc) {
            low = mid + 1;
        } else {
            high = mid;
        }
        mid = low + (high - low) / 2;
    }
    /* now <low> <mid> <high> point to the first integer which != 0xcccccccc */
    return 1 + ((pTcb->stack_start - (uint32)mid) * 100) / pTcb->stack_size;
#else
    uint8 *low = (uint8 *)(pTcb->stack_start - pTcb->stack_size);
    while (*low++ == 0xcc)
        ;
    low--;    /* <low> point to the first char which != 0xcc */

    return 1 + ((pTcb->stack_start - (uint32)low) * 100) / pTcb->stack_size;
#endif
}

/*==============================================================================
 * - task_create()
 *
 * - alloc the TCB and STACK, then push a default context frame to the stack, for load.
 */
OS_TCB *task_create(const char *task_name,
                    uint32      stack_size,
                    uint32      task_priority,
                    FUNC_PTR    task_entry,
                    uint32      arg1,
                    uint32      arg2)
{
    int cpsr_c;
    OS_TCB *pTcb   = NULL;
    uint32 *pStack = NULL;

    /* alloc tcb */
    pTcb = _alloc_tcb();
    if (pTcb == NULL) {
        return NULL;
    }

    /*
     * init the task control block
     */
    strncpy(&pTcb->task_name[0], task_name, NAME_LEN_MAX - 1);          /* except the null char, max 255 */
    pTcb->task_entry    = task_entry;
    pTcb->stack_size    = (stack_size + (DEFAULT_ALIGN_BYTE - 1)) & (~(DEFAULT_ALIGN_BYTE - 1));
    pTcb->task_priority = task_priority;    /* current priority */
    pTcb->origin_pri    = task_priority;    /* origin priority. constant */
    pTcb->arg1          = arg1;
    pTcb->arg2          = arg2;
    pTcb->status        = TASK_STATUS_READY;
    pTcb->pend_obj      = NULL;


    /* alloc stack */
    pStack = (uint32 *)_alloc_stack(pTcb->stack_size);
    if (pStack == NULL) {
        _release_tcb(pTcb);
        return NULL;
    }

    pTcb->stack_start = (uint32)pStack;
    /*
     * init the task context in stack
     */
#if 0
    pStack -= 16;      /* pStack -> a context frame top CPSR */
    pStack[0] = EN_IRQ | MOD_SVC;     /* cpsr */
    pStack[1] = (uint32)pTcb;         /* r0   _task_entry() argument */
    pStack[15]= (uint32)_task_entry;  /* pc   */

    pTcb->sp = (uint32)pStack;                  /* save stack point in tcb */
#else
    pTcb->sp = CPU_STACK_INIT((int)_task_entry, (int)pTcb, (int*)pStack);
#endif

    cpsr_c = CPU_LOCK();

    readyQ_put(pTcb);                           /* put it to readyQ */
    G_Task_Num++;                               /* increase task number */

    CPU_UNLOCK(cpsr_c);

    return (pTcb);
}

/*==============================================================================
 * - task_delete()
 *
 * - delete a task. give semMs, free alloced heap memory, free STACK, TCB
 */
OS_STATUS task_delete (OS_TCB *pTcb)
{
    int cpsr_c;

    cpsr_c = CPU_LOCK();

    /* remove the tcb from xx list */
    switch (pTcb->status) {
        case TASK_STATUS_READY:
            readyQ_remove (pTcb);
            break;
        case TASK_STATUS_DELAY:
            delayQ_remove (pTcb);
            break;
        case TASK_STATUS_PEND_SEM_B:
            semB_remove ((SEM_BIN *)(pTcb->pend_obj), pTcb);
            break;
        case TASK_STATUS_PEND_SEM_C:
            semC_remove ((SEM_CNT *)(pTcb->pend_obj), pTcb);
            break;
        case TASK_STATUS_PEND_SEM_M:
            semM_remove ((SEM_MUX *)(pTcb->pend_obj), pTcb);
            break;
        case TASK_STATUS_PEND_MSG_S:
        case TASK_STATUS_PEND_MSG_R:
            msgQ_remove ((MSG_QUE *)(pTcb->pend_obj), pTcb);
            break;
        default:
            break;
    }

    /* give semM(s) this task owned */
    semM_free (pTcb);

    /* free this task alloced heap memory */
    /*memH_free (pTcb);*/ /* T_shell alloc many memory for [yaffs] [net]...
      we can't free them when we exit shell */

    /* free task stack */
    _release_stack (pTcb);
    _release_tcb (pTcb);

    /* sub <G_Task_Num> */
    G_Task_Num--;                   /* task number sub 1 */
    CPU_UNLOCK(cpsr_c);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - tcb_shw()
 *
 * - show one task's information like this:
 *   tTask2    0x30012340      0  DELAY     0x3001D000           4  0x30205130  0x30095130
 */
OS_STATUS tcb_shw (OS_TCB *pTcb, int unused)
{
#define NAME_LEN       8
#define ENTRY_LEN      10
#define PRI_LEN        5 
#define STATUS_LEN     8

    int  index = 0;
    char temp_name[NAME_LEN + 1];
    static const char  *task_status_name[] = {"READY", "DELAY",
                                              "PEND_M", "PEND_B", "PEND_C",
                                              "MSG_S", "MSG_R"};
    static const char  *white_space[NAME_LEN + 1] = {""," ", "  ", "   ", "    ",   /* 0 -- 4 */
                                                     "     ", "      ", "       ",  /* 5 -- 7 */
                                                     "        "                     /* 8 */
                                                    };
    int stack_used_percent = 0;

    static const char  *bar[11] = {"|", "||", "|||", "||||", "|||||", "||||||", /* 01% -- 59% */
                                   "|||||||", "||||||||",                       /* 60% -- 79% */
                                   COLOR_FG_RED"|||||||||"COLOR_FG_WHITE,       /* 80-89% */
                                   COLOR_FG_RED"||||||||||"COLOR_FG_WHITE,      /* 90-99% */
                                   COLOR_FG_RED"|||||||||||"COLOR_FG_WHITE};    /* 100-101% */

    switch (pTcb->status) {
        case TASK_STATUS_READY:
            index = 0;
            break;
        case TASK_STATUS_DELAY:
            index = 1;
            break;
        case TASK_STATUS_PEND_SEM_M:
            index = 2;
            break;
        case TASK_STATUS_PEND_SEM_B:
            index = 3;
            break;
        case TASK_STATUS_PEND_SEM_C:
            index = 4;
            break;
        case TASK_STATUS_PEND_MSG_S:
            index = 5;
            break;
        case TASK_STATUS_PEND_MSG_R:
            index = 6;
            break;
        default:
            break;
    }

    strncpy(temp_name, pTcb->task_name, NAME_LEN + 1);
    if (temp_name[NAME_LEN] != '\0') {
        temp_name[NAME_LEN - 1] = '.';
        temp_name[NAME_LEN] = '\0';
    }

    stack_used_percent = _stack_used_percent(pTcb);

    serial_printf("%s%s  0x%8X  %5d  %s%s  0x%8X  %10u  0x%8X  0x%8X  %s%d%%\n",
                   temp_name,                                       /* name */
                   white_space[NAME_LEN - strlen(temp_name)],
                   pTcb->task_entry,                                /* entry */
                   pTcb->task_priority,                             /* priority */
                   task_status_name[index],                         /* status */
                   white_space[STATUS_LEN - strlen(task_status_name[index])],
                   pTcb->sp,                                        /* stack */
                   pTcb->delay_ticks,                               /* ticks */
                   pTcb->pend_obj,                                  /* pend_obj */
                   pTcb,                                            /* TCB */
                   bar[stack_used_percent / 10],
                   stack_used_percent);

    return OS_STATUS_OK;
}

/*==============================================================================
 * - task_show()
 *
 * - the command "i" operate function. show all task inforamtion like this
 *     NAME      ENTRY      PRI    STATUS       SP         DELAY       P_OBJ       TCB        STACK
 *   --------  ----------  -----  --------  ----------  ----------  ----------  ----------  ----------
 *   tTask2    0x30012340      0  DELAY     0x3001D000           4  0x30205130  0x30096204  ||||||
 *   tTask3    0x30012340      0  READY     0x3001D000           0
 *   tTask1    0x30012340     10  PEND      0x3001D000  4200001234
 *   ...
 *   tReport   0x30012340    255  PEND      0x3001D000        1234
 */
void task_show ()
{
#define TASK_SHOW_HEAD  "\n  NAME      ENTRY      PRI    STATUS       SP         DELAY       P_OBJ       TCB        STACK"\
                        "\n--------  ----------  -----  --------  ----------  ----------  ----------  ----------  ----------\n"
    int cpsr_c;

    cpsr_c = CPU_LOCK();

    serial_puts(TASK_SHOW_HEAD);

    readyQ_show((EACH_FUNC_PTR)tcb_shw);
    delayQ_show((EACH_FUNC_PTR)tcb_shw);

    semB_show((EACH_FUNC_PTR)tcb_shw);
    semC_show((EACH_FUNC_PTR)tcb_shw);
    semM_show((EACH_FUNC_PTR)tcb_shw);
    msgQ_show((EACH_FUNC_PTR)tcb_shw);

    CPU_UNLOCK(cpsr_c);
}

/*==============================================================================
 * - _task_entry()
 *
 * - all task entry. when a task end, this can load next task
 */
static void _task_entry (OS_TCB *pTcb)
{
    pTcb->task_entry(pTcb->arg1, pTcb->arg2);

    CPU_LOCK();                     /* disable interrupt */

    _release_stack(pTcb);
    _release_tcb(pTcb);

    G_Task_Num--;                   /* task number sub 1 */
    readyQ_remove(G_p_current_tcb); /* remove the tcb from readyQ */
    LOAD_HIGHEST_TASK();            /* load a new task */
}

/*==============================================================================
** FILE END
==============================================================================*/

