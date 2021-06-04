/*==============================================================================
** types.h -- this os used types define.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __TYPES_H__
#define __TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;
typedef unsigned int   size_t;
typedef void (*FUNC_PTR) (uint32 arg1, uint32 arg2);

#define OS_STATUS        int
#define OS_STATUS_OK     0
#define OS_STATUS_ERROR -1

#define FAST    register

#ifndef NULL
#define NULL    (void *)0
#endif

#if !defined(FALSE) || (FALSE!=(0))
#define FALSE   (0)
#endif

#if !defined(TRUE) || (TRUE!=(1))
#define TRUE    (1)
#endif

#ifndef BOOL
#define BOOL    int
#endif

#ifndef EOS
#define EOS     '\0' /* C string terminator */
#endif

#ifndef EOF
#define EOF     (-1) /* End of file */
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TYPES_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

