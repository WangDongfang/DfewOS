/****************************************Copyright (c)*****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**                                      
**                                 http://www.embedtools.com
**
**--------------File Info----------------------------------------------------------------------------------
** File name:               vi_fix.h
** Last modified Date:      2011-6-1 13:45:20
** Last Version:            1.0.0
** Descriptions:            This file
**
**---------------------------------------------------------------------------------------------------------
** Created by:              WangDongfang
** Last modified Date:      2011-6-1 13:45:20
** Version:                 1.0.0
** Descriptions:            This file
**
**********************************************************************************************************/

#ifndef __VIM_FIX_H
#define __VIM_FIX_H

#define DFEW_OS /* my operating system */

/*======================================================================
  libc struct & routines replace
======================================================================*/
#ifdef DFEW_OS

#include "yaffs_guts.h"
#undef strnlen /* yaffs define it */
#include "../config.h"
#include "../types.h"
#include "../string.h"
#include "../cmd.h"
#include <stdarg.h>
#include <malloc.h>
extern int sscanf (const char *str, const char *fmt, ...);
extern int sprintf (char *, const char *, ...);
extern int snprintf (char *buffer, size_t count, const char *fmt, ...);
extern int vsprintf (char *buffer, const char *fmt, va_list vaList);
extern char *strerror (int errcode);

/* here functions are defined in <yaffs_cmd.c> */
int my_yaffs_open(const char *file_name, int oflag, int mode);
int my_yaffs_stat(const char *file_name, struct yaffs_stat *buf);

#undef errno
extern int errno;
#define EIO   5

#else /* DFEW_OS */

#include <unistd.h>
#include <ioLib.h>
#include <selectLib.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#endif /* DFEW_OS */

/*********************************************************************************************************
  vi ÅäÖÃ
*********************************************************************************************************/
typedef  int    smallint;

#define CONFIG_FEATURE_VI_MAX_LEN               1024

#define RESERVE_CONFIG_BUFFER(buffer,len)       char    buffer[len]

#define ARRAY_SIZE(x)                           (sizeof(x) / sizeof((x)[0]))

#define SET_PTR_TO_GLOBALS(x) do {                      \
	    (*(struct globals**)&ptr_to_globals) = (x);     \
} while (0)

#define bb_show_usage()

/*======================================================================
  file system struct & routines repalce
======================================================================*/
#if 1 /* DFEW_OS */
#define stat                                    yaffs_stat
#define open(name, oflag, mode)                 my_yaffs_open(name, oflag, mode)
#define close(fd)                               yaffs_close(fd)

#define safe_write(fd, buf , count)             yaffs_write(fd, buf, count)
#define safe_read(fd, buf , count)              yaffs_read(fd, buf, count)
#else /* DFEW_OS */ /* use io system */
#include "../ios/lib/ioLib.h"
#define my_yaffs_stat                           stat
#define safe_write(fd,buf , count)              write(fd, buf, count)
#define safe_read(fd, buf , count)              read(fd, buf, count)
#endif /* DFEW_OS */

/*********************************************************************************************************
  vi_fix.c º¯ÊýÉùÃ÷
*********************************************************************************************************/

#define  __VI_TERMINAL_WIDTH                    80                      /*  chars in a line             */
#define  __VI_TERMINAL_HEIGHT                   24                      /*  lines                       */

extern   int        get_terminal_width_height(const int  fd, int  *width, int  *height);
extern   char      *last_char_is(const char *s, int c);
extern   void      *lib_xzalloc(size_t s);
extern   ssize_t    full_write(int fd, const void *buf, size_t len);
extern   int        isblank(int c);
extern   int        bb_putchar(int ch);


extern   void      *memrchr(const void *str, int c, size_t n);
extern   char      *strchrnul(const char *str, char c);
extern   size_t     strnlen(const char * s, size_t maxlen);
extern   char      *xstrndup (const char *s, size_t n);
extern   void      *xmalloc (size_t size);
extern   char      *xstrdup (const char *string);

#define xzalloc     lib_xzalloc
#define lib_strlen  strlen
#define lib_xmalloc malloc
#define lib_memset  memset

#define DEFAULT_FILE_NAME       "default_vi_create_file"


/*======================================================================
 IO routines replace
======================================================================*/
#ifdef DFEW_OS

#define fflush(fd)
#define fputc(c, fd)                    serial_putc(c)
#define fputs(s, fd)                    serial_puts(s)
#define fwrite(buf, size, count, fd)    serial_write(buf, size, count)
#define puts(s)                         serial_puts(s)

#endif /* DFEW_OS */

#endif                                                                  /*  __VIM_FIX_H                 */

/**********************************************************************************************************
  FILE END
**********************************************************************************************************/

