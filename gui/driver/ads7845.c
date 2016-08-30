#if 0
/*==============================================================================
** ads7845.c -- ads7845 5-wire touch screen panel controllor driver.
**
** MODIFY HISTORY:
**
** 2012-03-12 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../gui.h"

/*======================================================================
  configs
======================================================================*/
#define TSC_INT_OFF
#define TSC_SAMPLE_NUM   6
#define TSC_TASK_PRI     7

/**********************************************************************************************************
  寄存器
**********************************************************************************************************/
#define rGPCCON             (*(volatile unsigned *)0x7F008040)          /*  Port C control              */
#define rGPCDAT             (*(volatile unsigned *)0x7F008044)          /*  Port C data                 */

/**********************************************************************************************************
  编译控制宏
**********************************************************************************************************/
#define __TOUCH_DEBUG_OFF                                               /*  调试控制                     */

/*********************************************************************************************************
  管脚相关宏定义
*********************************************************************************************************/
#define	__ADS7845_CS                (1 << 3)                            /*  7845_CS   在 rGPCDAT 中位置 */
#define	__ADS7845_DCLK		        (1 << 1)                            /*  7845_DCLK 在 rGPCDAT 中位置 */
#define	__ADS7845_DIN               (1 << 2)                            /*  7845_DIN  在 rGPCDAT 中位置 */
#define	__ADS7845_DOUT              (1 << 0)                            /*  7845_DOUT 在 rGPCDAT 中位置 */

#define __ADS7845_CS_SET()		    rGPCDAT  |=   __ADS7845_CS          /*  置位ads7845片选引脚         */
#define __ADS7845_CS_CLR()          rGPCDAT  &= ~(__ADS7845_CS)         /*  清零EINT0 (使能ADS7845)     */

#define __ADS7845_DOUT_READ()	    (rGPCDAT & (__ADS7845_DOUT))        /*  读取ads7845数据输出引脚     */

#define __ADS7845_DIN_SET()         rGPCDAT  |=   __ADS7845_DIN         /*  控制ads7845数据输入引脚为1  */
#define __ADS7845_DIN_CLR()         rGPCDAT  &= ~(__ADS7845_DIN)        /*  控制ads7845数据输入引脚为0  */

#define __ADS7845_DCLK_SET()        rGPCDAT  |=   __ADS7845_DCLK        /*  控制ads7845时钟引脚为1      */
#define __ADS7845_DCLK_CLR()        rGPCDAT  &= ~(__ADS7845_DCLK)       /*  控制ads7845时钟引脚为0      */

/**********************************************************************************************************
  时钟延时控制宏定义
**********************************************************************************************************/
#define __DELAY_200NS              10                                   /*  操作时序控制宏               */

/**********************************************************************************************************
  触摸屏命令控制字
**********************************************************************************************************/
#define ADS7845AIN_Y               0x94                                 /*  ADS7845控制字采集X电压       */
#define ADS7845AIN_X               0xD4                                 /*  ADS7845控制字采集Y电压       */
#define ADS7845AIN_Z1              0xA4                                 /*  ADS7845控制字采集Z1电压      */
#define ADS7845AIN_Z2              0xE4                                 /*  ADS7845控制字采集是否按下    */

/**********************************************************************************************************
  最大差值定义
**********************************************************************************************************/
#define  __ADC_X_MAX_DIFF          120                                  /*  X轴最大差值                  */
#define  __ADC_Y_MAX_DIFF          120                                  /*  Y轴最大差值                  */

/**********************************************************************************************************
  调试控制宏
**********************************************************************************************************/

/**********************************************************************************************************
  helper functions prototypes
**********************************************************************************************************/
static unsigned int __measurementGet (unsigned char ucPosition);
static   void  __DelayNo (volatile unsigned int i);
void  T_ads7845Thread ();

/**********************************************************************************************************
** Function name:           ads7845_init
** Descriptions:            配置和ads7845相连的GPIO引脚功能，并初始化引脚值
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          正确返回 OK,  错误返回 ERROR
** Created by:              WangDongfang
** Created Date:            2011-03-04
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
int touch_screen_init (void)
{
#if 0
    gpioPinConfigSet(__ADS7845_CS, GPIO_OUTPUT);                        /*  配置7845_CS                  */
    gpioPinConfigSet(__ADS7845_DCLK, GPIO_OUTPUT);                      /*  配置7845_DCLK                */
    gpioPinConfigSet(__ADS7845_DIN, GPIO_OUTPUT);                       /*  配置7845_DIN                 */
    gpioPinConfigSet(__ADS7845_DOUT, GPIO_INPUT);                       /*  配置7845_DOUT                */
