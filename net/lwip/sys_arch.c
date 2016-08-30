
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#include  "arch\sys_arch.h"

#include  "lwip\debug.h"
#include  "lwip\def.h"
#include  "lwip\sys.h"
#include  "lwip\mem.h"
#include  "lwip\sio.h"
#include  "lwip\netdb.h"
#include  "lwip\stats.h"
#include  "lwip\init.h"
#include  "lwip\pbuf.h"
#include  "lwip/tcpip.h"

#define UINT32_MAX      0xffffffff
/*********************************************************************************************************
** ��������: sys_init
** ��������: ϵͳ�ӿڳ�ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_init (void)
{
}
/*********************************************************************************************************
** ��������: sys_mutex_new
** ��������: ����һ�� lwip ������
** �䡡��  : pmutex    �����Ļ�����
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_mutex_new (sys_mutex_t *pmutex)
{
    SYS_ARCH_DECL_PROTECT(lev);

    SEM_MUX *pNewMutex = semM_init(NULL);
    if (pNewMutex == NULL) {
        SYS_STATS_INC(mutex.err);
        return  (ERR_MEM);
    } else {
        if (pmutex) {
            *pmutex = pNewMutex;
        }
        SYS_STATS_INC(mutex.used);

        SYS_ARCH_PROTECT(lev);
        if (lwip_stats.sys.mutex.used > lwip_stats.sys.mutex.max) {
            lwip_stats.sys.mutex.max = lwip_stats.sys.mutex.used;
        }
        SYS_ARCH_UNPROTECT(lev);

        return  (ERR_OK);
    }

}
/*********************************************************************************************************
** ��������: sys_mutex_free
** ��������: ɾ��һ�� lwip ������
** �䡡��  : pmutex      ������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_free (sys_mutex_t *pmutex)
{
    SYS_STATS_DEC(mutex.used);
}
/*********************************************************************************************************
** ��������: sys_mutex_lock
** ��������: ����һ�� lwip ������
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_lock (sys_mutex_t  *pmutex)
{
    if (pmutex) {
        //serial_printf("Task: %8X Take SEM_M: %8X ... ", G_p_current_tcb, *pmutex);
        semM_take(*pmutex, WAIT_FOREVER);
        //serial_printf("after TAKE.\n");
    }
}
/*********************************************************************************************************
** ��������: sys_mutex_unlock
** ��������: ����һ�� lwip ������
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_unlock (sys_mutex_t  *pmutex)
{
    if (pmutex) {
        //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, *pmutex);
        semM_give(*pmutex);
        //serial_printf("AFTER GIVE.\n");
    }
}
/*********************************************************************************************************
** ��������: sys_mutex_valid
** ��������: ���һ�� lwip �������Ƿ���Ч
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  sys_mutex_valid (sys_mutex_t  *pmutex)
{
    if (pmutex) {
        if (*pmutex) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sys_mutex_set_invalid
** ��������: ��һ�� lwip ����������Ϊ��Ч
** �䡡��  : pmutex      ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mutex_set_invalid (sys_mutex_t *pmutex)
{
    if (pmutex) {
        *pmutex = SYS_MUTEX_NULL;
    }
}
/*********************************************************************************************************
** ��������: sys_sem_new
** ��������: ����һ�� lwip �ź���
** �䡡��  : psem      �������ź���
**           count     ��ʼ����ֵ
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_sem_new (sys_sem_t  *psem, u8_t  count)
{
    SYS_ARCH_DECL_PROTECT(lev);
    SEM_CNT *pNewSemC = semC_init(NULL, count, 1);
    
    if (pNewSemC == NULL) {
        serial_printf("sys_sem_new() error : can not create lwip sem.\n");
        SYS_STATS_INC(sem.err);
        return  (ERR_MEM);
    
    } else {
        if (psem) {
            *psem = pNewSemC;
        }
        SYS_STATS_INC(sem.used);
        
        SYS_ARCH_PROTECT(lev);
        if (lwip_stats.sys.sem.used > lwip_stats.sys.sem.max) {
            lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
        }
        SYS_ARCH_UNPROTECT(lev);
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_sem_free
** ��������: ɾ��һ�� lwip �ź���
** �䡡��  : psem   �ź������ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_sem_free (sys_sem_t  *psem)
{
    semC_delete (*psem);
    SYS_STATS_DEC(sem.used);
}
/*********************************************************************************************************
** ��������: sys_sem_signal
** ��������: ����һ�� lwip �ź���
** �䡡��  : psem   �ź������ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_sem_signal (sys_sem_t *psem)
{
    if (psem) {
        semC_give (*psem);
    }
}
/*********************************************************************************************************
** ��������: sys_arch_sem_wait
** ��������: �ȴ�һ�� lwip �ź���
** �䡡��  : psem   �ź������ָ��
** �䡡��  : �ȴ�ʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sys_arch_sem_wait (sys_sem_t *psem, u32_t timeout_ms)
{
    uint32      old_tick = tick_get();
    uint32      new_tick;
#ifdef DEBUG_SEM_C
    static int c = 0;
    int cpsr_c;
#endif

    uint32  timeout_ticks = (timeout_ms * SYS_CLK_RATE) / 1000;  /*  תΪ TICK ��                */
    
    if (psem == NULL) {
        return  (SYS_ARCH_TIMEOUT);
    }
    
