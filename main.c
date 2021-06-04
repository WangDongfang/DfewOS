#include "serial.h"
#include "types.h"
#include "config.h"
#include "dlist.h"
#include "task.h"
#include "readyQ.h"
#include "delayQ.h"
#include "msgQ.h"
#include "semB.h"
#include "semM.h"
#include "cmd.h"
#include "bsp/int.h"

void timer_init(); /* in bsp file timer.c */
void beep_init();
void libc_init();
void heap_init();
void T_shell();
void delay_task();
void delay(volatile int n);

SEM_BIN *_G_p_semB = NULL;
MSG_QUE *_G_p_msgQ = NULL;
#define MESSAGE "This is a message from Task2 to Task1"
/**********************************************************************************************************
  Task1 Function
**********************************************************************************************************/
void T_EntryTask1()
{
    int i = 0;

    char message[sizeof(MESSAGE)];

    _G_p_semB = semB_init(NULL);

    semB_take(_G_p_semB, 0);
    FOREVER {
        serial_printf("Task1....%d\n",i);
    
        if (msgQ_receive(_G_p_msgQ, message, sizeof(MESSAGE), WAIT_FOREVER) == OS_STATUS_OK) {
            serial_printf("Task1 receive message: %s\n", message);
        } else {
            serial_printf("Task1 receive message failed! ;)\n");
        }

        i++;
#if 0
        delayQ_delay(100);

        if(i >= 10) {
            semB_give(_G_p_semB);
        }
#endif
    }
}

/**********************************************************************************************************
  Task2 Function
**********************************************************************************************************/
void T_EntryTask2()
{
    FOREVER {
#if 0
        if (semB_take(_G_p_semB, 700) == OS_STATUS_ERROR) {
            serial_printf("====================TASK 2 TAKE SEMB FAILED!=\n");
        }
#endif
        msgQ_send(_G_p_msgQ, MESSAGE, sizeof(MESSAGE), 10);

        /* serial_printf("Task2---------Ticks = %d\n", G_ticks); */
        /* serial_printf("%8s%d\n", "start",4); */
        task_show ();
        delayQ_delay(300);
    }
}

/**********************************************************************************************************
  Task3 Function
**********************************************************************************************************/
void T_EntryTask3 (uint32 arg1, uint32 arg2)
{
    int i = 0;

    _G_p_msgQ = msgQ_init(NULL, 4, sizeof(MESSAGE));

    serial_puts("\n/**********************************/\n");
    serial_printf("Task3 --------- arg1 = %d arg2 = %d\n", arg1, arg2);
    serial_puts("/**********************************/\n");
    for (; i < 3; i++) {
        delay(20);
        serial_printf("Task3 --------- %d\n", i);
    }
}
void test_msgQ_example_1 ()
{
    task_create("tTask1", 1024, 2, T_EntryTask1, 0, 0);
    task_create("tTask2", 1024, 3, T_EntryTask2, 0, 0);
    task_create("tTask3", 1024, 1, T_EntryTask3, 2, 3);
}

/**********************************************************************************************************
  Test semM: TsemM1 <- A,B; TsemM2 <- A; TsemM3 <- B;
  This use case to test this: When a higher task try to take a semM that have been taked by a lower task,
  the lower task up it's priority. When a task gived a semM, it down to previous priority. 

PRI:1                           *****B -----b **
PRI:2              ======A ----                  -----a ==
PRI:3  ---A---B---                                         ------
    
TsemM1: -------   TsemM2: ======   TsemM3: ****** : Task is running.
       A,B: this task take semM A,B;  a,b: this task give semM A,B.
**********************************************************************************************************/
SEM_MUX *_G_p_semM_A = NULL;
SEM_MUX *_G_p_semM_B = NULL;
SEM_MUX *_G_p_semM_C = NULL;

void T_Test1_semM ()
{
    int i = 0;

    semM_take(_G_p_semM_A, NO_WAIT);
    semM_take(_G_p_semM_B, NO_WAIT);

    serial_puts(COLOR_FG_RED);
    serial_puts("\n***************** START TEST SEMAPHORE MUTEX ****************\n");
    serial_puts(COLOR_FG_WHITE);
    FOREVER {
        serial_printf("TsemM1....%d",i);
        task_show();
        delayQ_delay(90);
        if (i == 3) {
            semM_give(_G_p_semM_B);
        }
        if (i == 5) {
            semM_give(_G_p_semM_A);
            break;
        }
        i++;
    }
}
void T_Test2_semM ()
{
    delayQ_delay(100);
    semM_take(_G_p_semM_A, WAIT_FOREVER);
    serial_puts(COLOR_FG_RED);
    serial_puts("=============== after TsemM2 taske semM_A: ==================");
    serial_puts(COLOR_FG_WHITE);
    task_show();
}
void T_Test3_semM ()
{
    delayQ_delay(200);
    semM_take(_G_p_semM_B, WAIT_FOREVER);
    serial_puts(COLOR_FG_RED);
    serial_puts("=============== after TsemM3 taske semM_B: ==================");
    serial_puts(COLOR_FG_WHITE);
    task_show();
}

void test_semM_example_1 ()
{
    _G_p_semM_A = semM_init(NULL);
    _G_p_semM_B = semM_init(NULL);
    _G_p_semM_C = semM_init(NULL);

    task_create("tTsemM1", 1024, 3, T_Test1_semM, 0, 0);
    task_create("tTsemM2", 1024, 2, T_Test2_semM, 0, 0);
    task_create("tTsemM3", 1024, 1, T_Test3_semM, 0, 0);
}

