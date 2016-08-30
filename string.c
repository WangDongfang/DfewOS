/*==============================================================================
** string.c -- the simple libc.
**
** MODIFY HISTORY:
**
** 2011-08-15 wdf Create.
==============================================================================*/
#include "types.h"
#include "string.h"


/**********************************************************************************************************
  copy one string to another (ANSI)
**********************************************************************************************************/
char *strcpy (char       *s1,       /* string to copy to */
              const char *s2)       /* string to copy from */
{
    FAST char *d = s1;
    while ((*d++ = *s2++) != '\0')
        ;

    return s1;
}

/**********************************************************************************************************
  copy the first <n> characters from one string to another (ANSI)
**********************************************************************************************************/
char *strncpy (char       *s1,       /* string to copy to */
               const char *s2,       /* string to copy from */
               size_t      n)        /* max no. of characters to copy */
{
    FAST char *d = s1;

    if (n != 0) {
        while ((*d++ = *s2++) != '\0') {    /* copy <s2>, checking size <n> */
            if (--n == 0) {
                return (s1);
            }
        }

        while (--n > 0) {
            *d++ = 0;            /* NULL terminate string */
        }
    }

    return (s1);
}

/**********************************************************************************************************
  copy <src> to string <dst> of size <siz>. At most siz-1 characters will be copied.
  Always NULL terminates (unless <siz> == 0).
  Returns strlen(src); if retval >= siz, truncation occurred.
**********************************************************************************************************/
size_t  strlcpy (char *dst, const char *src, size_t siz)
{
    FAST const char *p = src;
    
    if (siz > 0) {
        while (*p != '\0' && siz > 1) {
            *dst++ = *p++;
            siz--;
        }
        *dst = '\0';
    }
    
    for (; *p != '\0'; ++p)
        /* nothing */;
    
    return ((size_t)(p - src));
}


/**********************************************************************************************************
  compare two strings lexicographically (ANSI)
**********************************************************************************************************/
int strcmp (const char *s1, const char *s2)
{
    while ((*s1 != 0) && (*s2 != 0)) {
        if (*s1 != *s2) {
            break;
        }
        s1++; s2++;
    }
    return (*s1 - *s2);
}

/**********************************************************************************************************
  compare the first <n> characters of two strings lexicographically (ANSI)
**********************************************************************************************************/
int strncmp (const char *s1,
             const char *s2,
             size_t      n)
{
    if (n == 0)
        return 0;

    while (*s1++ == *s2++) {
        if ((s1[-1] == '\0') || (--n == 0))
            return 0;
    }

    return (int)((unsigned char)s1[-1] - (unsigned char)s2[-1]);
}

/**********************************************************************************************************
  compare first n characters of two strings lexicographically without regard to case (POSIX)
**********************************************************************************************************/
int strncasecmp (const char *s1,   /* string to compare */
                 const char *s2,   /* string to compare <s1> to */
                 size_t      n)    /* man no. of characters to compare */
{
    int c = 0;

    int tolower(int c);
    /*
     * We could break out of the loop in one of three cases:
     * (a) when n <= 0
     * (b) when there is differences in the chars being compared
     * (c) when there are no characters left in one of the strings
     *
     * In case (a), we return the difference (c) computed for the previous
     *              character that was seen.
     * In case (b) or (c), we return the difference computed for the
     *             current character.
     */
    for (; 
         n > 0 && (c = tolower((unsigned char)*s1) - tolower((unsigned char)*s2)) == 0 && *s1; 
         ++s1, ++s2, --n
        );

    return (c);
}

/**********************************************************************************************************
  determine the length of a string (ANSI)
**********************************************************************************************************/
size_t strlen (const char *s)
{
    FAST const char *sc;

    for (sc = s; *sc != '\0'; ++sc)
        /* nothing */;

    return sc - s;
}

/**********************************************************************************************************
  concatenate one string to another (ANSI)
**********************************************************************************************************/
char *strcat (char       *dest,
              const char *append)
{
    FAST char *s = dest;

    while (*s++ != '\0')
        ;

    s--;

    while ((*s++ = *append++) != '\0') 
        ;

    return dest;
}

