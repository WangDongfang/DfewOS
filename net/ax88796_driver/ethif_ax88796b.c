/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               ethif_ax88796b.c
** Last modified Date:      2009-05-16
** Last Version:            1.0.0
** Descriptions:            AX88796B网卡设备驱动程序
**
**--------------------------------------------------------------------------------------------------------
** Created by:              XuGuizhou
** Created date:            2009-05-16
** Version:                 1.0.0
** Descriptions:            AX88796B网卡设备驱动程序
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             WangDongfang
** Modified date:           2011-08-24
** Version:
** Descriptions:            port to dfew os
**
*********************************************************************************************************/

#include <dfewos.h>
#include "ethif_ax88796b.h"
#include "netif\etharp.h"
#include "../../bsp/int.h"

#ifndef ulong
#define ulong uint32
#endif
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define IFNAME0         'e'
#define IFNAME1         'n'
#define ETH_ZLEN         60                                             /* 最小以太网数据包长度         */
#define DRV_NAME        "ax88796b"


#define OPTIMZE_DISABLE 1                                               /* Must define this macro       */
#define AX_8BIT_MODE    0

#define DMA_TIMEOUT_CNT 200000

/*********************************************************************************************************
  设备控制数据结构
*********************************************************************************************************/
typedef struct ax_dev
{
    const char *name;
    ulong       membase;
    uint8       ucPktData[AX88796_BUFSIZ];
    uint8       bus_width;
    uint8       mcfilter[8];
    uint8       ucHWFlowEn;                                             /* 硬件流量控制                 */
    uint8       media;
    uint8       media_curr;
    uint8       tx_curr_page;
    uint8       tx_prev_ctepr;
    uint8       tx_curr_ctepr;
    uint8       tx_full;
    uint8       tx_start_page;
    uint8       tx_stop_page;
    uint8       rx_start_page;
    uint8       stop_page;
    uint8       current_page;                                           /* 设备内部缓存的当前读指针     */
    uint32      rx_handling;                                            /* 接收状态                     */
    uint32      imask;                                                  /* 启用中断标志位               */
    ulong       ulChannel;                                              /* 中断等级                     */
    ulong       ulPrio;                                                 /* 中断优先级                   */
    SEM_MUX    *dev_sem;                                                /* 设备的发送信号量             */
    SEM_MUX    *fifo_sem;                                               /* 设备的FIFO互斥信号量         */

    struct netif    *netif;                                             /* 绑定在此物理设备上的网络接口 */

    unsigned irqlock:1;
} AX_DEVICE;
/*********************************************************************************************************
  设备状态宏定义
*********************************************************************************************************/
#define AX_RECVING_FLAG    0x01
/*********************************************************************************************************
  寄存器读写宏定义
*********************************************************************************************************/
#define AX88796_REG_BYTE_READ(base, off)         ((volatile uint8) *((volatile uint16*)(base + ((off)<<1))))
#define AX88796_REG_SHORT_READ(base, off)        (*((volatile uint16 *)(base + ((off)<<1))))
#define AX88796_REG_BYTE_WRITE(base, off, data)  (*((volatile uint8*)(base + ((off)<<1))) = ((uint8)data))
#define AX88796_REG_SHORT_WRITE(base, off, data) (*((volatile uint16 *)(base + ((off)<<1))) = ((uint16)data))

#define AX_REG_WR(data, base, off)  (*((volatile uint16 *)(base + ((off)<<1))) = (uint16)(data))
#define AX_REG_RD(base, off)        ((uint8) *((volatile uint16 *) (base + ((off)<<1))) )

#if (AX_8BIT_MODE == 1)
#define READ_FIFO(membase, off) (AX88796_REG_BYTE_READ (membase, off) | \
                                (((uint16) AX88796_REG_BYTE_READ (membase, off)) << 8))
#define WRITE_FIFO(membase, data) \
    do { \
        AX88796_REG_BYTE_WRITE((uint8)data , membase); \
        AX88796_REG_BYTE_WRITE((uint8)(data >> 8) , membase); \
    } while(0)

#else
#define READ_FIFO(membase, off)       AX88796_REG_SHORT_READ (membase, off)
#define WRITE_FIFO(membase, off, data) AX88796_REG_SHORT_WRITE (membase, off, data)
#endif

/*********************************************************************************************************
  设备数据结构定义
*********************************************************************************************************/
typedef struct ax_pkt_hdr {
  uint8 status;                                                         /* 数据包状态                   */
  uint8 next;                                                           /* 指向下一页的指针             */
  uint16 count;                                                         /* 包头加数据的总长度           */
} AX_PKT_HDR;

typedef struct ax_dev_config {
    uint32  uiUnitNum;                                                  /* 设备序号                     */
    ulong   membase;                                                    /* 设备基址                     */
    ulong   ulChannel;                                                  /* 中断等级                     */
    ulong   ulPrio;                                                     /* 中断优先级                   */
    uint32  uiUsed;                                                     /* 设备已占用                   */

    void (*pHWInitFunc) (void);                                         /* 设备硬件初始化函数           */
} AX_DEV_CONFIG;
/*********************************************************************************************************
  设备驱动程序函数接口声明
*********************************************************************************************************/
static void  ax_init        (AX_DEVICE *ax_local, int startp);
static err_t ax_input       (AX_DEVICE *ax_local);
static err_t ax_get_hdr     (AX_DEVICE *ax_local,
                             struct ax_pkt_hdr *hdr,
                             int ring_page);
static err_t ax_block_input (AX_DEVICE *ax_local,
                             int count, struct pbuf *p,
                             int ring_offset);
static err_t ax_block_output(AX_DEVICE *ax_local,
                              uint16 count,
                              struct pbuf *p,
                              const int start_page);
static void  ax_trigger_send(AX_DEVICE *ax_local,
                              unsigned int length,
                              int start_page);
static void  ax_interrupt   (AX_DEVICE *ax_local);
static void  ax_tx_intr     (AX_DEVICE *ax_local);
static void  ax_tx_err      (AX_DEVICE *ax_local);
static void  ax_rx_overrun  (AX_DEVICE *ax_local,
                             uint8 was_txing);
static void  ax_restart     (AX_DEVICE *ax_local);
static void  ax_reset       (AX_DEVICE *ax_local);
static err_t ax_dev_init    (AX_DEVICE *ax_local);
static void  ax_hw_init     (void);


static AX_DEV_CONFIG * ax_get_free_dev(void);                           /* 多网口支持                   */
/*********************************************************************************************************
  可供外部调用的函数接口声明
*********************************************************************************************************/
err_t ethdev_init           (struct netif *netif);
err_t ax_link_output        (struct netif *netif, struct pbuf *p);

err_t ax_hw_flow_ctrl       (struct netif *netif, uint8 enable);
/*********************************************************************************************************
  网络设备参数配置接口
*********************************************************************************************************/

/*********************************************************************************************************
  中断函数接口声明 EPCARM6410 使用外部中断 EINT11
*********************************************************************************************************/
static err_t ax_sys_irq_connect              (AX_DEVICE *ax_local, ulong ulIsrFunc);
static err_t ax_sys_irq_disconnect           (AX_DEVICE *ax_local);
static err_t ax_sys_irq_disable              (AX_DEVICE *ax_local);
static err_t ax_sys_irq_enable               (AX_DEVICE *ax_local);

/*********************************************************************************************************
  调试函数接口声明
*********************************************************************************************************/
#define KERN_INFO    "KERN_INFO:"
#define KERN_WARNING "KERN_WARNING:"
#define KERN_ERR     "KERN_ERR:"
#define KERN_NOTICE  "KERN_NOTICE:"
#define intDebugInfo  serial_printf

