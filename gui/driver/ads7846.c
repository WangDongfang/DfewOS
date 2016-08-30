/*==============================================================================
** ads7846.c -- ads7846 4-wire touch screen panel controllor driver.
**
** MODIFY HISTORY:
**
** 2012-05-05 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../gui.h"

/*======================================================================
  configs
======================================================================*/
#define TSC_SAMPLE_NUM   4
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
#define	__ADS7846_CS                (1 << 3)                            /*  7846_CS   �� rGPCDAT ��λ�� */
#define	__ADS7846_DCLK		        (1 << 1)                            /*  7846_DCLK �� rGPCDAT ��λ�� */
#define	__ADS7846_DIN               (1 << 2)                            /*  7846_DIN  �� rGPCDAT ��λ�� */
#define	__ADS7846_DOUT              (1 << 0)                            /*  7846_DOUT �� rGPCDAT ��λ�� */

#define __ADS7846_CS_SET()		    rGPCDAT  |=   __ADS7846_CS          /*  ��λads7846Ƭѡ����         */
#define __ADS7846_CS_CLR()          rGPCDAT  &= ~(__ADS7846_CS)         /*  ����EINT0 (ʹ��ADS7846)     */

#define __ADS7846_DOUT_READ()	    (rGPCDAT & (__ADS7846_DOUT))        /*  ��ȡads7846�����������     */

#define __ADS7846_DIN_SET()         rGPCDAT  |=   __ADS7846_DIN         /*  ����ads7846������������Ϊ1  */
#define __ADS7846_DIN_CLR()         rGPCDAT  &= ~(__ADS7846_DIN)        /*  ����ads7846������������Ϊ0  */

#define __ADS7846_DCLK_SET()        rGPCDAT  |=   __ADS7846_DCLK        /*  ����ads7846ʱ������Ϊ1      */
#define __ADS7846_DCLK_CLR()        rGPCDAT  &= ~(__ADS7846_DCLK)       /*  ����ads7846ʱ������Ϊ0      */

/**********************************************************************************************************
  ʱ����ʱ���ƺ궨��
**********************************************************************************************************/
#define __DELAY_200NS              10                                   /*  ����ʱ����ƺ�               */

/**********************************************************************************************************
  ���������������
**********************************************************************************************************/
#define ADS7846AIN_Y               0x93                                 /*  ADS7843�����ֲɼ�X��ѹ       */
#define ADS7846AIN_X               0xD3                                 /*  ADS7843�����ֲɼ�Y��ѹ       */
#define ADS7846AIN_Z1              0xB3                                 /*  ADS7843�����ֲɼ�Z1��ѹ      */
#define ADS7846AIN_Z2              0xC3                                 /*  ADS7843�����ֲɼ�Z2��ѹ      */

/**********************************************************************************************************
  ���������Ժ궨��
**********************************************************************************************************/
#define RxTOUCH_PLATE              595                                  /*  ������X+��X-֮��ĵ���       */
#define RyTOUCH_PLATE              358                                  /*  ������Y+��Y-֮��ĵ���       */

/**********************************************************************************************************
  ����ֵ����
**********************************************************************************************************/
#define  __ADC_X_MAX_DIFF          60                                   /*  X������ֵ                  */
#define  __ADC_Y_MAX_DIFF          90                                   /*  Y������ֵ                  */
/**********************************************************************************************************
  ѹ������ֵ����
**********************************************************************************************************/
#define  __PRESS_Z_MAX             1500                                 /*  �������ѹ������ֵ           */
#define  __PRESS_Z1_MIN            65                                   /*  ������Z1��Сֵ               */

/**********************************************************************************************************
  ���Կ��ƺ�
**********************************************************************************************************/

/**********************************************************************************************************
  helper functions prototypes
**********************************************************************************************************/
static   void  __DelayNo (volatile unsigned int i);
void  T_ads7846Thread ();

/**********************************************************************************************************
** Function name:           ads7846_init
** Descriptions:            ���ú�ads7846������GPIO���Ź��ܣ�����ʼ������ֵ
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
    gpioPinConfigSet(__ADS7846_CS, GPIO_OUTPUT);                        /*  ����7846_CS                  */
    gpioPinConfigSet(__ADS7846_DCLK, GPIO_OUTPUT);                      /*  ����7846_DCLK                */
    gpioPinConfigSet(__ADS7846_DIN, GPIO_OUTPUT);                       /*  ����7846_DIN                 */
    gpioPinConfigSet(__ADS7846_DOUT, GPIO_INPUT);                       /*  ����7846_DOUT                */
#else 
    rGPCCON &= ~(0x7 << 12);                                            /*  ����7846_CS                  */
    rGPCCON |=  (1 << 12);

    rGPCCON &= ~(0x7 << 4);                                             /*  ����7846_DCLK                */
    rGPCCON |=  (1 << 4);

    rGPCCON &= ~(0x7 << 8);                                             /*  ����7846_DIN                 */
    rGPCCON |=  (1 << 8);

    rGPCCON &= ~(0x7 << 0);                                             /*  ����7846_DOUT                */
