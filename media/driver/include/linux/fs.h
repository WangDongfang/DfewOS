
#ifndef	_LINUX_FS_H
#define	_LINUX_FS_H

#include <limits.h>
#include "ioctl.h"

#ifndef O_NONBLOCK
#define O_NONBLOCK	00004000
#endif

struct file	{
	unsigned int 		f_flags;
	void			*private_data;
};

struct inode {

};

#endif																	/*	_LINUX_FS_H					*/
