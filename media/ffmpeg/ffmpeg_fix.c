
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include <dfewos.h>
#include "../string.h"


/*********************************************************************************************************
** Function name:           gettimeofday
** Descriptions:            ��õ�ǰʱ��,���ȵ�΢��
** input parameters:        tzp: ����ΪNULL,��������δ����
** output parameters:       tp:  ��ǰʱ��
** Returned value:          0:   �ɹ�
*********************************************************************************************************/
int _gettimeofday (struct timeval *tp, void *tzp)
{
    unsigned long ulTicks;                                              /*  ʱ�ӽ�����                  */
    unsigned int  uiClkRate;                                            /*  ʱ�ӽ���Ƶ��                */


    ulTicks   = tick_get();
    uiClkRate = SYS_CLK_RATE;

    tp->tv_sec  = ulTicks / uiClkRate;
    tp->tv_usec = (ulTicks % uiClkRate) * 1000000ul / uiClkRate;
    return 0;
}
/*********************************************************************************************************
** Function name:           tempnam
** Descriptions:            ���Ψһ��ʱ�ļ���
** input parameters:        path: ·��
**                          prefix: �ļ���ǰ׺
** output parameters:       none
** Returned value:          �ļ���
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
** Descriptions:            ΢����ʱ
** input parameters:        useconds: ��ʱ��ʱ����
** output parameters:       none
** Returned value:          0:  �ɹ�
**                          -1: ʧ��
*********************************************************************************************************/
int usleep (unsigned long useconds)
{
    useconds = useconds * SYS_CLK_RATE / 1000000ul;
    useconds == 0 ? delayQ_delay(1) : delayQ_delay(useconds);
    return 0;
}

/*********************************************************************************************************
** Function name:           gethrtime
** Descriptions:            linux��get high resolution time since the system bootup��������Ϊ��λ��
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