/**********************************************************************************************************
  concatenate one string to another (ANSI)
  appends up to <n> characters from string <append> to the end of string <dest>.
  <dest> terminated by '\0' aways.
**********************************************************************************************************/
char *strncat (char       *dest, 
               const char *append,
               size_t n)
{
    char *s = dest;

    while (*s++ != '\0')
        ;

    s--;

    while ((*s++ = *append++) != '\0') {
        if (--n == 0) {
            *s = '\0';
            break;
        }
    }

    return dest;

}

/**********************************************************************************************************
  find char in string (ANSI)
**********************************************************************************************************/
char *strchr (const char *s, int c)
{
    while (*s != (char)c) {
        if (*s++ == '\0') {
            return NULL;
        }
    }
    return (char *)s;
}

/**********************************************************************************************************
  find the last occurrence of a character in a string (ANSI)
  RETURNS:
  A pointer to the last occurrence of the character, or NULL if the character is not found.
**********************************************************************************************************/
char *strrchr (const char *s, int c)
{
    char *save = NULL;

    do {                  /* loop, searching for character */
        if (*s == (char)c) {
            save = (char *)s;
        }
    } while (*s++ != EOS);

    return save;
}

/**********************************************************************************************************
  search a block of memory for a character (ANSI)
**********************************************************************************************************/
void *memchr (const void *m,        /* block of memory */
              int         c,        /* character to search for */
              size_t      n)        /* size of memory to search */
{
    unsigned char *p = (unsigned char *)(m);

    if (n != 0) {
        do {
            if (*p++ == (unsigned char) c)
                return ((void *)(p - 1));
        } while (--n != 0);
    }

    return (NULL);
}

/**********************************************************************************************************
  find a char in memory from right, return the char addr if find or NULL
**********************************************************************************************************/
void *memrchr(const void *str, int c, size_t n)
{
    register char *pcBuf = (char *)str;

    pcBuf += (n - 1);

    while (pcBuf >= (char *)str) {
        if (*pcBuf == c) {
            return  ((void *)pcBuf);
        }
        pcBuf--;
    }

    return  (NULL);
}


#define ALIGNMENT   3
void bcopy (const char *source,     /* pointer to source buffer      */
            char *destination,      /* pointer to destination buffer */
            int nbytes)             /* number of bytes to copy       */
{
    FAST char *dstend;
    FAST long *src;
    FAST long *dst;
    int tmp = destination - source;

    if (tmp <= 0 || tmp >= nbytes) {
        /* forward copy */

        dstend = destination + nbytes;

        /* do byte copy if less than ten or alignment mismatch */

        if (nbytes < 10 || (((int)destination ^ (int)source) & ALIGNMENT))
            goto byte_copy_fwd;

        /* if odd-aligned copy byte */

        while ((int)destination & ALIGNMENT)
            *destination++ = *source++;

        src = (long *) source;
        dst = (long *) destination;

        do {
            *dst++ = *src++;
        } while (((char *)dst + sizeof (long)) <= dstend);

        destination = (char *)dst;
        source      = (char *)src;

byte_copy_fwd:
        while (destination < dstend)
            *destination++ = *source++;
    } else {
        /* backward copy */

        dstend       = destination;
        destination += nbytes;
        source      += nbytes;

        /* do byte copy if less than ten or alignment mismatch */

        if (nbytes < 10 || (((int)destination ^ (int)source) & ALIGNMENT))
            goto byte_copy_bwd;

        /* if odd-aligned copy byte */

        while ((int)destination & ALIGNMENT)
            *--destination = *--source;

        src = (long *) source;
        dst = (long *) destination;

        do {
            *--dst = *--src;
        } while (((char *)dst - sizeof(long)) >= dstend);

        destination = (char *)dst;
        source      = (char *)src;

byte_copy_bwd:
        while (destination > dstend)
            *--destination = *--source;
    }
}