#define AX_REG_DEBUG 0

#if AX_REG_DEBUG
uint8 bound;
uint8 curr_rx;
uint8 imr;
uint8 isr_stat;
#endif
/*********************************************************************************************************
  IGMP支持
*********************************************************************************************************/
/*#define AX_IGMP_SUPPORT LWIP_IGMP*/
#define AX_IGMP_SUPPORT 0
#if AX_IGMP_SUPPORT
err_t ax_igmp_mac_filter(struct netif *netif, struct ip_addr *group, u8_t action);
#endif

/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
/*
 * 空闲设备数组
 * 存储设备基本信息
 * 指示设备是否被占用
 */
AX_DEV_CONFIG gAXDevGroup[] =
    {
        {
            0,                                                            /* 设备序号                   */
            AX88796B_BASE,                                                /* 设备基址                   */
            INT_NUMBER_EINT1,                                             /* 中断号                     */
            0,                                                            /* 中断优先级                 */
            FALSE,                                                        /* 设备未占用                 */
            ax_hw_init                                                    /* 设备硬件初始化函数         */
        },

        {
            0,                                                            /* 设备序号                   */
            0,                                                            /* 设备基址                   */
            0,                                                            /* 中断等级                   */
            0,                                                            /* 中断优先级                 */
            FALSE,                                                        /* 设备未占用                 */
            NULL
        }
    };


/*********************************************************************************************************
** Function name:           ax_get_free_dev
** Descriptions:            从空闲设备数组中获取一个空闲网卡设备
** input parameters:        NONE
** output parameters:
** Returned value:          AX_DEV_CONFIG *   空闲的网卡设备指针
** Created by:              XuGuizhou
** Created Date:            2009/06/12
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static AX_DEV_CONFIG * ax_get_free_dev(void)
{
    int i;

    for(i=0; gAXDevGroup[i].membase != 0; i++)
    {
        if(gAXDevGroup[i].uiUsed == 0)
        {
            return &gAXDevGroup[i];
        }
    }
    return NULL;
}

/*********************************************************************************************************
** Function name:           ethdev_init
** Descriptions:            驱动程序对外接口，初始化网络设备；并将网络设备与硬件设备绑定
** input parameters:        netif          网络设备指针，指向空间由上层应用程序申请
** output parameters:
** Returned value:          ERR_OK
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
err_t ethdev_init (struct netif *netif)
{
      AX_DEVICE *ax_local;

    ax_local = (AX_DEVICE *) malloc(sizeof(AX_DEVICE));
    if (ax_local == NULL)
        return ERR_MEM;

    /* 记录绑定的网络接口 */
    ax_local->netif = netif;

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    /* netif->input is init in app task by user
     * ex. netif->input = tcpip_input
     */
    netif->output     = etharp_output;
    netif->linkoutput = ax_link_output;

    /* init by user in app task */
    netif->state = ax_local;
    netif->flags = 0;

#if LWIP_ARP
    netif->flags |= NETIF_FLAG_ETHARP;
#endif                                                                  /* LWIP_ARP                     */

    /* broadcast capability */
    netif->flags |= NETIF_FLAG_BROADCAST;

#if AX_IGMP_SUPPORT
    netif->flags |= NETIF_FLAG_IGMP;
    netif->igmp_mac_filter = ax_igmp_mac_filter;
#endif

    /* If mac is NOT set, set a default value. Just for Testing */
    if(netif->hwaddr_len == 0)
    {
        netif->hwaddr[0] = 0x00;
        netif->hwaddr[1] = 0x01;
        netif->hwaddr[2] = 0x22;
        netif->hwaddr[3] = 0x23;
        netif->hwaddr[4] = 0x67;
        netif->hwaddr[5] = 0x98;

        netif->hwaddr_len = 6;
    }

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* set default configuration of ax dev */
    if(ax_dev_init(ax_local) != ERR_OK)
    {
        free(ax_local);
        return ERR_USE;
    }

    /* connect system irq */
    ax_sys_irq_connect(ax_local, (ulong)ax_interrupt);

    /* init ax dev according to default configuration */
    ax_init(ax_local, 1);

#ifdef AX_WATCHDOG_SUPPORT
    ax_watchdog_create(ax_local);                                       /* Create Watchdog              */
    ax_watchdog_start(ax_local);                                        /* Start Watchdog               */
#endif

    etharp_init();

    //intDebugInfo(KERN_INFO "ethdev_init init over.\n");

    return ERR_OK;
}

/*********************************************************************************************************
** Function name:           ax_init
** Descriptions:            初始化AX88796B网络设备，并根据startp的值决定是否启动设备
** input parameters:        ax_local          设备驱动程序数据结构
                            startp         是否启动设备
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:             XuGuizhou
** Modified date:           2009/06/10
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_init (AX_DEVICE *ax_local, int startp)
{
    struct netif *netif    = ax_local->netif;
    ulong         ax_base  = ax_local->membase;

    int cpsr_c;

    cpsr_c = CPU_LOCK();

    /* disable system irq */
    ax_sys_irq_disable(ax_local);

    /* Clear the pending interrupts and mask. */
    AX_REG_WR(0xFF, ax_base, EN0_ISR);
    AX_REG_WR(0x00, ax_base, EN0_IMR);

    /* Follow National Semi's recommendations for initing the DP83902. */
    AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_STOP, ax_base, E8390_CMD);  /* 0x21                         */
    AX_REG_WR(ax_local->bus_width, ax_base , EN0_DCFG);                 /* 8-bit or 16-bit              */

    delayQ_delay(1);

    /* Use default value is ok */
    #if 0
    AX_REG_WR(E8390_NODMA+E8390_PAGE1+E8390_STOP, ax_base,E8390_CMD);
    AX_REG_WR(ENBTCR_INT_ACT_HIGH, ax_base + EN0_BTCR);
    AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_STOP, ax_base , E8390_CMD);
    #endif

    //intDebugInfo(KERN_INFO "ax_init, BTCR: %02X\n", AX_REG_RD(ax_base, EN0_BTCR));

    /* Clear the remote byte count registers. */
    AX_REG_WR(0x00,  ax_base, EN0_RCNTLO);
    AX_REG_WR(0x00,  ax_base, EN0_RCNTHI);

    /* Set to monitor and loopback mode -- this is vital!. */
    AX_REG_WR(E8390_RXOFF, ax_base, EN0_RXCR);                          /* 0x20                         */
    AX_REG_WR(E8390_TXOFF, ax_base, EN0_TXCR);                          /* 0x02                         */

    /* Set the transmit page and receive ring. */
    AX_REG_WR(NESM_START_PG, ax_base, EN0_TPSR);
    AX_REG_WR(NESM_RX_START_PG, ax_base, EN0_STARTPG);
    AX_REG_WR(NESM_RX_START_PG, ax_base, EN0_BOUNDARY);

    ax_local->current_page = NESM_RX_START_PG + 1;                        /* assert boundary+1            */
    AX_REG_WR(NESM_STOP_PG, ax_base, EN0_STOPPG);

    ax_local->tx_prev_ctepr = 0;
    ax_local->tx_start_page = NESM_START_PG;
    ax_local->tx_curr_page  = NESM_START_PG;
    ax_local->tx_stop_page  = NESM_START_PG + TX_PAGES;

    ax_local->imask         = ENISR_ALL;                                /* Enable all IRQ               */

    /* Disable Flow control. */
    AX_REG_WR (0x7, ax_base, EN0_FLOW);

    /* Copy the station address into the DS8390 registers. */
    AX_REG_WR(E8390_NODMA + E8390_PAGE1 + E8390_STOP,
                ax_base, E8390_CMD);                                    /* 0x61                         */
    AX_REG_WR(NESM_START_PG + TX_PAGES + 1, ax_base, EN1_CURPAG);

    /* Change for NO Warning */
    AX_REG_WR(netif->hwaddr[0], ax_base,  1);
    AX_REG_WR(netif->hwaddr[1], ax_base,  2);
    AX_REG_WR(netif->hwaddr[2], ax_base,  3);
    AX_REG_WR(netif->hwaddr[3], ax_base,  4);
    AX_REG_WR(netif->hwaddr[4], ax_base,  5);
    AX_REG_WR(netif->hwaddr[5], ax_base,  6);

    AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_STOP, ax_base, E8390_CMD);

    if (startp)
    {

        /* Enable Flow control. High Water free Page Count is 0x7 */
        if(ax_local->ucHWFlowEn)
            AX_REG_WR (ENFLOW_ENABLE, ax_base, EN0_FLOW);

        /* Enable AX88796B TQC */
        AX_REG_WR((uint16)(AX_REG_RD (ax_base, EN0_MCR) | ENTQC_ENABLE), ax_base, EN0_MCR);

        /* Enable AX88796B Transmit Buffer Ring */
        AX_REG_WR(E8390_NODMA+E8390_PAGE3+E8390_STOP, ax_base,E8390_CMD);
        AX_REG_WR(ENTBR_ENABLE, ax_base, EN3_TBR);
        AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_STOP, ax_base,E8390_CMD);

    #ifdef AX_MII_SUPPORT
        if(ax_local->media != ax_local->media_curr)
            ax88796_PHY_init (ax_local);
    #endif

        AX_REG_WR(0xff,  ax_base ,EN0_ISR);
        AX_REG_WR(ENISR_ALL,  ax_base, EN0_IMR);
        AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_START, ax_base, E8390_CMD);
        AX_REG_WR(E8390_TXCONFIG, ax_base, EN0_TXCR);                   /* xmit on.                     */

        AX_REG_WR(E8390_RXCONFIG, ax_base, EN0_RXCR);                   /* rx on,                       */
        //do_set_multicast_list (dev);                                    /* (re)load the mcast table     */

        /* Enable system irq */
        ax_sys_irq_enable(ax_local);
    }

    CPU_UNLOCK(cpsr_c);
}

