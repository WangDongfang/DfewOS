/*==============================================================================
** beep.c -- beep driver.
**
** MODIFY HISTORY:
**
** 2011-10-14 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "lib/iosLib.h"

#define BEEP_REG_CON       (*(volatile unsigned int*)0x50000000)
#define BEEP_REG_DAT       (*(volatile unsigned int*)0x50000000)
#define BEEP_REG_UP        (*(volatile unsigned int*)0x50000000)

/*======================================================================
  Beep operation
======================================================================*/
#define BEEP_ASSERT()      BIT_CLEAR(BEEP_REG_DAT, 10)
#define BEEP_DEASSERT()    BIT_SET(BEEP_REG_DAT, 10)

/*======================================================================
  configs
======================================================================*/
#define BEEP_INIT_CNT       3    /* when init beep driver alarm times */
#define BEEP_MAX_CNT        10   /* max accept application request */
#define BEEP_TASK_PRI       1

/*======================================================================
  Beep device
======================================================================*/
typedef struct beep_dev {
    DEV_HDR  dev_hdr;
    SEM_CNT  beep_semC;
    OS_TCB   *beep_task_id;
} BEEP_DEV;

/*======================================================================
  Function forward declare
======================================================================*/
static void _T_beep (BEEP_DEV *pBeepDev);

/*==============================================================================
 * - _beep_ioctl()
 *
 * - beep alarm in 1/10 second
 */
int _beep_ioctl (BEEP_DEV *pBeepDev, int cmd, int arg)
{
    semC_give (&pBeepDev->beep_semC);
    return 0;
}
int _beep_open (BEEP_DEV *pBeepDev, const char *file_name, int flags, int mode)
{
    return (int)pBeepDev;
}
int _beep_close (BEEP_DEV *pBeepDev)
{
    return 0;
}
void beep ()
{
}


/*==============================================================================
 * - beepDrv()
 *
 * - init beep hardware, and install to I/O system
 */
void beepDrv ()
{
    static int initialized = 0;
    int        drv_num;
    BEEP_DEV   *pBeepDev = NULL;

    if (initialized) {
        return;
    }

    drv_num = iosDrvInstall (NULL, NULL, _beep_open, _beep_close,
                             NULL, NULL, _beep_ioctl);

    pBeepDev = malloc (sizeof (BEEP_DEV));

    semC_init(&pBeepDev->beep_semC, BEEP_INIT_CNT, BEEP_MAX_CNT);
    pBeepDev->beep_task_id = task_create ("tBeep", 4 * KB, BEEP_TASK_PRI,
    						(FUNC_PTR)_T_beep, (int)pBeepDev, 0);

    iosDevAdd ((DEV_HDR *)pBeepDev, "/beep", drv_num);

    BEEP_REG_CON = (BEEP_REG_CON & ~(0x3 << 20)) | (0x1 << 20);
    BIT_CLEAR(BEEP_REG_UP,  10);
    BEEP_DEASSERT();

    initialized = 1;
}

/*==============================================================================
 * - _T_beep()
 *
 * - the task to process application's request
 */
static void _T_beep (BEEP_DEV *pBeepDev)
{
    FOREVER {
        semC_take (&pBeepDev->beep_semC, WAIT_FOREVER);

        //BEEP_ASSERT();
        delayQ_delay (SYS_CLK_RATE / 10); /* alarm 1/10 second */
        //BEEP_DEASSERT();
        delayQ_delay (SYS_CLK_RATE / 10); /* sleep 1/10 second */
    }
}

/*==============================================================================
** FILE END
==============================================================================*/

