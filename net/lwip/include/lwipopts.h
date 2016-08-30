/*==============================================================================
** lwipopts.h -- lwip user config file.
**
** MODIFY HISTORY:
**
** 2011-08-24 wdf Create.
==============================================================================*/

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#define DFEW_LWIP_DEBUG  0  /* debug off */
/*******************************************  DFEW OS  **************************************************/
#define CPU_WORD_LEN     32 /* one word 32 bits */
#define CPU_BYTE_LEN     8  /* one byte 8  bits */
#define _KB              1024
#define _MB             (1024 * 1024)
#define INT_MAX          2147483647   /* this file not used, use in opt.h */
/*******************************************  DFEW OS  **************************************************/
  
/*********************************************************************************************************
  Platform specific locking
*********************************************************************************************************/
//#define SYS_LIGHTWEIGHT_PROT            1

/*********************************************************************************************************
  Memory options
*********************************************************************************************************/
#define MEM_ALIGNMENT                  (CPU_WORD_LEN / CPU_BYTE_LEN)    /*  内存对齐情况                */
#define MEM_SIZE                       (512 * _KB)                      /*  malloc 堆大小               */
#define MEMP_NUM_PBUF                   256                             /*  npbufs                      */
#define PBUF_POOL_SIZE                  512                             /*  pool num                    */
#define PBUF_POOL_BUFSIZE               1600                            /*  pool block size             */

#if MEM_SIZE >= (32 * _MB)
#define MEMP_NUM_REASSDATA             150                              /*  同时进行重组的 IP 数据包    */
#elif MEM_SIZE >= (1 * _MB)
#define MEMP_NUM_REASSDATA             100
#elif MEM_SIZE >= (512 * _KB)
#define MEMP_NUM_REASSDATA              80
#elif MEM_SIZE >= (256 * _KB)
#define MEMP_NUM_REASSDATA              40
#elif MEM_SIZE >= (128 * _KB)
#define MEMP_NUM_REASSDATA              20
#else
#define MEMP_NUM_REASSDATA              5
#endif                                                                  /*  MEM_SIZE >= ...             */

/*********************************************************************************************************
  ...PCB
*********************************************************************************************************/
#define MEMP_NUM_RAW_PCB                15
#define MEMP_NUM_UDP_PCB                50
#define MEMP_NUM_TCP_PCB                60
#define MEMP_NUM_TCP_PCB_LISTEN         60

#define MEMP_NUM_NETCONN                (MEMP_NUM_RAW_PCB + MEMP_NUM_UDP_PCB + MEMP_NUM_UDP_PCB)

#define MEMP_NUM_TCPIP_MSG_API          (MEMP_NUM_RAW_PCB + MEMP_NUM_UDP_PCB + MEMP_NUM_UDP_PCB)
#define MEMP_NUM_TCPIP_MSG_INPKT        512            /*  tcp input msgqueue use      */

/*********************************************************************************************************
  check sum
*********************************************************************************************************/
//#define LWIP_CHECKSUM_ON_COPY           1                               /*  拷贝数据包同时计算 chksum   */

/*********************************************************************************************************
  IP packet
*********************************************************************************************************/
//#define IP_FORWARD                      1                               /*  允许 IP 转发                */
#define IP_REASS_MAX_PBUFS              (MEMP_NUM_PBUF / 2)

//#define IP_SOF_BROADCAST                1                               /*  Use the SOF_BROADCAST       */
//#define IP_SOF_BROADCAST_RECV           1

#define MEMP_NUM_SYS_TIMEOUT            (MEMP_NUM_RAW_PCB + MEMP_NUM_UDP_PCB + MEMP_NUM_UDP_PCB + 20)
#define MEMP_NUM_NETBUF                 256

/*********************************************************************************************************
  transmit layer (注意: 如果 TCP_WND 大于网卡接收缓冲, 可能造成批量传输时, 网卡芯片缓冲溢出.
                        如果出现这种情况, 请在这里自行配置 TCP_WND 大小. 并确保大于 2 * TCP_MSS)
*********************************************************************************************************/
//#define LWIP_UDP                        1
//#define LWIP_UDPLITE                    1
//#define LWIP_NETBUF_RECVINFO            1

#define LWIP_TCP                        1
#define TCP_LISTEN_BACKLOG              1
#define LWIP_TCP_TIMESTAMPS             1

#define TCP_QUEUE_OOSEQ                 1

#ifndef TCP_MSS
#define TCP_MSS                         1460                            /*  usually                     */
#endif                                                                  /*  TCP_MSS                     */

#define TCP_CALCULATE_EFF_SEND_MSS      1                               /*  use effective send MSS      */

#if MEM_SIZE >= (32 * _MB)
#define TCP_WND                         ((64 * _KB) - 1)                /*  MAX WINDOW                  */
#define TCP_SND_BUF                     (128 * _KB)
#elif MEM_SIZE >= (16 * _MB)
#define TCP_WND                         ((64 * _KB) - 1)
#define TCP_SND_BUF                     (64  * _KB)
#elif MEM_SIZE >= (4 * _MB)
#define TCP_WND                         (32  * _KB)
#define TCP_SND_BUF                     (32  * _KB)
#elif MEM_SIZE >= (1 * _MB)
#define TCP_WND                         (16  * _KB)
#define TCP_SND_BUF                     (32  * _KB)
#elif MEM_SIZE >= (512 * _KB)
#define TCP_WND                         ( 8  * _KB)
#define TCP_SND_BUF                     (16  * _KB)
#elif MEM_SIZE >= (128 * _KB)
#define TCP_WND                         ( 8  * _KB)
#define TCP_SND_BUF                     ( 8  * _KB)
#elif MEM_SIZE >= (64 * _KB)  /* MEM_SIZE < 128 _KB  SMALL TCP_MSS XXX */
#undef  TCP_MSS
#define TCP_MSS                         536                             /*  small mss                   */
#define TCP_WND                         ( 4  * TCP_MSS)
#define TCP_SND_BUF                     ( 4  * TCP_MSS)
#else
#undef  TCP_MSS
#define TCP_MSS                         256                             /*  small mss                   */
#define TCP_WND                         ( 4  * TCP_MSS)
#define TCP_SND_BUF                     ( 4  * TCP_MSS)
#endif                                                                  /*  MEM_SIZE >= ...             */