/*********************************************************************************************************
** Function name:           axBusInit
** Descriptions:            配置CPU总线控制接口参数
**                          IRQ PIN: GPN11  EINT11
**                          RST PIN: GPE3
** input parameters:        NONE
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void axBusInit(void)
{

    rSROM_BW     &=  ~(0xf << 4);                       /* clear setting                */
    rSROM_BW     |=  0x1 << 4;                          /* 16 bit                       */

    rSROM_BC1    =  (SROM_BAND1_Tacs << 28) |
                    (SROM_BAND1_Tcos << 24) |
                    (SROM_BAND1_Tacc << 16) |
                    (SROM_BAND1_Tcoh << 12) |
                    (SROM_BAND1_Tcah << 8)  |
                    (SROM_BAND1_Tacp << 4);             /* SROM Timing config           */
}

/*********************************************************************************************************
** Function name:           ax_hw_init
** Descriptions:            通过配置AX88796B管脚进行硬初始化，并初始化中断管脚和中断方式
** input parameters:        NONE
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_hw_init(void)
{
    unsigned int    uiReg = 0;

    axBusInit();

    uiReg       =       rGPECON;                                        /* config RST_LAN/CTL_LAN GPE3/4*/
    uiReg       &=      ~((0xfu  <<  12) | (0xfu    <<  16));
    uiReg       |=      (0x1u    <<  12) | (0x1u    <<  16);
    rGPECON     =       uiReg;
    uiReg       =       rGPEPUD;                                        /* config RST_LAN GPE3/4 PULLUP */
    uiReg       &=      ~((0x3u  <<  6)  | (0x3u    <<  8));
    uiReg       |=      (0x2u    <<  6)  | (0x2u    <<  8);
    rGPEPUD     =       uiReg;

    rGPEDAT     |=      (0x1u <<  3) | (0x1u <<  4);                    /* config CTL_LAN HIGH          */

    uiReg       =       rGPNCON;                                        /* config IRQ_LAN GPN11         */
    uiReg       &=      ~(0x3u   <<  22);
    uiReg       |=      (0x2u    <<  22);
    rGPNCON     =       uiReg;
    uiReg       =       rGPNPUD;
    uiReg       &=      ~(0x3u   <<  22);
    uiReg       |=      (0x2u    <<  22);
    rGPNPUD     =       uiReg;                                          /* config to PULL UP            */
    rGPNDAT     |=      0x1u <<  11;                                    /* config IRQ_LAN HIGH          */

    rGPEDAT     &=      ~(0x1u <<  3);
    delayQ_delay(10);                                                   /* Delay 20 ms wait AX88796     */
    rGPEDAT     |=      (0x1u  <<  3);

    rEINT0CON0   &=  ~(0x7   <<  20);                    /* Set to Low Level IRQ         */
    rEINT0PEND   =   (0x1u   <<  11);                    /* Clear Pending INT            */
    rEINT0MASK   &=  ~(0x1u  <<  11);                    /* Enable EINT11 FIXME ???      */

    /* Wait 50ms for device ready */
    delayQ_delay(5);
}

/*********************************************************************************************************
** Function name:           ax_reset
** Descriptions:            通过配置AX88796B寄存器进行软复位
** input parameters:        ax_local       设备驱动程序数据结构
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_reset(AX_DEVICE *ax_local)
{
    ulong  ax_base   = ax_local->membase;
    ulong  retry_cnt = 10;
    uint32 dummy;                                                        /* delete waring while compling */

    dummy = AX_REG_RD(ax_base, EN0_RESET);

    /* This check _should_not_ be necessary, omit eventually. */
    while ((AX_REG_RD (ax_base, EN0_ISR) & ENISR_RESET) == 0)
    {
        delayQ_delay(1);
        if (retry_cnt == 0) {
            intDebugInfo (KERN_WARNING "%s: ax_reset() did not complete.\n", ax_local->name);
            break;
        }
        retry_cnt--;
    }

    AX_REG_WR (ENISR_RESET, ax_base, EN0_ISR);                            /* Ack intr.                    */
}
/*********************************************************************************************************
** Function name:           ax_dev_init
** Descriptions:            初始化硬件设备控制数据结构
** input parameters:        ax_local       设备驱动程序数据结构
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_dev_init(AX_DEVICE *ax_local)
{
    AX_DEV_CONFIG *pAXDevConfig = ax_get_free_dev();

    if(pAXDevConfig == NULL)
        return ERR_USE;

    ax_local->name          = DRV_NAME;
    ax_local->membase       = pAXDevConfig->membase;
    ax_local->ulChannel     = pAXDevConfig->ulChannel;
    ax_local->ulPrio        = pAXDevConfig->ulPrio;

    pAXDevConfig->uiUsed    = TRUE;                                     /* 设备已被占用                 */

    ax_local->tx_start_page = NESM_START_PG;
    ax_local->rx_start_page = NESM_RX_START_PG;
    ax_local->stop_page     = NESM_STOP_PG;
    ax_local->media         = MEDIA_AUTO;
    ax_local->media_curr    = MEDIA_AUTO;
    ax_local->rx_handling   = 0;
    ax_local->irqlock       = 0;

    ax_local->imask         = ENISR_ALL;

    ax_local->ucHWFlowEn    = 0;                                        /* 默认关闭硬件流量控制         */

#if (AX_8BIT_MODE == 1)
    ax_local->bus_width = 0;
#else
    ax_local->bus_width = 1;
#endif

/* XXX */
    ax_local->dev_sem  = semM_init(NULL);
    ax_local->fifo_sem = semM_init(NULL);
