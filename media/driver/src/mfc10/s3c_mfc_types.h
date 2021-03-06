/* linux/driver/media/video/mfc/s3c_mfc_types.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver 
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics 
 * http://www.samsungsemi.com/ 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_TYPES_H
#define _S3C_MFC_TYPES_H

#include <linux/types.h>

#ifdef _WRS_KERNEL
//#undef FALSE
//#undef TRUE
//typedef enum {FALSE, TRUE} S3_BOOL;
//#undef BOOL
//#define BOOL S3_BOOL
#else
//typedef enum {FALSE, TRUE} BOOL;
#endif                  // _WRS_KERNEL by cmj

#endif /* _S3C_MFC_TYPES_H */
