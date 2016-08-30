/*==============================================================================
** syscall.c -- those routines is compiler used for libc.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#if 1
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "serial.h"
#include "types.h"
#include "config.h"
#include "dlist.h"
#include "task.h"
#include "semB.h"

#undef errno
extern int errno;

void _hold ()
{
    FOREVER{}
}

void _exit( int id)__attribute__ ( ( naked) );
void _exit( int id)
{
    _hold();

}

int execve( char *name, char **argv, char **env)
{
    _hold();
    errno = ENOMEM;
    return -1;
}

int fork(void)
{
    _hold();
    errno = EAGAIN;
    return -1;
}


int getpid ( void )
{
    _hold();
    return 1;
}

int _getpid ( void )
{
    _hold();
    return 1;
}

int _isatty(int file)
{
    _hold();
    return 1;
}


int kill(int pid, int sig)
{
    _hold();
    errno = EINVAL;
    return -1;
}

int _kill(int pid, int sig)
{
    _hold();
    errno = EINVAL;
    return -1;
}


caddr_t sbrk( int incr )
{
    _hold();
#if 0
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = (char *)CONFIG_MEM_HEAP_LOW;
    }
    prev_heap_end = heap_end;
    if ( (int)(heap_end + incr) > CONFIG_MEM_HEAP_HIGH ) {
        //UartPrintf ("Heap and Stack collision\n" );
        abort();
    }
    heap_end += incr;
    return (caddr_t) prev_heap_end;
#else
    _hold();
    return NULL;
#endif
}

int wait( int *status)
{
    _hold();
    errno = ECHILD;
    return -1;
}


int _open_r( void *reent, const char *file, int flags, int mode)
{
    _hold();
    /* return yaffs_open(file, flags, 0); */
    return 0;
}

int _close_r( void *reent, int fd)
{
    _hold();
    /* return yaffs_close(fd); */
    return 0;
}

off_t _lseek_r( void *reent, int fd, off_t pos, int whence)
{
    _hold();
    /* return yaffs_lseek(fd, pos, whence); */
    return 0;
}

long _read_r( void *reent, int fd, void *buf, size_t cnt)
{
    _hold();
    /* return yaffs_read(fd, buf, cnt); */
    return 0;
}

long _write_r( void *reent, int fd, const void *buf, size_t cnt)
{
    _hold();
    /* return yaffs_write(fd, buf, cnt); */
    return 0;
}


int _wait_r( void *reent, int *status)
{
    _hold();
    errno = ECHILD;
    return -1;
}

int _stat_r( void *reent, const char *file, struct stat *pstat)
{
    _hold();
	pstat->st_mode = S_IFCHR;
    return 0;
}

int _fstat_r( void *reent, int fd, struct stat *pstat )
{
    _hold();
	pstat->st_mode = S_IFCHR;
    return 0;
}

int _unlink_r( struct _reent *reent, const char *name)
{
    serial_printf("not supported\n");
    _hold();
    return -1;
}


char *_sbrk_r( void *reent, size_t incr)
{
#if 1
    static char *heap_start = (char *)CONFIG_MEM_PAGE_HIGH;
    char *prev_heap_end;

    prev_heap_end = heap_start;
    if ( (int)(heap_start + incr) > CONFIG_NOCACHE_START ) {
    	serial_puts("HEAP OVER!!\n");
        _hold();
    }
    heap_start += incr;
    return (char *) prev_heap_end;
#else 
    _hold();
    return NULL;
#endif
}

#if 0

static SEM_BIN _G_semB;
void __malloc_lock(struct _reent * preent)
{
	semB_take(&_G_semB, WAIT_FOREVER);
}
void __malloc_unlock(struct _reent *preent)
{
	semB_give(&_G_semB);
}

void libc_init( void )
{
	semB_init(&_G_semB);
}
#endif
#endif
/*==============================================================================
** FILE END
==============================================================================*/