#ifdef DEBUG_SEM_C
    cpsr_c = CPU_LOCK();
    serial_printf("*** semC_take(%d) ***\n", c);
    c++;
    CPU_UNLOCK(cpsr_c);
#endif
    if (timeout_ms == 0) {
        timeout_ticks =  WAIT_FOREVER;          /*  ���� */
    } else {
        timeout_ticks = MAX(1, timeout_ticks);  /*  ������Ҫһ����������        */
    }
    
    if (semC_take((SEM_CNT *)*psem, timeout_ticks) == OS_STATUS_ERROR) {
        return  (SYS_ARCH_TIMEOUT);
    } else {
    
        new_tick = tick_get();
        new_tick = (new_tick >= old_tick) 
                  ? (new_tick -  old_tick) 
                  : (UINT32_MAX - old_tick + new_tick);         /*  ���� TICK ʱ��              */
    
        timeout_ms = (u32_t)((new_tick * 1000) / SYS_CLK_RATE); /*  תΪ������                  */
        
        return  (timeout_ms);
    }
}
/*********************************************************************************************************
** ��������: sys_sem_valid
** ��������: ��� lwip �ź����������Ƿ� > 0
** �䡡��  : psem   �ź������ָ��
** �䡡��  : 1: ��Ч 0: ��Ч
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  sys_sem_valid (sys_sem_t *psem)
{
    if (psem) {
        if (*psem) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sys_sem_set_invalid
** ��������: ��� lwip �ź���������.
** �䡡��  : psem   �ź������ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_sem_set_invalid (sys_sem_t *psem)
{
    if (psem) {
        *psem = SYS_SEM_NULL;
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_new
** ��������: ����һ�� lwip ͨ������
** �䡡��  : pmbox     ��Ҫ�����������
**           size      ��С(����)
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_mbox_new (sys_mbox_t *pmbox, int  size)
{
    SYS_ARCH_DECL_PROTECT(lev);

    MSG_QUE  *pMsgQ = msgQ_init(NULL, LWIP_MSGQUEUE_SIZE, sizeof(void *));
    if (pMsgQ == 0ul) {
        serial_printf("sys_mbox_new() error : can not create lwip msgqueue.\n");
        SYS_STATS_INC(mbox.err);
        return  (ERR_MEM);
    
    } else {
        if (pmbox) {
            *pmbox = pMsgQ;
        }
        SYS_STATS_INC(mbox.used);
        
        SYS_ARCH_PROTECT(lev);
        if (lwip_stats.sys.mbox.used > lwip_stats.sys.mbox.max) {
            lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
        }
        SYS_ARCH_UNPROTECT(lev);
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_free
** ��������: �ͷ�һ�� lwip ͨ������
** �䡡��  : pmbox  ������ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mbox_free (sys_mbox_t *pmbox)
{
    msgQ_delete (*pmbox);
    SYS_STATS_DEC(mbox.used);
}
/*********************************************************************************************************
** ��������: sys_mbox_post
** ��������: ����һ��������Ϣ, һ����֤�ɹ�. ֻ������Ϣָ��
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#define __LWIP_MBOX_POST_RETRY_DELAY    20
void  sys_mbox_post (sys_mbox_t *pmbox, void *msg)
{
    OS_STATUS status;
    
    if (pmbox == NULL) {
        return;
    }
    
    do {
        //ulError = API_MsgQueueSend(*pmbox, &msg, sizeof(PVOID));
        status = msgQ_send(*pmbox, &msg, sizeof(void *), NO_WAIT);
        
        if (status == OS_STATUS_OK) {                                   /*  ���ͳɹ�                    */
            break;
        }
        serial_printf("sys_mbox_post() log : message full, sleep a little while.\n");
        delayQ_delay(__LWIP_MBOX_POST_RETRY_DELAY * SYS_CLK_RATE / 1000);/*  �������ȴ� 20 �����ٷ���    */
    } while (1);
}
/*********************************************************************************************************
** ��������: sys_mbox_trypost
** ��������: ����һ��������Ϣ(�����˳�)
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
err_t  sys_mbox_trypost (sys_mbox_t *pmbox, void *msg)
{
    OS_STATUS status;
    
    if (pmbox == NULL) {
        return  (ERR_MEM);
    }
    
    //ulError = API_MsgQueueSend(*pmbox, &msg, sizeof(PVOID));
    status = msgQ_send(*pmbox, &msg, sizeof(void *), NO_WAIT);
    
    if (status == OS_STATUS_OK) {                                   /*  ���ͳɹ�                    */
        return  (ERR_OK);
    } else {
        serial_printf("lwip sys_mbox_trypost() msgqueue error.\n");
        return  (ERR_MEM);
    }
}
/*********************************************************************************************************
** ��������: sys_arch_mbox_fetch
** ��������: ����һ��������Ϣ. �յ���Ϣָ�룬Ȼ�󸳸�����msgָ�������
** �䡡��  : pmbox  ������ָ��
**           msg        ��Ϣ
**           timeout    ��ʱʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t   sys_arch_mbox_fetch (sys_mbox_t *pmbox, void  **msg, u32_t  timeout_ms)
{
    uint32      old_tick = tick_get();
    uint32      new_tick;
    void       *pvMsg;

    uint32  timeout_ticks = (timeout_ms * SYS_CLK_RATE) / 1000;  /*  תΪ TICK ��                */
    
    if (pmbox == NULL) {
        return  (SYS_ARCH_TIMEOUT);
    }
    
