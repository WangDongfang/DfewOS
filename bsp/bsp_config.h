/*==============================================================================
** bsp_config.h -- system config.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __BSP_CONFIG_H__
#define __BSP_CONFIG_H__

/*======================================================================
  CPU mode value in cspr
======================================================================*/
#define NO_INT      0xC0    /* I bit is 1, mask IRQ */
#define EN_IRQ      0x40    /* I bit is 0, enable IRQ */

#define MOD_IRQ     0x12    /* CPU in IRQ mode */
#define MOD_SVC     0x13    /* CPU in SVC mode */
#define MOD_SYS		0x1f    /* CPU in SYS mode */
#define MOD_USR		0x10    /* CPU in USR mode */

#define KB                      (1024)
#define MB                      (1024 * 1024)

/**********************************************************************************************************
  Memory Address Config
***********************************************************************************************************
      <---- IRQ_STACK       STACK_LOW  <----->  STACK_HIGH            <-------- SVC_STACK
            CODE_START <--> CODE_END            HEAP_LOW <------> HEAP_HIGH
50. 50.40   50010000        50800000            51000000          56000000      56C00000     57C00000   58.
-----------------------------------------------------------------------------------------------------------
|vec|IRQ_STACK| CODE = 8M - 64K |  TASK_STACK = 8M  |   HEAP = 80M   |PAGE=32K| SVC |NO_CACHE=16M| LCD FB |
-----------------------------------------------------------------------------------------------------------
   64B       64K               8M                  16M              96M            108M         124M   128M
**********************************************************************************************************/
#define CONFIG_EXC_VECTOR           0x50000000                          /*  start exception vector 64K   */
#define CONFIG_IRQ_STACK            0x50010000                          /*  irq stack top & code start   */

/* task stack area */
#define CONFIG_MEM_STACK_LOW        0x50800000                          /*  task stack low               */
#define CONFIG_MEM_STACK_SIZE       (8 * MB)                            /*  task stack size              */
#define CONFIG_MEM_STACK_HIGH       0x51000000                          /*  task stack high              */
#define CONFIG_MEM_STACK_PAGE_SIZE  (4 * KB)                            /*  task stack page size         */
#define CONFIG_MEM_STACK_PAGE_NUM   (CONFIG_MEM_STACK_SIZE / CONFIG_MEM_STACK_PAGE_SIZE)/*  page number  */

/* malloc() free() heap area */
#define CONFIG_MEM_HEAP_LOW         0x51000000                          /*  malloc low address           */
#define CONFIG_MEM_HEAP_SIZE        (80 * MB)                           /*  malloc heap size             */
#define CONFIG_MEM_HEAP_HIGH        0x56000000                          /*  malloc high address          */

/* page_table area */
#define CONFIG_MEM_PAGE_LOW         0x56000000                          /*  page table low address       */
#define CONFIG_MEM_PAGE_SIZE        (32 * KB)                           /*  page table size              */
#define CONFIG_MEM_PAGE_HIGH        0x56008000                          /*  page table high address      */

/* nocache_malloc nocache_free() area */
#define CONFIG_NOCACHE_START        0x56C00000
#define CONFIG_NOCACHE_SIZE         (16 * MB)
#define CONFIG_NOCACHE_END          0x57C00000

/* LCD frame buffer */
#define CONFIG_FRAME_BUFFER_START   0x57C00000
#define CONFIG_FRAME_BUFFER_SIZE    (4 * MB)
#define CONFIG_FRAME_BUFFER_END     0x58000000

/* others */
#define CONFIG_SVC_STACK            CONFIG_NOCACHE_START                /*  naked of svc stack top       */
#define IRQ_LR_R0_ADDR              CONFIG_IRQ_STACK                    /*  memory to store irq lr, r0   */

/* whole memory */
#define CONFIG_MEM_START_ADDR       0x50000000
#define CONFIG_MEM_SIZE             (128 * MB)
#define CONFIG_MEM_END_ADDR         0x54000000
/**********************************************************************************************************
 * the CONFIG_IRQ_STACK first 2 words used to store IRQ mode's LR and R0 when in TICK context switch
**********************************************************************************************************/

/**********************************************************************************************************
  TICK
**********************************************************************************************************/
#define SYS_CLK_RATE				100									/* 100 ticks per second			 */

/**********************************************************************************************************
  CLOCK
**********************************************************************************************************/
#define FCLK                        533000000                           /* 533 MHz                       */
#define HCLK                        133000000                           /* 133 MHz                       */
#define PCLK                        66500000                            /* 66.5 MHz                      */

/**********************************************************************************************************
  CPU -- MEMORY
**********************************************************************************************************/
#define DEFAULT_ALIGN_BYTE          4                                   /* 4 byte alignment              */

#endif /* __BSP_CONFIG_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

