
/*********************************************************************************************************
  裁剪控制
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
** 函数名称: sys_init
** 功能描述: 系统接口初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_init (void)
{
}
/*********************************************************************************************************
** 函数名称: sys_mutex_new
** 功能描述: 创建一个 lwip 互斥量
** 输　入  : pmutex    创建的互斥量
** 输　出  : 错误码
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_mutex_free
** 功能描述: 删除一个 lwip 互斥量
** 输　入  : pmutex      互斥量
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_mutex_free (sys_mutex_t *pmutex)
{
    SYS_STATS_DEC(mutex.used);
}
/*********************************************************************************************************
** 函数名称: sys_mutex_lock
** 功能描述: 锁定一个 lwip 互斥量
** 输　入  : pmutex      互斥量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_mutex_unlock
** 功能描述: 解锁一个 lwip 互斥量
** 输　入  : pmutex      互斥量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_mutex_valid
** 功能描述: 检查一个 lwip 互斥量是否有效
** 输　入  : pmutex      互斥量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_mutex_set_invalid
** 功能描述: 将一个 lwip 互斥量设置为无效
** 输　入  : pmutex      互斥量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_mutex_set_invalid (sys_mutex_t *pmutex)
{
    if (pmutex) {
        *pmutex = SYS_MUTEX_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: sys_sem_new
** 功能描述: 创建一个 lwip 信号量
** 输　入  : psem      创建的信号量
**           count     初始计数值
** 输　出  : 错误码
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_sem_free
** 功能描述: 删除一个 lwip 信号量
** 输　入  : psem   信号量句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_sem_free (sys_sem_t  *psem)
{
    semC_delete (*psem);
    SYS_STATS_DEC(sem.used);
}
/*********************************************************************************************************
** 函数名称: sys_sem_signal
** 功能描述: 发送一个 lwip 信号量
** 输　入  : psem   信号量句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_sem_signal (sys_sem_t *psem)
{
    if (psem) {
        semC_give (*psem);
    }
}
/*********************************************************************************************************
** 函数名称: sys_arch_sem_wait
** 功能描述: 等待一个 lwip 信号量
** 输　入  : psem   信号量句柄指针
** 输　出  : 等待时间
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
u32_t  sys_arch_sem_wait (sys_sem_t *psem, u32_t timeout_ms)
{
    uint32      old_tick = tick_get();
    uint32      new_tick;
#ifdef DEBUG_SEM_C
    static int c = 0;
    int cpsr_c;
#endif

    uint32  timeout_ticks = (timeout_ms * SYS_CLK_RATE) / 1000;  /*  转为 TICK 数                */
    
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
        timeout_ticks =  WAIT_FOREVER;          /*  无限 */
    } else {
        timeout_ticks = MAX(1, timeout_ticks);  /*  至少需要一个周期阻塞        */
    }
    
    if (semC_take((SEM_CNT *)*psem, timeout_ticks) == OS_STATUS_ERROR) {
        return  (SYS_ARCH_TIMEOUT);
    } else {
    
        new_tick = tick_get();
        new_tick = (new_tick >= old_tick) 
                  ? (new_tick -  old_tick) 
                  : (UINT32_MAX - old_tick + new_tick);         /*  计算 TICK 时差              */
    
        timeout_ms = (u32_t)((new_tick * 1000) / SYS_CLK_RATE); /*  转为毫秒数                  */
        
        return  (timeout_ms);
    }
}
/*********************************************************************************************************
** 函数名称: sys_sem_valid
** 功能描述: 获得 lwip 信号量计数器是否 > 0
** 输　入  : psem   信号量句柄指针
** 输　出  : 1: 有效 0: 无效
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_sem_set_invalid
** 功能描述: 清空 lwip 信号量计数器.
** 输　入  : psem   信号量句柄指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_sem_set_invalid (sys_sem_t *psem)
{
    if (psem) {
        *psem = SYS_SEM_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: sys_mbox_new
** 功能描述: 创建一个 lwip 通信邮箱
** 输　入  : pmbox     需要保存的邮箱句柄
**           size      大小(忽略)
** 输　出  : 错误码
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_mbox_free
** 功能描述: 释放一个 lwip 通信邮箱
** 输　入  : pmbox  邮箱句柄指针
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  sys_mbox_free (sys_mbox_t *pmbox)
{
    msgQ_delete (*pmbox);
    SYS_STATS_DEC(mbox.used);
}
/*********************************************************************************************************
** 函数名称: sys_mbox_post
** 功能描述: 发送一个邮箱消息, 一定保证成功. 只发送消息指针
** 输　入  : pmbox  邮箱句柄指针
**           msg    消息
** 输　出  : 
** 全局变量: 
** 调用模块: 
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
        
        if (status == OS_STATUS_OK) {                                   /*  发送成功                    */
            break;
        }
        serial_printf("sys_mbox_post() log : message full, sleep a little while.\n");
        delayQ_delay(__LWIP_MBOX_POST_RETRY_DELAY * SYS_CLK_RATE / 1000);/*  邮箱满等待 20 毫秒再发送    */
    } while (1);
}
/*********************************************************************************************************
** 函数名称: sys_mbox_trypost
** 功能描述: 发送一个邮箱消息(满则退出)
** 输　入  : pmbox  邮箱句柄指针
**           msg    消息
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
err_t  sys_mbox_trypost (sys_mbox_t *pmbox, void *msg)
{
    OS_STATUS status;
    
    if (pmbox == NULL) {
        return  (ERR_MEM);
    }
    
    //ulError = API_MsgQueueSend(*pmbox, &msg, sizeof(PVOID));
    status = msgQ_send(*pmbox, &msg, sizeof(void *), NO_WAIT);
    
    if (status == OS_STATUS_OK) {                                   /*  发送成功                    */
        return  (ERR_OK);
    } else {
        serial_printf("lwip sys_mbox_trypost() msgqueue error.\n");
        return  (ERR_MEM);
    }
}
/*********************************************************************************************************
** 函数名称: sys_arch_mbox_fetch
** 功能描述: 接收一个邮箱消息. 收到消息指针，然后赋给参数msg指向的数据
** 输　入  : pmbox  邮箱句柄指针
**           msg        消息
**           timeout    超时时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
u32_t   sys_arch_mbox_fetch (sys_mbox_t *pmbox, void  **msg, u32_t  timeout_ms)
{
    uint32      old_tick = tick_get();
    uint32      new_tick;
    void       *pvMsg;

    uint32  timeout_ticks = (timeout_ms * SYS_CLK_RATE) / 1000;  /*  转为 TICK 数                */
    
    if (pmbox == NULL) {
        return  (SYS_ARCH_TIMEOUT);
    }
    
