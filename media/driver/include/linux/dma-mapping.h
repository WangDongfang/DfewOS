
#ifndef _LINUX_DMA_MAPPING_H
#define _LINUX_DMA_MAPPING_H

#include <linux/module.h>


/* These definitions mirror those in pci.h, so they can be used
 * interchangeably with their PCI_ counterparts */
enum dma_data_direction {
    DMA_BIDIRECTIONAL = 0,
    DMA_TO_DEVICE = 1,
    DMA_FROM_DEVICE = 2,
    DMA_NONE = 3,
};

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

/*
 * NOTE: do not use the below macros in new code and do not add new definitions
 * here.
 *
 * Instead, just open-code DMA_BIT_MASK(n) within your driver
 */
#define DMA_64BIT_MASK  DMA_BIT_MASK(64)
#define DMA_48BIT_MASK  DMA_BIT_MASK(48)
#define DMA_47BIT_MASK  DMA_BIT_MASK(47)
#define DMA_40BIT_MASK  DMA_BIT_MASK(40)
#define DMA_39BIT_MASK  DMA_BIT_MASK(39)
#define DMA_35BIT_MASK  DMA_BIT_MASK(35)
#define DMA_32BIT_MASK  DMA_BIT_MASK(32)
#define DMA_31BIT_MASK  DMA_BIT_MASK(31)
#define DMA_30BIT_MASK  DMA_BIT_MASK(30)
#define DMA_29BIT_MASK  DMA_BIT_MASK(29)
#define DMA_28BIT_MASK  DMA_BIT_MASK(28)
#define DMA_24BIT_MASK  DMA_BIT_MASK(24)

#define DMA_MASK_NONE   0x0ULL



#define dma_cache_maint(start, size, direction)                         /*  DMA 与 Cache 间的一致性函数 */


#endif                                                                  /*  _LINUX_DMA_MAPPING_H        */
