/*==============================================================================
** iosLib.c -- I/O system library.
**
** MODIFY HISTORY:
**
** 2012-03-07 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include <string.h>
#include "iosLib.h"

/*======================================================================
  configs
======================================================================*/
#define IOS_MAX_FILE_DES    50
#define IOS_MAX_DRIVERS     50

/*======================================================================
  FD_ENTRY - entries in FILE_DESCRIPTOR TABLE
======================================================================*/
typedef struct fd_entry {
    int   drv_num;     /* driver number */
    void *open_data;   /* store driver's open routine return value */
    BOOL  inuse;
} FD_ENTRY;

/*======================================================================
  DRV_ENTRY - entries in DRIVER TABLE
======================================================================*/
typedef struct drv_entry {
    DRV_FUNC_PTR    de_create;
    DRV_FUNC_PTR    de_delete;
    DRV_FUNC_PTR    de_open;
    DRV_FUNC_PTR    de_close;
    DRV_FUNC_PTR    de_read;
    DRV_FUNC_PTR    de_write;
    DRV_FUNC_PTR    de_ioctl;
    BOOL        de_inuse;
} DRV_ENTRY;

/*======================================================================
  Global Variables
======================================================================*/
static FD_ENTRY  _G_fd_table[IOS_MAX_FILE_DES]; /* FILE_DESCRIPTOR TAB*/
static DL_LIST   _G_dev_list = {NULL, NULL};    /* DEVICE LIST */
static DRV_ENTRY _G_drv_table[IOS_MAX_DRIVERS]; /* DRIVER TABLE */

/*======================================================================
  I/O System lock
======================================================================*/
static  SEM_MUX         _G_ios_dev;
#define IOS_LOCK_INIT() semM_init(&_G_ios_dev)
#define IOS_LOCK()      semM_take(&_G_ios_dev, WAIT_FOREVER)
#define IOS_UNLOCK()    semM_give(&_G_ios_dev)


/*==============================================================================
 * - iosLibInit()
 *
 * - init I/O system
 */
