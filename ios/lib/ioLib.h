/*==============================================================================
** ioLib.h -- I/O system interface.
**
** MODIFY HISTORY:
**
** 2012-03-07 wdf Create.
==============================================================================*/

#ifndef __IOLIB_H__
#define __IOLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Rewack the FXXXXX values as _FXXXX so that _POSIX_C_SOURCE works.
 */
#define	_FOPEN		(-1)	/* from sys/file.h, kernel use only */
#define	_FREAD		0x0001	/* read enabled */
#define	_FWRITE		0x0002	/* write enabled */
#define	_FNDELAY	0x0004	/* non blocking I/O (4.2 style) */
#define	_FAPPEND	0x0008	/* append (writes guaranteed at the end) */
#define	_FMARK		0x0010	/* internal; mark during gc() */
#define	_FDEFER		0x0020	/* internal; defer for next gc pass */
#define	_FASYNC		0x0040	/* signal pgrp when data ready */
#define	_FSHLOCK	0x0080	/* BSD flock() shared lock present */
#define	_FEXLOCK	0x0100	/* BSD flock() exclusive lock present */
#define	_FCREAT		0x0200	/* open with file create */
#define	_FTRUNC		0x0400	/* open with truncation */
#define	_FEXCL		0x0800	/* error on open if file exists */
#define	_FNBIO		0x1000	/* non blocking I/O (sys5 style) */
#define	_FSYNC		0x2000	/* do all writes synchronously */
#define	_FNONBLOCK	0x4000	/* non blocking I/O (POSIX style) */
#define	_FNOCTTY	0x8000	/* don't assign a ctty on this open */
#define	_FDSYNC	       0x10000	/* file data only integrity while writing */
#define	_FRSYNC	       0x20000	/* sync read operations at same level by */
				/* _FSYNC and _FDSYNC flags */
#define	_FCNTRL	       0x40000	/* special open mode for vnode ioctl's */
#define	_FNOATIME      0x80000	/* special open mode to not update access */
				/* time on close */
#define	_FFD_CLOEXEC   0x100000	/* special flag for FD_CLOEXEC */
#define _FTEXT	       0x200000 /* text translation mode */

#define	O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)

#ifndef O_RDONLY
/*
 * Flag values for open(2) and fcntl(2)
 * The kernel adds 1 to the open modes to turn it into some
 * combination of FREAD and FWRITE.
 */
#define	O_RDONLY	0		/* +1 == FREAD */
#define	O_WRONLY	1		/* +1 == FWRITE */
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */
#define	O_CREAT		_FCREAT
#define O_TEXT		_FTEXT		/* text translation mode */

/*
 * added for future POSIX extensions 
 */
#define	O_APPEND	_FAPPEND
#define	O_TRUNC		_FTRUNC
#define	O_EXCL		_FEXCL
#define	O_NONBLOCK	_FNONBLOCK
#define	O_NOCTTY	_FNOCTTY
#define	O_SYNC		_FSYNC
#define	O_DSYNC		_FDSYNC
#define	O_RSYNC		_FRSYNC

#endif /* O_RDONLY */

#ifndef SEEK_SET
#define SEEK_SET           0       /* absolute offset, was L_SET */
#define SEEK_CUR           1       /* relative to current offset, was L_INCR */
#define SEEK_END           2       /* relative to end of file, was L_XTND */
#endif
    
/*
 * Ioctl function number ranges:
 *
 * 0x00000000 - 0x1FFFFFFF - reserved for WindRiver use
 * 0x20000000 - 0x2FFFFFFF - unix style ioctls (networking)
 * 0x30000000 - 0x3FFFFFFF - User reserved codes
 * 0x40000000 - 0x4FFFFFFF - unix style ioctls (networking)
 * 0x50000000 - 0x7FFFFFFF - reserved for future use
 * 0x80000000 - 0x8FFFFFFF - unix style ioctls (networking)
 * 0x90000000 - 0xBFFFFFFF - reserved for future use
 * 0XC0000000 - 0xCFFFFFFF - unix style ioctls (networking)
 * 0xD0000000 - 0xFFFFFFFE - reserved for future use
 * 0xFFFFFFFF                reserved, invalid code number
 *
 * For VxWorks sub systems, the recommended way to generate
 * ioctl numbers is to use the module number (shifted right) as a base.
 * This will be okay as long as module numbers are not more than
 * 16 bits in length. (remember, module numbers are already
 * shifted left 16 bits).
 *
 *     i.e.  #define SUBSYS_IOCTL_1  ((M_subSystem>>8) | 1)
 */

/* ioctl function codes */

