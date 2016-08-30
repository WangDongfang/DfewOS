/*==============================================================================
** sys_arch.h -- hardware and software system relative define.
**
** MODIFY HISTORY:
**
** 2011-08-24 wdf Create.
==============================================================================*/
#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include <dfewos.h>

#define BYTE_ORDER        LITTLE_ENDIAN
#define LWIP_PROVIDE_ERRNO    /* LWIP 在 arch.h 提供错误号 */
/*======================================================================
  数据类型
======================================================================*/
typedef unsigned char                u8_t;
typedef signed   char                s8_t;
typedef unsigned short               u16_t;
typedef signed   short               s16_t;
typedef unsigned int                 u32_t;
typedef signed   int                 s32_t;

typedef unsigned long                mem_ptr_t;

#define U16_F                       "u"
#define U32_F                       "u"
#define S16_F                       "d"
#define S32_F                       "d"
#define X16_F                       "X"
#define X32_F                       "X"
#define SZT_F                       "u"
#define X8_F                        "02X"

/*======================================================================
  OS 数据类型
======================================================================*/
typedef SEM_MUX*                     sys_mutex_t;
typedef SEM_CNT*                     sys_sem_t;
typedef MSG_QUE*                     sys_mbox_t;
typedef OS_TCB*                      sys_thread_t;

#define SYS_MUTEX_NULL               NULL
#define SYS_SEM_NULL                 NULL
#define SYS_MBOX_NULL                NULL

/*======================================================================
  编译器结构缩排相关
======================================================================*/
#define PACK_STRUCT_FIELD(x)        x
#define PACK_STRUCT_STRUCT          __attribute__((packed))
#define PACK_STRUCT_BEGIN           
#define PACK_STRUCT_END             

/*********************************************************************************************************
  调试输出
*********************************************************************************************************/
#define LWIP_PLATFORM_DIAG(x)	    {   serial_printf x;   }
#define LWIP_PLATFORM_ASSERT(x)     {   serial_printf("lwip assert: %s\n", x); while(1); }

/*********************************************************************************************************
  关键区域保护
*********************************************************************************************************/
#define SYS_ARCH_DECL_PROTECT(x)    int cspr_c
#define SYS_ARCH_PROTECT(x)         cspr_c = CPU_LOCK()
#define SYS_ARCH_UNPROTECT(x)       CPU_UNLOCK(cspr_c)

/*********************************************************************************************************
  Measurement calls made throughout lwip, these can be defined to nothing.
*********************************************************************************************************/
#define PERF_START                                                      /* null definition              */
#define PERF_STOP(x)                                                    /* null definition              */

#endif /* __SYS_ARCH_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

