/*==============================================================================
** sysALib.S -- PLL SDRAM MMU init.
**
** MODIFY HISTORY:
**
** 2011-10-19 wdf Create.
==============================================================================*/
#include "bsp_config.h"

/*======================================================================
  MMU Control register initial value
======================================================================*/
#define MMUCR_W_ENABLE	 (1<<3)  /* write buffer enable */
#define MMUCR_PROG32	 (1<<4)  /* PROG32 */
#define MMUCR_DATA32	 (1<<5)  /* DATA32 */
#define MMUCR_L_ENABLE	 (1<<6)  /* Late abort on earlier CPUs */
#define MMUCR_ROM	     (1<<9)  /* ROM bit, modifies MMU protections */

#define MMUCR_MODE       MMUCR_ROM /* This MUST remain ROM for shared libraries */
#define MMUCR_ASYNC      (3<<30) /* Asynchronous mode */

#define MMU_INIT_VALUE (MMUCR_PROG32 | MMUCR_DATA32 | MMUCR_L_ENABLE | \
                        MMUCR_MODE | MMUCR_W_ENABLE)

/*********************************************************************************************************
  size
*********************************************************************************************************/
	.equ    SZ_0                           ,0x00000000
	.equ    SZ_1                           ,0x00000001
	.equ    SZ_2                           ,0x00000002
	.equ    SZ_4                           ,0x00000004
	.equ    SZ_8                           ,0x00000008
	.equ    SZ_16                          ,0x00000010
	.equ    SZ_32                          ,0x00000020
	.equ    SZ_48                          ,0x00000030
	.equ    SZ_64                          ,0x00000040
	.equ    SZ_128                         ,0x00000080
	.equ    SZ_256                         ,0x00000100
	.equ    SZ_512                         ,0x00000200
	.equ    SZ_1K                          ,0x00000400
	.equ    SZ_2K                          ,0x00000800
	.equ    SZ_4K                          ,0x00001000
	.equ    SZ_8K                          ,0x00002000
	.equ    SZ_16K                         ,0x00004000
	.equ    SZ_32K                         ,0x00008000
	.equ    SZ_64K                         ,0x00010000
	.equ    SZ_128K                        ,0x00020000
	.equ    SZ_256K                        ,0x00040000
	.equ    SZ_512K                        ,0x00080000
	.equ    SZ_1M                          ,0x00100000
	.equ    SZ_2M                          ,0x00200000
	.equ    SZ_3M                          ,0x00300000
	.equ    SZ_4M                          ,0x00400000
	.equ    SZ_8M                          ,0x00800000
	.equ    SZ_16M                         ,0x01000000
	.equ    SZ_32M                         ,0x02000000
	.equ    SZ_48M                         ,0x03000000
	.equ    SZ_64M                         ,0x04000000
	.equ    SZ_128M                        ,0x08000000
	.equ    SZ_256M                        ,0x10000000
	.equ    SZ_512M                        ,0x20000000
	.equ    SZ_1G                          ,0x40000000
	.equ    SZ_2G                          ,0x80000000

/*
 *************************************************************************
 *
 * vx_sysInit:
 * Init MMU PLL BANKs timing
 *
 * from vx.
 *
 *************************************************************************
 */
.globl vx_sysInit
.text
.balign 4
vx_sysInit:

    /*
     * Set processor and MMU to known state as follows (we may have not
     * been entered from a reset). We must do this before setting the CPU
     * mode as we must set PROG32/DATA32.
     *
     * MMU Control Register layout.
     *
     * bit
     *  0 M 0 MMU disabled
     *  1 A 0 Address alignment fault disabled, initially
     *  2 C 0 Data cache disabled
     *  3 W 0 Write Buffer disabled
     *  4 P 1 PROG32
     *  5 D 1 DATA32
     *  6 L 1 Should Be One (Late abort on earlier CPUs)
     *  7 B ? Endianness (1 => big)
     *  8 S 0 System bit to zero } Modifies MMU protections, not really
     *  9 R 1 ROM bit to one     } relevant until MMU switched on later.
     * 10 F 0 Should Be Zero
     * 11 Z 0 Should Be Zero (Branch prediction control on 810)
     * 12 I 0 Instruction cache control
     */

    /*
     * Setup MMU Control Register
     */
    mov r1, #MMU_INIT_VALUE                                             /* Defined in mmuArmLib.h       */

    mcr p15, 0, r1, c1, c0, 0                                           /* Write to MMU CR              */

    /*
     * If MMU was on before this, then we'd better hope it was set
     * up for flat translation or there will be problems. The next
     * 2/3 instructions will be fetched "translated" (number depends
     * on CPU).
     * 
     * And we'd better clean and invalidate data cache before we 
     * reinitialize MMU, due to the image of VxWorks may be cached.
     *
     * We would like to discard the contents of the Write-Buffer
     * altogether, but there is no facility to do this. Failing that,
     * we do not want any pending writes to happen at a later stage,
     * so drain the Write-Buffer, i.e. force any pending writes to
     * happen now.
     */
    /*
     * clean and invalidate D-CACHE
     */
    mov r1,#0
    MCR p15, 0, R1, c7, c14, 0

    /* 
     * drain write-buffer           
     */
    mov r1, #0                                                          /* data SBZ                     */
    mcr p15, 0, r1, c7, c10, 4

    /*
     * Flush (invalidate) both I and D caches
     */
    mcr p15, 0, r1, c7, c7, 0                                           /* R1 = 0 from above, data SBZ  */

    /*
     * Set Process ID Register to zero, this effectively disables
     * the process ID remapping feature.
     */
    mov r1, #0
    mcr p15, 0, r1, c13, c0, 0
