/*==============================================================================
** ioLib.c -- I/O interface library.
**
** MODIFY HISTORY:
**
** 2012-03-07 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include <stdarg.h>

#include "iosLib.h"

/*==============================================================================
 * - _io_create_or_open()
 *
 * - create or open a file
 */
static int _io_create_or_open (
    const char* name,   /* name of the file to create    */
    int        flags,   /* O_RDONLY, O_WRONLY, O_RDWR or O_CREAT (open call) */
    int        mode,    /* mode of file to create (if open call) */
    BOOL       create)   /* set to TRUE if this is a creat() call */
{
    DEV_HDR *pDevMatch;
    int      open_data;
    const char    *file_name; /* remove device name from full path name */
    int      fd;

    /*
     * check argument
     */
    if ((name == NULL) || (name[0] == EOS)) {
        return OS_STATUS_ERROR;
	}
    
    /*
     * alloc fd
     */
    fd = iosAllocFd ();

    /*
     * find which device match the <name>:
     * whose name is initial substring of <name>
     */
    pDevMatch = iosDevFind (name, &file_name);

    /*
     * call driver's create | open routine
     */
    if (create == TRUE) {
        open_data = iosCreate (pDevMatch, file_name, flags);
    } else {
        open_data = iosOpen (pDevMatch, file_name, flags, mode);
    }

    if (open_data == OS_STATUS_ERROR) {
        iosFreeFd (fd);
        fd = -1;
    } else {
        iosSaveDevNum (fd, pDevMatch);
    }

    return fd;
}

/*==============================================================================
 * - creat()
 *
 * - create a file and return fd
 */
int creat (const char *name, int flag)
{
    return _io_create_or_open (name, flag, 0, TRUE);
}

/*==============================================================================
 * - remove()
 *
 * - delele a file in file system
 */
int remove (const char *name)
{
    DEV_HDR *pDevMatch;
    const char    *file_name; /* remove device name from full path name */

    /*
     * check argument
     */
    if ((name == NULL) || (name[0] == EOS)) {
        return OS_STATUS_ERROR;
	}
    
    /*
     * find which device match the <name>:
     * whose name is initial substring of <name>
     */
    pDevMatch = iosDevFind (name, &file_name);

    return iosDelete (pDevMatch, file_name);
}

/*==============================================================================
 * - open()
 *
 * - open a file and return fd, we assume <name> is full path name
 */
int open (const char *name, int flags, int mode)
{
    return _io_create_or_open (name, flags, mode, FALSE);
}

/*==============================================================================
 * - close()
 *
 * - close
 */
int close (int fd)
{
    int ret;
    ret = iosClose (fd);

    iosFreeFd (fd);
    return ret;
}

/*==============================================================================
 * - read()
 *
 * - read
 */
int read (int fd, char *buffer, int maxbytes)
{
    if (maxbytes == 0) {
        return (0);
	}
    
    return iosRead (fd, buffer, maxbytes);
}

/*==============================================================================
 * - write()
 *
 * - write
 */
int write (int fd, char *buffer, int nbytes)
{
    return iosWrite (fd, buffer, nbytes);
}

/*==============================================================================
 * - ioctl()
 *
 * - ioctl
 */
int ioctl (int fd, int function, ...)
{
    va_list vaList;
    int arg;

    /*
     * Should do something more clever than "always grab one more int"
     * here, but this suffices for now to fix WIND00060945.
     */
    va_start (vaList, function);
    arg = va_arg (vaList, int);
    va_end (vaList);

    /*
     * syscall flag set to false because ioctl from user side
     * will not call ioctl().
     */
    return iosIoctl (fd, function, arg);
}



#include <sys/stat.h>
#include <errno.h>
#undef errno
extern int errno;
extern void _hold (); /* defined in syscall.c */

#include "yaffs_guts.h"
int stat(const char *name, struct stat *st)
{
    const char  *file_name; /* remove device name from full path name */
    int          yaffs_retval;
    struct yaffs_stat stat_buf;

    iosDevFind (name, &file_name);

    yaffs_retval = yaffs_stat(file_name, &stat_buf);

    st->st_dev     = stat_buf.st_dev;      /* device */
    st->st_ino     = stat_buf.st_ino;      /* inode */
    st->st_mode    = stat_buf.st_mode;     /* protection */
    st->st_nlink   = stat_buf.st_nlink;    /* number of hard links */
    st->st_uid     = stat_buf.st_uid;      /* user ID of owner */
    st->st_gid     = stat_buf.st_gid;      /* group ID of owner */
    st->st_rdev    = stat_buf.st_rdev;     /* device type (if inode device) */
    st->st_size    = stat_buf.st_size;     /* total size, in bytes */
    st->st_blksize = stat_buf.st_blksize;  /* blocksize for filesystem I/O */
    st->st_blocks  = stat_buf.st_blocks;   /* number of blocks allocated */

    return 0;

    _hold();
    st->st_mode = S_IFCHR;
    return 0;
}


int fstat(int fd, struct stat *st)
{
    int    yaffs_fd = (int)iosGetOpenData(fd);
    int    yaffs_retval;
    struct yaffs_stat stat_buf;

    yaffs_retval = yaffs_fstat(yaffs_fd, &stat_buf);

    st->st_dev     = stat_buf.st_dev;      /* device */
    st->st_ino     = stat_buf.st_ino;      /* inode */
    st->st_mode    = stat_buf.st_mode;     /* protection */
    st->st_nlink   = stat_buf.st_nlink;    /* number of hard links */
    st->st_uid     = stat_buf.st_uid;      /* user ID of owner */
    st->st_gid     = stat_buf.st_gid;      /* group ID of owner */
    st->st_rdev    = stat_buf.st_rdev;     /* device type (if inode device) */
    st->st_size    = stat_buf.st_size;     /* total size, in bytes */
    st->st_blksize = stat_buf.st_blksize;  /* blocksize for filesystem I/O */
    st->st_blocks  = stat_buf.st_blocks;   /* number of blocks allocated */

    return 0;

    _hold();
    st->st_mode = S_IFCHR;
    return 0;
}


int isatty(int fd)
{
    //_hold();
    return 1;
}

int link(char *old, char *new)
{
    const char  *old_file_name; /* remove device name from full path name */
    const char  *new_file_name; /* remove device name from full path name */
    int          yaffs_retval;

    iosDevFind (old, &old_file_name);
    iosDevFind (new, &new_file_name);

    yaffs_retval = yaffs_link(old_file_name, new_file_name);

    _hold();
    errno = EMLINK;
    return -1;
}


int unlink(char *name)
{
    const char  *file_name; /* remove device name from full path name */
    int          yaffs_retval;

    iosDevFind (name, &file_name);

    yaffs_retval = yaffs_unlink(file_name);

    _hold();
    errno = ENOENT;
    return -1;
}

int lseek(int fd, int ptr, int dir)
{
    int yaffs_fd = (int)iosGetOpenData(fd);
    int yaffs_retval;

    yaffs_retval = yaffs_lseek (yaffs_fd, ptr, dir);

    return yaffs_retval;

    _hold();
    return 0;
}

/*==============================================================================
** FILE END
==============================================================================*/

