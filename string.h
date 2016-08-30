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

char *strcpy (char *s1, const char *s2);
char *strncpy (char *s1, const char *s2, size_t n);
size_t strlcpy (char *dst, const char *src, size_t siz);
size_t strlen (const char *s);
int strcmp (const char *s1, const char *s2);
int strncmp (const char *s1, const char *s2, size_t n);
int strncasecmp (const char *s1, const char *s2, size_t n);
char *strcat (char *dest, const char *append);
char *strncat (char *dest, const char *append, size_t n);
char *strchr (const char *s, int c);
char *strrchr (const char *s, int c);
void *memchr (const void *m, int c, size_t n);


void bcopy (const char *source, char *destination, int nbytes);
void *memcpy (void *destination, const void *source, size_t size);
void *memmove (void *destination, const void *source, size_t size);
void bfill (FAST char *buf, int nbytes, FAST int ch);
void *memset (void *m, int c,  size_t size);
void *memrchr(const void *str, int c, size_t n);

int memcmp (const void *m1, const void *m2, size_t n);

#ifndef isspace
int isspace(int c);
#endif
#ifndef islower
int islower(int c);
#endif
#ifndef isupper
int isupper(int c);
#endif
#ifndef isalnum
int isalnum(int c);
#endif
#ifndef ispunct
int ispunct(int c);
#endif
#ifndef isdigit
int isdigit(int c);
#endif
#ifndef isprint
int isprint(int c);
#endif
#ifndef tolower
int tolower(int c);
#endif
#ifndef toupper
int toupper(int c);
#endif
#ifndef isxdigit
int isxdigit(int c);
#endif

int abs(int i);

#define EOS '\0'

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STRING_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