#else 
    rGPCCON &= ~(0x7 << 12);                                            /*  配置7845_CS                  */
    rGPCCON |=  (1 << 12);

    rGPCCON &= ~(0x7 << 4);                                             /*  配置7845_DCLK                */
    rGPCCON |=  (1 << 4);

    rGPCCON &= ~(0x7 << 8);                                             /*  配置7845_DIN                 */
    rGPCCON |=  (1 << 8);

    rGPCCON &= ~(0x7 << 0);                                             /*  配置7845_DOUT                */
#endif

    __ADS7845_CS_SET();                                                 /*  初始化ads7845片选引脚为1     */
    __ADS7845_DCLK_CLR();                                               /*  初始化ads7845时钟引脚为0     */
    __ADS7845_DIN_CLR();                                                /*  初始化ads7845输入引脚为0     */
    
    __DelayNo(__DELAY_200NS);

#ifdef TSC_INT_ON
    /*
     * 注册中断
     */
    {
#include "../../bsp/int.h"
#define rGPNCON         (*(volatile unsigned int *)0x7F008830)      // 0x0              Port N Configuration Register
#define rGPNDAT         (*(volatile unsigned int *)0x7F008834)      // 0x0              Port N Data Register Undefined
#define rGPNPUD         (*(volatile unsigned int *)0x7F008838)      // 0x55555555       Port N Pull-up/down Register

#define rEINT0CON0      (*(volatile unsigned int *)0x7F008900)
#define rEINT0MASK      (*(volatile unsigned int *)0x7F008920)
#define rEINT0PEND      (*(volatile unsigned int *)0x7F008924)

    unsigned int uiReg;

    uiReg       =    rGPNCON;                                        /* config TCH_INT GPN10         */
    uiReg       &=   ~(0x3u   <<  20);
    uiReg       |=   (0x2u    <<  20);
    rGPNCON     =    uiReg;

    uiReg       =    rGPNPUD;
    uiReg       &=   ~(0x3u   <<  20);
    uiReg       |=   (0x2u    <<  20);
    rGPNPUD     =    uiReg;                                          /* config to PULL UP            */
    rGPNDAT     |=   0x1u <<  10;                                    /* config IRQ_LAN HIGH          */
    
    rEINT0CON0   &=  ~(0x7   <<  20);                    /* Set to Low Level IRQ         */
    //rEINT0CON0   |=  (0x2   <<  20);                    /* Set to Falling edge IRQ         */
    rEINT0PEND   =   (0x1u   <<  10);                    /* Clear Pending INT            */
    rEINT0MASK   &=  ~(0x1u  <<  10);                    /* Enable EINT10                */

    void ads7845_isr ();
    int_connect (INT_NUMBER_EINT1, ads7845_isr, 0);
    int_enable (INT_NUMBER_EINT1);
    }
#else
    task_create ("tTouch", 8 * KB, TSC_TASK_PRI, T_ads7845Thread, 0, 0);
#endif /* TSC_INT_ON */

    return 0;
}

#ifdef TSC_INT_ON
void ads7845_isr ()
{
    rEINT0PEND = (0x3 << 10);        /*  clear EINT10 flag*/

    serial_printf("in ads7845_isr()", 0,1,2,3,4,5);
}
#endif /* TSC_INT_ON */

/**********************************************************************************************************
** Function name:           measurementGet
** Descriptions:            测量触摸面板上的电压
** input parameters:        ucPosition    要测量的轴
**
** output parameters:       NONE
** Returned value:          获取到的电压值
** Created by:              Wangfeng
** Created Date:            2008/09/01
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static unsigned int __measurementGet (unsigned char ucPosition)
{
    unsigned char   i;
    unsigned int    uiResult;

    ucPosition |= 0x80;

    __DelayNo(__DELAY_200NS);
    __ADS7845_CS_CLR();                                                 /*  cs = 0 ，片选ADS7845         */
    __DelayNo(__DELAY_200NS);

    for (i = 0; i < 8; i++) {                                           /*  发送一个字节命令字           */

        if ((ucPosition & 0x80) != 0 ) {

            __ADS7845_DIN_SET();                                        /*  DIN = 1                      */
        } else {

            __ADS7845_DIN_CLR();                                        /*  DIN = 0                      */
        }

        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_SET();                                           /*  DCLK = 1                     */
        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_CLR();                                           /*  DCLK = 0                     */

        ucPosition <<= 1;
    }

    __ADS7845_DIN_CLR();                                                /*  DIN = 0                      */
    __DelayNo(__DELAY_200NS * 3);                                       /*  延时3个CLK时间               */
    __ADS7845_DCLK_SET();                                               /*  DCLK = 1                     */
    __DelayNo(__DELAY_200NS);
    __ADS7845_DCLK_CLR();                                               /*  DCLK = 0                     */

    uiResult = 0;

    for (i = 0; i < 12; i++) {                                          /*  接收12位AD转换值             */

        uiResult <<= 1;

        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_SET();                                           /*  DCLK = 1                     */

        if (__ADS7845_DOUT_READ() != 0) {

            uiResult |= 1;
        }
        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_CLR();                                           /*  DCLK = 0                     */

        if (i == 6) {

            __DelayNo(__DELAY_200NS * 2);
        }
    }

    for (i = 0; i < 3; i++) {                                           /*  剩余的3个时钟                */

        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_SET();
        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_CLR();
    }

    __DelayNo(__DELAY_200NS);
    __ADS7845_CS_SET();                                                 /*  CS = 1                       */

   return (uiResult);                                                   /*  返回结果                     */
}

