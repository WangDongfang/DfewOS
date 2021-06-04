/****************************************Copyright (c)*****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**                                      
**                                 http://www.embedtools.com
**
**--------------File Info----------------------------------------------------------------------------------
** File name:               timeLib.c
** Last modified Date:      2011-7-2 14:41:15
** Last Version:            1.0.0
** Descriptions:            This file implement a software timer lib
**
**---------------------------------------------------------------------------------------------------------
** Created by:              WangDongfang
** Last modified Date:      2011-7-2 14:41:15
** Version:                 1.0.0
** Descriptions:            This file implement a software timer lib
**
**---------------------------------------------------------------------------------------------------------
** Modified by:             WangDongfang
** Modified date:           2011-10-21
** Version:                 1.0.0
** Description:             porting to dfewos
**
**********************************************************************************************************/
#include "timeLib.h"

#undef USE_TICK_SET

/**********************************************************************************************************
  forward functions declare
**********************************************************************************************************/
static int isLeapyear(int iYear);
static int monthDays(int iYear, int iMonth);
static STATUS checkTime(const TIME_SOCKET *ptsTime);
static void updateDayOfYear(TIME_SOCKET *ptsTime);
static void updateWeekday(TIME_SOCKET *ptsTime);
static void addDays(TIME_SOCKET *ptsTime, int iDays);
static TIME_SOCKET calculateNowTime();

/**********************************************************************************************************
  The base time, only set time can change it.
**********************************************************************************************************/
static TIME_SOCKET __GtsBaseTime = {1970, 1, 1, 0, 0, 0, 6, 0};         /*  1970-01-01 00:00:00          */

/**********************************************************************************************************
  mutex access the global variable __GtsBaseTime
**********************************************************************************************************/
#ifdef OS_VXWORKS
static SEM_ID      __GsemBaseTimeMutex = NULL;                          /*  互斥__GtsBaseTime的访问      */
#define OS_MUX_INIT __GsemBaseTimeMutex = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
#define OS_BASETIME_LOCK    semTake(__GsemBaseTimeMutex, WAIT_FOREVER); /*  获取互斥信号量               */
#define OS_BASETIME_UNLOCK  semGive(__GsemBaseTimeMutex);
#elif defined(OS_DFEWOS)
static SEM_MUX*    __GsemBaseTimeMutex = NULL;                          /*  互斥__GtsBaseTime的访问      */
#define OS_MUX_INIT         __GsemBaseTimeMutex = semM_init(NULL);
#define OS_BASETIME_LOCK    semM_take(__GsemBaseTimeMutex, WAIT_FOREVER);/* 获取互斥信号量               */
#define OS_BASETIME_UNLOCK  semM_give(__GsemBaseTimeMutex);
#endif

#ifndef  USE_TICK_SET
static UINT32      __GuiSetMomentTicks = 0;                             /*  设定时间时的tick值           */
#endif                                                                  /*  USE_TICK_SET                 */