#if 1
    if (timeout_ms == 0) {
        timeout_ticks =  WAIT_FOREVER;          /*  ���� */
    } else {
        timeout_ticks = MAX(1, timeout_ticks);  /*  ������Ҫһ����������        */
    }
#endif
    
    if (msgQ_receive(*pmbox, &pvMsg, sizeof(void *), timeout_ticks) == OS_STATUS_ERROR) {
        return  (SYS_ARCH_TIMEOUT);
    } else {
        new_tick = tick_get();
        new_tick = (new_tick >= old_tick) 
                  ? (new_tick -  old_tick) 
                  : (UINT32_MAX - old_tick + new_tick);         /*  ���� TICK ʱ��              */
    
        timeout_ms = (u32_t)((new_tick * 1000) / SYS_CLK_RATE); /*  תΪ������                  */
        if (msg != NULL) {
            *msg = pvMsg;                                               /*  ��Ҫ������Ϣ                */
        }
        
        return  (timeout_ms);
    }
}
/*********************************************************************************************************
** ��������: sys_arch_mbox_tryfetch
** ��������: ����������һ��������Ϣ
** �䡡��  : pmbox  ������ָ��
**           msg    ��Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t   sys_arch_mbox_tryfetch (sys_mbox_t *pmbox, void  **msg)
{
    void *pvMsg;
    
    if (pmbox == NULL) {
        return  (SYS_MBOX_EMPTY);
    }
    
    //ulError = API_MsgQueueTryReceive(*pmbox, &pvMsg, sizeof(PVOID), &ulMsgLen);
    if (msgQ_receive(*pmbox, &pvMsg, sizeof(void *), NO_WAIT) == OS_STATUS_ERROR) {
        return  (SYS_MBOX_EMPTY);
    } else {
        if (msg) {
            *msg = pvMsg;                                               /*  ��Ҫ������Ϣ                */
        }
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** ��������: sys_mbox_valid
** ��������: ��� lwip �����Ƿ���Ч
** �䡡��  : pmbox    ������ָ��
** �䡡��  : 1: ��Ч 0: ��Ч
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  sys_mbox_valid (sys_mbox_t *pmbox)
{
    if (pmbox) {
        if (*pmbox) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: sys_mbox_set_invalid
** ��������: ��� lwip ���������е���Ϣ
** �䡡��  : pmbox    ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_mbox_set_invalid (sys_mbox_t *pmbox)
{
    if (pmbox) {
        *pmbox = SYS_MBOX_NULL;
    }
}
/*********************************************************************************************************
  thread
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: sys_thread_new
** ��������: ����һ���߳�
** �䡡��  : name           �߳���
**           thread         �߳�ָ��
**           arg            ��ڲ���
**           stacksize      ��ջ��С
**           prio           ���ȼ�
** �䡡��  : �߳̾��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
sys_thread_t  sys_thread_new (const char *name, lwip_thread_fn thread, void *arg, int  stacksize, int prio)
{
    OS_TCB *pNewThread = NULL;
    
    pNewThread = task_create(name, stacksize, (uint8)prio, (FUNC_PTR)thread, (uint32)arg, 0);
    if (pNewThread == NULL) {
        serial_printf("sys_thread_new() error : can not create lwip thread.\n");
    }

    return  (pNewThread);
}
/*********************************************************************************************************
  time
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: sys_jiffies
** ��������: ����ϵͳʱ��
** �䡡��  : NONE
** �䡡��  : ϵͳʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sys_jiffies (void)
{
    return  ((u32_t)tick_get());
}

/*********************************************************************************************************
** ��������: sys_arch_msleep
** ��������: �ӳ� ms 
** �䡡��  : ms  ��Ҫ�ӳٵ� ms
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  sys_arch_msleep (u32_t ms)
{
    uint32 ticks;
    
    ticks = (ms * SYS_CLK_RATE) / 1000;
    if (ticks == 0) {
        ticks =  1;
    }
    
    delayQ_delay(ticks);
}
/*********************************************************************************************************
** ��������: sys_now
** ��������: ���ص�ǰʱ�� (��λ : ms) 
** �䡡��  : NONE
** �䡡��  : ��ǰʱ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
u32_t  sys_now (void)
{
    return (tick_get() * (1000 / SYS_CLK_RATE));
}
/*********************************************************************************************************
 * FILE END
*********************************************************************************************************/