/**********************************************************************************************************
** Function name:           __sortPoint
** Descriptions:            连续采样点从小到大排序(泡排序)
** input parameters:        piBuf               缓冲区
**                          iNum                需要排序的数量
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/28
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static  void    __sortPoint (int  *piBuf, int  iNum)
{
    int     i, j;
    int     iTmp;                                                       /*  交换两值时临时变量           */
    int     iChanged = 0;                                               /*  当前趟是否有交换出现         */

    for (i = 1; i < iNum; i++) {                                        /*  iNum - 1 趟                  */
        iChanged = 0;

        for (j = 0; j < iNum - i; j++) {                                /*  排序                         */

            if (piBuf[j] > piBuf[j + 1]) {                              /*  交换                         */
                iTmp         = piBuf[j];
                piBuf[j]     = piBuf[j + 1];
                piBuf[j + 1] = iTmp;

                iChanged = 1;
            }
        }

        if (iChanged == 0) {                                            /*  本趟无交换，结束排序         */
            break;
        }
    }
}
/**********************************************************************************************************
** Function name:           touchGetXY
** Descriptions:            获得触摸屏物理电压 X Y (理论值域: [0,4095])
** input parameters:        NONE
** output parameters:       piX              X 轴方向电压
**                          piY              Y 轴方向电压
** Returned value:          1: 表示点有效   0: 表示点无效
** Created by:              Hanhui
** Created Date:            2007/09/28
**---------------------------------------------------------------------------------------------------------
** Modified by:             Wangfeng
** Modified date:           2008-09-06
** Descriptions             调用ads7845驱动完成 X 和 Y 轴的多次采样，采样到的值均为允许压力范围内的值
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
int  touchGetXY (int  *piX, int  *piY)
{
    int             i;

    int             iXTmp[TSC_SAMPLE_NUM];                            /*  临时点存放                   */
    int             iYTmp[TSC_SAMPLE_NUM];

    int             iXDiff, iYDiff;                                   /*  X Y 方向的最大差值           */

    unsigned int    is_press;                                         /*  用于保存压力值               */


    if (piX == NULL || piY == NULL) {                                 /*  指针错误                     */
        return  (0);
    }

    *piX = 0;                                                         /*  初始化为无效点               */
    *piY = 0;

    for (i = 0; i < TSC_SAMPLE_NUM; i++) {                            /*  连续采样                     */
        
        is_press     = __measurementGet(ADS7845AIN_Z2);
        if (is_press == 0) {
            return 0;
        }

        iXTmp[i] = __measurementGet(ADS7845AIN_X);
        iYTmp[i] = __measurementGet(ADS7845AIN_Y);
    }

    __sortPoint(iXTmp, TSC_SAMPLE_NUM);                               /*  X 排序                       */
    __sortPoint(iYTmp, TSC_SAMPLE_NUM);                               /*  Y 排序                       */

    iXDiff = iXTmp[TSC_SAMPLE_NUM - 2] - iXTmp[1];                    /*  计算最大差值                 */
    iYDiff = iYTmp[TSC_SAMPLE_NUM - 2] - iYTmp[1];                    /*  以上算法去掉最大值和最小值   */

    if (iXDiff > __ADC_X_MAX_DIFF) {                                  /*  X 轴无效测量                 */
        return  (0);
    }

    if (iYDiff > __ADC_Y_MAX_DIFF) {                                  /*  Y 轴无效测量                 */
        return  (0);
    }