/**********************************************************************************************************
** Function name:           isLeapyear
** Descriptions:            test the year is leapyear or not
** input parameters:        iYear: the year to test
** output parameters:       NONE
** Returned value:          1: leapyear
                            0: not leapyear
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static int isLeapyear (int iYear)
{
    return (iYear % 400 == 0) || ((iYear % 4 == 0) && (iYear % 100 != 0));
}

/**********************************************************************************************************
** Function name:           monthDays
** Descriptions:            compute the number of days of the special year, month 
** input parameters:        iYear:  the year
                            iMonth: the month
** output parameters:       NONE
** Returned value:          [28, 31]: the day number
                            0:        invalid month
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static int monthDays (int iYear, int iMonth)
{
    switch (iMonth) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return (31);
        case 4:
        case 6:
        case 9:
        case 11:
            return (30);
        case 2:
            return isLeapyear(iYear) ? (29) : (28);
        default:
            return 0;
    }
}

/**********************************************************************************************************
** Function name:           checkTime
** Descriptions:            check the special time is available
** input parameters:        ptsTime: the time structure pointer to check
** output parameters:       NONE
** Returned value:          OK:    available
                            ERROR: invalid
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static STATUS checkTime (const TIME_SOCKET *ptsTime)
{
    STATUS sRet = OK;

    if (ptsTime->iSec < 0 || ptsTime->iSec > 59) {
        printf("WARNING: When set time, invalid second:%d\n", ptsTime->iSec);
        sRet = ERROR;
    }
    if (ptsTime->iMin < 0 || ptsTime->iMin > 59) {
        printf("WARNING: When set time, invalid minute:%d\n", ptsTime->iMin);
        sRet = ERROR;
    }
    if (ptsTime->iHour < 0 || ptsTime->iHour > 23) {
        printf("WARNING: When set time, invalid hour:%d\n", ptsTime->iHour);
        sRet = ERROR;
    }
    if (ptsTime->iYear < 1000 || ptsTime->iYear > 9999) {
        printf("WARNING: When set time, invalid year:%d\n", ptsTime->iYear);
        sRet = ERROR;
    }
    if (ptsTime->iMon < 1 || ptsTime->iMon > 12) {
        printf("WARNING: When set time, invalid month:%d\n", ptsTime->iMon);
        sRet = ERROR;
    }
    if (ptsTime->iDay < 1 || ptsTime->iDay > monthDays(ptsTime->iYear, ptsTime->iMon)) {
        printf("WARNING: When set time, invalid day:%d\n", ptsTime->iDay);
        sRet = ERROR;
    }

    return sRet;
}

/**********************************************************************************************************
** Function name:           updateDayOfYear
** Descriptions:            compute the days since January 1
** input parameters:        ptsTime: the time structure pointer to update day of year
** output parameters:       NONE
** Returned value:          ptsTime->iYday: change to right value
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static void updateDayOfYear (TIME_SOCKET *ptsTime)
{
    ptsTime->iYday = ptsTime->iDay - 1;

    int iMonth = ptsTime->iMon - 1;
    while (iMonth > 0) {
        ptsTime->iYday += monthDays(ptsTime->iYear, iMonth);
        iMonth--;
    }
}

/**********************************************************************************************************
** Function name:           updateWeekday
** Descriptions:            compute what's the weekday today
** input parameters:        ptsTime: the time structure pointer to update weekday
** output parameters:       NONE
** Returned value:          ptsTime->iWday: change to right value
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static void updateWeekday (TIME_SOCKET *ptsTime)
{
    int iNewYearDay;
    int iYear = ptsTime->iYear;

    updateDayOfYear(ptsTime);
    iNewYearDay = (iYear + (iYear - 1) / 400 - (iYear - 1) / 100 + (iYear - 1) / 4) % 7;
    ptsTime->iWday = (iNewYearDay + ptsTime->iYday) % 7;
}

/**********************************************************************************************************
** Function name:           addDays
** Descriptions:            add some days on the special time
** input parameters:        ptsTime: the time structure pointer to added some days
                            iDays:   the number of day to add
** output parameters:       ptsTime: change to new year, month, day
** Returned value:          NONE
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static void addDays (TIME_SOCKET *ptsTime, int iDays)
{
    while (iDays > 0) {
        if (ptsTime->iDay < monthDays(ptsTime->iYear, ptsTime->iMon)) {
            ptsTime->iDay++;
        } else {
            ptsTime->iDay = 1;
            ptsTime->iMon++;
            if (ptsTime->iMon == 13) {
                ptsTime->iMon = 1;
                ptsTime->iYear++;
            }
        }
        iDays--;
    }
}

/**********************************************************************************************************
** Function name:           calculateNowTime
** Descriptions:            base the __GtsBaseTime, across ticks
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          the calculated time structure
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static TIME_SOCKET calculateNowTime ()
{
    TIME_SOCKET tsNowTime;                                              /*  to store the calculated time */
    int iCarry;
    int iSeconds;
    int iMinutes;
    int iHours;
    int iDays;

#ifdef USE_TICK_SET
    iSeconds = tickGet() / sysClkRateGet();
