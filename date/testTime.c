/****************************************Copyright (c)*****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**                                      
**                                 http://www.embedtools.com
**
**--------------File Info----------------------------------------------------------------------------------
** File name:               testTime.c
** Last modified Date:      2011-7-5 9:32:10
** Last Version:            1.0.0
** Descriptions:            This file
**
**---------------------------------------------------------------------------------------------------------
** Created by:              WangDongfang
** Last modified Date:      2011-7-5 9:32:10
** Version:                 1.0.0
** Descriptions:            This file
**
**---------------------------------------------------------------------------------------------------------
** Modified by:             WangDongfang
** Modified date:           2011-10-21
** Version:                 1.0.0
** Description:             porting to dfewos
**
**********************************************************************************************************/
#include "timeLib.h"
#include "nongli.h"
int sscanf(const char *str, char const *fmt, ...);

/**********************************************************************************************************
  Weekday strings
**********************************************************************************************************/
static char *__GpcWeeks[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static char *__GpcWeeksShort[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

/**********************************************************************************************************
  VT100+ Color codes
**********************************************************************************************************/
#define COLOR_FG_BLACK  "\033[30m"                                      /*  ºÚ   ×Ö                      */
#define COLOR_FG_RED    "\033[31m"                                      /*  ºì   ×Ö                      */
#define COLOR_FG_GREEN  "\033[32m"                                      /*  ÂÌ   ×Ö                      */
#define COLOR_FG_YELLOW "\033[33m"                                      /*  »Æ   ×Ö                      */
#define COLOR_FG_BLUE   "\033[34m"                                      /*  À¶   ×Ö                      */
#define COLOR_FG_MAGEN  "\033[35m"                                      /*  Æ·ºì ×Ö                      */
#define COLOR_FG_CYAN   "\033[36m"                                      /*  Çà   ×Ö                      */
#define COLOR_FG_WHITE  "\033[37m"                                      /*  °×   ×Ö                      */

#define COLOR_BG_BLACK  "\033[40m"                                      /*  ºÚ   µ×                      */
#define COLOR_BG_YELLOW "\033[43m"                                      /*  »Æ   µ×                      */
#define COLOR_BG_BLUE   "\033[44m"                                      /*  À¶   µ×                      */

/**********************************************************************************************************
  Config Colors
**********************************************************************************************************/
#define MODE_BEGIN_STRING_DATE  printf(COLOR_FG_YELLOW);
#define MODE_BEGIN_FRAME        printf(COLOR_FG_GREEN);
#define MODE_BEGIN_WEEKDAY      printf(COLOR_FG_WHITE);
#define MODE_BEGIN_WEEKEND      printf(COLOR_FG_MAGEN);
#define MODE_BEGIN_TODDAY       printf(COLOR_FG_RED); printf(COLOR_BG_BLUE);
#define MODE_END_TODDAY         printf(COLOR_BG_BLACK);
#define MODE_END_DATE           printf(COLOR_FG_WHITE); printf(COLOR_BG_BLACK);

STATUS testTime ()
{
    TIME_SOCKET tsTimeSet = {2011, 7, 4, 9, 34, 0};

    timeSet(tsTimeSet);
    return OK;
}

static STATUS isLeapyear (int iYear)
{
    return (iYear % 400 == 0) || ((iYear % 4 == 0) && (iYear % 100 != 0));
}
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
            if (isLeapyear(iYear)) {
                return (29);
            } else {
                return (28);
            }
        default:
            return 0;
    }
}