#ifdef  __TOUCH_DEBUG_ON
    {
        for (i = 0; i < TSC_SAMPLE_NUM; i++) {
            serial_printf("x[%d]: %d y[%d]\n", i, iXTmp[i], i, iYTmp[i]);
        }
    }
#endif

    *piX = iXTmp[TSC_SAMPLE_NUM >> 1];
    *piY = iYTmp[TSC_SAMPLE_NUM >> 1];                                /*  置信平均值                   */

#ifdef  __TOUCH_DEBUG_ON
    serial_printf("x = %d y = %d\n", *piX, *piY);
#endif                                                                /*  __TOUCH_DEBUG_ON             */

    return  (1);
}

/**********************************************************************************************************
** Function name:           __DelayNo
** Descriptions:            短软件延时
** input parameters:        i       延时参数，值越大，延时越久
**
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Wangfeng
** Created Date:            2008/09/01
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static void  __DelayNo (volatile unsigned int i)
{
    volatile unsigned int j;

    for (; i > 0; i--) {
        for (j = 0; j < 71; j++);                                      /*  GCC(-O3):71 ARMCC:150         */
    }
}

/**********************************************************************************************************
  坐标校验宏(1301-320 DTTI屏, 其他情况需另得数据)
**********************************************************************************************************/
#define __TOUCH_X_MIN           380                                     /*  触摸屏测量x最小值            */
#define __TOUCH_Y_MIN           380                                     /*  触摸屏测量y最小值            */
#define __TOUCH_X_MAX           3450                                    /*  触摸屏测量x最大值            */
#define __TOUCH_Y_MAX           3580                                    /*  触摸屏测量y最大值            */

#define __SCREEN_WIDTH          800                                     /*  LCD屏最大x值                 */
#define __SCREEN_HEIGHT         600                                     /*  LCD屏最大y值                 */

/*==============================================================================
 * - _touch_to_screen()
 *
 * - convert the touch point to screen point
 */
static void _touch_to_screen (int tx, int ty, int *sx, int *sy)
{

    *sx = (int)(__SCREEN_WIDTH *                           /*  校验x值                      */
            (tx - __TOUCH_X_MIN) / 
            (__TOUCH_X_MAX - __TOUCH_X_MIN));
    *sy = (int)(__SCREEN_HEIGHT *                          /*  校验y值                      */
            (ty - __TOUCH_Y_MIN) / 
            (__TOUCH_Y_MAX - __TOUCH_Y_MIN));

    *sx = __SCREEN_WIDTH - *sx;         /* 颠倒左右 */
    *sy = __SCREEN_HEIGHT - *sy;        /* 颠倒上下 */
}

/*==============================================================================
 * - touch_read()
 *
 * - try to read touch screen input
 */
int touch_read (GUI_COOR *p_scr_coor)
{
    int iTouchX, iTouchY;       /*  触摸屏测量坐标               */
    int iScreenX, iScreenY;     /*  校验后LCD屏坐标              */
    int status; /* 触摸屏状态: 0 未被按下, 1 被按下 */

    if ((status = touchGetXY(&iTouchX, &iTouchY))) {

        _touch_to_screen (iTouchX, iTouchY, &iScreenX, &iScreenY);

        p_scr_coor->x = iScreenX;
        p_scr_coor->y = iScreenY;
    }

    return status;
}

int G_message_delay = 10;
/*==============================================================================
 * - T_ads7845Thread()
 *
 * - 向GUI的job消息队列中发送 [按下] 和 [释放] 的消息,
 *   发送 [按下] 消息时，附带本次检测到的测量坐标
 *   发送 [释放] 消息时，附带上次检测到的测量坐标
 */
void  T_ads7845Thread ()
{
#define _TRY_TIMES  2
    GUI_COOR scr_coor;
    int try;

    FOREVER {
        if (touch_read (&scr_coor)) { /* press */

_press:
            do {

                gui_job_add (&scr_coor, GUI_MSG_TOUCH_DOWN);
                delayQ_delay (G_message_delay);

            } while (touch_read (&scr_coor));

            /* try again again ... again */
            try = _TRY_TIMES;
            while (try--) {
                if (touch_read (&scr_coor)) {
                    goto _press;
                }
            }

            /* i still can't read press, so i'm up */
            gui_job_add (&scr_coor, GUI_MSG_TOUCH_UP);
        }

        delayQ_delay (4);
    }
#undef _TRY_TIMES
}

/**********************************************************************************************************
  END FILE
**********************************************************************************************************/


/*==============================================================================
** FILE END
==============================================================================*/

#endif /* #if 0 */