#else                                                                   /*  USE_TICK_SET                 */
    iSeconds = (tickGet() - __GuiSetMomentTicks) / sysClkRateGet();
#endif                                                                  /*  USE_TICK_SET                 */

    tsNowTime.iSec = __GtsBaseTime.iSec + iSeconds % 60;
    if (tsNowTime.iSec > 59) {
        tsNowTime.iSec %= 60;
        iCarry = 1;
    } else {
        iCarry = 0;
    }
    iMinutes = iSeconds / 60 + iCarry;
    tsNowTime.iMin = __GtsBaseTime.iMin + iMinutes % 60;
    if (tsNowTime.iMin > 59) {
        tsNowTime.iMin %= 60;
        iCarry = 1;
    } else {
        iCarry = 0;
    }
    iHours = iMinutes / 60 + iCarry;
    tsNowTime.iHour = __GtsBaseTime.iHour + iHours % 24;
    if (tsNowTime.iHour > 23) {
        tsNowTime.iHour %= 24;
        iCarry = 1;
    } else {
        iCarry = 0;
    }
    iDays = iHours / 24 + iCarry;

    tsNowTime.iYear = __GtsBaseTime.iYear;
    tsNowTime.iMon = __GtsBaseTime.iMon;
    tsNowTime.iDay = __GtsBaseTime.iDay;
    addDays(&tsNowTime, iDays);

    return tsNowTime;
}

STATUS timeSet (TIME_SOCKET tsNowTime)
{
    if (checkTime(&tsNowTime) == ERROR) {
        return ERROR;
    }
    if (__GsemBaseTimeMutex == NULL) {
        OS_MUX_INIT;
    }
    OS_BASETIME_LOCK;
    __GtsBaseTime = tsNowTime;
    OS_BASETIME_UNLOCK;
#ifdef  USE_TICK_SET
    tickSet(0);
#else                                                                   /*  USE_TICK_SET                 */
    __GuiSetMomentTicks = tickGet();
#endif                                                                  /*  USE_TICK_SET                 */

    return OK;
}
TIME_SOCKET timeGet ()
{
    TIME_SOCKET tsNowTime;

    if (__GsemBaseTimeMutex == NULL) {
        OS_MUX_INIT;
    }
    OS_BASETIME_LOCK;
    tsNowTime = calculateNowTime();
    OS_BASETIME_UNLOCK;
    updateWeekday(&tsNowTime);

    return tsNowTime;
}

STATUS setYear (int iYear)
{
    TIME_SOCKET tsNowTime;
    tsNowTime = timeGet();
    tsNowTime.iYear = iYear;

    return timeSet(tsNowTime);
}

STATUS setMonth (int iMonth)
{
    TIME_SOCKET tsNowTime;
    tsNowTime = timeGet();
    tsNowTime.iMon = iMonth;

    return timeSet(tsNowTime);
}

STATUS setDay (int iDay)
{
    TIME_SOCKET tsNowTime;
    tsNowTime = timeGet();
    tsNowTime.iDay = iDay;

    return timeSet(tsNowTime);
}

STATUS setHour (int iHour)
{
    TIME_SOCKET tsNowTime;
    tsNowTime = timeGet();
    tsNowTime.iHour = iHour;

    return timeSet(tsNowTime);
}

STATUS setMinute (int iMinute)
{
    TIME_SOCKET tsNowTime;
    tsNowTime = timeGet();
    tsNowTime.iMin = iMinute;

    return timeSet(tsNowTime);
}

STATUS setSecond (int iSecond)
{
    TIME_SOCKET tsNowTime;
    tsNowTime = timeGet();
    tsNowTime.iSec = iSecond;

    return timeSet(tsNowTime);
}


int getYear() { return (timeGet().iYear); }

int getMonth() { return (timeGet().iMon); }

int getDay() { return (timeGet().iDay); }

int getHour() { return (timeGet().iHour); }

int getMinute() { return (timeGet().iMin); }

int getSecond() { return (timeGet().iSec); }

/**********************************************************************************************************
  FILE END
**********************************************************************************************************/