#endif

    __ADS7846_CS_SET();                                                 /*  ��ʼ��ads7846Ƭѡ����Ϊ1     */
    __ADS7846_DCLK_CLR();                                               /*  ��ʼ��ads7846ʱ������Ϊ0     */
    __ADS7846_DIN_CLR();                                                /*  ��ʼ��ads7846��������Ϊ0     */
    
    __DelayNo(__DELAY_200NS);

    task_create ("tTouch", 8 * KB, TSC_TASK_PRI, T_ads7846Thread, 0, 0);
    return 0;
}
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
    __ADS7846_CS_CLR();                                                 /*  cs = 0 ��ƬѡADS7846         */
    __DelayNo(__DELAY_200NS);

    for (i = 0; i < 8; i++) {                                           /*  ����һ���ֽ�������           */

        if ((ucPosition & 0x80) != 0 ) {

            __ADS7846_DIN_SET();                                        /*  DIN = 1                      */
        } else {

            __ADS7846_DIN_CLR();                                        /*  DIN = 0                      */
        }

        __DelayNo(__DELAY_200NS);
        __ADS7846_DCLK_SET();                                           /*  DCLK = 1                     */
        __DelayNo(__DELAY_200NS);
        __ADS7846_DCLK_CLR();                                           /*  DCLK = 0                     */

        ucPosition <<= 1;
    }

    __ADS7846_DIN_CLR();                                                /*  DIN = 0                      */
    __DelayNo(__DELAY_200NS * 3);                                       /*  ��ʱ3��CLKʱ��               */
    __ADS7846_DCLK_SET();                                               /*  DCLK = 1                     */
    __DelayNo(__DELAY_200NS);
    __ADS7846_DCLK_CLR();                                               /*  DCLK = 0                     */

    uiResult = 0;

    for (i = 0; i < 12; i++) {                                          /*  ����12λADת��ֵ             */

        uiResult <<= 1;

        __DelayNo(__DELAY_200NS);
        __ADS7846_DCLK_SET();                                           /*  DCLK = 1                     */

        if (__ADS7846_DOUT_READ() != 0) {

            uiResult |= 1;
        }
        __DelayNo(__DELAY_200NS);
        __ADS7846_DCLK_CLR();                                           /*  DCLK = 0                     */

        if (i == 6) {

            __DelayNo(__DELAY_200NS * 2);
        }
    }

    for (i = 0; i < 3; i++) {                                           /*  ʣ���3��ʱ��                */

        __DelayNo(__DELAY_200NS);
        __ADS7846_DCLK_SET();
        __DelayNo(__DELAY_200NS);
        __ADS7846_DCLK_CLR();
    }

    __DelayNo(__DELAY_200NS);
    __ADS7846_CS_SET();                                                 /*  CS = 1                       */

   return (uiResult);                                                   /*  ���ؽ��                     */
}
/**********************************************************************************************************
** Function name:           pressureGet
** Descriptions:            ����ѹ��
** input parameters:        uiX     X �����ֵ
**                          uiZ1    Z1�����ֵ
**                          uiZ1    Z2�����ֵ
**
** output parameters:       NONE
** Returned value:          �������ѹ��ֵ
** Created by:              Wangfeng
** Created Date:            2008/09/06
**---------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
static  unsigned int  __pressureGet (unsigned int uiX, unsigned int uiZ1, unsigned int uiZ2)
{
    unsigned int    iPressure;

    if (uiZ1 == 0) {
        return (0);
    }

    iPressure = ((uiX * RxTOUCH_PLATE) / 4096) * (uiZ2 / uiZ1 - 1);

    return (iPressure);
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
** Descriptions             ����ads7846������� X �� Y ��Ķ�β�������������ֵ��Ϊ����ѹ����Χ�ڵ�ֵ
**---------------------------------------------------------------------------------------------------------
**********************************************************************************************************/
int  touchGetXY (int  *piX, int  *piY)
{
    int             i;

    int             iXTmp[TSC_SAMPLE_NUM];                            /*  ��ʱ����                   */
    int             iYTmp[TSC_SAMPLE_NUM];

    int             iXDiff, iYDiff;                                   /*  X Y ���������ֵ           */

    unsigned int    uiz1, uiz2, uiz;                                  /*  ���ڱ���ѹ��ֵ               */


    if (piX == NULL || piY == NULL) {                                 /*  ָ�����                     */
        return  (0);
    }

    *piX = 0;                                                         /*  ��ʼ��Ϊ��Ч��               */
    *piY = 0;

    for (i = 0; i < TSC_SAMPLE_NUM; i++) {                            /*  ��������                     */
        
        iXTmp[i] = __measurementGet(ADS7846AIN_X);
        iYTmp[i] = __measurementGet(ADS7846AIN_Y);
        uiz1     = __measurementGet(ADS7846AIN_Z1);
        uiz2     = __measurementGet(ADS7846AIN_Z2);

#ifdef  __TOUCH_DEBUG_ON
        serial_printf("uiz1 = %d; uiz2 = %d\n", uiz1,uiz2);
#endif 
        
        uiz1    &= 0xfffffff0;                                        /*  ȥ�� Z1 �� Z2 ��λֵ         */
        uiz2    &= 0xfffffff0;

#ifdef  __TOUCH_DEBUG_ON        
        serial_printf("uiz1 = %d; uiz2 = %d\n", uiz1,uiz2);
#endif 

        uiz   = __pressureGet(iXTmp[i], uiz1, uiz2);                  /*  ����ѹ��ֵ                   */

#ifdef  __TOUCH_DEBUG_ON        
        serial_printf("uiz = %d\n", uiz);
#endif 

        if ((uiz > __PRESS_Z_MAX) || (uiz1 < __PRESS_Z1_MIN)) {       /*  �ж�ѹ���Ƿ�������ķ�Χ��   */
            return (0);                                               /*  ����ѹ����Χ�������������� */
        }
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
            serial_printf("X: %d    ", iXTmp[i]);
            serial_printf("Y: %d\r\n", iYTmp[i]);
        }
    }
