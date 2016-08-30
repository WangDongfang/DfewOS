#include <dfewos.h>


#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <stdarg.h>
#include <stddef.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#define printk          serial_printf
#define VOID			void
#define UINT32      	uint32

#define KERN_EMERG      "<0>"   /* system is unusable           */
#define KERN_ALERT      "<1>"   /* action must be taken immediately */
#define KERN_CRIT       "<2>"   /* critical conditions          */
#define KERN_ERR        "<3>"   /* error conditions         */
#define KERN_WARNING    "<4>"   /* warning conditions           */
#define KERN_NOTICE     "<5>"   /* normal but significant condition */
#define KERN_INFO       "<6>"   /* informational            */
#define KERN_DEBUG      "<7>"   /* debug-level messages         */

#ifndef kmalloc
#define kmalloc(size, mode)             malloc(size)
#define kfree(ptr)                      free(ptr)
#endif

#if 0
#define MAX_ERRNO   4095
#define unlikely(x) __builtin_expect(!!(x), 0)
#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)
static inline long IS_ERR(const void *ptr)
{
    return IS_ERR_VALUE((unsigned long)ptr);
}
#else
#define IS_ERR(a)	0
#endif

#define __iomem                                                         /*  IO 空间地址的标识           */
#define __force
#define __user

#endif
