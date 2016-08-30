
#ifndef _ASM_PAGE_H
#define _ASM_PAGE_H


#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

typedef struct { unsigned long pgprot; } pgprot_t;

#endif                                                                  /*  _ASM_PAGE_H                 */