/* XXX */

    /*
     * HW init...VITAL, make sure the HW init is successful
     */
    pAXDevConfig->pHWInitFunc();


    /* Reset AX88796 registers */
    ax_reset(ax_local);

    /* Wait 10ms for device ready */
    delayQ_delay(1);

    return ERR_OK;
}
/*********************************************************************************************************
** Function name:           ax_input
** Descriptions:            数据接收函数
** input parameters:        ax_local          设备驱动程序数据结构
** output parameters:
** Returned value:          ERR_OK      成功返回
**                          ERR_MEM     内存错误
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:             XuGuizhou
** Modified date:           2009-06-10
** Descriptions:            增加出错返回点，集中处理出错返回
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_input(AX_DEVICE *ax_local)
{
    struct netif *netif = ax_local->netif;
    ulong      ax_base  = ax_local->membase;
    uint8      rxing_page;
    uint8      this_frame;
    uint8      next_frame;
    uint16     current_offset;

    struct ax_pkt_hdr rx_frame;
    int num_rx_pages = ax_local->stop_page - ax_local->rx_start_page;

    /* int cpsr_c; */

    while (ax_local->rx_handling)
    {
        int pkt_len, pkt_stat;

        /* cpsr_c = CPU_LOCK(); */
        ax_sys_irq_disable(ax_local);

        /* Get the rx page (incoming packet pointer). */
        AX_REG_WR(E8390_NODMA+E8390_PAGE1, ax_base, E8390_CMD);
        rxing_page = AX_REG_RD (ax_base, EN1_CURPAG);
        AX_REG_WR(E8390_NODMA+E8390_PAGE0, ax_base, E8390_CMD);

        /* Remove one frame from the ring.  Boundary is always a page behind. */
        this_frame = (uint8) (AX_REG_RD (ax_base, EN0_BOUNDARY) + 1);

        /* CPU_UNLOCK(cpsr_c); */
        ax_sys_irq_enable(ax_local);

        if (this_frame >= ax_local->stop_page)
            this_frame = ax_local->rx_start_page;

        if (this_frame != ax_local->current_page && (this_frame!=0x0 || rxing_page!=0xFF))
        {
            intDebugInfo(KERN_WARNING "ax_input mismatched read page pointers\n");
            goto AX_INPUT_ERROR;                                        /* Error out                    */
        }

        if (this_frame == rxing_page) {                                    /* Read all the frames?         */
            break;                                                        /* Done for now                 */
        }
        current_offset = (uint16) (this_frame << 8);

        if(ax_get_hdr (ax_local, &rx_frame, this_frame) != ERR_OK)
            goto AX_INPUT_ERROR;                                        /* Error out                    */

        pkt_len = rx_frame.count - sizeof (struct ax_pkt_hdr);
        pkt_stat = rx_frame.status;
        next_frame = (uint8) (this_frame + 1 + ((pkt_len+4)>>8));

        /* Check for bogosity warned by 3c503 book: the status byte is never
           written.  This happened a lot during testing! This code should be
           cleaned up someday. */
        if (rx_frame.next != next_frame
            && rx_frame.next != next_frame + 1
            && rx_frame.next != next_frame - num_rx_pages
            && rx_frame.next != next_frame + 1 - num_rx_pages) {
            ax_local->current_page = rxing_page;
            AX_REG_WR(ax_local->current_page-1, ax_base, EN0_BOUNDARY);
        #ifdef NET_STAT
            ax_local->stat.rx_errors++;
        #endif

        #ifdef LWIP_LINK_STAT
            AX_LINK.drop ++;
        #endif

        #ifdef LINK_SNMP_STAT
            snmp_inc_ifindiscards(netif);
        #endif
            continue;
        }

        if (pkt_len < 60  ||  pkt_len > 1518)
        {
        #ifdef NET_STAT
            ax_local->stat.rx_errors++;
            ax_local->stat.rx_length_errors++;
        #endif

        #ifdef LWIP_LINK_STAT
            AX_LINK.drop ++;
            AX_LINK.lenerr ++;
        #endif

        #ifdef LINK_SNMP_STAT
            snmp_inc_ifindiscards(netif);
        #endif
        }
        else if ((pkt_stat & 0x0F) == ENRSR_RXOK)
        {
            struct pbuf *p;

            /* Request for a net buf, copy data in dev buf into net buf */
            #if ETH_PAD_SIZE
                  pkt_len += ETH_PAD_SIZE;                                /* Allow room for Ethernet padding */
            #endif
            p = pbuf_alloc(PBUF_RAW, (uint16)pkt_len, PBUF_POOL);
            if(!p)
            {
                intDebugInfo(KERN_WARNING "ax_input CAN NOT alloc pbuf\n");
                /* enable device irq */
                ax_sys_irq_disable(ax_local);
                ax_local->rx_handling = 0;
                ax_local->imask      |= ENISR_RX + ENISR_RX_ERR;
                AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
                ax_sys_irq_enable(ax_local);

                return ERR_MEM;
            }

            /* Get packet data from AX88796 buffer */
            if(ax_block_input (ax_local, pkt_len, p, current_offset + sizeof (rx_frame)) == ERR_OK)
            {
                netif->input(p, netif);

            #ifdef NET_STAT
                ax_local->stat.rx_packets++;
                ax_local->stat.rx_bytes += pkt_len;

                if (pkt_stat & ENRSR_PHY)
                    ax_local->stat.multicast++;
            #endif

            #ifdef LWIP_LINK_STAT
                AX_LINK.recv ++;
            #endif

            #ifdef LINK_SNMP_STAT
                snmp_add_ifinoctets(netif, pkt_len);

                if (pkt_stat & ENRSR_PHY)
                {
                    snmp_inc_ifinnucastpkts(netif);
                }
                else
                    snmp_inc_ifinucastpkts(netif);
            #endif
            }
            else {
                pbuf_free(p);

            #ifdef NET_STAT
                ax_local->stat.rx_errors++;
                /* NB: The NIC counts CRC, frame and missed errors. */
                if (pkt_stat & ENRSR_FO)
                    ax_local->stat.rx_fifo_errors++;
            #endif

            #ifdef LWIP_LINK_STAT
                AX_LINK.drop ++;
                if (pkt_stat & ENRSR_FO)
                    AX_LINK.err ++;
            #endif

            #ifdef LINK_SNMP_STAT
                snmp_inc_ifindiscards(netif);
            #endif

                goto AX_INPUT_ERROR;                                    /* Error out                    */
            }
        }
        else
        {
        #ifdef NET_STAT
            ax_local->stat.rx_errors++;
            /* NB: The NIC counts CRC, frame and missed errors. */
            if (pkt_stat & ENRSR_FO)
                ax_local->stat.rx_fifo_errors++;
        #endif

        #ifdef LWIP_LINK_STAT
            AX_LINK.drop ++;
            if (pkt_stat & ENRSR_FO)
                AX_LINK.err ++;
        #endif

        #ifdef LINK_SNMP_STAT
            snmp_inc_ifindiscards(netif);
        #endif
        }
        next_frame = rx_frame.next;

        /* This _should_ never happen: it's here for avoiding bad clones. */
        if (next_frame >= ax_local->stop_page) {
            intDebugInfo(KERN_ERR "%s: next frame inconsistency, %#2x\n", ax_local->name, next_frame);
            /*next_frame = ax_local->rx_start_page;*/
            goto AX_INPUT_ERROR;                                        /* Error out                    */
        }
        /* cpsr_c = CPU_LOCK(); */
        ax_sys_irq_disable(ax_local);
        ax_local->current_page = next_frame;
        AX_REG_WR(next_frame-1, ax_base, EN0_BOUNDARY);
        /* CPU_UNLOCK(cpsr_c); */
        ax_sys_irq_enable(ax_local);
    }

    /* enable device irq */
    ax_sys_irq_disable(ax_local);
    /* cpsr_c = CPU_LOCK(); */
    ax_local->rx_handling = 0;
    ax_local->imask |= ENISR_RX + ENISR_RX_ERR;
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
    /* CPU_UNLOCK(cpsr_c); */
    ax_sys_irq_enable(ax_local);
    return ERR_OK;

