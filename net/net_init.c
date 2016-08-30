/*==============================================================================
** net_init.c -- init net component.
**
** MODIFY HISTORY:
**
** 2011-09-02 wdf Create.
==============================================================================*/

#include <dfewos.h>
#include "../string.h"
#include "ax88796_driver/ethif_ax88796b.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"

/*======================================================================
  shell command
======================================================================*/
static int C_simple_ifconfig (int  argc, char **argv);
static CMD_INF _G_ifcfg_cmd = {"ifconfig", "internet face config", C_simple_ifconfig};

/*======================================================================
  app init function declare
======================================================================*/
extern void T_chat_server_start ();
extern void T_telnet_server_start ();
extern void T_web_server_start ();
extern void ftp_server_init (const char *path);

/*==============================================================================
 * - _net_stack_init()
 *
 * - init lwip stack and create net job task
 */
static void  _net_stack_init (void)
{
    /*
     * init lwip stack
     */
    tcpip_init(NULL, NULL);

    /*
     * create a task proccess job created by ax88796 interrupt
     */
    net_job_init();
}

/*==============================================================================
 * - _netif_attch()
 *
 * - attch ax88796 netif to lwip stack
 */
static void  _netif_attch (void)
{
    static struct netif    ax88796if;

    ip_addr_t         gw;
    ip_addr_t         ipaddr;
    ip_addr_t         netmask;

    IP4_ADDR(&ipaddr,  192, 168, 9, 167);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw,      192, 168, 9, 254);

    netif_add(&ax88796if, &ipaddr, &netmask, &gw, NULL, ethdev_init,
              tcpip_input);
    netif_set_up(&ax88796if);

    netif_set_default(&ax88796if);
}

/*==============================================================================
 * - net_init()
 *
 * - init net component
 */
void net_init ()
{
    _net_stack_init();
    _netif_attch();

    task_create("tChat_S", 10240, 20, T_chat_server_start, 0, 0);
    task_create("tTelnet", 20480, 20, T_telnet_server_start, 0, 0);
    task_create("tHttp", 20480, 20, T_web_server_start, 0, 0);
    ftp_server_init(NULL);

    cmd_add(&_G_ifcfg_cmd);
}

/****************************************************************************************************
 **  ifconfig shell command
****************************************************************************************************/
static void  _netif_show (char *netif_name, const struct netif  *netif_block);

/*==============================================================================
 * - C_simple_ifconfig()
 *
 * - ifconfig command operat function
 */
int C_simple_ifconfig (int argc, char **argv)
{
    struct netif *netif = netif_list;

    for (; netif != NULL; netif = netif->next) {
        _netif_show(NULL, netif);
    }
    return CMD_OK;
}

/*==============================================================================
 * - _netif_show()
 *
 * - print netif infomation.
 *   <netif_name> or <netif_block> could be NULL
 */
static void  _netif_show (char *netif_name, const struct netif *netif_block)
{
#define printf serial_printf
    struct netif    *netif;
    ip_addr_t        ipaddrBroadcast;
    int              i;

    if ((netif_name == NULL) && (netif_block == NULL)) {
        return;
    }

    if (netif_block) {
        netif = (struct netif *)netif_block;
    } else {
        netif = netif_find(netif_name);
    }

    if (netif == NULL) {
        return;
    }

    /*
     *  打印网口基本信息
     */
    printf("%c%c%d       ", netif->name[0], netif->name[1], netif->num);
    printf("enable : %s  ", (netif_is_up(netif) > 0) ? "true" : "false");
    printf("linkup : %s\n", (netif_is_link_up(netif) > 0) ? "true" : "false");
    printf("          mtu : %d  ", netif->mtu);
    printf("multicast : %s\n", (netif->flags & NETIF_FLAG_IGMP) ? "true" : "false");

    /*
     *  打印路由信息
     */
    if (netif == netif_default) {                                       /*  route interface             */
        printf("          metric : 1 ");
    } else {
        printf("          metric : 0 ");
    }

    /*
     *  打印网口硬件地址信息
     */
    if (netif->flags & NETIF_FLAG_ETHARP) {
        printf(" type : ethernet\n");                                   /*  以太网络                    */
        printf("          physical address : ");
        for (i = 0; i < netif->hwaddr_len - 1; i++) {
            printf("%02X:", netif->hwaddr[i]);
        }
        printf("%02X\n", netif->hwaddr[netif->hwaddr_len - 1]);
    } else if (netif->flags & NETIF_FLAG_POINTTOPOINT) {
        printf(" type : point-to-point\n");                             /*  点对点网络接口              */
    } else {
        printf(" type : general\n");                                    /*  通用网络接口                */
    }
    
    /*
     *  打印网口协议地址信息
     */
    printf("          inet      : %d.%d.%d.%d\n", ip4_addr1(&netif->ip_addr),
                                                  ip4_addr2(&netif->ip_addr),
                                                  ip4_addr3(&netif->ip_addr),
                                                  ip4_addr4(&netif->ip_addr));
    printf("          netmask   : %d.%d.%d.%d\n", ip4_addr1(&netif->netmask),
                                                  ip4_addr2(&netif->netmask),
                                                  ip4_addr3(&netif->netmask),
                                                  ip4_addr4(&netif->netmask));
    printf("          gateway   : %d.%d.%d.%d\n", ip4_addr1(&netif->gw),
                                                  ip4_addr2(&netif->gw),
                                                  ip4_addr3(&netif->gw),
                                                  ip4_addr4(&netif->gw));
    if (netif->flags & NETIF_FLAG_BROADCAST) {                          /*  打印广播地址信息            */
        ipaddrBroadcast.addr = (netif->ip_addr.addr | (~netif->netmask.addr));
        printf("          broadcast : %d.%d.%d.%d\n", ip4_addr1(&ipaddrBroadcast),
                                                      ip4_addr2(&ipaddrBroadcast),
                                                      ip4_addr3(&ipaddrBroadcast),
                                                      ip4_addr4(&ipaddrBroadcast));
    }
#if LWIP_SNMP
    printf("          speed : %d(bps)\n", netif->link_speed);           /*  打印链接速度                */

    /*
     *  打印网口收发数据信息
     */
    printf("          RX ucast packets:%d nucast packets:%d dropped:%d\n", netif->ifinucastpkts,
                                                                           netif->ifinnucastpkts,
                                                                           netif->ifindiscards);
    printf("          TX ucast packets:%d nucast packets:%d dropped:%d\n", netif->ifoutucastpkts,
                                                                           netif->ifoutnucastpkts,
                                                                           netif->ifoutdiscards);
    printf("          RX bytes:%d TX bytes:%d\n", netif->ifinoctets,
                                                  netif->ifoutoctets);
#endif /* LWIP_SNMP */
}

/*==============================================================================
** FILE END
==============================================================================*/

