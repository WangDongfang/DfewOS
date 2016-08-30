/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**                                      
**                                 http://www.embedtools.com
** 
**---------------Fil Info---------------------------------------------------------------------------------
**  File name:              mtdtypes.h
**  Last modified Date:     
**  Last Version:           
**  Descriptions:           
** 
**--------------------------------------------------------------------------------------------------------
**  Modified by:            
**  Modified date:          
**  Version:                
**  Descriptions:           
**                          
**--------------------------------------------------------------------------------------------------------
**  Created by:             Yaozhan Liang
**  Created date:           Fri Feb 11 08:42:02 2011
**  Version:                1.0.0
**  Descriptions:           
**                          
*********************************************************************************************************/
#ifndef __MTDTYPES_H__
#define __MTDTYPES_H__

#ifndef int64_t
#define int64_t signed long long
#endif

#ifndef uint64_t
#define uint64_t unsigned long long
#endif
#ifndef u_long
#define u_long unsigned long
#endif

#ifndef uint32_t
#define uint32_t unsigned long
#endif
#ifndef u_int32_t
#define u_int32_t unsigned long
#endif
#ifndef u_int
#define u_int unsigned int
#endif
#ifndef u_char
#define u_char unsigned char
#endif

#if 0 /* Dfew */
#ifndef size_t
#define size_t unsigned long
#endif
#endif

#ifndef uint8_t
#define uint8_t unsigned char
#endif
#ifndef u_int8_t
#define u_int8_t unsigned char
#endif

#ifndef int8_t
#define int8_t signed char
#endif
#ifndef u16
#define u16 unsigned short
#endif

#ifndef loff_t
#define loff_t unsigned long
#endif
#ifndef ulong
#define ulong unsigned long
#endif

#ifndef u32
#define u32 unsigned long
#endif
#ifndef phys_addr_t
#define phys_addr_t unsigned long
#endif

#define cpu_to_le16(x) ((u16)(x))

#define WATCHDOG_RESET()

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#define ffs(x) generic_ffs(x)
static inline int generic_ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}


#ifndef NULL
#define NULL ((void*)0)
#endif

#define reset_timer mtd_reset_timer
#define get_timer mtd_get_timer
#define set_timer mtd_set_timer


#endif                                                                  /*  __MTDTYPES_H__              */
/*********************************************************************************************************
**   END FILE
*********************************************************************************************************/