/**********************************************************************************************************
  copy memory from one location to another (ANSI)
**********************************************************************************************************/
void *memcpy (void       *destination,   /* destination of copy */
              const void *source,        /* source of copy */
              size_t      size)          /* size of memory to copy */
{
    bcopy ((char *) source, (char *) destination, (size_t) size);
    return (destination);
}

/**********************************************************************************************************
  copy memory from one location to another (ANSI)
**********************************************************************************************************/
void *memmove (void       *destination,   /* destination of copy */
               const void *source,        /* source of copy */
               size_t      size)          /* size of memory to copy */
{
    bcopy ((char *)source, (char *)destination, (int)size);
    return (destination);
}


/*******************************************************************************
*
* bfill - fill a buffer with a specified character
*
* This routine fills the first <nbytes> characters of a buffer with the
* character <ch>.  Filling is done in the most efficient way possible,
* which may be long-word, or even multiple-long-word stores, on some
* architectures.  In general, the fill will be significantly faster if
* the buffer is long-word aligned.  (For filling that is restricted to
* byte stores, see the manual entry for bfillBytes().)
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* SEE ALSO: bfillBytes()
*/

void bfill (FAST char *buf,           /* pointer to buffer              */
            int        nbytes,        /* number of bytes to fill        */
            FAST int   ch)              /* char with which to fill buffer */
{
    FAST long *pBuf;
    char *bufend = buf + nbytes;
    FAST char *buftmp;
    FAST long val;

    if (nbytes < 10)
        goto byte_fill;

    val = (ch << 24) | (ch << 16) | (ch << 8) | ch;

    /* start on necessary alignment */

    while ((int)buf & ALIGNMENT)
        *buf++ = ch;

    buftmp = bufend - sizeof (long); /* last word boundary before bufend */

    pBuf = (long *)buf;

    /* fill 4 bytes at a time; don't exceed buf endpoint */

    do {
        *pBuf++ = val;
    } while ((char *)pBuf < buftmp);

    buf = (char *)pBuf - sizeof (long);

    /* fill remaining bytes one at a time */

byte_fill:
    while (buf < bufend)
        *buf++ = ch;
}

/**********************************************************************************************************
  set a block of memory (ANSI)
**********************************************************************************************************/
void *memset (void  *m,               /* block of memory */
              int    c,               /* character to store */
              size_t size)            /* size of memory */
{
    bfill ((char *) m, (int) size, c);
    return (m);
}

/**********************************************************************************************************
  compare memory data
**********************************************************************************************************/
int memcmp (const void *m1, const void *m2, size_t n)
{
    const unsigned char *p1 = m1;
    const unsigned char *p2 = m2;
    if (n == 0)
        return 0;

    while (*p1++ == *p2++) {
        if (--n == 0)
            return 0;
    }

    return (int)(p1[-1] - p2[-1]);
}

#if 0
/**********************************************************************************************************
  simple determine a character is bank or not
**********************************************************************************************************/
int isspace(int c)
{
    return ((c == ' ') || (c == '\t'));
}

/**********************************************************************************************************
  convert an upper-case letter to its lower-case equivalent (ANSI)
**********************************************************************************************************/
int tolower(int c)
{
    return (c |= 0x20);
}
#endif

/**********************************************************************************************************
  compute the absolute value of an integer (ANSI)
**********************************************************************************************************/
int abs (int i)
{
    return (i >= 0 ? i : -i);
}

/**********************************************************************************************************
  conver string to unsigned integer (ANSI)
**********************************************************************************************************/
#define ULONG_MAX   4294967295LU
#define ERANGE      34 /* Math result not representable */
int errno;

#define BASE_MAX  36
static const char digits[] = { "0123456789abcdefghijklmnopqrstuvwxyz" };
static const char ndigs[BASE_MAX + 1] = { /* unsigned long 32-bits! */
    0, 0, 33, 21, 17, 14, 13, 12, 11, 11, /* base:  0 -- 9 */
    10, 10, 9, 9, 9, 9, 9, 8, 8, 8,       /* base: 10 -- 19 */
    8, 8, 8, 8, 7, 7, 7, 7, 7, 7,         /* base: 20 -- 29 */
    7, 7, 7, 7, 7, 7, 7                   /* base: 30 -- 36 */
};