OS_STATUS iosLibInit ()
{
    IOS_LOCK_INIT();

    /*
     * reserve 0,1,2 fd
     */
    iosAllocFd ();
    iosAllocFd ();
    iosAllocFd ();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - iosAllocFd()
 *
 * - alloc a file descriptor entry
 */
int iosAllocFd ()
{
    int fd = 0;

    IOS_LOCK();

    /*
     * try to find a fd slot
     */
    for (; fd < IOS_MAX_FILE_DES; fd++) {
        if (!_G_fd_table[fd].inuse) { /* not used */
            break;
        }
    }

    /*
     * can't find a fd slot
     */
    if (fd == IOS_MAX_FILE_DES) {
        IOS_UNLOCK();
        return -1;
    }

    /*
     * mark the fd entry is used
     */
    _G_fd_table[fd].inuse = TRUE;

    IOS_UNLOCK();

    return fd;
}

/*==============================================================================
 * - iosFreeFd()
 *
 * - free a file descriptor entry
 */
OS_STATUS iosFreeFd (int fd)
{
    if (fd < 0 || fd >= IOS_MAX_FILE_DES) {
        return OS_STATUS_ERROR;
    }

    IOS_LOCK();

    _G_fd_table[fd].inuse = FALSE;

    IOS_UNLOCK();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - iosDrvInstall()
 *
 * - install a kernel I/O DRIVER TABLE
 */
int iosDrvInstall (DRV_FUNC_PTR pCreate,    /* pointer to driver create function */
                   DRV_FUNC_PTR pRemove,    /* pointer to driver remove function */
                   DRV_FUNC_PTR pOpen,      /* pointer to driver open function */
                   DRV_FUNC_PTR pClose,     /* pointer to driver close function */
                   DRV_FUNC_PTR pRead,      /* pointer to driver read function */
                   DRV_FUNC_PTR pWrite,     /* pointer to driver write function */
                   DRV_FUNC_PTR pIoctl)     /* pointer to driver ioctl function */
{
    int drv_num = 0;

    IOS_LOCK();

    /*
     * try to find a driver slot
     */
    for (; drv_num < IOS_MAX_DRIVERS; drv_num++) {
        if (!_G_drv_table[drv_num].de_inuse) {
            break;
        }
    }

    /*
     * can't find a driver slot
     */
    if (drv_num == IOS_MAX_DRIVERS) {
        IOS_UNLOCK();
        return -1;
    }

    /*
     * store the driver routines
     */
    _G_drv_table[drv_num].de_inuse  = TRUE;

    _G_drv_table[drv_num].de_create = pCreate;
    _G_drv_table[drv_num].de_delete = pRemove;
    _G_drv_table[drv_num].de_open   = pOpen;
    _G_drv_table[drv_num].de_close  = pClose;
    _G_drv_table[drv_num].de_read   = pRead;
    _G_drv_table[drv_num].de_write  = pWrite;
    _G_drv_table[drv_num].de_ioctl  = pIoctl;

    IOS_UNLOCK();

    return drv_num;
}

/*==============================================================================
 * - iosDevAdd()
 *
 * - add a device to the kernel I/O system DEVICE LIST
 */
OS_STATUS iosDevAdd (DEV_HDR    *pDevHdr, /* pointer to device's structure */
                     const char *name,    /* name of device */
                     int         drv_num) /* no. of servicing driver, */
		                                  /* returned by iosDrvInstall() */
{
    DEV_HDR *pDevMatch;
    char    *copiedName;
    
    /*
     * check argument
     */
    if (pDevHdr == NULL ||
        name == NULL ||
        *name == EOS ||
        drv_num < 0 ||
        drv_num >= IOS_MAX_DRIVERS ||
        _G_drv_table[drv_num].de_inuse == FALSE) {

        return OS_STATUS_ERROR;
	}
    
    /* 
     * copy device name
     */
    copiedName = (char *)malloc ((unsigned)(strlen (name) + 1));
    if (copiedName == NULL) {
        return OS_STATUS_ERROR;
    }
    strcpy (copiedName, name);

    IOS_LOCK();

    pDevMatch = iosDevMatch (name); /* check already devices */

    /*
     * Don't add a device with a name that already exists in the device list.
     * iosDevMatch will return NON-NULL if a device name is a substring of the
     * named argument, so check that the two names really are identical.
     */
    if ((pDevMatch != NULL) && (strcmp (pDevMatch->name, name) == 0)) {
        IOS_UNLOCK ();
        free (copiedName);
        return OS_STATUS_ERROR;
	}

    /*
     * add device to DEVICE LIST
     */
    pDevHdr->name = copiedName;
    pDevHdr->drv_num = (short)drv_num;
    pDevHdr->drvRefCount = 0;
    pDevHdr->drvRefFlag = 0;      /* No flags initially set */

    dlist_add (&_G_dev_list, (DL_NODE *)pDevHdr);
    
    IOS_UNLOCK();

    return OS_STATUS_OK;
}

/*==============================================================================
 * - iosDevMatch()
 *
 * - find kernel device whose name matches specified string
 */
DEV_HDR *iosDevMatch (const char *name)
{
    DEV_HDR *  pDevHdr = (DEV_HDR *)DL_FIRST(&_G_dev_list);
    int        len;
    DEV_HDR *  pBestDevHdr = NULL;
    int        maxLen = 0;

    for (; pDevHdr != NULL; pDevHdr = (DEV_HDR *)DL_NEXT(pDevHdr)) {
        len = strlen (pDevHdr->name);
        if (strncmp (pDevHdr->name, name, len) == 0) {
            /*
             * This device name is initial substring of <name>;
             * if it is longer than any other such device name so far,
             * remember it.
             */
            if (len > maxLen) {
                pBestDevHdr = pDevHdr;
                maxLen = len;
            }
        }
    }
    return (pBestDevHdr);
}

/*==============================================================================
 * - iosDevFind()
 *
 * - find kernel device and jump device name
 */
DEV_HDR *iosDevFind (const char *name,         /* name of the device */
                     const char *(*pNameTail)) /* where to put ptr to tail of name */
{
    DEV_HDR *pDevHdr;

    IOS_LOCK ();

    pDevHdr = (name != NULL && *name != EOS) ? iosDevMatch (name) : NULL;

    if (pDevHdr != NULL) {
        if (pNameTail != NULL) {
            *pNameTail = name + strlen (pDevHdr->name);
        }
	}

    IOS_UNLOCK ();

    return (pDevHdr);
}

int iosSaveDevNum (int fd, DEV_HDR *pDevHdr)
{
    _G_fd_table[fd].drv_num = pDevHdr->drv_num;
    return 0;
}

/*==============================================================================
 * - iosCreate()
 *
 * - invoke driver to create new file
 */
int iosCreate (DEV_HDR *pDevHdr, const char *file_name, int mode)
{
    DRV_FUNC_PTR drvCreate = _G_drv_table[pDevHdr->drv_num].de_create;

    return ((*drvCreate) (pDevHdr, file_name, mode));
}

/*==============================================================================
 * - iosDelete()
 *
 * - invoke driver to delete file
 */
int iosDelete (DEV_HDR *pDevHdr, const char *file_name)
{
    DRV_FUNC_PTR drvDelete = _G_drv_table[pDevHdr->drv_num].de_delete;

    return ((*drvDelete)(pDevHdr, file_name));
}

/*==============================================================================
 * - iosOpen()
 *
 * - invoke driver to open file
 */
int iosOpen (DEV_HDR *pDevHdr, const char *file_name, int flags, int mode)
{
    DRV_FUNC_PTR drvOpen = _G_drv_table[pDevHdr->drv_num].de_open;

    return (*drvOpen)(pDevHdr, file_name, flags, mode);
}

/*==============================================================================
 * - iosClose()
 *
 * - invoke driver to close file
 */
int iosClose (int fd)
{
    DRV_FUNC_PTR drvClose = _G_drv_table[_G_fd_table[fd].drv_num].de_close;

    return (*drvClose)(_G_fd_table[fd].open_data);
}

/*==============================================================================
 * - iosRead()
 *
 * - invoke driver read routine
 */
int iosRead (int fd, char *buffer, int maxbytes)
{
    DRV_FUNC_PTR drvRead = _G_drv_table[_G_fd_table[fd].drv_num].de_read;

    return (*drvRead)(_G_fd_table[fd].open_data, buffer, maxbytes);
}

/*==============================================================================
 * - iosWrite()
 *
 * - invoke driver write routine
 */
int iosWrite (int fd, char *buffer, int nbytes)
{
    DRV_FUNC_PTR drvWrite = _G_drv_table[_G_fd_table[fd].drv_num].de_write;

    return (*drvWrite)(_G_fd_table[fd].open_data, buffer, nbytes);
}

/*==============================================================================
 * - iosIoctl()
 *
 * - invoke driver ioctl routine from kernel
 */
int iosIoctl (int fd, int cmd, int arg)
{
    DRV_FUNC_PTR drvIoCtl = _G_drv_table[_G_fd_table[fd].drv_num].de_ioctl;

    return (*drvIoCtl)(_G_fd_table[fd].open_data, cmd, arg);
}

/*==============================================================================
 * - iosGetOpenData()
 *
 * - get a opened file descriptor data
 */
void *iosGetOpenData (int fd)
{
    return _G_fd_table[fd].open_data;
}
/**********************************************************************************************************
  show the device brief which in _G_dev_list
**********************************************************************************************************/
static OS_STATUS _show_dev_info (DEV_HDR *pDevHdr, int *pCnt)
{
    (*pCnt)++;
    serial_printf("%2d %s\n", *pCnt, pDevHdr->name);
    return OS_STATUS_OK;
}
/*==============================================================================
 * - ios_devs_show ()
 *
 * - show devices in kernel I/O system
 */
int ios_devs_show ()
{
    int cnt = 0;
    dlist_each (&_G_dev_list, (EACH_FUNC_PTR)_show_dev_info, (int)&cnt);
    return cnt;
}

/*==============================================================================
 ** FILE END
==============================================================================*/

