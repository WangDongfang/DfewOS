
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include <dfewos.h>
#include "../string.h"


/*********************************************************************************************************
** Function name:           gettimeofday
** Descriptions:            获得当前时间,精度到微秒
** input parameters:        tzp: 必须为NULL,否则结果是未定义
** output parameters:       tp:  当前时间
** Returned value:          0:   成功
*********************************************************************************************************/
int _gettimeofday (struct timeval *tp, void *tzp)
{
    unsigned long ulTicks;                                              /*  时钟节拍数                  */
    unsigned int  uiClkRate;                                            /*  时钟节拍频率                */


    ulTicks   = tick_get();
    uiClkRate = SYS_CLK_RATE;

    tp->tv_sec  = ulTicks / uiClkRate;
    tp->tv_usec = (ulTicks % uiClkRate) * 1000000ul / uiClkRate;
    return 0;
}
/*********************************************************************************************************
** Function name:           tempnam
** Descriptions:            获得唯一临时文件名
** input parameters:        path: 路径
**                          prefix: 文件名前缀
** output parameters:       none
** Returned value:          文件名
*********************************************************************************************************/
char *tempnam (const char *path, const char *prefix )
{
    static char s[256];
    static int index;

    s[0] = 0;
    if (path != NULL) {
        strncat(s, path, sizeof(s) - 10);
    }
    if (prefix != NULL) {
        strncat(s, "/", sizeof(s) - 10);
        strncat(s, prefix, sizeof(s) - 10);
    }
    sprintf(s + strlen(s), "%d", index);
    index++;
    return s;
}
/*********************************************************************************************************
** Function name:           usleep
** Descriptions:            微秒延时
** input parameters:        useconds: 延时的时间数
** output parameters:       none
** Returned value:          0:  成功
**                          -1: 失败
*********************************************************************************************************/
int usleep (unsigned long useconds)
{
    useconds = useconds * SYS_CLK_RATE / 1000000ul;
    useconds == 0 ? delayQ_delay(1) : delayQ_delay(useconds);
    return 0;
}

/*********************************************************************************************************
** Function name:           gethrtime
** Descriptions:            linux下get high resolution time since the system bootup，以纳秒为单位。
** input parameters:        none
** output parameters:       none
** Returned value:          ns from bootup
*********************************************************************************************************/
int gethrtime ()
{
    return tick_get() * 10000000ul;
}

/*********************************************************************************************************
** Function name:           time
** Descriptions:            determine the current calendar time (ANSI)
** input parameters:        calendar time in seconds
** output parameters:       none
** Returned value:          ns from bootup
*********************************************************************************************************/
time_t time (time_t *timer) 
{
    int seconds = tick_get() / SYS_CLK_RATE;
    if (timer != NULL)
        *timer = seconds;
    return seconds;
}

clock_t _times (struct tms *ptms)
{
    return 0;
}
#if 0
int localtime_r (const time_t *timer,     /* calendar time in seconds */
                 struct tm *timeBuffer)   /* buffer for the broken-down time */
{
    return 0;
}
#endif



#if defined(vax)||defined(tahoe)
#ifdef vax
#define _0x(A,B)        0x/**/A/**/B
#else   /* vax */
#define _0x(A,B)        0x/**/B/**/A
#endif  /* vax */
static long Lx[] = {_0x(0000,5c00),_0x(0000,0000)};     /* 2**55 */
#define L *(double *) Lx
#else   /* defined(vax)||defined(tahoe) */
static double L = 4503599627370496.0E0;         /* 2**52 */
#endif  /* defined(vax)||defined(tahoe) */

double rint (double x)
{
        double s,t,one = 1.0,copysign();
#if !defined(vax)&&!defined(tahoe)
        if (x != x)                             /* NaN */
                return (x);
#endif  /* !defined(vax)&&!defined(tahoe) */
        if (copysign(x,one) >= L)               /* already an integer */
            return (x);
        s = copysign(L,x);
        t = x + s;                              /* x+s rounded to integer */

        return (t - s);
}

#if 1
long int lrintf(float x)
{
	return (long)rint(x);
}
long long int llrint(double x)
{
	return (long long)rint(x);
}
long long int llrintf(float x)
{
	return (long long)rint(x);
}
#endif