/**********************************************************************************************************
** Function name:           date
** Descriptions:            display now month calendar with color, like this:
**                          -----------------------------
**                           SUN MON TUE WED THU FRI SAT
**                          -----------------------------
**                                                 1   2
**                             3   4   5   6   7   8   9
**                            10  11  12  13  14  15  16
**                            17  18  19  20  21  22  23
**                            24  25  26  27  28  29  30
**                            31
**                          -----------------------------
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          OK: Always
** Created by:              WangDongfang
** Created Date:            2010-07-02
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
STATUS C_date (int argc, char **argv)
{
    int iFirstWeekday;
    int iDays;
    int i;
    int set_val;
    NONGLI_DATE nong_date;

    TIME_SOCKET tsTimeSet;
    TIME_SOCKET tsTimeNow;
    tsTimeSet = timeGet();

    /*
     * set time if need
     */
    if (argc >= 2) {
        sscanf (argv[1], "%d", (unsigned int *)&set_val);
        if (set_val != 0) { tsTimeSet.iYear = set_val; }
    }
    if (argc >= 3) {
        sscanf (argv[2], "%d", (unsigned int *)&set_val);
        if (set_val != 0) { tsTimeSet.iMon = set_val; }
    }
    if (argc >= 4) {
        sscanf (argv[3], "%d", (unsigned int *)&set_val);
        if (set_val != 0) { tsTimeSet.iDay = set_val; }
    }
    if (argc >= 5) {
        sscanf (argv[4], "%d", (unsigned int *)&tsTimeSet.iHour);
    }
    if (argc >= 6) {
        sscanf (argv[5], "%d", (unsigned int *)&tsTimeSet.iMin);
    }
    if (argc >= 7) {
        sscanf (argv[6], "%d", (unsigned int *)&tsTimeSet.iSec);
    }

    if (argc >= 2) {
        timeSet (tsTimeSet);
    }

    /* 
     * get now time
     */
    tsTimeNow = timeGet();

    iFirstWeekday = (7 - (tsTimeNow.iDay - 1) % 7 + tsTimeNow.iWday) % 7;

    iDays = monthDays(tsTimeNow.iYear, tsTimeNow.iMon);

    MODE_BEGIN_STRING_DATE
    printf("Date  : %d-%02d-%02d\n"
           "Today : %s\n",
            tsTimeNow.iYear,
            tsTimeNow.iMon,
            tsTimeNow.iDay,
            __GpcWeeks[tsTimeNow.iWday]);

    MODE_BEGIN_FRAME
    printf("-----------------------------\n");
    for (i = 0; i < 7; i++) {                                           /*  SUN MON TUE ... SAT          */
        printf(" %s", __GpcWeeksShort[i]);
    }
    printf("\n");
    printf("-----------------------------\n");
    for (i = 0; i < iFirstWeekday; i++) {                               /*  The Blanks before 1#         */
        printf("    ");
    }
    for (i = 1; i <= iDays; i++) {                                      /*  The days 1# ... 31#          */
        if (iFirstWeekday == 6 || iFirstWeekday == 0) {
            MODE_BEGIN_WEEKEND
        } else {
            MODE_BEGIN_WEEKDAY
        }
        if (i == tsTimeNow.iDay) {
            printf("  ");
            MODE_BEGIN_TODDAY
            printf("%2d", i);
            MODE_END_TODDAY
        } else {
            printf("%4d", i);
        }

        iFirstWeekday = (iFirstWeekday + 1) % 7;
        if (iFirstWeekday == 0) {
            printf("\n");
        }
    }                                                                   /*  The days 1# ... 31#  END     */
    if (iFirstWeekday != 0) {
        printf("\n");
    }
    MODE_BEGIN_FRAME
    printf("-----------------------------\n");

    MODE_BEGIN_STRING_DATE
    printf("Time : %02d:%02d:%02d",
            tsTimeNow.iHour,
            tsTimeNow.iMin,
            tsTimeNow.iSec);
    /*
     * nong li
     */
    nongli_get_date (tsTimeNow.iYear, tsTimeNow.iMon, tsTimeNow.iDay,
                     &nong_date);
    printf ("\nÅ©Àú : ");
    printf ("%sÄê %s ¡¾%s¡¿\n", nong_date.tian_di, nong_date.ri_qi, nong_date.sheng_xiao);
    printf ("%s%s", nong_date.jie_qi, nong_date.bei_zhu);

    MODE_END_DATE
    return OK;
}

/**********************************************************************************************************
  FILE END
**********************************************************************************************************/

