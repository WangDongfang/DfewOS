#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H


#include <linux/types.h>
#include <asm/memory.h>

#define __raw_writel(v, c)      (*(volatile unsigned long *)(c) = (v))
#define __raw_readl(c)          (*(volatile unsigned long *)(c))

#define readl(c)                   __raw_readl(c)
#define writel(v,c)                __raw_writel(v,c)


#define ioremap(physAddr, size)         (void*)(physAddr)               /*  重映射 IO 空间的区域        */
#define ioremap_nocache(cookie,size)    ((void *)(cookie))
#define iounmap(virtAddr)                                               /*  解除重映射                  */



#endif                                                                  /*  __ASM_ARM_IO_H              */
