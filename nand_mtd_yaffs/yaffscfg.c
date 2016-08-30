/*==============================================================================
** yaffscfg.c -- yaffs module port file.
**
** MODIFY HISTORY:
**
** 2011-08-17 wdf Create.
==============================================================================*/
/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * yaffscfg.c  The configuration for the "direct" use of yaffs.
 *
 * This file is intended to be modified to your requirements.
 * There is no need to redistribute this file.
 */

/* yaffs/direct/ dir */
#include "yaffscfg.h"
#include "yaffsfs.h"

/* yaffs/ dir */
#include "yaffs_guts.h"
#include "yaffs_trace.h"
#include "yaffs_linux.h"
#include "yaffs_packedtags2.h"
#include "yaffs_mtdif.h"
#include "yaffs_mtdif2.h"

/* mtd/ dir */
#include "operate_nand.h"

/* dfewos: boot/ dir */
#include <dfewos.h>
#include "../string.h"

/*======================================================================
  local function prototype
======================================================================*/
static int  _fat_time(void);

/*======================================================================
  yaffs trace mask value
======================================================================*/
unsigned yaffs_trace_mask =	YAFFS_TRACE_SCAN | YAFFS_TRACE_GC |
                            YAFFS_TRACE_ERASE | YAFFS_TRACE_ERROR |
                            YAFFS_TRACE_TRACING | YAFFS_TRACE_ALLOCATE |
                            YAFFS_TRACE_BAD_BLOCKS | YAFFS_TRACE_VERIFY | 0;

/***********************************************************************
  yaffs need os glue routines           ...START
***********************************************************************/
/*======================================================================
  global variables
======================================================================*/
#undef errno
extern int errno;              /* errno */
static SEM_MUX _G_yaffs_semM;  /* mutex semaphore */
#define DFEW_YAFFS_LOCK()       semM_take(&_G_yaffs_semM, WAIT_FOREVER)
#define DFEW_YAFFS_UNLOCK()     semM_give(&_G_yaffs_semM)

void yaffsfs_SetError(int err)
{
    errno = err;
}

int yaffsfs_GetLastError(void)
{
	return errno;
}

void yaffsfs_Lock(void)
{
    int   error = errno;
    DFEW_YAFFS_LOCK();
    errno = error;
}

void yaffsfs_Unlock(void)
{
    int   error = errno;
    DFEW_YAFFS_UNLOCK();
    errno = error;
}

u32 yaffsfs_CurrentTime(void)
{
    return (u32)_fat_time();    /*  use fat time format */
}

void *yaffsfs_malloc(size_t size)
{
    return malloc(size);
}

void yaffsfs_free(void *ptr)
{
    free(ptr);
}

void yaffsfs_OSInitialisation(void)
{
    semM_init(&_G_yaffs_semM);
}
/***********************************************************************
  yaffs need os glue routines       ...END
***********************************************************************/


/*==============================================================================
 * - _fat_time()
 *
 * - format a fat time
 */
static int _fat_time(void)
{
    return	 (int)(((2009-1980) << 25) |
                         (0x03  << 21) |
                         (0x01  << 16) |
                         (0x08  << 11) |
                         (0x00  <<  5) |
                         (0x00 / 2));
}

/*==============================================================================
** FILE END
==============================================================================*/