unsigned long _Stoul (const char *s, char **endptr, int base)
{
    const char *sc, *sd;
    const char *s1, *s2;
    char sign;
    char dig;
    int  n;
    unsigned long x, y;

    /* skip leading space */
    for (sc = s; isspace(*sc); sc++)
        ;

    /* get sign */
    sign = (*sc == '-' || *sc == '+') ? *sc++ : '+';

    /* get base */
    if (base < 0 || base == 1 || base > BASE_MAX) {
        if (endptr)
            *endptr = (char *)s;
        return 0;
    } else if (base) { /* user specify base */
        if (base == 16 && *sc == '0' && (sc[1] == 'x' || sc[1] == 'X'))
            sc += 2;
    } else if (*sc != '0') {
        base = 10; /* user no specify base, and str no started by '0' */
    } else if (sc[1] == 'x' || sc[1] == 'X') {
        base = 16; /* user no specify base, and str started by '0x|0X' */
        sc += 2;
    } else {
        base = 8; /* user no specify base, and str started by '0' */
    }

    /* skip leading zeros */
    for (s1 = sc; *sc == '0'; ++sc)
        ;

    x = 0;

    /* accumulate digits */
    for (s2 = sc; (sd = memchr(digits, tolower(*sc), base)) != NULL; sc++) {
        y = x;
        dig = (char)(sd - digits);
        x = x * base + dig;
    }

    /* i can read a char */
    if (s1 == sc) {
        if (endptr)
            *endptr = (char *)s;
        return 0;
    }

    n = sc - s2 - ndigs[base]; 
    if (n < 0) {
        ; /* no overflow */
    } else if (n > 0 ||
               x < x - dig ||
               (x - dig) / base != y) { /* overflow */

        errno = ERANGE;
        x = ULONG_MAX;
    }

    if (sign == '-')
        x = -x;

    if (endptr)
        *endptr = (char *)s;

    return x;
}

int atoi (const char *s)
{
    return (int)_Stoul (s, NULL, 0);
}




/*******************************************************************************
*
* strspn - return the string length up to the first character not in a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of
* string <s> that consists entirely of characters from the string <sep>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strcspn()
*/
 
size_t strspn(const char * s,    /* string to search */
              const char * sep)  /* set of characters to look for in <s> */
{
    const char *save;
    const char *p;
    char c1;
    char c2;

    for (save = s + 1; (c1 = *s++) != EOS; ) { /* for every char in <s> */
        for (p = sep; (c2 = *p++) != c1; ) { /* for every char in <sep> */
            if (c2 == EOS) { /* find the char in <s> that not in <sep> */
                return (s - save);
            }
        }
    }

    return (s - save);
}

/*******************************************************************************
*
* strcspn - return the string length up to the first character from a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of string
* <s1> that consists entirely of characters not included in string <s2>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strpbrk(), strspn()
*/
 
size_t strcspn(const char * s1, /* string to search */
               const char * s2) /* set of characters to look for in <s1> */
{
    const char *save;
    const char *p;
    char c1;
    char c2;

    for (save = s1 + 1; (c1 = *s1++) != EOS; ) { /* search for EOS */
        for (p = s2; (c2 = *p++) != EOS; ) { /* search for first occurance */
            if (c1 == c2) {
                return (s1 - save); /* return index of substring */
            }
        }
    }

    return (s1 - save);
}


/*******************************************************************************
*
* strpbrk - find the first occurrence in a string of a character from a given set (ANSI)
*
* This routine locates the first occurrence in string <s1> of any character
* from string <s2>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* A pointer to the character found in <s1>, or
* NULL if no character from <s2> occurs in <s1>.
*
* SEE ALSO: strcspn()
*/
#ifndef CHAR_FROM_CONST
#define CHAR_FROM_CONST(x) (char *)(x)
#endif /* CHAR_FROM_CONST */

