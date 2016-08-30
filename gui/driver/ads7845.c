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
  �Ĵ���
**********************************************************************************************************/
#define rGPCCON             (*(volatile unsigned *)0x7F008040)          /*  Port C control              */
#define rGPCDAT             (*(volatile unsigned *)0x7F008044)          /*  Port C data                 */

/**********************************************************************************************************
  ������ƺ�
**********************************************************************************************************/
#define __TOUCH_DEBUG_OFF                                               /*  ���Կ���                     */

/*********************************************************************************************************
  �ܽ���غ궨��
*********************************************************************************************************/
#define	__ADS7845_CS                (1 << 3)                            /*  7845_CS   �� rGPCDAT ��λ�� */
#define	__ADS7845_DCLK		        (1 << 1)                            /*  7845_DCLK �� rGPCDAT ��λ�� */
#define	__ADS7845_DIN               (1 << 2)                            /*  7845_DIN  �� rGPCDAT ��λ�� */
#define	__ADS7845_DOUT              (1 << 0)                            /*  7845_DOUT �� rGPCDAT ��λ�� */

#define __ADS7845_CS_SET()		    rGPCDAT  |=   __ADS7845_CS          /*  ��λads7845Ƭѡ����         */
#define __ADS7845_CS_CLR()          rGPCDAT  &= ~(__ADS7845_CS)         /*  ����EINT0 (ʹ��ADS7845)     */

#define __ADS7845_DOUT_READ()	    (rGPCDAT & (__ADS7845_DOUT))        /*  ��ȡads7845�����������     */

#define __ADS7845_DIN_SET()         rGPCDAT  |=   __ADS7845_DIN         /*  ����ads7845������������Ϊ1  */
#define __ADS7845_DIN_CLR()         rGPCDAT  &= ~(__ADS7845_DIN)        /*  ����ads7845������������Ϊ0  */

#define __ADS7845_DCLK_SET()        rGPCDAT  |=   __ADS7845_DCLK        /*  ����ads7845ʱ������Ϊ1      */
#define __ADS7845_DCLK_CLR()        rGPCDAT  &= ~(__ADS7845_DCLK)       /*  ����ads7845ʱ������Ϊ0      */

/**********************************************************************************************************
  ʱ����ʱ���ƺ궨��
**********************************************************************************************************/
#define __DELAY_200NS              10                                   /*  ����ʱ����ƺ�               */

/**********************************************************************************************************
  ���������������
**********************************************************************************************************/
#define ADS7845AIN_Y               0x94                                 /*  ADS7845�����ֲɼ�X��ѹ       */
#define ADS7845AIN_X               0xD4                                 /*  ADS7845�����ֲɼ�Y��ѹ       */
#define ADS7845AIN_Z1              0xA4                                 /*  ADS7845�����ֲɼ�Z1��ѹ      */
#define ADS7845AIN_Z2              0xE4                                 /*  ADS7845�����ֲɼ��Ƿ���    */

/**********************************************************************************************************
  ����ֵ����
**********************************************************************************************************/
#define  __ADC_X_MAX_DIFF          120                                  /*  X������ֵ                  */
#define  __ADC_Y_MAX_DIFF          120                                  /*  Y������ֵ                  */

/**********************************************************************************************************
  ���Կ��ƺ�
**********************************************************************************************************/

/**********************************************************************************************************
  helper functions prototypes
**********************************************************************************************************/
static unsigned int __measurementGet (unsigned char ucPosition);
static   void  __DelayNo (volatile unsigned int i);
void  T_ads7845Thread ();

/**********************************************************************************************************
** Function name:           ads7845_init
** Descriptions:            ���ú�ads7845������GPIO���Ź��ܣ�����ʼ������ֵ
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          ��ȷ���� OK,  ���󷵻� ERROR
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
    gpioPinConfigSet(__ADS7845_CS, GPIO_OUTPUT);                        /*  ����7845_CS                  */
    gpioPinConfigSet(__ADS7845_DCLK, GPIO_OUTPUT);                      /*  ����7845_DCLK                */
    gpioPinConfigSet(__ADS7845_DIN, GPIO_OUTPUT);                       /*  ����7845_DIN                 */
    gpioPinConfigSet(__ADS7845_DOUT, GPIO_INPUT);                       /*  ����7845_DOUT                */
