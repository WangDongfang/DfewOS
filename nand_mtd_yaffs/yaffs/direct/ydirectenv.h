/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system. 
 *
 * Copyright (C) 2002-2010 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 */

/*
 * ydirectenv.h: Environment wrappers for YAFFS direct.
 */

#ifndef __YDIRECTENV_H__
#define __YDIRECTENV_H__

/* Direct interface*/

#define D_FEW_OS
#ifdef  D_FEW_OS             /* DFEW_OS */

#include "../../../serial.h"
#define YBUG() do { *((int *)1) = 1;} while(0)

#else                        /* OTHERS */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"

#define YBUG() assert(0)
#endif                       /* END OS */

#include "yaffs_osglue.h"
#include "yaffs_hweight.h"

#define YCHAR char
#define YUCHAR unsigned char
#define _Y(x) x

#define hweight8(x)	    yaffs_hweight8(x)
#define hweight32(x)	yaffs_hweight32(x)

void yaffs_qsort(void *aa, size_t n, size_t es,
                 int (*cmp)(const void *, const void *));

#define sort(base, n, sz, cmp_fn, swp) yaffs_qsort(base, n, sz, cmp_fn)
        
#define YAFFS_PATH_DIVIDERS  "/"

#ifdef NO_inline
#define inline
#else
#define inline __inline__
#endif

#ifndef kmalloc
#define kmalloc(x,flags) malloc(x)
#define kfree(x)         free(x)
#endif
#define vmalloc(x)       malloc(x)
#define vfree(x)         free(x)
#define strnlen(x, m)    strlen(x)

#define cond_resched()  do {} while(0)

#define yaffs_trace(msk, fmt, ...) do { \
    if ((msk) == YAFFS_TRACE_ERASE || (msk) == YAFFS_TRACE_ALLOCATE) { \
        putchar('.');\
    } else if ((msk) == YAFFS_TRACE_ERROR) { \
		printf("yaffs: " fmt "\n", ##__VA_ARGS__); \
    } else {\
    }\
} while(0)

#define YAFFS_LOSTNFOUND_NAME		"lost+found"
#define YAFFS_LOSTNFOUND_PREFIX		"obj"

#include "yaffscfg.h"

#define Y_CURRENT_TIME yaffsfs_CurrentTime()
#define Y_TIME_CONVERT(x) x

#define YAFFS_ROOT_MODE				0666
#define YAFFS_LOSTNFOUND_MODE		0666

#include "yaffs_list.h"

#include "yaffsfs.h"

#endif