#define FIONREAD	1		/* get num chars available to read */
#define FIOFLUSH	2		/* flush any chars in buffers */
#define FIOOPTIONS	3		/* set options (FIOSETOPTIONS) */
#define FIOBAUDRATE	4		/* set serial baud rate */
#define FIODISKFORMAT	5		/* format disk */
#define FIODISKINIT	6		/* initialize disk directory */
#define FIOSEEK		7		/* set current file char position */
#define FIOWHERE	8		/* get current file char position */
#define FIODIRENTRY	9		/* return a directory entry (obsolete)*/
#define FIORENAME	10		/* rename a directory entry */
#define FIOREADYCHANGE	11		/* return TRUE if there has been a
					   media change on the device */
#define FIONWRITE	12		/* get num chars still to be written */
#define FIODISKCHANGE	13		/* set a media change on the device */
#define FIOCANCEL	14		/* cancel read or write on the device */
#define FIOSQUEEZE	15		/* OBSOLETED since RT11 is obsoleted */
#define FIONBIO		16		/* set non-blocking I/O; SOCKETS ONLY!*/
#define FIONMSGS	17		/* return num msgs in pipe */
#define FIOGETNAME	18		/* return file name in arg */
#define FIOGETOPTIONS	19		/* get options */
#define FIOSETOPTIONS	FIOOPTIONS	/* set options */
#define FIOISATTY	20		/* is a tty */
#define FIOSYNC		21		/* sync to disk */
#define FIOPROTOHOOK	22		/* specify protocol hook routine */
#define FIOPROTOARG	23		/* specify protocol argument */
#define FIORBUFSET	24		/* alter the size of read buffer  */
#define FIOWBUFSET	25		/* alter the size of write buffer */
#define FIORFLUSH	26		/* flush any chars in read buffers */
#define FIOWFLUSH	27		/* flush any chars in write buffers */
#define FIOSELECT	28		/* wake up process in select on I/O */
#define FIOUNSELECT	29		/* wake up process in select on I/O */
#define FIONFREE        30              /* get free byte count on device */
#define FIOMKDIR        31              /* create a directory */
#define FIORMDIR        32              /* remove a directory */
#define FIOLABELGET     33              /* get volume label */
#define FIOLABELSET     34              /* set volume label */
#define FIOATTRIBSET    35              /* set file attribute */
#define FIOCONTIG       36              /* allocate contiguous space */
#define FIOREADDIR      37              /* read a directory entry (POSIX) */
#define FIOFSTATGET_OLD 38              /* get file status info - legacy */
#define FIOUNMOUNT      39              /* unmount disk volume */
#define FIOSCSICOMMAND  40              /* issue a SCSI command */
#define FIONCONTIG      41              /* get size of max contig area on dev */
#define FIOTRUNC        42              /* truncate file to specified length */
#define FIOGETFL        43		/* get file mode, like fcntl(F_GETFL) */
#define FIOTIMESET      44		/* change times on a file for utime() */
#define FIOINODETONAME  45		/* given inode number, return filename*/
#define FIOFSTATFSGET   46              /* get file system status info */
#define FIOMOVE         47              /* move file, ala mv, (mv not rename) */

/* 64-bit ioctl codes, "long long *" expected as 3rd argument */

#define FIOCHKDSK	48
#define FIOCONTIG64	49
#define FIONCONTIG64	50
#define FIONFREE64	51
#define FIONREAD64	52
#define FIOSEEK64	53
#define FIOWHERE64	54
#define FIOTRUNC64	55

#define FIOCOMMITFS	56
#define FIODATASYNC	57		/* sync to I/O data integrity */
#define FIOLINK		58		/* create a link */
#define FIOUNLINK	59		/* remove a link */
#define FIOACCESS	60		/* support POSIX access() */
#define FIOPATHCONF	61		/* support POSIX pathconf() */
#define FIOFCNTL	62		/* support POSIX fcntl() */
#define FIOCHMOD	63		/* support POSIX chmod() */
#define FIOFSTATGET     64              /* get stat info - POSIX  */
#define FIOUPDATE       65              /* update dosfs default create option */


/* These are for HRFS but can be used for other future file systems */
#define FIOCOMMITPOLICYGETFS 66		/* get the file system commit policy */
#define FIOCOMMITPOLICYSETFS 67		/* set the file system commit policy */
#define FIOCOMMITPERIODGETFS 68		/* get the file system's periodic  */
                                        /* commit interval in milliseconds */
#define FIOCOMMITPERIODSETFS 69		/* set the file system's periodic  */
#define FIOFSTATFSGET64      70              /* get file system status info */
                                        /* commit interval in milliseconds */
    
/*======================================================================
  ioLib module support routines for other modules
======================================================================*/
int creat (const char *name, int flag);
int remove (const char *name);
int open (const char *name, int flags, int mode);
int close (int fd);
int read (int fd, char *buffer, int maxbytes);
int write (int fd, char *buffer, int nbytes);
int ioctl (int fd, int function, ...);

int stat(const char *name, struct stat *st);
int fstat(int file, struct stat *st);
int isatty(int fd);
int link( char *old, char *new);
int unlink( char *name);

#ifdef __cplusplus
}
#endif

#endif /* __IOLIB_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