AX_INPUT_ERROR:
    /* enable device irq */
    ax_sys_irq_disable(ax_local);
    /* cpsr_c = CPU_LOCK(); */
    ax_local->rx_handling = 0;
    /* CPU_UNLOCK(cpsr_c); */
    ax_sys_irq_enable(ax_local);

    ax_restart(ax_local);
    return ERR_RST;
}
/*********************************************************************************************************
** Function name:           ax_get_hdr
** Descriptions:            读取网卡设备内部缓存中的数据包头
** input parameters:        ax_local          设备驱动程序数据结构
** output parameters:
** Returned value:          ERR_OK      成功返回
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:             XuGuizhou
** Modified date:           2009/06/10
** Descriptions:            超时退出不重新启动AX88796，重启动作返回到ax_input之后做
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_get_hdr (AX_DEVICE *ax_local, struct ax_pkt_hdr *hdr, int ring_page)
{
    ulong      ax_base  = ax_local->membase;
    uint16    *buf      = (uint16 *)hdr;
    int        iRetry   = DMA_TIMEOUT_CNT;

    /* Get FIFO semaphore */
    //serial_printf("Task: %8X Take SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
    semM_take (ax_local->fifo_sem, WAIT_FOREVER);
    //serial_printf("after take.\n");

    /* Disable Device IRQ */
    AX_REG_WR(0x00, ax_base, EN0_IMR);

    AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_START, ax_base,  E8390_CMD);
    AX_REG_WR(sizeof (struct ax_pkt_hdr), ax_base, EN0_RCNTLO);
    AX_REG_WR(0, ax_base, EN0_RCNTHI);
    AX_REG_WR(0, ax_base, EN0_RSARLO);        /* On page boundary */
    AX_REG_WR(ring_page, ax_base, EN0_RSARHI);
    AX_REG_WR(E8390_RREAD+E8390_START, ax_base, E8390_CMD);

    /* Wait for DMA complete. if timeout, return error */
    while (( AX_REG_RD (ax_base, EN0_SR) & ENSR_DMA_READY) == 0)
    {
        iRetry--;
        if(iRetry < 0)
        {
            intDebugInfo(KERN_WARNING "ax_get_hdr timeout.Device not ready for DMA\n");
            /* Release FIFO semaphore */
            //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
            semM_give(ax_local->fifo_sem);
            //serial_printf("AFTER GIVE.\n");
            return ERR_TIMEOUT;
        }
    }

    *buf     = READ_FIFO (ax_base, EN0_DATAPORT);
    *(++buf) = READ_FIFO (ax_base, EN0_DATAPORT);

    AX_REG_WR(ENISR_RDC, ax_base, EN0_ISR);    /* Ack intr. */

    /* hack, BIG Endian should change byte order into host format */
    //hdr->count = ntohs(hdr->count);

    /* Enable Device IRQ */
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);

    /* Release FIFO semaphore */
    //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
    semM_give(ax_local->fifo_sem);
    //serial_printf("AFTER GIVE.\n");

    return ERR_OK;

}
/*********************************************************************************************************
** Function name:           pbufToSendbuf
** Descriptions:            读取网卡设备内部缓存数据，复制到设备控制数据结构的缓存中
** input parameters:        ax_local       设备驱动程序数据结构
**                          p              协议栈的缓存指针
** output parameters:
** Returned value:          ERR_OK      成功返回
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:             XuGuizhou
** Modified date:           2009-06-03
** Descriptions:            修改pbufToSendbuf函数，修正pbuf中payload指针不是16位对齐时引起data abort问题
**
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
err_t pbufToSendbuf(AX_DEVICE *ax_local, struct pbuf *p)
{
    struct pbuf *q;
    uint32       i;

    uint8  *buf;
    uint8  *psendbuf = (uint8 *)ax_local->ucPktData;

    uint16 *pShortBuf;
    uint16 *pShortSendBuf = (uint16 *)ax_local->ucPktData;

    uint32  uiNotAlign = 0;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);            /* drop the padding word */
#endif

    for(q = p; q != NULL; q = q->next) {
        buf = (uint8 *)q->payload;

        if((ulong)buf & 0x1)                                            /* addr is NOT 16bit align      */
        {
            uiNotAlign = 1;                                             /* Set not aligned flag         */
        }

        if(uiNotAlign)
        {
            for (i = 0; i < q->len; i++)                                /* Byte copy                    */
            {
                *psendbuf++ = *buf++;
            }
            intDebugInfo(KERN_NOTICE "pbuf address NOT aligned\n");
        }
        else                                                            /* Short copy                   */
        {
            pShortBuf = (uint16 *) buf;

            for (i = 0; i < q->len/2; i++)
            {
                *pShortSendBuf++ = *pShortBuf++;
            }

            if(q->len & 0x1)
            {
                *(uint8 *)pShortSendBuf = *(uint8 *)pShortBuf;
                uiNotAlign = 1;                                         /* Set not aligned flag         */
            }
            psendbuf += q->len;
        }
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);            /* reclaim the padding word */
#endif

    return ERR_OK;
}

/*********************************************************************************************************
** Function name:           ax_link_output
** Descriptions:            将发送数据复制到网卡设备内部缓存数据，并激活设备将数据发送
** input parameters:        netif          网络设备指针
**                          p              协议栈的缓存指针
** output parameters:
** Returned value:          ERR_OK      成功返回
**                          ERR_MEM     内存错误
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:             XuGuizhou
** Modified date:           2009-06-10
** Descriptions:            增加出错返回点，集中处理出错返回
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/

err_t ax_link_output(struct netif *netif, struct pbuf *p)
{
    AX_DEVICE *ax_local = (AX_DEVICE *)netif->state;
    ulong      ax_base  = ax_local->membase;
    uint16 send_length;
    uint8 ctepr      = 0;
    uint8 free_pages = 0;
    uint8 need_pages;

    /* check for link status */
    if (!(AX_REG_RD (ax_base, EN0_SR) & ENSR_LINK)) {
        return 0;
    }

    /* Get device semaphore */
    semM_take (ax_local->dev_sem, WAIT_FOREVER);

    /* Add irq disable & enable */
    ax_sys_irq_disable(ax_local);
    ax_local->imask &= ~(ENISR_TX+ ENISR_TX_ERR);
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
    ax_sys_irq_enable(ax_local);

    send_length = p->tot_len;
#if ETH_PAD_SIZE
    send_length -= ETH_PAD_SIZE;