#if 0
	/* Peri port setup */
	ldr	r1, =0x70000000
	orr	r1, r1, #0x13
	mcr	p15,0,r0,c15,c2,4       									    /* 256M(0x70000000-0x7fffffff)  */
#endif

	/* Disable Vector Int */
	ldr r1, =0x71200000
	ldr r2, =0xFFFFFFFF	                                                /* disable int                  */
	str r2, [r1, #0x14]
	ldr r1, =0x71300000
	str r2, [r1, #0x14]

	mov	ip, lr

    bl mmuSetup  /* startup MMU */

	mov	lr, ip

    mov	pc, lr /* return to _start() */

/*
 *************************************************************************
 *
 * mmuSetup:
 * Startup MMU
 * 参考 <<ARM嵌入式系统开发>> 484,485页
 *
 * from vx.
 * set page table PTEs & enable mmu function
 *
 * 0x32000000 --|---------------| ------> <L1_PAGE_TABLE_ADDR>
 *              | L1 page table |
 *              |               |
 *              |     16KB      |
 *              |               |
 * 0x32004000 --|---------------| ------> <L2_PAGE_TABLE_ADDR>
 *              | L2 page table |
 *              |      4KB      |
 * 0x32005000 --|---------------|
 *
 *************************************************************************
 */
#define L1_PAGE_TABLE_ADDR     CONFIG_MEM_PAGE_LOW 
#define L1_PAGE_TABLE_SIZE     SZ_16K
#define L2_PAGE_TABLE_ADDR     L1_PAGE_TABLE_ADDR + L1_PAGE_TABLE_SIZE
#define L2_PAGE_TABLE_SIZE     SZ_4K

mmuSetup:
/*
 * Initialize MMU Translate table as flat map table. (4GB)
 * set L1 all PTEs: 
 * 设置所有一级页表项为 "段项" [31..20]为基地址
 *
 * *(0x32000000) = 0x000_00C12  --+
 * *(0x32000004) = 0x001_00C12    |
 * *(0x32000008) = 0x002_00C12    |
 *      ...           ...         |- 4096 个段项，每段 1MB
 * *(0x32003FF4) = 0xFFD_00C12    |
 * *(0x32003FF8) = 0xFFE_00C12    |
 * *(0x32003FFC) = 0xFFF_00C12  --+
 *
 */        
#if 0 /* VxWorks 太难理解 */

 		ldr     r1, =L1_PAGE_TABLE_ADDR  /* L1 起始地址 */
 		mov     r3, #SZ_16K  /* 最末+1项偏移地址 */
 1:
 		subs 	r3, r3, #4
 		add		r4, r1, r3       /* PTE addr */
 		mov     r5, r3, LSL #18  /* base addr area */ /* 项偏移--基地址: 左移18位 */
 		add		r5, r5, #(3<<10) /* AP area */
 		add     r5, r5, #0x12    /* CB=0 section */
 		str		r5, [r4]    /* store a PTE */
 		bne		1b		
#else /* dfewos 我好理解 */

 		ldr     r1, =L1_PAGE_TABLE_ADDR  /* L1 起始地址 */
        mov     r3, #0x00000000

1:
        add     r4, r1, r3, LSR #18 /* address --> offset */
        mov     r5, r3
        add     r5, r5, #(3<<10)
        add     r5, r5, #0x12
        str     r5, [r4]
        adds    r3, r3, #(1 * MB)
        bcc		1b
#endif
/*
 * using a fine page table to manage 0x00000000 - 0x00100000 (1MB)
 * set L1 first PTE:
 * 修改一级页表的第一项为 "细页表"  [31..12]为基地址
 *
 * *(0x32000000) = 0x32004_013
 *
 */
 		add     r5, r1, #L1_PAGE_TABLE_SIZE /* base addr area */
 		add     r5, r5, #0x13               /* fine page */
 		str     r5, [r1]

/*
 * Make address [0x30000000 --- (0x34000000 - 1)] cachable
 * set L1 PTEs that map DRAM memory space:
 * 修改一级页表项中映射DRAM的页表项的第3(C)位和第2(B)为[11], 对这些段使能cache
 * 
 * *(0x32000C00) = 0x300_00C1E  --+
 * *(0x32000C04) = 0x301_00C1E    |
 * *(0x32000C08) = 0x302_00C1E    |
 *      ...           ...         |- 64 个段项，每段 1MB
 * *(0x32000CF4) = 0x33D_00C1E    |
 * *(0x32000CF8) = 0x33E_00C1E    |
 * *(0x32000CFC) = 0x33F_00C1E  --+
 *
 */
#if 0 /* VxWorks 太难理解 */

		mov     r3, #CONFIG_MEM_SIZE
 		mov     r3, r3, LSR #18   /* r3 是DRAM最末+1项PTE的偏移 */ /* 相当于DRAM第一项 */
        mov     r2, #CONFIG_MEM_START_ADDR
		ldr     r1, =L1_PAGE_TABLE_ADDR
		add     r1, r2, LSR #18	  /* r1 是DRAM的第一项PTE的地址 */
1:
 		subs 	r3, r3, #4
 		add		r4, r1, r3  /* r4 是该项PTE的地址 */
 		add     r5, r2, r3, LSL #18
 		add		r5, r5, #(3<<10)
 		add     r5, r5, #0x1e
 		str		r5, [r4]
 		bne		1b	
#else /* dfewos 我好理解 */

        mov     r3, #CONFIG_MEM_START_ADDR
        ldr     r6, =CONFIG_NOCACHE_START
1:
        add     r4, r1, r3, LSR #18 /* address --> offset */
        mov     r5, r3
        add     r5, r5, #(3<<10)
        add     r5, r5, #0x1e
        str     r5, [r4]
        add     r3, r3, #(1 * MB)
        cmp     r3, r6
        blt     1b  /* r3 < CONFIG_NOCACHE_START */

#endif


/*
 * Initialize fine page table
 * set L2 all PTEs: 
 * 设置所有二级页表项为 "微页表"  [31..10]为基物理地址
 *
 * *(0x32004000) = 0x00000[00_00]3F  --+
 * *(0x32004004) = 0x00000[01_00]3F    |
 * *(0x32004008) = 0x00000[10_00]3F    |
 *      ...           ...              |- 1024 个微页表，每页 1KB
 * *(0x32004FF4) = 0x000FF[01_00]3F    |
 * *(0x32004FF8) = 0x000FF[10_00]3F    |
 * *(0x32004FFC) = 0x000FF[11_00]3F  --+
 *
 */
 		ldr     r1, =L2_PAGE_TABLE_ADDR
 		mov     r3, #SZ_4K
 1:		
 		subs    r3, r3, #4
 		add     r4, r1, r3
 		mov     r5, r3, LSL #8
 		add     r5, r5, #(3<<4)|0xf
 		str     r5, [r4]
 		bne     1b
    
/*
 * map 0x00000000-0x00000400 to 0x30000000 - 0x30000400 (1KB)
 * set L2 first PTE:
 * 修改二级页表的第一项的基址为0x30000[00]  [31..10]为基物理地址
 *
 * *(0x32004000) = 0x30000[00_00]33
 *
 */
        ldr r1, =L2_PAGE_TABLE_ADDR  /* r1 为二级页表第一项地址 */
        ldr r5, =CONFIG_EXC_VECTOR   /* base addr area */
        add r5, r5, #(3<<4)|3        /* AP tiny */
        str r5, [r1]
/*===================== 页表设置结束 ========================*/
 		
/* 
 * Set Translation Table Base 
 */
  		ldr     r1, =L1_PAGE_TABLE_ADDR
        mcr     p15, 0, r1, c2, c0, 0                                   /* Set Translation Table Base   */
                                                                        /* Register                     */

/* 
 * Invalidate Instruction, Data TLBs 
 */
        mov     r0,  #0
        mcr     p15, 0, r0, c8, c7, 0                                   /* Flush I & D TLBs             */

/* 
 * Set Domain Access Control Register 
 */
        ldr     r0, =0x1                                                /* Set All 16 domains to manager*/
                                                                        /* access                       */
        mcr     p15, 0, r0, c3, c0, 0                                   /* Set Domain Permissions       */
/*
 * Set bit S=1 R=0,so memory is read-only in super mode and unaccess in user mode.
 */
 		mrc 	p15, 0, r0, c1, c0, 0
 		ldr 	r1, =0xfffffcff
 		and 	r0, r0, r1
 		orr		r0, r0, #0x00000100
 		mcr		p15, 0, r0, c1, c0, 0
/* 
 * Enable MMU 
 */
        mrc     p15, 0, r0, c1, c0, 0                                   /* Read Control Register        */
        orr     r0, r0, #0x00000001                                     /* Enable MMU                   */

        mcr     p15, 0, r0, c1, c0, 0                                   /* Write Back the Control       */
                                                                        /* Register                     */

/* 
 * Enable Instruction Cache 
 */
        mrc     p15, 0, r0, c1, c0, 0                                   /* Read Control Register        */
        orr     r0, r0, #0x1000                                         /* Set I-Cache bit              */
        mcr     p15, 0, r0, c1, c0, 0                                   /* Write Back Control Register  */
        
/*
 * Enable Data Cache 
 */
        mrc     p15, 0, r0, c1, c0, 0                                   /* Read Control Reg             */
        orr     r0, r0, #0x00000004                                     /* Enable Data Cache            */
        mcr     p15, 0, r0, c1, c0, 0                                   /* Write Back                   */
        
		mov     pc,lr

/*==============================================================================
** FILE END
==============================================================================*/

