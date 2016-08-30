/*==============================================================================
** yaffsDrv.c -- yaffs driver.
**
** MODIFY HISTORY:
**
** 2012-03-28 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "lib/iosLib.h"
#include "yaffs_guts.h"

/*======================================================================
  Yaffs device
======================================================================*/
typedef struct _yaffs_dev {
    DEV_HDR  dev_hdr;
} YAFFS_DEV;


/*==============================================================================
 * - _yaffs_create()
 *
 * - create a yaffs file
 */
int _yaffs_create (YAFFS_DEV *YaffsDev, const char *file_name, int mode)
{
    int yaffs_fd;

    yaffs_fd = yaffs_open (file_name, O_CREAT | O_RDWR | O_TRUNC, mode);

    return yaffs_fd;
}

/*==============================================================================
 * - _yaffs_delete()
 *
 * - yaffs delete a file
 */
int _yaffs_delete (YAFFS_DEV *YaffsDev, const char *file_name)
{
    int yaffs_retval;

    yaffs_retval = yaffs_unlink(file_name);

    return yaffs_retval;
}

/*==============================================================================
 * - _yaffs_open()
 *
 * - open a yaffs file
 */
int _yaffs_open (YAFFS_DEV *YaffsDev, const char *file_name, int flags, int mode)
{
    int yaffs_fd;

    yaffs_fd = yaffs_open (file_name, flags, mode);

    if (yaffs_fd < 0) {
    	return OS_STATUS_ERROR;
    }

    return yaffs_fd;
}

/*==============================================================================
 * - _yaffs_read()
 *
 * - yaffs read
 */
int _yaffs_read (int yaffs_fd, void *buf, unsigned int nbyte)
{
    int read_bytes;

    read_bytes = yaffs_read (yaffs_fd, buf, nbyte);

    return read_bytes;
}

/*==============================================================================
 * - _yaffs_write()
 *
 * - yaffs write
 */
int _yaffs_write (int yaffs_fd, const void *buf, unsigned int nbyte)
{
    int write_bytes;

    write_bytes = yaffs_write (yaffs_fd, buf, nbyte);

    return write_bytes;
}

/*==============================================================================
 * - _yaffs_ioctl()
 *
 * - yaffs ioctl
 */
int _yaffs_ioctl (int yaffs_fd, int cmd, int arg)
{
    return 0;
}

/*==============================================================================
 * - _yaffs_close()
 *
 * - close a yaffs file
 */
int _yaffs_close (int yaffs_fd)
{
    yaffs_close (yaffs_fd);
    return 0;
}

/*==============================================================================
 * - yaffsDrv()
 *
 * - init yaffs hardware, and install to I/O system
 */
void yaffsDrv ()
{
    static int initialized = 0;
    int        drv_num;
    YAFFS_DEV *YaffsDev = NULL;

    if (initialized) {
        return;
    }

    drv_num = iosDrvInstall (_yaffs_create, _yaffs_delete,
                             _yaffs_open,   _yaffs_close,
                             _yaffs_read,   _yaffs_write,
                             _yaffs_ioctl);

    YaffsDev = malloc (sizeof (YAFFS_DEV));

    iosDevAdd ((DEV_HDR *)YaffsDev, "/yaffs2", drv_num);

    initialized = 1;
}

/*==============================================================================
** FILE END
==============================================================================*/