#endif
    if(send_length < ETH_ZLEN)
        send_length = ETH_ZLEN;

    need_pages = (uint8) ((send_length -1)/256 +1);

    ctepr = (uint8) (AX_REG_RD (ax_base, EN0_CTEPR) & 0x7f);

    if (ctepr == 0) {
        if (ax_local->tx_curr_page == ax_local->tx_start_page && ax_local->tx_prev_ctepr == 0)
            free_pages = TX_PAGES;
        else
            free_pages = (uint8) (ax_local->tx_stop_page - ax_local->tx_curr_page);
    }
    else if (ctepr < ax_local->tx_curr_page - 1) {
        free_pages = (uint8) (ax_local->tx_stop_page - ax_local->tx_curr_page +
                     ctepr - ax_local->tx_start_page + 1);
    }
    else if (ctepr > ax_local->tx_curr_page - 1) {
        free_pages = (uint8) (ctepr + 1 - ax_local->tx_curr_page);
    }
    else if (ctepr == ax_local->tx_curr_page - 1) {
        if (ax_local->tx_full)
            free_pages = 0;
        else
            free_pages = TX_PAGES;
    }



    if (free_pages < need_pages) {
        intDebugInfo(KERN_NOTICE "free_pages < need_pages\n");
        /* AX88796 send buffer is full, set buffer full flag */
        ax_local->tx_full = 1;
        ax_local->irqlock = 0;

        /* release dev sem & enable dev irq */
        ax_sys_irq_disable(ax_local);
        ax_local->imask |= (ENISR_TX+ ENISR_TX_ERR);
        AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
        ax_sys_irq_enable(ax_local);
        semM_give(ax_local->dev_sem);

        return ERR_MEM;
    }

#ifdef OPTIMZE_DISABLE
    if(pbufToSendbuf(ax_local, p) != ERR_OK)
    {
        ax_local->irqlock = 0;

        /* release dev sem & enable dev irq */
        ax_sys_irq_disable(ax_local);
        ax_local->imask |= (ENISR_TX+ ENISR_TX_ERR);
        AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
        ax_sys_irq_enable(ax_local);
        semM_give(ax_local->dev_sem);
        return ERR_MEM;
    }
#endif

    /* copy data to dev buf, trigger dev send the data out */
    if(ax_block_output (ax_local, send_length, p, ax_local->tx_curr_page) != ERR_OK)
    {
    #ifdef LWIP_LINK_STAT
        AX_LINK.err ++;
    #endif

        goto AX_LINK_ERR_OUT;                                           /* Error out                    */
    }

    ax_trigger_send (ax_local, send_length, ax_local->tx_curr_page);

    if (free_pages == need_pages) {
        /* AX88796 send buffer is full, set buffer full flag */
        ax_local->tx_full = 1;
    }
    ax_local->tx_prev_ctepr = ctepr;
    ax_local->tx_curr_page = (uint8) ( ax_local->tx_curr_page + need_pages < ax_local->tx_stop_page ?
            ax_local->tx_curr_page + need_pages :
            need_pages - (ax_local->tx_stop_page - ax_local->tx_curr_page) + ax_local->tx_start_page );


#ifdef NET_STAT
    ax_local->stat.tx_bytes += send_length;
#endif

#ifdef LINK_SNMP_STAT
    snmp_add_ifoutoctets(netif, send_length);

    if(ax_local->ucPktData[0] & 0x1)
    {
        snmp_inc_ifoutnucastpkts(netif);
    }
    else
    {
        snmp_inc_ifoutucastpkts(netif);
    }
#endif

    ax_local->irqlock = 0;


    /* Release dev sem & enable dev irq */
    ax_sys_irq_disable(ax_local);
    ax_local->imask |= (ENISR_TX+ ENISR_TX_ERR);
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
    ax_sys_irq_enable(ax_local);

    semM_give(ax_local->dev_sem);

    return ERR_OK;

AX_LINK_ERR_OUT:
    /* 发送出错，重置设备状态 */
    ax_restart(ax_local);
    semM_give(ax_local->dev_sem);
    intDebugInfo(KERN_WARNING "ax_link_output error\n");
    return ERR_RST;
}

