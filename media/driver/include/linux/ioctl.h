/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               ioctl_cmd.h
** Last modified Date:      2010-11-09
** Last Version:            1.0.0
** Descriptions:            IOCTL ������Ϻ�
**
**--------------------------------------------------------------------------------------------------------
** Created by:              jiaojinxing
** Created date:            2010-11-09
** Version:                 1.0.0
** Descriptions:            IOCTL ������Ϻ�
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
*********************************************************************************************************/
#ifndef __IOCTL_CMD_H__
#define __IOCTL_CMD_H__

#define _IOC_NRBITS                 8                                   /*  �������λ��                */
#define _IOC_TYPEBITS               8                                   /*  ��������λ��                */
#define _IOC_SIZEBITS               14                                  /*  ���ݴ����Сλ��            */
#define _IOC_DIRBITS                2                                   /*  ���ݴ��䷽��λ��            */


#define _IOC_NRMASK                 ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK               ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK               ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK                ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT                0                                   /*  �������ƫ��                */
#define _IOC_TYPESHIFT              (_IOC_NRSHIFT   + _IOC_NRBITS)      /*  ��������ƫ��                */
#define _IOC_SIZESHIFT              (_IOC_TYPESHIFT + _IOC_TYPEBITS)    /*  ���ݴ����Сƫ��            */
#define _IOC_DIRSHIFT               (_IOC_SIZESHIFT + _IOC_SIZEBITS)    /*  ���ݴ��䷽��ƫ��            */

#define _IOC_NONE                   0U                                  /*  �����ݴ���                  */
#define _IOC_WRITE                  1U                                  /*  ���豸д������              */
#define _IOC_READ                   2U                                  /*  ���豸��ȡ����              */

#define _IOC_TYPECHECK(type)        (sizeof(type))                      /*  ���ͼ��                    */

/*
 * IOCTL ������Ϻ�
 */
#define _IOC(dir, type, nr, size)                                \
                                    (((dir) << _IOC_DIRSHIFT)  | \
                                    ((type) << _IOC_TYPESHIFT) | \
                                    ((nr)   << _IOC_NRSHIFT)   | \
                                    ((size) << _IOC_SIZESHIFT))

/*
 * �����ݴ��� IOCTL ������Ϻ�
 */
#define _IO(type, nr)               _IOC(_IOC_NONE, (type), (nr), 0)

/*
 * ���豸��ȡ���� IOCTL ������Ϻ�
 */
#define _IOR(type, nr, size)        _IOC(_IOC_READ, (type), (nr), (_IOC_TYPECHECK(size)))

/*
 * ���豸д������ IOCTL ������Ϻ�
 */
#define _IOW(type, nr, size)        _IOC(_IOC_WRITE, (type), (nr), (_IOC_TYPECHECK(size)))

/*
 * �����豸д������, �ִ��豸��ȡ���� IOCTL ������Ϻ�
 */
#define _IOWR(type, nr, size)       _IOC((_IOC_READ | _IOC_WRITE), (type), (nr), (_IOC_TYPECHECK(size)))



/* used to decode them.. */
#define _IOC_DIR(nr)        (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)       (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)         (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)       (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)



#endif                                                                  /*  __IOCTL_CMD_H__             */
/*********************************************************************************************************
    END FILE
*********************************************************************************************************/