char * strpbrk(const char * s1,  /* string to search */
               const char * s2)  /* set of characters to look for in <s1> */
{
    char *scanp;
    int   c;
    int   sc;

    while ((c = *s1++) != 0) { /* wait until end of string */

        /* loop, searching for character */
        for (scanp = CHAR_FROM_CONST(s2); (sc = *scanp++) != 0;) {
            if (sc == c) {
                return (CHAR_FROM_CONST(s1 - 1));
            }
        }
    }

    return (NULL);
}

/*****************************************************************************
*
* strtok_r - break down a string into tokens (reentrant) (POSIX)
*
* This routine considers the null-terminated string <string> as a sequence
* of zero or more text tokens separated by spans of one or more characters
* from the separator string <separators>.  The argument <ppLast> points to a
* user-provided pointer which in turn points to the position within <string>
* at which scanning should begin.
*
* In the first call to this routine, <string> points to a null-terminated
* string; <separators> points to a null-terminated string of separator
* characters; and <ppLast> points to a NULL pointer.  The function returns a
* pointer to the first character of the first token, writes a null character
* into <string> immediately following the returned token, and updates the
* pointer to which <ppLast> points so that it points to the first character
* following the null written into <string>.  (Note that because the
* separator character is overwritten by a null character, the input string
* is modified as a result of this call.)
*
* In subsequent calls <string> must be a NULL pointer and <ppLast> must
* be unchanged so that subsequent calls will move through the string <string>,
* returning successive tokens until no tokens remain.  The separator
* string <separators> may be different from call to call.  When no token
* remains in <string>, a NULL pointer is returned.
*
* INCLUDE FILES: string.h
* 
* RETURNS
* A pointer to the first character of a token,
* or a NULL pointer if there is no token.
*
* SEE ALSO: strtok()
*/

char * strtok_r(char       * string,      /* string to break into tokens */
                const char * separators,  /* the separators */
                char      ** ppLast)      /* pointer to serve as string index */
{
    if ((string == NULL) && ((string = *ppLast) == NULL))
        return (NULL);

    /* find the char not in <separators> and strip to there */
    if (*(string += strspn (string, separators)) == EOS)
        return (*ppLast = NULL);

    /* find the char in <separators> and terminate it */
    if ((*ppLast = strpbrk (string, separators)) != NULL)
        *(*ppLast)++ = EOS;

    return (string);
}


/******************************************************************************
*
* strtok - break down a string into tokens (ANSI)
*
* A sequence of calls to this routine breaks the string <string> into a
* sequence of tokens, each of which is delimited by a character from the
* string <separator>.  The first call in the sequence has <string> as its
* first argument, and is followed by calls with a null pointer as their
* first argument.  The separator string may be different from call to call.
* 
* The first call in the sequence searches <string> for the first character
* that is not contained in the current separator string.  If the character
* is not found, there are no tokens in <string> and strtok() returns a
* null pointer.  If the character is found, it is the start of the first
* token.
* 
* strtok() then searches from there for a character that is contained in the
* current separator string.  If the character is not found, the current
* token expands to the end of the string pointed to by <string>, and
* subsequent searches for a token will return a null pointer.  If the
* character is found, it is overwritten by a null character, which
* terminates the current token.  strtok() saves a pointer to the following
* character, from which the next search for a token will start.
* (Note that because the separator character is overwritten by a null
* character, the input string is modified as a result of this call.)
* 
* Each subsequent call, with a null pointer as the value of the first
* argument, starts searching from the saved pointer and behaves as
* described above.
* 
* The implementation behaves as if strtok() is called by no library functions.
*
* REENTRANCY
* This routine is not reentrant; the reentrant form is strtok_r().
*
* INCLUDE FILES: string.h
* 
* RETURNS
* A pointer to the first character of a token, or a NULL pointer if there is
* no token.
*
* SEE ALSO: strtok_r()
*/ 

char * strtok(char       * string,     /* string */
              const char * separator)  /* separator indicator */
{
    static char *last = NULL;

    return (strtok_r (string, separator, &last));
}

/*==============================================================================
 ** FILE END
==============================================================================*/