/*********************************************************************************************************
** Function name:           ax_block_input
** Descriptions:            复制网卡设备内部缓存数据到协议栈缓存
** input parameters:        ax_local       设备驱动程序数据结构
**                          p              协议栈的缓存指针
** output parameters:
** Returned value:          ERR_OK      成功返回
**                          ERR_MEM     内存错误
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_block_input (AX_DEVICE *ax_local, int count, struct pbuf *p, int ring_offset)
{
    ulong      ax_base  = ax_local->membase;

    struct pbuf *q;

    uint16 *buf;
    uint16 i;
    int    iRetry = DMA_TIMEOUT_CNT;

    /* Get FIFO semaphore */
    //serial_printf("Task: %8X Take SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
    semM_take (ax_local->fifo_sem, WAIT_FOREVER);
    //serial_printf("after take.\n");

    /* Disable Device IRQ */
    AX_REG_WR(0x00, ax_base, EN0_IMR);
    ax_sys_irq_disable(ax_local);

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);            /* drop the padding word */
#endif

    AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_START, ax_base, E8390_CMD);
    AX_REG_WR(count & 0xff, ax_base, EN0_RCNTLO);
    AX_REG_WR(count >> 8, ax_base, EN0_RCNTHI);
    AX_REG_WR(ring_offset & 0xff, ax_base, EN0_RSARLO);
    AX_REG_WR(ring_offset >> 8, ax_base, EN0_RSARHI);
    AX_REG_WR(E8390_RREAD+E8390_START, ax_base, E8390_CMD);

    /* 等待设备就绪 */
    while (( AX_REG_RD (ax_base, EN0_SR) & ENSR_DMA_READY) == 0)
    {
        iRetry--;
        if(iRetry < 0)
        {
            intDebugInfo(KERN_WARNING "ax_block_input timeout.Device not ready for DMA\n");

        #ifdef LWIP_LINK_STAT
            AX_LINK.drop ++;
        #endif

            /* Release FIFO semaphore */
            //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
            semM_give(ax_local->fifo_sem);
            //serial_printf("AFTER GIVE.\n");
            return ERR_TIMEOUT;
        }
    }

    for(q = p; q != NULL; q = q->next) {
        buf = (uint16 *)q->payload;
        for (i = 0; i < q->len; i += 2) {
            *buf++ = READ_FIFO (ax_base, EN0_DATAPORT);
        }
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);            /* reclaim the padding word */
#endif

    AX_REG_WR(ENISR_RDC, ax_base, EN0_ISR);    /* Ack intr. */

    /* Enable Device IRQ */
    ax_sys_irq_enable(ax_local);
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);

    /* Release FIFO semaphore */
    //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
    semM_give(ax_local->fifo_sem);
    //serial_printf("AFTER GIVE.\n");

    return ERR_OK;
}
/*********************************************************************************************************
** Function name:           ax_block_output
** Descriptions:            将发送的数据复制到网卡设备内部缓存
** input parameters:        ax_local       设备驱动程序数据结构
**                          count          发送数据的总字节数
**                          p              协议栈的缓存指针
**                          start_page     指向网卡设备内部发送缓存起始地址
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_block_output (AX_DEVICE *ax_local, uint16 count, struct pbuf *p, const int start_page)
{
    ulong      ax_base  = ax_local->membase;
    ulong      dma_start;

    uint16     *buf;
    uint32     i;

#ifndef OPTIMZE_DISABLE
    struct pbuf *q;
#endif

    /* Get FIFO semaphore */
    //serial_printf("Task: %8X Take SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
    semM_take (ax_local->fifo_sem, WAIT_FOREVER);
    //serial_printf("after take.\n");

    /* Disable Device IRQ */
    AX_REG_WR(0x00, ax_base, EN0_IMR);
    ax_sys_irq_disable(ax_local);

    /* We should already be in page 0, but to be safe... */
    AX_REG_WR(E8390_PAGE0+E8390_START+E8390_NODMA, ax_base, E8390_CMD);
    AX_REG_WR(ENISR_RDC, ax_base, EN0_ISR);
    /* Now the normal output. */
    AX_REG_WR(count & 0xff, ax_base, EN0_RCNTLO);
    AX_REG_WR(count >> 8,   ax_base, EN0_RCNTHI);
    AX_REG_WR(0x00, ax_base, EN0_RSARLO);
    AX_REG_WR(start_page, ax_base, EN0_RSARHI);
    AX_REG_WR(E8390_RWRITE+E8390_START, ax_base, E8390_CMD);

    buf = (uint16 *)ax_local->ucPktData;

    for (i = 0; i < count; i += 2, buf++) {
        WRITE_FIFO (ax_base , EN0_DATAPORT, *buf);
    }

    /* 等待数据传输完成，20ms后超时退出 */
    dma_start = tick_get();
    while ((AX_REG_RD(ax_base , EN0_ISR) & 0x40) == 0) {
        if (tick_get() - dma_start > 2 * SYS_CLK_RATE / 100) {    /* 20ms                         */
            intDebugInfo(KERN_WARNING "%s: timeout waiting for Tx RDC.\n", ax_local->name);
            /* Release FIFO semaphore */
            //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
            semM_give(ax_local->fifo_sem);
            //serial_printf("AFTER GIVE.\n");
            return ERR_TIMEOUT;
        }
    }

    AX_REG_WR(ENISR_RDC, ax_base, EN0_ISR);                                /* Ack intr.                    */

    /* Enable Device IRQ */
    ax_sys_irq_enable(ax_local);
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);


    /* Release FIFO semaphore */
    //serial_printf("Task: %8X Give SEM_M: %8X ... ", G_p_current_tcb, ax_local->fifo_sem);
    semM_give(ax_local->fifo_sem);
    //serial_printf("AFTER GIVE.\n");

    return ERR_OK;
}
/*********************************************************************************************************
** Function name:           ax_trigger_send
** Descriptions:            激活设备，将数据发送出去
** input parameters:        ax_local       设备驱动程序数据结构
**                          length         发送数据的总字节数
**                          start_page     指向网卡设备内部发送缓存起始地址
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_trigger_send (AX_DEVICE *ax_local, unsigned int length, int start_page)
{
    ulong      ax_base  = ax_local->membase;
    AX_REG_WR(E8390_NODMA+E8390_PAGE0, ax_base, E8390_CMD);
    AX_REG_WR(length & 0xff, ax_base, EN0_TCNTLO);
    AX_REG_WR(length >> 8, ax_base, EN0_TCNTHI);
    AX_REG_WR(start_page, ax_base, EN0_TPSR);
    AX_REG_WR((E8390_NODMA|E8390_TRANS|E8390_START), ax_base, E8390_CMD);
}

/*********************************************************************************************************
** Function name:           ax_interrupt
** Descriptions:            中断处理函数，处理设备各种中断
** input parameters:        NONE
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_interrupt (AX_DEVICE *ax_local)
{
    int interrupts;

    ulong ax_base;

    if((rEINT0PEND & (1 << 11)) == 0) {
        return;
    }

    if (ax_local == NULL)
    {
        intDebugInfo(KERN_WARNING "in interrupt, no ax_local\n");
    	goto CLEAR_IRQ;
    }

    ax_base = ax_local->membase;

    AX_REG_WR(E8390_NODMA+E8390_PAGE0, ax_base, E8390_CMD);

    if ((interrupts = AX_REG_RD (ax_base, EN0_ISR)) == 0) {
    	goto CLEAR_IRQ;
    }
    

    /* Disable AX88796 interrupt */
    AX_REG_WR(0x00, ax_base, EN0_IMR);

    interrupts &= ax_local->imask;

    AX_REG_WR(interrupts, ax_base, EN0_ISR);                        /* Ack the interrupts           */

    if (interrupts & ENISR_TX)
        ax_tx_intr (ax_local);

    if (interrupts & ENISR_OVER) {
        /*
         * Record whether a Tx was in progress and then issue the
         * stop command.
         */
        uint32 was_txing;

        was_txing = AX_REG_RD (ax_base, E8390_CMD) & E8390_TRANS;
        AX_REG_WR(E8390_NODMA+E8390_PAGE0+E8390_STOP, ax_base, E8390_CMD);
        if(net_job_add((EXE_FUNC_PTR)ax_rx_overrun, (uint32)ax_local, was_txing) == OS_STATUS_ERROR)
        {
            /* SHOULD NOT come here. if it does, Driver will hang up */
            intDebugInfo(KERN_ERR "net_job_add() ax_rx_overrun fail\n");
        }

        ax_local->rx_handling = 1;                                      /* set up rx_handling flag      */

        goto CLEAR_IRQ;                                                 /* return immediately           */
        
    }

    if (interrupts & (ENISR_RX+ENISR_RX_ERR) && ax_local->rx_handling == 0)    {
        ax_local->rx_handling = 1;                                      /* set up rx_handling flag      */
        ax_local->imask &= ~(ENISR_RX+ENISR_RX_ERR);                    /* Disable RX interrupt         */
        AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);

        if(net_job_add((EXE_FUNC_PTR)ax_input, (uint32)ax_local, 0) == OS_STATUS_ERROR)
        {
            intDebugInfo(KERN_ERR "netJobAdd ax_input fail\n");
            ax_local->rx_handling = 0;
            ax_local->imask |= (ENISR_RX+ENISR_RX_ERR);
            AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
        }
    }

    if (interrupts & ENISR_TX_ERR)
        ax_tx_err (ax_local);

    if (interrupts & ENISR_COUNTERS)
    {
    #ifdef NET_STAT
        ax_local->stat.rx_frame_errors += AX_REG_RD (ax_base, EN0_COUNTER0);
        ax_local->stat.rx_crc_errors   += AX_REG_RD (ax_base, EN0_COUNTER1);
        ax_local->stat.rx_missed_errors+= AX_REG_RD (ax_base, EN0_COUNTER2);
    #endif

        AX_REG_WR(ENISR_COUNTERS, ax_base, EN0_ISR);                /* Ack intr.                    */
    }

    if (interrupts & ENISR_RDC)
        AX_REG_WR(ENISR_RDC, ax_base, EN0_ISR);

    interrupts = AX_REG_RD (ax_base, EN0_ISR);

    /* ReEnable AX88796 interrupt */
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);

CLEAR_IRQ:
    rEINT0PEND  =   1   <<  11;                          /* Clear Pending External INT11 */
}

/*********************************************************************************************************
** Function name:           ax_tx_intr
** Descriptions:            发送中断处理函数
** input parameters:        ax_local       设备驱动程序数据结构
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_tx_intr (AX_DEVICE *ax_local)
{
#if defined(NET_STAT) || defined(LWIP_LINK_STAT)
    ulong ax_base = ax_local->membase;
    int status = AX_REG_RD (ax_base, EN0_TSR);
#endif

    /* AX88796 send buffer is NOT full now */
    ax_local->tx_full = 0;

    /* Minimize Tx latency: update the statistics after we restart TXing. */
#ifdef NET_STAT
    if (status & ENTSR_COL)
        ax_local->stat.collisions++;
    if (status & ENTSR_PTX)
        ax_local->stat.tx_packets++;
    else
    {
        ax_local->stat.tx_errors++;
        if (status & ENTSR_ABT)
        {
            ax_local->stat.tx_aborted_errors++;
            ax_local->stat.collisions += 16;
        }
        if (status & ENTSR_CRS)
            ax_local->stat.tx_carrier_errors++;
        if (status & ENTSR_FU)
            ax_local->stat.tx_fifo_errors++;
        if (status & ENTSR_CDH)
            ax_local->stat.tx_heartbeat_errors++;
        if (status & ENTSR_OWC)
            ax_local->stat.tx_window_errors++;
    }
#endif

#ifdef LWIP_LINK_STAT
    if (status & ENTSR_PTX)
        AX_LINK.xmit ++;
#endif
}