#else 
    rGPCCON &= ~(0x7 << 12);                                            /*  ����7845_CS                  */
    rGPCCON |=  (1 << 12);

    rGPCCON &= ~(0x7 << 4);                                             /*  ����7845_DCLK                */
    rGPCCON |=  (1 << 4);

    rGPCCON &= ~(0x7 << 8);                                             /*  ����7845_DIN                 */
    rGPCCON |=  (1 << 8);

    rGPCCON &= ~(0x7 << 0);                                             /*  ����7845_DOUT                */
#endif

    __ADS7845_CS_SET();                                                 /*  ��ʼ��ads7845Ƭѡ����Ϊ1     */
    __ADS7845_DCLK_CLR();                                               /*  ��ʼ��ads7845ʱ������Ϊ0     */
    __ADS7845_DIN_CLR();                                                /*  ��ʼ��ads7845��������Ϊ0     */
    
    __DelayNo(__DELAY_200NS);

#ifdef TSC_INT_ON
    /*
     * ע���ж�
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
** Descriptions:            ������������ϵĵ�ѹ
** input parameters:        ucPosition    Ҫ��������
**
** output parameters:       NONE
** Returned value:          ��ȡ���ĵ�ѹֵ
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
    __ADS7845_CS_CLR();                                                 /*  cs = 0 ��ƬѡADS7845         */
    __DelayNo(__DELAY_200NS);

    for (i = 0; i < 8; i++) {                                           /*  ����һ���ֽ�������           */

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
    __DelayNo(__DELAY_200NS * 3);                                       /*  ��ʱ3��CLKʱ��               */
    __ADS7845_DCLK_SET();                                               /*  DCLK = 1                     */
    __DelayNo(__DELAY_200NS);
    __ADS7845_DCLK_CLR();                                               /*  DCLK = 0                     */

    uiResult = 0;

    for (i = 0; i < 12; i++) {                                          /*  ����12λADת��ֵ             */

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

    for (i = 0; i < 3; i++) {                                           /*  ʣ���3��ʱ��                */

        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_SET();
        __DelayNo(__DELAY_200NS);
        __ADS7845_DCLK_CLR();
    }

    __DelayNo(__DELAY_200NS);
    __ADS7845_CS_SET();                                                 /*  CS = 1                       */

   return (uiResult);                                                   /*  ���ؽ��                     */
}

/**********************************************************************************************************
** Function name:           __sortPoint
** Descriptions:            �����������С��������(������)
** input parameters:        piBuf               ������
**                          iNum                ��Ҫ���������
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
    int     iTmp;                                                       /*  ������ֵʱ��ʱ����           */
    int     iChanged = 0;                                               /*  ��ǰ���Ƿ��н�������         */

    for (i = 1; i < iNum; i++) {                                        /*  iNum - 1 ��                  */
        iChanged = 0;

        for (j = 0; j < iNum - i; j++) {                                /*  ����                         */

            if (piBuf[j] > piBuf[j + 1]) {                              /*  ����                         */
                iTmp         = piBuf[j];
                piBuf[j]     = piBuf[j + 1];
                piBuf[j + 1] = iTmp;

                iChanged = 1;
            }
        }

        if (iChanged == 0) {                                            /*  �����޽�������������         */
            break;
        }
    }
}
/**********************************************************************************************************
** Function name:           touchGetXY
** Descriptions:            ��ô����������ѹ X Y (����ֵ��: [0,4095])
** input parameters:        NONE
** output parameters:       piX              X �᷽���ѹ
**                          piY              Y �᷽���ѹ
** Returned value:          1: ��ʾ����Ч   0: ��ʾ����Ч
** Created by:              Hanhui
** Created Date:            2007/09/28
**---------------------------------------------------------------------------------------------------------
** Modified by:             Wangfeng
** Modified date:           2008-09-06
** Descriptions             ����ads7845������� X �� Y ��Ķ�β�������������ֵ��Ϊ����ѹ����Χ�ڵ�ֵ
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
int  touchGetXY (int  *piX, int  *piY)
{
    int             i;

    int             iXTmp[TSC_SAMPLE_NUM];                            /*  ��ʱ����                   */
    int             iYTmp[TSC_SAMPLE_NUM];

    int             iXDiff, iYDiff;                                   /*  X Y ���������ֵ           */

    unsigned int    is_press;                                         /*  ���ڱ���ѹ��ֵ               */


    if (piX == NULL || piY == NULL) {                                 /*  ָ�����                     */
        return  (0);
    }

    *piX = 0;                                                         /*  ��ʼ��Ϊ��Ч��               */
    *piY = 0;

    for (i = 0; i < TSC_SAMPLE_NUM; i++) {                            /*  ��������                     */
        
        is_press     = __measurementGet(ADS7845AIN_Z2);
        if (is_press == 0) {
            return 0;
        }

        iXTmp[i] = __measurementGet(ADS7845AIN_X);
        iYTmp[i] = __measurementGet(ADS7845AIN_Y);
    }

    __sortPoint(iXTmp, TSC_SAMPLE_NUM);                               /*  X ����                       */
    __sortPoint(iYTmp, TSC_SAMPLE_NUM);                               /*  Y ����                       */

    iXDiff = iXTmp[TSC_SAMPLE_NUM - 2] - iXTmp[1];                    /*  ��������ֵ                 */
    iYDiff = iYTmp[TSC_SAMPLE_NUM - 2] - iYTmp[1];                    /*  �����㷨ȥ�����ֵ����Сֵ   */

    if (iXDiff > __ADC_X_MAX_DIFF) {                                  /*  X ����Ч����                 */
        return  (0);
    }

    if (iYDiff > __ADC_Y_MAX_DIFF) {                                  /*  Y ����Ч����                 */
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
    *piY = iYTmp[TSC_SAMPLE_NUM >> 1];                                /*  ����ƽ��ֵ                   */

#ifdef  __TOUCH_DEBUG_ON
    serial_printf("x = %d y = %d\n", *piX, *piY);
#endif                                                                /*  __TOUCH_DEBUG_ON             */

    return  (1);
}

