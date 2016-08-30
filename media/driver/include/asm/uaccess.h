
#ifndef _ASM_UACCESS_H
#define _ASM_UACCESS_H

#include <linux/sched.h>
#include <asm/memory.h>

#define copy_from_user(to, from, n)     (memcpy((void *)(to), (const void *)(from), (n)), (0))
#define copy_to_user(to, from, n)       (memcpy((void *)(to), (const void *)(from), (n)), (0))

#define get_user(x, p)                  (x) = *(p)

#endif                                                                  /*  _ASM_UACCESS_H              */