#if 1
    if (timeout_ms == 0) {
        timeout_ticks =  WAIT_FOREVER;          /*  无限 */
    } else {
        timeout_ticks = MAX(1, timeout_ticks);  /*  至少需要一个周期阻塞        */
    }
#endif
    
    if (msgQ_receive(*pmbox, &pvMsg, sizeof(void *), timeout_ticks) == OS_STATUS_ERROR) {
        return  (SYS_ARCH_TIMEOUT);
    } else {
        new_tick = tick_get();
        new_tick = (new_tick >= old_tick) 
                  ? (new_tick -  old_tick) 
                  : (UINT32_MAX - old_tick + new_tick);         /*  计算 TICK 时差              */
    
        timeout_ms = (u32_t)((new_tick * 1000) / SYS_CLK_RATE); /*  转为毫秒数                  */
        if (msg != NULL) {
            *msg = pvMsg;                                               /*  需要保存消息                */
        }
        
        return  (timeout_ms);
    }
}
/*********************************************************************************************************
** 函数名称: sys_arch_mbox_tryfetch
** 功能描述: 无阻塞接收一个邮箱消息
** 输　入  : pmbox  邮箱句柄指针
**           msg    消息
** 输　出  : 
** 全局变量: 
** 调用模块: 
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
            *msg = pvMsg;                                               /*  需要保存消息                */
        }
        return  (ERR_OK);
    }
}
/*********************************************************************************************************
** 函数名称: sys_mbox_valid
** 功能描述: 获得 lwip 邮箱是否有效
** 输　入  : pmbox    邮箱句柄指针
** 输　出  : 1: 有效 0: 无效
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_mbox_set_invalid
** 功能描述: 清空 lwip 忽略邮箱中的消息
** 输　入  : pmbox    邮箱句柄指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_thread_new
** 功能描述: 创建一个线程
** 输　入  : name           线程名
**           thread         线程指针
**           arg            入口参数
**           stacksize      堆栈大小
**           prio           优先级
** 输　出  : 线程句柄
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_jiffies
** 功能描述: 返回系统时钟
** 输　入  : NONE
** 输　出  : 系统时钟
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
u32_t  sys_jiffies (void)
{
    return  ((u32_t)tick_get());
}

/*********************************************************************************************************
** 函数名称: sys_arch_msleep
** 功能描述: 延迟 ms 
** 输　入  : ms  需要延迟的 ms
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: sys_now
** 功能描述: 返回当前时间 (单位 : ms) 
** 输　入  : NONE
** 输　出  : 当前时间
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
u32_t  sys_now (void)
{
    return (tick_get() * (1000 / SYS_CLK_RATE));
}
/*********************************************************************************************************
 * FILE END
*********************************************************************************************************/