/**********************************************************************************************************
** Function name:           __DelayNo
** Descriptions:            �������ʱ
** input parameters:        i       ��ʱ������ֵԽ����ʱԽ��
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
  ����У���(1301-320 DTTI��, ����������������)
**********************************************************************************************************/
#define __TOUCH_X_MIN           380                                     /*  ����������x��Сֵ            */
#define __TOUCH_Y_MIN           380                                     /*  ����������y��Сֵ            */
#define __TOUCH_X_MAX           3450                                    /*  ����������x���ֵ            */
#define __TOUCH_Y_MAX           3580                                    /*  ����������y���ֵ            */

#define __SCREEN_WIDTH          800                                     /*  LCD�����xֵ                 */
#define __SCREEN_HEIGHT         600                                     /*  LCD�����yֵ                 */

/*==============================================================================
 * - _touch_to_screen()
 *
 * - convert the touch point to screen point
 */
static void _touch_to_screen (int tx, int ty, int *sx, int *sy)
{

    *sx = (int)(__SCREEN_WIDTH *                           /*  У��xֵ                      */
            (tx - __TOUCH_X_MIN) / 
            (__TOUCH_X_MAX - __TOUCH_X_MIN));
    *sy = (int)(__SCREEN_HEIGHT *                          /*  У��yֵ                      */
            (ty - __TOUCH_Y_MIN) / 
            (__TOUCH_Y_MAX - __TOUCH_Y_MIN));

    *sx = __SCREEN_WIDTH - *sx;         /* �ߵ����� */
    *sy = __SCREEN_HEIGHT - *sy;        /* �ߵ����� */
}

/*==============================================================================
 * - touch_read()
 *
 * - try to read touch screen input
 */
int touch_read (GUI_COOR *p_scr_coor)
{
    int iTouchX, iTouchY;       /*  ��������������               */
    int iScreenX, iScreenY;     /*  У���LCD������              */
    int status; /* ������״̬: 0 δ������, 1 ������ */

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
 * - ��GUI��job��Ϣ�����з��� [����] �� [�ͷ�] ����Ϣ,
 *   ���� [����] ��Ϣʱ���������μ�⵽�Ĳ�������
 *   ���� [�ͷ�] ��Ϣʱ�������ϴμ�⵽�Ĳ�������
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
