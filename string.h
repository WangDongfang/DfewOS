/*==============================================================================
** string.h -- mini libc header
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/

#ifndef __STRING_H__
#define __STRING_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "types.h"

/*=======================================================
 * string function
**=====================================================*/
char  *strcpy (char *s1, const char *s2);
char  *strncpy (char *s1, const char *s2, size_t n);
size_t strlcpy (char *dst, const char *src, size_t siz);
size_t strlen (const char *s);
int    strcmp (const char *s1, const char *s2);
int    strncmp (const char *s1, const char *s2, size_t n);
int    strncasecmp (const char *s1, const char *s2, size_t n);
char  *strcat (char *dest, const char *append);
char  *strncat (char *dest, const char *append, size_t n);
char  *strchr (const char *s, int c);
char  *strrchr (const char *s, int c);
void  *memchr (const void *m, int c, size_t n);

/*=======================================================
 * memory function
**=====================================================*/
void bcopy (const char *source, char *destination, int nbytes);
void bfill (FAST char *buf, int nbytes, FAST int ch);
void *memcpy (void *destination, const void *source, size_t size);
void *memmove (void *destination, const void *source, size_t size);
void *memset (void *m, int c,  size_t size);
void *memrchr(const void *str, int c, size_t n);
int   memcmp (const void *m1, const void *m2, size_t n);

/*=======================================================
 * ctype
**=====================================================*/
#ifndef isalnum
int isalnum(int c);
#endif
#ifndef isalpha
int isalpha(int c);
#endif
#ifndef iscntrl
int iscntrl(int c);
#endif
#ifndef isdigit
int isdigit(int c);
#endif
#ifndef isgraph
int isgraph(int c);
#endif
#ifndef islower
int islower(int c);
#endif
#ifndef isupper
int isupper(int c);
#endif
#ifndef isprint
int isprint(int c);
#endif
#ifndef ispunct
int ispunct(int c);
#endif
#ifndef isspace
int isspace(int c);
#endif
#ifndef isxdigit
int isxdigit(int c);
#endif
#ifndef isblank
int isblank(int c);
#endif

/*=======================================================
 * convert
**=====================================================*/
int tolower(int c);
int toupper(int c);
int toascii(int c);
int abs(int i);


/*=======================================================
 * stdlib.h
**=====================================================*/
void srand (unsigned int seed);
int  rand ();


/*=======================================================
 * stdarg.h
**=====================================================*/
typedef __builtin_va_list  va_list;
#define va_start(v, l)   __builtin_va_start(v,l)
#define va_end(v)        __builtin_va_end(v)
#define va_arg(v, l)     __builtin_va_arg(v,l)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STRING_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

