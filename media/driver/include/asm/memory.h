
#ifndef __ASM_ARM_MEMORY_H
#define __ASM_ARM_MEMORY_H

#define __virt_to_phys(x)       ((void *)(x))

#define __phys_to_pfn(paddr)    ((paddr) >> PAGE_SHIFT)
//#define virt_to_phys(virtAddr)          (unsigned long)(virtAddr)               /*  逻辑地址转物理地址          */
#define phys_to_virt(x)         ((void *)(x))


#endif                                                                  /*  __ASM_ARM_MEMORY_H          */
