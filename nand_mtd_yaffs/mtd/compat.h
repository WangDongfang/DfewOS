#ifndef _LINUX_COMPAT_H_
#define _LINUX_COMPAT_H_

#include "mtdtypes.h"
#include "err.h"

#define __user
#define __iomem


#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))

#define __arch_putb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a)		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))

#define readb(a)			__arch_getb(a)
#define readw(a)			__arch_getw(a)
#define readl(a)			__arch_getl(a)

#define writeb(v,a)			__arch_putb(v,a)
#define writew(v,a)			__arch_putw(v,a)
#define writel(v,a)			__arch_putl(v,a)


#define DECLARE_WAITQUEUE(...)	do { } while (0)
#define add_wait_queue(...)	do { } while (0)
#define remove_wait_queue(...)	do { } while (0)

#define KERNEL_VERSION(a,b,c)	(((a) << 16) + ((b) << 8) + (c))

#ifndef BUG
#define BUG() do { \
	printf("mtd BUG at %s:%d!\n", __FILE__, __LINE__); \
} while (0)
#define BUG_ON(condition) do { if (condition) BUG(); } while(0)
#endif /* BUG */

#define PAGE_SIZE	4096

#define MTD_NEED_STD_DEF_FUNC /* Dfew */
#ifdef MTD_NEED_STD_DEF_FUNC

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max at all, of course.
 */
#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

#define min(a, b) min_t(int, a, b)
#if 0 /* Dfew */
#define printk	printf
#else
#define printk	serial_printf
#endif

#define KERN_EMERG
#define KERN_ALERT
#define KERN_CRIT
#define KERN_ERR
#define KERN_WARNING  ""
#define KERN_NOTICE
#define KERN_INFO
#define KERN_DEBUG

#ifndef __YDIRECTENV_H__  /* Dfew */
#define kmalloc(size, flags)	malloc(size)
#define vmalloc(size)		malloc(size)
#define kfree(ptr)		free(ptr)
#define vfree(ptr)		free(ptr)
#endif /* __YDIRECTENV_H__ */
#define kzalloc(size, flags)	calloc(size, 1)

/* Dfew */
#include "../../config.h"
#include "../../types.h"
#include "../../serial.h"
#include "../../string.h"
#include <malloc.h>
int sprintf(char *buf, const char *, ...);
/* Dfew */

#endif

#endif
