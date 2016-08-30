
#ifndef __IOSLIB_H__
#define __IOSLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================
  DEV_HDR - device header for all device structures
======================================================================*/
typedef struct dev_hdr {
    DL_NODE    dev_list_node; /* device linked list node */
    short      drv_num;        /* driver number for this device */
    const char *name;         /* device name */
    int        drvRefCount;   /* counter of concurrent driver invocations */
    int        drvRefFlag;    /* driver reference flags */
} DEV_HDR;

/*======================================================================
  Driver Routines Type those  are ptr to function returning int
======================================================================*/
typedef int (*DRV_FUNC_PTR) ();

/*======================================================================
  iosLib module support routines for other modules
======================================================================*/
OS_STATUS iosLibInit ();
int iosAllocFd ();
OS_STATUS iosFreeFd (int fd);
int iosDrvInstall (DRV_FUNC_PTR pCreate,    /* pointer to driver create function */
                   DRV_FUNC_PTR pRemove,    /* pointer to driver remove function */
                   DRV_FUNC_PTR pOpen,      /* pointer to driver open function */
                   DRV_FUNC_PTR pClose,     /* pointer to driver close function */
                   DRV_FUNC_PTR pRead,      /* pointer to driver read function */
                   DRV_FUNC_PTR pWrite,     /* pointer to driver write function */
                   DRV_FUNC_PTR pIoctl);    /* pointer to driver ioctl function */
OS_STATUS iosDevAdd (DEV_HDR    *pDevHdr, /* pointer to device's structure */
                     const char *name,    /* name of device */
                     int         drv_num);/* no. of servicing driver, */
		                                  /* returned by iosDrvInstall() */
DEV_HDR *iosDevMatch (const char *name);
DEV_HDR *iosDevFind (const char *name,         /* name of the device */
                     const char *(*pNameTail));/* where to put ptr to tail of name */
int iosSaveDevNum (int fd, DEV_HDR *pDevHdr);

int iosCreate (DEV_HDR *pDevHdr, const char *file_name, int mode);
int iosDelete (DEV_HDR *pDevHdr, const char *file_name);
int iosOpen (DEV_HDR *pDevHdr, const char *file_name, int flags, int mode);
int iosClose (int fd);
int iosRead (int fd, char *buffer, int maxbytes);
int iosWrite (int fd, char *buffer, int nbytes);
int iosIoctl (int fd, int cmd, int arg);

void *iosGetOpenData (int fd);

#ifdef __cplusplus
}
#endif

#endif /* __IOSLIB_H__ */

/*==============================================================================
** FILE END
==============================================================================*/