/*********************************************************************************************************
** Function name:           ax_rx_overrun
** Descriptions:            接收缓冲区溢出中断处理
** input parameters:        ax_local       设备驱动程序数据结构
**                          was_txing      正在发送数据标识
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_rx_overrun (AX_DEVICE *ax_local, uint8 was_txing)
{
    ulong ax_base = ax_local->membase;

    uint8 must_resend = 0;

    intDebugInfo(KERN_WARNING "%s: Receiver overrun.\n", ax_local->name);
#ifdef NET_STAT
    ax_local->stat.rx_over_errors++;
#endif

#ifdef LWIP_LINK_STAT
    AX_LINK.err ++;
#endif

    AX_REG_WR(0x00, ax_base, EN0_RCNTLO);
    AX_REG_WR(0x00, ax_base, EN0_RCNTHI);

    /*
     * See if any Tx was interrupted or not. According to NS, this
     * step is vital, and skipping it will cause no end of havoc.
     */

    if (was_txing)
    {
        uint8 tx_completed = (uint8) (AX_REG_RD (ax_base, EN0_ISR) & (ENISR_TX+ENISR_TX_ERR));
        if (!tx_completed)
            must_resend = 1;
    }

    /*
     * Have to enter loopback mode and then restart the NIC before
     * you are allowed to slurp packets up off the ring.
     */
    AX_REG_WR(E8390_TXOFF, ax_base, EN0_TXCR);
    AX_REG_WR(E8390_NODMA + E8390_PAGE0 + E8390_START, ax_base, E8390_CMD);

    /* no need to set rx_handling flag,
     * the device is already overrun
     * ax_local->rx_handling = 1;
     */

    /*
     * Clear the Rx ring of all the debris, and ack the interrupt.
     */
    ax_input (ax_local);

    /*
     * Leave loopback mode, and resend any packet that got stopped.
     */
    AX_REG_WR(E8390_TXCONFIG, ax_base, EN0_TXCR);
    if (must_resend)
            AX_REG_WR(E8390_NODMA + E8390_PAGE0 + E8390_START + E8390_TRANS, ax_base, E8390_CMD);

    /* ReEnable AX88796 interrupt */
    AX_REG_WR(ax_local->imask, ax_base, EN0_IMR);
}

/*********************************************************************************************************
** Function name:           ax_restart
** Descriptions:            重新初始化和启动设备
** input parameters:        ax_local       设备驱动程序数据结构
**
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/06/10
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_restart (AX_DEVICE *ax_local)
{
    ax_reset(ax_local);
    ax_init(ax_local, 1);
}

/*********************************************************************************************************
** Function name:           ax_close
** Descriptions:            关闭AX88796B网络设备，并释放为其分配的内存
** input parameters:        ax_local       设备驱动程序数据结构
**                          was_txing      正在发送数据标识
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void ax_close(AX_DEVICE *ax_local)
{
    struct netif *netif = ax_local->netif;

    netif_set_down(netif);

    /* init ax dev according to default configuration */
    ax_init(ax_local, 0);

    /* connect system irq */
    ax_sys_irq_disconnect(ax_local);

    /* free memory of device driver */
    free(ax_local);

    /* clear device in netif */
    netif->state = NULL;
}


/*
 *    Collect the stats. This is called unlocked and from several contexts.
 */
#ifdef NET_STAT
static struct net_device_stats *get_stats (AX_DEVICE *ax_local)
{
    struct netif *netif   = ax_local->netif;
    ulong         ax_base = ax_local->membase;

    ulong         flags;

    /* If the card is stopped, just return the present stats. */
    if (!netif_running (netif))
        return &ax_local->stat;

    ax_sys_irq_disable(ax_local);
    /* Read the counter registers, assuming we are in page 0. */
    ax_local->stat.rx_frame_errors += AX_REG_RD (ax_base, EN0_COUNTER0);
    ax_local->stat.rx_crc_errors   += AX_REG_RD (ax_base, EN0_COUNTER1);
    ax_local->stat.rx_missed_errors+= AX_REG_RD (ax_base, EN0_COUNTER2);

    ax_sys_irq_enable(ax_local);
    return &ax_local->stat;
}
#endif

/*********************************************************************************************************
** Function name:           ax_tx_err
** Descriptions:            发送错误中断处理
** input parameters:        ax_local       设备驱动程序数据结构
** output parameters:
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static void ax_tx_err (AX_DEVICE *ax_local)
{
    ulong ax_base = ax_local->membase;

    uint8 txsr = AX_REG_RD (ax_base, EN0_TSR);
    uint8 tx_was_aborted = (uint8) (txsr & (ENTSR_ABT+ENTSR_FU));

    if (tx_was_aborted)
        ax_tx_intr (ax_local);
    else
    {
    #ifdef NET_STAT
        ax_local->stat.tx_errors++;
        if (txsr & ENTSR_CRS)
            ax_local->stat.tx_carrier_errors++;
        if (txsr & ENTSR_CDH)
            ax_local->stat.tx_heartbeat_errors++;
        if (txsr & ENTSR_OWC)
            ax_local->stat.tx_window_errors++;
    #endif

    #ifdef LWIP_LINK_STAT
        AX_LINK.err ++;
    #endif
    }
}

/*********************************************************************************************************
** Function name:           ax_sys_irq_connect
** Descriptions:            注册设备中断处理到内核
** input parameters:        ax_local          设备驱动程序数据结构
**                          ulIsrFunc         中断处理函数
** output parameters:                         执行结果
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_sys_irq_connect(AX_DEVICE *ax_local,  ulong  ulIsrFunc)
{
    int_connect (ax_local->ulChannel, (FUNC_ISR)ulIsrFunc, (void *)ax_local);
    return 0;
}
/*********************************************************************************************************
** Function name:           ax_sys_irq_disconnect
** Descriptions:            卸载设备中断处理函数
** input parameters:        ax_local          设备驱动程序数据结构
** output parameters:                         执行结果
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_sys_irq_disconnect(AX_DEVICE *ax_local)
{
    int_disconnect (ax_local->ulChannel);
    return 0;
}
/*********************************************************************************************************
** Function name:           ax_sys_irq_disable
** Descriptions:            禁用设备对应的系统中断
** input parameters:        ax_local          设备驱动程序数据结构
** output parameters:                         执行结果
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_sys_irq_disable(AX_DEVICE *ax_local)
{
    int_disable(ax_local->ulChannel);
    return  ERR_OK;
}
/*********************************************************************************************************
** Function name:           ax_sys_irq_enable
** Descriptions:            使能设备对应的系统中断
** input parameters:        ax_local          设备驱动程序数据结构
** output parameters:                         执行结果
** Returned value:          NONE
** Created by:              XuGuizhou
** Created Date:            2009/05/22
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static err_t ax_sys_irq_enable(AX_DEVICE *ax_local)
{
    int_enable(ax_local->ulChannel);
    return  ERR_OK;
}

/*********************************************************************************************************
** Function name:           ax_hw_flow_ctrl
** Descriptions:            配置
** input parameters:        netif             网络设备指针，指向空间由上层应用程序申请
**                          enable            硬件流控功能配置 1 -- 启用； 0 -- 关闭
** output parameters:
** Returned value:          ERR_OK
** Created by:              XuGuizhou
** Created Date:            2009/09/28
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Descriptions:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
err_t ax_hw_flow_ctrl(struct netif *netif, uint8 enable)
{
    AX_DEVICE *ax_local = (AX_DEVICE *) netif->state;

    if(ax_local == NULL || ax_local->ucHWFlowEn == enable)
        return ERR_OK;

    if(enable)
        ax_local->ucHWFlowEn = 1;
    else
        ax_local->ucHWFlowEn = 0;

    ax_init(ax_local, 1);

    return ERR_OK;
}