/**********************************************************************************************************
  Test semM: TsemM1 <- A; TsemM2 <- B; TsemM3 <- B;
  This use case to test this:

PRI:1                         *****B -----a =====b **
PRI:2           ===B===A ----                         ==
PRI:3  ---A----                                          ------
    
       TsemM1: -------   TsemM2: ======   TsemM3: ******
       A,B: this task take semM A,B;  a,b: this task give semM A,B
**********************************************************************************************************/
void T_semM_Test1 ()
{
    int i = 0;
    int is_after_give_A = 0;

    semM_take(_G_p_semM_A, NO_WAIT);                                    /*  I have semM_A                */

    serial_puts(COLOR_FG_RED);
    serial_puts("\n***************** START TEST SEMAPHORE MUTEX ****************\n");
    serial_puts(COLOR_FG_WHITE);
    FOREVER {
        serial_printf("TsemM1....%d\n",i);

        /*
         * because TsemM2 want to take semM_A, so TsemM2 rise this task to pri 2
         */
        if (G_p_current_tcb->task_priority == 2) {
            serial_puts(COLOR_FG_RED);
            serial_puts("============== after TsemM2 want take semM_A: ===============");
            serial_puts(COLOR_FG_WHITE);
            task_show();
        }

        /*
         * because TsemM3 want to take semM_B, but semM_B's owner is TsemM2
         * so TsemM3 rise TsemM2 to pri 1, correspond rise this task to pri 1
         */
        if (G_p_current_tcb->task_priority == 1) {
            serial_puts(COLOR_FG_RED);
            serial_puts("============== after TsemM3 want take semM_B: ===============");
            serial_puts(COLOR_FG_WHITE);
            task_show();

            semM_give(_G_p_semM_A);                                     /*  I give semM_A                */
            is_after_give_A = 1;
        }

        /*
         * after give semM_A, I must return to pri 3
         */
        if (G_p_current_tcb->task_priority == 3 && is_after_give_A) {
            serial_puts(COLOR_FG_RED);
            serial_puts("============== after TsemM1 give semM_A: ====================");
            serial_puts(COLOR_FG_WHITE);
            task_show();

            break;
        }

        delayQ_delay(100);
        i++;
    }
}
void T_semM_Test2 ()
{
    delayQ_delay(100);
    semM_take(_G_p_semM_B, NO_WAIT);

    semM_take(_G_p_semM_A, WAIT_FOREVER);

    /*
     * because TsemM3 want my semM_B, so I think my pri is 1
     */
    if (G_p_current_tcb->task_priority == 1) {
        serial_puts(COLOR_FG_RED);
        serial_puts("============== after TsemM2 taked semM_A: ===================\n");
        serial_puts("============== before TsemM2 give semM_B: ===================");
        serial_puts(COLOR_FG_WHITE);
        task_show();
    }
    semM_give(_G_p_semM_B);                                     /*  I give semM_A                */

    /*
     * after give semM_B, I must return pri 2
     */
    if (G_p_current_tcb->task_priority == 2) {
        serial_puts(COLOR_FG_RED);
        serial_puts("============== after TsemM2 give semM_B: ====================");
        serial_puts(COLOR_FG_WHITE);
        task_show();
    }
}
void T_semM_Test3 ()
{
    delayQ_delay(200);
    semM_take(_G_p_semM_B, WAIT_FOREVER);

    serial_puts(COLOR_FG_RED);
    serial_puts("============== I'm TsemM3, I taked semM_B: ==================");
    serial_puts(COLOR_FG_WHITE);
    task_show();
}

void test_semM_example_2 ()
{
    _G_p_semM_A = semM_init(NULL);
    _G_p_semM_B = semM_init(NULL);
    _G_p_semM_C = semM_init(NULL);

    task_create("tTsemM1", 1024, 3, T_semM_Test1, 0, 0);
    task_create("tTsemM2", 1024, 2, T_semM_Test2, 0, 0);
    task_create("tTsemM3", 1024, 1, T_semM_Test3, 0, 0);
}

/**********************************************************************************************************
  Idle task Function
**********************************************************************************************************/
void T_idle ()
{
    FOREVER {}
}

/*==============================================================================
 * - main()
 *
 * - the first C routine, called by _start
 */
int main()
{
    heap_init();        /* init heap memory, must be first */
    task_init();        /* clear task resoure */
    readyQ_init();      /* clear readyQ list, can omit */
    //libc_init();
    int_init();
    timer_init();       /* init TIMER hardware, enable tick intrrupt */

    serial_init();      /* init UART hardware, enable receive intrrupt */
    //beep_init();        /* init BEEP hardware, create beep task */

    task_create("tIdle",  1 * KB, 255, T_idle,  0, 0);
    task_create("tShell", 100 * KB,  0, T_shell, 0, 0);
    /* test_msgQ_example_1(); */
    /* test_semM_example_1(); */
    test_semM_example_2();


    LOAD_HIGHEST_TASK(); /* load highest task and tick start */

    /* never reach here */
    return 0;
}

void delay(volatile int n)
{
    volatile int i = 1000000;
    while (i-- > 0) {
        while (n-- > 0) {
            ;
        }
    }
}