#if TCP_WND < (4  * TCP_MSS)
#define TCP_WND                         ( 4  * TCP_MSS)                 /*  must bigger than 4 * TCP_MSS*/
#endif                                                                  /*  TCP_WND < (4  * TCP_MSS)    */

#define MEMP_NUM_TCP_SEG                (8 * TCP_SND_QUEUELEN)

#define LWIP_TCP_KEEPALIVE              1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_RCVBUF                  1

#define SO_REUSE                        1                               /*  enable SO_REUSEADDR         */
#define SO_REUSE_RXTOALL                1

#if 0  /* goto end */
/*********************************************************************************************************
  Network Interfaces options
*********************************************************************************************************/
#if 1
#define LWIP_NETIF_HOSTNAME             1                               /*  netif have nostname member  */
#define LWIP_NETIF_API                  1                               /*  support netif api           */
#define LWIP_NETIF_STATUS_CALLBACK      1                               /*  interface status change     */
#define LWIP_NETIF_LINK_CALLBACK        1                               /*  link status change          */
#define LWIP_NETIF_HWADDRHINT           1                               /*  XXX                         */
#define LWIP_NETIF_LOOPBACK             1
#endif

/*********************************************************************************************************
  eth net options
*********************************************************************************************************/
#if 0
#define LWIP_ARP                        1
#define ARP_QUEUEING                    1
#define ARP_TABLE_SIZE                  30
#define ETHARP_TRUST_IP_MAC             0
#define ETHARP_SUPPORT_VLAN             1                               /*  IEEE 802.1q VLAN            */
#define ETH_PAD_SIZE                    2
#define ETHARP_SUPPORT_STATIC_ENTRIES   1
#endif

/*********************************************************************************************************
  loop interface
*********************************************************************************************************/
#define LWIP_HAVE_LOOPIF                1                               /*  127.0.0.1                   */

/*********************************************************************************************************
  inet thread
*********************************************************************************************************/
#define TCPIP_THREAD_NAME               "tLwip"
#define TCPIP_THREAD_STACKSIZE          4096
#define TCPIP_THREAD_PRIO               10

#define SLIPIF_THREAD_NAME              "t_slip"
#define SLIPIF_THREAD_STACKSIZE         4096
#define SLIPIF_THREAD_PRIO              10

#define PPP_THREAD_NAME                 "t_ppp"
#define PPP_THREAD_STACKSIZE            4096
#define PPP_THREAD_PRIO                 10

#define DEFAULT_THREAD_NAME             "t_netdef"
#define DEFAULT_THREAD_STACKSIZE        4096
#define DEFAULT_THREAD_PRIO             10

/*********************************************************************************************************
  Socket options 
*********************************************************************************************************/
#define ERRNO                                                           /*  include errno               */
#define LWIP_SOCKET                     1
#define LWIP_COMPAT_SOCKETS             1                               /*  some function conflict      */

//#define LWIP_DNS_API_HOSTENT_STORAGE    1                               /*  have bsd DNS                */
//#define LWIP_DNS_API_DECLARE_STRUCTS    1                               /*  use lwip DNS struct         */

/*********************************************************************************************************
  Statistics options
*********************************************************************************************************/
#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              1
#define LWIP_STATS_LARGE                1
                                                                        
/*********************************************************************************************************
  get_opt 冲突
*********************************************************************************************************/
#ifdef  opterr
#undef  opterr
#endif                                                                  /*  opterr                      */

/*********************************************************************************************************
  END
*********************************************************************************************************/
#endif /* goto end */
                                                                        
/*********************************************************************************************************
  Debugging options (TCP UDP IP debug & only can print message)
*********************************************************************************************************/
#if DFEW_LWIP_DEBUG > 0
#define LWIP_DEBUG
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_OFF              /*  允许全部主动打印信息        */
#define TCP_DEBUG                       (LWIP_DBG_ON | 0)               /*  仅允许 TCP UDP IP debug     */
#define UDP_DEBUG                       (LWIP_DBG_ON | 0)
#define IP_DEBUG                        (LWIP_DBG_ON | 0)
#define SOCKETS_DEBUG                   (LWIP_DBG_ON | 0)
#define TCPIP_DEBUG                     (LWIP_DBG_ON | 0)               /*  debug tLwip(tcpip_thread)  */
#else
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_SEVERE           /*  不允许主动打印信息          */
#endif /* DFEW_LWIP_DEBUG > 0 */

/*********************************************************************************************************
  mbox msg queue size
*********************************************************************************************************/
#define LWIP_MSGQUEUE_SIZE              512            /*  sys_mbox_new size           */

#define TCPIP_THREAD_NAME              "tLwip"
#define TCPIP_THREAD_STACKSIZE          40960
#define TCPIP_THREAD_PRIO               10

#endif /* __LWIPOPTS_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

