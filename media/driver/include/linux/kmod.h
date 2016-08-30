
#ifndef __LINUX_KMOD_H__
#define __LINUX_KMOD_H__

#include <stddef.h>
#include <linux/errno.h>

#define __GFP_WAIT  ((__force gfp_t)0x10u)  /* Can wait and reschedule? */
#define __GFP_IO    ((__force gfp_t)0x40u)  /* Can start physical IO? */
#define __GFP_FS    ((__force gfp_t)0x80u)  /* Can call down to low-level FS? */

#define GFP_KERNEL  (__GFP_WAIT | __GFP_IO | __GFP_FS)


#endif                                                                  /*  __LINUX_KMOD_H__            */