#endif                                                                /*  __TOUCH_DEBUG_ON             */

    *piX = iXTmp[TSC_SAMPLE_NUM >> 1];
    *piY = iYTmp[TSC_SAMPLE_NUM >> 1];                                /*  ����ƽ��ֵ                   */

    /* serial_printf ("x = %d y = %d\n", *piX, *piY); */

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
        for (j = 0; j < 71; j++);                                       /*  GCC(-O3):71 ARMCC:150        */
    }
}

/**********************************************************************************************************
  ����У���(1301-320 DTTI��, ����������������)
**********************************************************************************************************/
#if 0
#define __TOUCH_X_MIN           80                                      /*  ����������x��Сֵ            */
#define __TOUCH_Y_MIN           130                                     /*  ����������y��Сֵ            */
#define __TOUCH_X_MAX           4000                                    /*  ����������x���ֵ            */
#define __TOUCH_Y_MAX           3900                                    /*  ����������y���ֵ            */
/**********************************************************************************************************
  ����У���(AMT 9105��, ����������������) (5.8��, ����)
**********************************************************************************************************/
#define __TOUCH_X_MIN           360                                     /*  ����������x��Сֵ            */
#define __TOUCH_Y_MIN           268                                     /*  ����������y��Сֵ            */
#define __TOUCH_X_MAX           3700                                    /*  ����������x���ֵ            */
#define __TOUCH_Y_MAX           3720                                    /*  ����������y���ֵ            */
#else
/**********************************************************************************************************
  ����У���(AMT 9537��, ����������������) (10.4��)
**********************************************************************************************************/
#define __TOUCH_X_MIN           280                                     /*  ����������x��Сֵ            */
#define __TOUCH_Y_MIN           366                                     /*  ����������y��Сֵ            */
#define __TOUCH_X_MAX           3775                                    /*  ����������x���ֵ            */
#define __TOUCH_Y_MAX           3720                                    /*  ����������y���ֵ            */
#endif

#define __SCREEN_WIDTH          800                                     /*  LCD�����xֵ                 */
#define __SCREEN_HEIGHT         600                                     /*  LCD�����yֵ                 */

/*==============================================================================
 * - _touch_to_screen()
 *
 * - convert the touch point to screen point
 */
static void _touch_to_screen (int tx, int ty, int *sx, int *sy)
{
    /* int tmp = tx; tx = ty; ty = tmp; */

    *sx = (int)(__SCREEN_WIDTH *                           /*  У��xֵ                      */
            (tx - __TOUCH_X_MIN) / 
            (__TOUCH_X_MAX - __TOUCH_X_MIN));
    *sy = (int)(__SCREEN_HEIGHT *                          /*  У��yֵ                      */
            (ty - __TOUCH_Y_MIN) / 
            (__TOUCH_Y_MAX - __TOUCH_Y_MIN));

    /* *sx = __SCREEN_WIDTH - *sx; */
    /* *sy = __SCREEN_HEIGHT - *sy; */
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
 * - T_ads7846Thread()
 *
 * - ��GUI��job��Ϣ�����з��� [����] �� [�ͷ�] ����Ϣ,
 *   ���� [����] ��Ϣʱ���������μ�⵽�Ĳ�������
 *   ���� [�ͷ�] ��Ϣʱ�������ϴμ�⵽�Ĳ�������
 */
void  T_ads7846Thread ()
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

