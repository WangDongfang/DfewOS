/*==============================================================================
** ftp.c -- ftp server base on lwip and yaffs
**
** MODIFY HISTORY:
**
** 2011-09-01 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "yaffs_guts.h"
#include "lwip/netdb.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ftp_code.h"
int sprintf(char *, const char *, ...);
int sscanf(const char *str, char const *fmt, ...);
int snprintf(char *str, size_t n, char const *fmt, ...);

/*********************************************************************************************************
  FTP server debug
*********************************************************************************************************/
#define FTP_SERVER_DEBUG(d)        serial_printf(d)  /* print debug info */

#define FTP_SERVER_LOG_ON                            /* log ftp server message */
#ifdef FTP_SERVER_LOG_ON
#define FTP_SERVER_LOG(s)          serial_printf s
#else /* FTP_SERVER_LOG_ON */
#define FTP_SERVER_LOG(s)
#endif /* FTP_SERVER_LOG_ON */

/*********************************************************************************************************
  FTP server config
*********************************************************************************************************/
#define FTPS_LISTEN_TASK_STACK_SIZE         (10 * KB)               /*  the listen task stack size  */
#define FTPS_LISTEN_TASK_PRIORITY            19                     /*  the listen task priority    */
#define FTP_SESSION_TASK_STACK_SIZE         (32 * KB)               /*  every session task stack size */
#define FTP_SESSION_TASK_PRIORITY            20                     /*  every session task priority */

#define PATH_LEN_MAX                         256                    /*  yaffs path max length       */
#define FTP_SERVER_LISTEN_BACKLOG            3                      /*  FTP 服务器监听数量          */
#define FTP_SERVER_MAX_LINKS                 20
#define FTP_SERVER_LOCAL_BUF_SIZE            1024                   /*  FTP 命令接收缓冲区大小      */
#define FTP_SERVER_PATH_BUF_SIZE             PATH_LEN_MAX           /*  文件目录缓冲大小            */
#define FTP_SERVER_MAX_USR_NAME_LEN          64                     /*  用户名最大长度              */
#define FTP_SERVER_MAX_RETRY                 10                     /*  数据链路创建重试次数        */
#define FTP_SERVER_DATA_SEND_BUF_SIZE       (32 * KB)               /*  文件发送缓冲大小            */
#define FTP_SERVER_DATA_RECV_BUF_SIZE       (12 * KB)               /*  文件接收缓冲                */

/*********************************************************************************************************
  链接参数配置
*********************************************************************************************************/
static int _G_DEFAULT_TIMEOUT = 2000;       /*  默认链接超时时间             */
static int _G_IDLE_TIMEOUT    = 60 * 1000;  /*  1 分钟不访问将会关闭控制连接 */
static int _G_TCP_KEEP_IDLE   = 60;         /*  空闲时间, 单位秒             */
static int _G_TCP_KEEP_INTVL  = 60;         /*  两次探测间的时间间, 单位秒   */
static int _G_TCP_KEEP_CNT    = 3;          /*  探测 N 次失败认为是掉线      */

/*********************************************************************************************************
  FTP session status
*********************************************************************************************************/
#define SESSION_STATUS_LOGIN         0x0001                          /*  用户已登录                  */

/*======================================================================
  FTP data transfer mode
======================================================================*/
typedef enum _trans_mode {
    TRANS_MODE_ASCII,      /* Text File */
    TRANS_MODE_EBCDIC,     /* EBCDIC File */
    TRANS_MODE_BIN,        /* Binary File */
    TRANS_MODE_LOCAL       /* */
} FILE_TRANS_MODE;

/*======================================================================
  append usr relative path to current path make a yaffs absolute path
======================================================================*/
#define MAKE_ABS_PATH(abs_path, usr_path)      { \
                strcpy (abs_path, pFtpSession->current_path); \
                strcat (abs_path, usr_path); \
}    /* careful, strcat maybe overflow */

/*======================================================================
  session infomation struct
======================================================================*/
typedef struct {
    DL_NODE                 session_list_node; /* _G_session_list NODE */
    int                     session_status;
    char                    user_name[FTP_SERVER_MAX_USR_NAME_LEN];
    char                    current_path[PATH_LEN_MAX];         /*  当前路径                    */
   
    struct sockaddr_in      ctrl_sockaddr_in;                   /*  控制连接地址                */
    struct sockaddr_in      data_sockaddr_in;                   /*  数据连接地址                */
    struct sockaddr_in      default_sockaddr_in;                /*  默认链接地址                */
   
    BOOL                    use_default_addr;                   /*  是否使用默认地址进行数据连接*/
    struct in_addr          remote_ip;                          /*  远程地址, show 操作使用     */
    
    int                     ctrl_sock_id;                       /*  会话 socket                 */
    int                     data_sock_id;                       /*  数据 socket                 */
    int                     pasv_sock_id;                       /*  PASV socket                 */
    
    FILE_TRANS_MODE         trans_mode;                         /*  仅支持 ASCII / BIN          */
    
    OS_TCB                 *server_task;                        /*  服务线程句柄                */
    uint32                  start_tick;                         /*  连接起始时间                */
} FTP_SESSION;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static char                   *_G_root_path = NULL;             /*  ftp server root path        */
static SEM_MUX                 _G_session_list_semM;            /*  会话链表锁                  */
static DL_LIST                 _G_session_list = {NULL, NULL};  /*  会话管理链表表头            */
#define SESSION_LIST_LOCK()    semM_take(&_G_session_list_semM, WAIT_FOREVER)
#define SESSION_LIST_UNLOCK()  semM_give(&_G_session_list_semM)

/*********************************************************************************************************
  shell command functions declare
*********************************************************************************************************/
static int  C_ftps_show(int  argc, char **argv);
static int  C_ftps_path(int  argc, char **argv);
static void ftps_cmd_init();

/*********************************************************************************************************
  helper function declare
*********************************************************************************************************/
static void  _ftps_close_data_sock(FTP_SESSION *pFtpSession);
static void  _ftps_dispatch_cmd(char *pcBuffer, char **ppcCmd, char **ppcOpts, char **ppcArgs);
static OS_STATUS _ftps_cd (char *cur_path, char *dir);

/*==============================================================================
 * - _ftps_send_reply()
 *
 * - reply client
 */
static void  _ftps_send_reply (FTP_SESSION *pFtpSession, int  iCode, const char *  cpcMessage)
{
    char send_buf[256];
    char *pcTail = (pFtpSession->trans_mode == TRANS_MODE_ASCII) ? "\r" : "";
    
    if (cpcMessage != NULL) {
        sprintf(send_buf, "%d %.70s%s\n", iCode, cpcMessage, pcTail);
    } else {
        sprintf(send_buf, "%d%s\n", iCode, pcTail);
    }
    send (pFtpSession->ctrl_sock_id, send_buf, strlen (send_buf), 0);
}

/*==============================================================================
 * - _ftps_send_mode_reply()
 *
 * - send transform mode to client
 */
static void  _ftps_send_mode_reply (FTP_SESSION *pFtpSession)
{
    if (pFtpSession->trans_mode == TRANS_MODE_BIN) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_OK, 
                        "Opening BINARY mode data connection.");
    } else {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_OK, 
                        "Opening ASCII mode data connection.");
    }
}

/*********************************************************************************************************
** 函数名称: _ftps_send_list_line
** 功能描述: ftp 针对 LIST 命令, 生成一个单条的文件信息回复
** 输　入  : iSock        发送端 socket
**           bWide        是否发送文件属性信息 (FALSE 表示仅发送文件名)
**           timeNow      当前时间
**           path         目录
**           pcAdd        追加目录 (如果存在需要与目录 path 链接生成完整目录)
**           pcFileName   需要输出的文件名
**           pcBuffer     临时缓冲
** 输　出  : 发送的文件数量 1: 发送一个 0: 发送错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static int  _ftps_send_list_line (int         iSock, 
                                  BOOL        bWide, 
                                  uint32      timeNow,  /* no use */
                                  const char *path, 
                                  const char *pcAdd,
                                  const char *pcFileName,
                                  char       *pcBuffer)
{
    if (bWide) {                                                        /*  需要发送完整信息            */
        struct yaffs_stat   statFile;
        int                 iPathLen = strlen(path);
        int                 iAddLen  = strlen(pcAdd);
        
        if (iPathLen == 0) {
            pcBuffer[0] = '\0';
        } else {
            strcpy(pcBuffer, path);
            if (iAddLen > 0 && pcBuffer[iPathLen - 1] != '/') {
                pcBuffer[iPathLen++] = '/';
                if (iPathLen >= FTP_SERVER_PATH_BUF_SIZE) {
                    return  (0);                                        /*  too long                    */
                }
                pcBuffer[iPathLen] = '\0';
            }
        }
        if (iPathLen + iAddLen >= FTP_SERVER_PATH_BUF_SIZE) {
            return  (0);                                                /*  too long                    */
        }
        strcpy(pcBuffer + iPathLen, pcAdd);                             /*  合成完整目录                */
        
        if (yaffs_stat(pcBuffer, &statFile) == 0) {                     /*  查看目录信息                */
            int         iLen;
#if 0
            struct tm   tmFile;
            time_t      timeFile = statFile.st_mtime;                   /*  文件时间                    */
            
            enum { 
                SIZE = 80 
            };
            enum { 
                SIX_MONTHS = 365 * 24 * 60 * 60 / 2
            };
            
            char        cTimeBuf[SIZE];
            
            lib_gmtime_r(&timeFile, &tmFile);                           /*  生成 tm 时间结构            */
            
            if ((timeNow > timeFile + SIX_MONTHS) || (timeFile > timeNow + SIX_MONTHS)) {
                strftime(cTimeBuf, SIZE, "%b %d  %Y", &tmFile);
            } else {
                strftime(cTimeBuf, SIZE, "%b %d %H:%M", &tmFile);
            }
#endif
            char cTimeBuf[80] = "Aug 31 2011";
            
            /*
             *  构建发送格式数据包
             */
            iLen = snprintf(pcBuffer, FTP_SERVER_PATH_BUF_SIZE,
                            "%c%c%c%c%c%c%c%c%c%c  1 %5d %5d %11u %s %s\r\n",
                            (S_ISLNK(statFile.st_mode) ? ('l') : (S_ISDIR(statFile.st_mode) ? ('d') : ('-'))),
                            (statFile.st_mode & S_IRUSR) ? ('r') : ('-'),
                            (statFile.st_mode & S_IWUSR) ? ('w') : ('-'),
                            (statFile.st_mode & S_IXUSR) ? ('x') : ('-'),
                            (statFile.st_mode & S_IRGRP) ? ('r') : ('-'),
                            (statFile.st_mode & S_IWGRP) ? ('w') : ('-'),
                            (statFile.st_mode & S_IXGRP) ? ('x') : ('-'),
                            (statFile.st_mode & S_IROTH) ? ('r') : ('-'),
                            (statFile.st_mode & S_IWOTH) ? ('w') : ('-'),
                            (statFile.st_mode & S_IXOTH) ? ('x') : ('-'),
                            (int)statFile.st_uid,
                            (int)statFile.st_gid,
                            (int)statFile.st_size,
                            cTimeBuf,
                            pcFileName);
            FTP_SERVER_LOG(("LIST send : %s",pcBuffer));
            if (send(iSock, pcBuffer, iLen, 0) != iLen) {               /*  发送详细信息                */
                return  (0);
            }
        }
    } else {
        int iLen = snprintf(pcBuffer, FTP_SERVER_PATH_BUF_SIZE, "%s\r\n", pcFileName);
        
        if (send(iSock, pcBuffer, iLen, 0) != iLen) {                   /*  发送文件名                  */
            return  (0);
        }
    }
    
    return  (1);                                                        /*  成功发送的一个              */
}

/*==============================================================================
 * - _ftps_get_data_sock()
 *
 * - get data socket
 */
static int  _ftps_get_data_sock (FTP_SESSION *pFtpSession)
{
    int iSock = pFtpSession->pasv_sock_id;

    if (iSock < 0) {                                                    /*  如果没有建立 pasv 数据连接  */
        iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (iSock < 0) {
            _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_FAILED,
                            "Can't create data socket.");
        } else {
            struct sockaddr_in  sockaddrinLocalData;
            int                 i;
            int                 iOn = 1;
            
            sockaddrinLocalData = pFtpSession->ctrl_sockaddr_in;
            sockaddrinLocalData.sin_port = htons(20);                   /*  本地数据端口号              */
        
            setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, &iOn, sizeof(iOn));
                                                                        /*  保留本地地址供下次使用      */
            /*
             *  服务器主动链接客户机, 本地端口必须绑定为 20
             */
            for (i = 0; i < FTP_SERVER_MAX_RETRY; i++) {
                errno = 0;
                if (bind(iSock, (struct sockaddr *)&sockaddrinLocalData, 
                         sizeof(sockaddrinLocalData)) >= 0) {
                    break;
                } else if (errno != EADDRINUSE) {                       /*  如果不是地址被占用          */
                    i = FTP_SERVER_MAX_RETRY;                           /*  直接错误退出                */
                } else {
                    delayQ_delay(SYS_CLK_RATE);                         /*  等待一秒                    */
                }
            }
            
            if (i >= FTP_SERVER_MAX_RETRY) {
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_FAILED,
                                "Can't bind data socket.");
                close(iSock);
                iSock = -1;
            } else {
                /*
                 *  如果客户机已经通过 PORT 命令通知了自己的数据链路地址, 那么服务器将连接指定的地址
                 *  如果客户机没有指定数据链路地址, 那么服务器将链接默认数据地址.
                 */
                struct sockaddr_in  *psockaddrinDataDest = (pFtpSession->use_default_addr) 
                                                         ? (&pFtpSession->default_sockaddr_in)
                                                         : (&pFtpSession->data_sockaddr_in);
                if (connect(iSock, (struct sockaddr *)psockaddrinDataDest, 
                            sizeof(struct sockaddr_in)) < 0) {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_FAILED, 
                                    "Can't connect data socket.");
                    close(iSock);
                    iSock = -1;
                }
            }
        }
    }
    
    pFtpSession->data_sock_id   = iSock;
    pFtpSession->use_default_addr = TRUE;
    if (iSock > 0) {
        setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, 
                   (const void *)&_G_DEFAULT_TIMEOUT, 
                   sizeof(int));                                        /*  设置链接与接收操作          */
    }
    
    return  (iSock);
}

/*==============================================================================
 * - _ftps_cmd_LIST()
 *
 * - list one file, or files and directories in a directory
 *   <pcFileName> is a relative path
 */
static int  _ftps_cmd_LIST (FTP_SESSION *pFtpSession, const char *pcFileName, int  iWide)
{
    int                data_sock_id;
    struct yaffs_stat  statFile;
    char               data_send_buf[FTP_SERVER_PATH_BUF_SIZE];

    char  yaffs_abs_path[PATH_LEN_MAX];

    
    yaffs_DIR     *pdir    = NULL;
    yaffs_dirent  *pdirent = NULL;
    uint32         timeNow;
    int            iSuc = 1;

    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_OK, 
                    "Opening ASCII mode data connection for LIST.");
                    
    data_sock_id = _ftps_get_data_sock(pFtpSession);                                   /*  获得数据连接 socket         */
    if (data_sock_id < 0) {
        FTP_SERVER_LOG(("ftpd: error connecting to data socket."));
        return  (OS_STATUS_OK);
    }

    MAKE_ABS_PATH(yaffs_abs_path, pcFileName);
    
    if (yaffs_stat (yaffs_abs_path, &statFile) < 0) {                  /*  对象未找到                */
        snprintf(data_send_buf, FTP_SERVER_PATH_BUF_SIZE,
                 "%s: No such file or directory.\r\n", yaffs_abs_path);
        send(data_sock_id, data_send_buf, strlen(data_send_buf), 0);
    
    } else if (S_ISDIR(statFile.st_mode) && 
               (NULL == (pdir = yaffs_opendir(yaffs_abs_path)))) {     /*  是目录并且无法打开          */
        snprintf(data_send_buf, FTP_SERVER_PATH_BUF_SIZE,
                 "%s: Can not open directory.\r\n", yaffs_abs_path);
        send(data_sock_id, data_send_buf, strlen(data_send_buf), 0);
    
    } else {                                                           /* file or dir */
        timeNow = tick_get();
        if (!pdir && *pcFileName) { /* one file */
            iSuc = iSuc && _ftps_send_list_line(data_sock_id, iWide, timeNow, yaffs_abs_path,
                                             pcFileName, pcFileName, data_send_buf);
        } else {
            do {
                pdirent = yaffs_readdir(pdir);
                if (pdirent == NULL) {
                    break;
                }
                iSuc = iSuc && _ftps_send_list_line(data_sock_id, iWide, timeNow, yaffs_abs_path, 
                                                 pdirent->d_name, pdirent->d_name, data_send_buf);
            } while (iSuc && pdirent);
        }
    }
    
    if (pdir) {
        yaffs_closedir(pdir);
    }
    _ftps_close_data_sock(pFtpSession);
    
    if (iSuc) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATACLOSE_NOERR, "Transfer complete.");
    } else {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_ABORT, "Connection aborted.");
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_CWD()
 *
 * - change current work directory if can
 */
static int  _ftps_cmd_CWD (FTP_SESSION *pFtpSession, char *pcDir)
{
    if (_ftps_cd(pFtpSession->current_path, pcDir) == OS_STATUS_OK) { /* try to change dir */
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_OP_OK, "CWD command successful.");
    } else {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, "CWD command failed.");
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_CDUP()
 *
 * - go to father directory if can
 */
static int  _ftps_cmd_CDUP (FTP_SESSION *pFtpSession)
{
    if (_ftps_cd(pFtpSession->current_path, "..") == OS_STATUS_OK) { /* try to change dir */
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_OP_OK, "CDUP command successful.");
    } else {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, "CDUP command failed.");
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_PORT()
 *
 * - record client's data IP and PORT
 */
static int  _ftps_cmd_PORT (FTP_SESSION *pFtpSession, char *pcArg)
{
    enum { 
        NUM_FIELDS = 6 
    };
    uint32    uiTemp[NUM_FIELDS];
    int     iNum;
    
    _ftps_close_data_sock(pFtpSession);                                 /*  关闭数据连接                */
    
    iNum = sscanf(pcArg, "%u,%u,%u,%u,%u,%u", uiTemp + 0, 
                                              uiTemp + 1, 
                                              uiTemp + 2, 
                                              uiTemp + 3, 
                                              uiTemp + 4, 
                                              uiTemp + 5);              /*  注意: 这里必须整型宽        */
    if (NUM_FIELDS == iNum) {
        int     i;
        unsigned char   ucTemp[NUM_FIELDS];

        for (i = 0; i < NUM_FIELDS; i++) {
            if (uiTemp[i] > 255) {
                break;
            }
            ucTemp[i] = (unsigned char)uiTemp[i];
        }
        if (i == NUM_FIELDS) {
            /*
             *  这里不允许 port 命令参数 IP 地址与原始客户端不相等.
             */
            u32_t   uiIp = (u32_t)((ucTemp[0] << 24)
                         |         (ucTemp[1] << 16)
                         |         (ucTemp[2] <<  8)
                         |         (ucTemp[3]));                        /*  构建 IP 地址                */
            
            u16_t   usPort = (u16_t)((ucTemp[4] << 8)
                           |         (ucTemp[5]));                      /*  构建端口号                  */
            
            uiIp   = ntohl(uiIp);
            usPort = ntohs(usPort);
            
            if (uiIp == pFtpSession->default_sockaddr_in.sin_addr.s_addr) {
                pFtpSession->data_sockaddr_in.sin_addr.s_addr = uiIp;
                pFtpSession->data_sockaddr_in.sin_port        = usPort; /*  已经为网络序, 不用转换      */
                pFtpSession->data_sockaddr_in.sin_family      = AF_INET;
                memset(pFtpSession->data_sockaddr_in.sin_zero, 0, 
                           sizeof(pFtpSession->data_sockaddr_in.sin_zero));
               
                /*
                 *  之后的正向数据连接将使用客户机指定的地址.
                 */
                pFtpSession->use_default_addr = FALSE;                  /*  不再使用默认地址            */
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_CMD_OK, 
                                "PORT command successful.");
                
                return  (OS_STATUS_OK);
            
            } else {
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_FAILED, 
                                "Address doesn't match peer's IP.");
                return  (OS_STATUS_OK);
            }
        }
    }

    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_SYNTAX_ERR, "Syntax error.");
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_PASV()
 *
 * - PASV command
 */
static int  _ftps_cmd_PASV (FTP_SESSION *pFtpSession)
{
    int  iErrNo = 1;
    int  iSock;
    int  one = 1;

    _ftps_close_data_sock(pFtpSession);                                 /*  关闭数据连接                */
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);                  /*  创建数据链接端点            */
    if (iSock < 0) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_FAILED, 
                        "Can't open passive socket.");
        FTP_SERVER_LOG(("ftpd: error creating PASV socket.\n"));
        return  (OS_STATUS_OK);
        
    } else {
        struct sockaddr_in  sockaddrin;
        socklen_t           uiAddrLen = sizeof(sockaddrin);

        sockaddrin          = pFtpSession->ctrl_sockaddr_in;
        sockaddrin.sin_port = htons(0);                                 /*  需要自动分配端口            */
        
        if (bind(iSock, (struct sockaddr *)&sockaddrin, uiAddrLen) < 0) {
            FTP_SERVER_LOG(("ftpd: error binding PASV socket.\n"));
        } else if (listen(iSock, 1) < 0) {
            FTP_SERVER_LOG(("ftpd: error listening on PASV socket.\n")); 
        } else {
            char            cArgBuffer[65];
            ip_addr_t       ipaddr;
            u16_t           usPort;
            
            setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, 
                       (const void *)&_G_DEFAULT_TIMEOUT, 
                       sizeof(int));                                    /*  设置链接与接收操作          */
                       
            getsockname(iSock, 
                        (struct sockaddr *)&sockaddrin, 
                        &uiAddrLen);                                    /*  获得数据连接地址            */
            /*
             *  构建回应参数
             */
            ipaddr.addr = sockaddrin.sin_addr.s_addr;
            usPort      = ntohs(sockaddrin.sin_port);
            snprintf(cArgBuffer, 64, "Entering passive mode (%u,%u,%u,%u,%u,%u).",
                     ip4_addr1(&ipaddr), ip4_addr2(&ipaddr), ip4_addr3(&ipaddr), ip4_addr4(&ipaddr), 
                     (usPort >> 8), (usPort & 0xFF));
            
            _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_INTO_PASV, cArgBuffer);
            
            /*
             *  开始等待客户数据链接
             */
            pFtpSession->pasv_sock_id = accept(iSock, (struct sockaddr *)&sockaddrin, &uiAddrLen);
            if (pFtpSession->pasv_sock_id < 0) {
                FTP_SERVER_LOG(("ftpd: error accepting PASV connection.\n"));
            } else {                                                    /*  链接成功                    */
                /*
                 *  关闭 listen socket 仅保留数据连接 socket
                 */
                close(iSock);
                iSock  = -1;
                iErrNo = 0;
                
                /*
                 *  设置保鲜参数.
                 */
                setsockopt(pFtpSession->pasv_sock_id, SOL_SOCKET,  SO_KEEPALIVE,  (const void *)&one, sizeof(int));
                setsockopt(pFtpSession->pasv_sock_id, IPPROTO_TCP, TCP_KEEPIDLE,  (const void *)&_G_TCP_KEEP_IDLE, sizeof(int));
                setsockopt(pFtpSession->pasv_sock_id, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&_G_TCP_KEEP_INTVL, sizeof(int));
                setsockopt(pFtpSession->pasv_sock_id, IPPROTO_TCP, TCP_KEEPCNT,   (const void *)&_G_TCP_KEEP_CNT, sizeof(int));
            }
        }
    }
    
    if (iErrNo) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATALINK_FAILED, 
                        "Can't open passive connection.");
        close(iSock);                                                   /*  关闭临时链接                */
    }
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_PWD()
 *
 * - send current work directory to client
 */
static int  _ftps_cmd_PWD (FTP_SESSION *pFtpSession)
{
    static char const   cTxt[] = "\" is the current directory.";
    char    cBuffer[FTP_SERVER_PATH_BUF_SIZE + 32] = "\""; 
    
    strcat (cBuffer, pFtpSession->current_path);
    strcat (cBuffer, cTxt);

    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_MAKE_DIR_OK, cBuffer);

    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_MDTM()
 *
 * - send file update time info to client
 */
static int  _ftps_cmd_MDTM (FTP_SESSION *pFtpSession, const char *  pcFileName)
{
    char  cBuffer[FTP_SERVER_PATH_BUF_SIZE];

    /* FAKE TIME */
    snprintf(cBuffer, FTP_SERVER_PATH_BUF_SIZE, "%04d%02d%02d%02d%02d%02d",
            2011, 8, 31,
            17, 24, 25);
    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_STATUS, cBuffer);
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_RNFR()
 *
 * - rename file
 */
static int  _ftps_cmd_RNFR (FTP_SESSION *pFtpSession, const char *pcFileName)
{
    char  cBuffer[FTP_SERVER_PATH_BUF_SIZE + 32];                /*  缓冲新的文件名              */
    char  old_abs_path[PATH_LEN_MAX];
    char  new_abs_path[PATH_LEN_MAX];
    char *pcCmd;
    char *pcOpts;
    char *pcArgs;

    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_NEED_INFO, "RNFR need RNTO info.");
    
    /*
     * 等待 RNTO 命令
     */
    if (recv (pFtpSession->ctrl_sock_id, cBuffer, FTP_SERVER_LOCAL_BUF_SIZE, 0) <= 0) {
        _ftps_dispatch_cmd(cBuffer, &pcCmd, &pcOpts, &pcArgs);   /*  分析命令                    */
        if (!strcmp(pcCmd, "RNTO")) {
            MAKE_ABS_PATH(old_abs_path, pcFileName);
            MAKE_ABS_PATH(new_abs_path, pcArgs);

            if (yaffs_rename(old_abs_path, new_abs_path) == 0) {
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_OP_OK,
                        "RNTO complete.");
            } else {
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_FILE_NAME_ERROR,
                        "File name error.");
            }
            return  (OS_STATUS_OK);
        }
    }
    
    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_SYNTAX_ERR, "RNTO not follow by RNFR.");
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_RETR()
 *
 * - send a file to client. <pcFileName> is ABS path
 */
static int  _ftps_cmd_RETR (FTP_SESSION *pFtpSession, const char *pcFileName)
{
    int   iSock;
    int   fd;
    char  cLocalBuffer[FTP_SERVER_LOCAL_BUF_SIZE];
    char *pcTransBuffer = NULL;
    
    int   iBufferSize;
    int   iResult = 0;

    struct yaffs_stat statBuf;
    
    FTP_SERVER_LOG(("in RETR cmd try to open file : %s\n", pcFileName));
    fd = yaffs_open(pcFileName, O_RDONLY, 0666);
    if (fd < 0) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, "Error opening file.");
        return  (OS_STATUS_OK);
    }
    if (0 > yaffs_stat(pcFileName, &statBuf)) {
        yaffs_close(fd);
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, "Error stating file.");
        return  (OS_STATUS_OK);
    }
    
    if (statBuf.st_size < FTP_SERVER_LOCAL_BUF_SIZE) {
        pcTransBuffer = cLocalBuffer;                             /*  使用局部缓冲传输            */
        iBufferSize   = FTP_SERVER_LOCAL_BUF_SIZE;
    } else {
#if 0
        iBufferSize   = (uint32)MIN(FTP_SERVER_DATA_SEND_BUF_SIZE, statBuf.st_size);
        pcTransBuffer = (char *)malloc(iBufferSize);              /*  使用最优缓冲区大小          */
#endif
        if (pcTransBuffer == NULL) {
            pcTransBuffer =  cLocalBuffer;
            iBufferSize   = FTP_SERVER_LOCAL_BUF_SIZE;            /*  开辟失败, 使用本地缓冲      */
        }
    }
    
    _ftps_send_mode_reply(pFtpSession);                           /*  通知对方传输方式            */
    
    iSock = _ftps_get_data_sock(pFtpSession);                     /*  获得数据连接 socket         */
    if (iSock >= 0) {
        int     iN = -1;
        
        if (statBuf.st_size == 0) {                               /*  文件没有数据                */
            iN = 0;
            
        } else if (pFtpSession->trans_mode == TRANS_MODE_BIN) {   /*  二进制类型传输              */
            while ((iN = (int)yaffs_read(fd, pcTransBuffer, (ssize_t)iBufferSize)) > 0) {
                if (send(iSock, pcTransBuffer, iN, 0) != iN) {
                    break;                                        /*  发送失败                    */
                }
            }
            
        } else if (pFtpSession->trans_mode == TRANS_MODE_ASCII) {       /*  文本传输                    */
            int     iReset = 0;
            
            while (iReset == 0 && (iN = (int)yaffs_read(fd, pcTransBuffer, (ssize_t)iBufferSize)) > 0) {
                char *   e = pcTransBuffer;
                char *   b;
                int     i;
                
                iReset = iN;
                do {
                    char  cLf = '\0';

                    b = e;
                    for (i = 0; i < iReset; i++, e++) {
                        if (*e == '\n') {                               /*  行结束变换                  */
                            cLf = '\n';
                            break;
                        }
                    }
                    if (send(iSock, b, i, 0) != i) {                    /*  发送单行数据                */
                        break;
                    }
                    if (cLf == '\n') {
                        if (send(iSock, "\r\n", 2, 0) != 2) {           /*  发送行尾                    */
                            break;
                        }
                        e++;
                        i++;
                    }
                } while ((iReset -= i) > 0);
            }
        }
        
        if (0 == iN) {                                                  /*  读取数据完毕                */
            if (0 == yaffs_close(fd)) {
                fd     = -1;
                iResult = 1;
            }
        }
    }
    
    if (fd >= 0) {
        yaffs_close(fd);
    }
    
    if (pcTransBuffer && (pcTransBuffer != cLocalBuffer)) {
        free(pcTransBuffer);
    }
    _ftps_close_data_sock(pFtpSession);
    
    if (0 == iResult) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_ABORT, "File read error.");
    } else {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATACLOSE_NOERR, "Transfer complete.");
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_cmd_STOR()
 *
 * - receive a file from client. <pcFileName> is ABS path
 */
static int  _ftps_cmd_STOR (FTP_SESSION *pFtpSession, const char *pcFileName)
{
    int         iSock;
    int         fd;
    char        cLocalBuffer[FTP_SERVER_LOCAL_BUF_SIZE];
    char *      pcTransBuffer = NULL;
    int         iBufferSize;
    int         iResult = 0;
    
    int         iN = 0;
    
    _ftps_send_mode_reply(pFtpSession);                                 /*  通知对方传输方式            */
    
    iSock = _ftps_get_data_sock(pFtpSession);                           /*  获得数据连接 socket         */
    if (iSock < 0) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, "Error creating data link.");
        return  (OS_STATUS_OK);
    }
    
    fd = yaffs_open(pcFileName, (O_WRONLY | O_CREAT | O_TRUNC), 0666);
    if (fd < 0) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, "Error creating file.");
        _ftps_close_data_sock(pFtpSession);
        return  (OS_STATUS_OK);
    }
    
    pcTransBuffer = (char *)malloc(FTP_SERVER_DATA_RECV_BUF_SIZE);      /*  开辟接收缓冲                */
    if (pcTransBuffer) {
        iBufferSize = FTP_SERVER_DATA_RECV_BUF_SIZE;
    } else {
        pcTransBuffer = cLocalBuffer;
        iBufferSize = FTP_SERVER_LOCAL_BUF_SIZE;
    }
    
    if (pFtpSession->trans_mode == TRANS_MODE_BIN) {              /*  二进制类型传输              */
        while ((iN = (int)recv(iSock, pcTransBuffer, (ssize_t)iBufferSize, 0)) > 0) {
            if (yaffs_write(fd, pcTransBuffer, iN) != iN) {
                break;                                            /*  接收失败                    */
            }
        }
    
    } else if (pFtpSession->trans_mode == TRANS_MODE_ASCII) {     /*  文本传输                    */
        while ((iN = (int)recv(iSock, pcTransBuffer, (ssize_t)iBufferSize, 0)) > 0) {
            char *e = pcTransBuffer;
            char *b;
            int   i;
            int   iCounter = 0;
            
            do {
                int     iCr = 0;
                b = e;
                for (i = 0; i < (iN - iCounter); i++, e++) {
                    if (*e == '\r') {
                        iCr = 1;
                        break;
                    }
                }
                if (i > 0) {
                    if (yaffs_write(fd, b, i) != i) {                   /*  保存单行数据                */
                        goto    __recv_over;                            /*  接收错误                    */
                    }
                }
                iCounter += (i + iCr);                                  /*  已经保存的数据              */
                if (iCr) {
                    e++;                                                /*  忽略 \r                     */
                }
            } while (iCounter < iN);
        }
    } else {
        iN = 1;                                                         /*  错误标志                    */
    }
    
__recv_over:
    yaffs_close(fd);                                                    /*  关闭文件                    */

    if (0 >= iN) {                                                      /*  发送完毕                    */
        iResult = 1;
    } else {
        yaffs_unlink(pcFileName);                                       /*  传输失败删除文件            */
    }
    
    if (pcTransBuffer && (pcTransBuffer != cLocalBuffer)) {
        free(pcTransBuffer);                                            /*  释放缓冲区                  */
    }
    
    _ftps_close_data_sock(pFtpSession);                                 /*  关闭数据连接                */
    
    if (0 == iResult) {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DREQ_ABORT, "File write error.");
    } else {
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_DATACLOSE_NOERR, "Transfer complete.");
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_do_cmd()
 *
 * - process command. the hugest function
 */
static int  _ftps_do_cmd (FTP_SESSION *pFtpSession, char *pcCmd, char *pcArg)
{
    char        cFileName[256];                                     /*  文件名                      */
    char  yaffs_abs_path[PATH_LEN_MAX];

    pFtpSession->session_status = SESSION_STATUS_LOGIN;/* 假装登陆 */

    if (!strcmp("USER", pcCmd)) {                                   /*  用户名                      */
        strlcpy(pFtpSession->user_name, pcArg, FTP_SERVER_MAX_USR_NAME_LEN);
        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_PW_REQ, 
                        "Password required.");                      /*  等待密码输入                */
        pFtpSession->session_status &= ~SESSION_STATUS_LOGIN;
        
    } else if (!strcmp("PASS", pcCmd)) {                            /*  密码输入                    */
        /*
         *  TODO: 目前暂不检查用户的合法性.
         */

        _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_USER_LOGIN, "User logged in.");
        pFtpSession->session_status |= SESSION_STATUS_LOGIN;
        
    } else if (!strcmp("HELP", pcCmd)) {                            /*  帮助信息                    */
        /*
         *  TODO: 目前暂不打印帮助清单.
         */
    } else {
        /*
         *  下面的操作需要登陆
         */
        if ((pFtpSession->session_status & SESSION_STATUS_LOGIN) == 0) {
            _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_LOGIN_FAILED,
                            "USER and PASS required.");             /*  需要用户登录                */
        } else {
            if (!strcmp("CWD", pcCmd)) {                            /*  设置工作目录                */
                if ((pcArg != NULL) && (pcArg[0] != '\0')) {
                    sscanf(pcArg, "%254s", cFileName);
                } else {
                    strcpy(cFileName, ".");                         /*  当前文件                    */
                }
                return  (_ftps_cmd_CWD(pFtpSession, cFileName));
            
            } else if (!strcmp("CDUP", pcCmd)) {                    /*  返回上级目录                */
                return  (_ftps_cmd_CDUP(pFtpSession));
            
            } else if ((!strcmp("PWD", pcCmd)) ||
                       (!strcmp("XPWD", pcCmd))) {                  /*  当前目录                    */
                return  (_ftps_cmd_PWD(pFtpSession));
                
            } else if (!strcmp("ALLO", pcCmd) ||
                       !strcmp("ACCT", pcCmd)) {                    /*  用户类型识别                */
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_CMD_UNSUPPORT, 
                                "Allocate and Account not required.");
                return  (OS_STATUS_OK);
                
            } else if (!strcmp("PORT", pcCmd)) {                    /*  数据连接信息                */
                return  (_ftps_cmd_PORT(pFtpSession, pcArg));
                
            } else if (!strcmp("PASV", pcCmd)) {                    /*  PASV 数据连接信息           */
                return  (_ftps_cmd_PASV(pFtpSession));
                
            } else if (!strcmp("TYPE", pcCmd)) {                    /*  设定传输模式                */
                if (pcArg[0] == 'I') {
                    pFtpSession->trans_mode = TRANS_MODE_BIN;
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_CMD_OK, 
                                    "Type set to I.");
                } else if (pcArg[0] == 'A') {
                    pFtpSession->trans_mode = TRANS_MODE_ASCII;
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_CMD_OK, 
                                    "Type set to A.");
                } else {
                    pFtpSession->trans_mode = TRANS_MODE_BIN;
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_UNSUP_WITH_ARG, 
                                    "Type not implemented.  Set to I.");
                }
                return  (OS_STATUS_OK);
                
            } else if (!strcmp("LIST", pcCmd)) {                    /*  打印当前目录列表            */
                if ((pcArg != NULL) && (pcArg[0] != '\0')) {
                    sscanf(pcArg, "%254s", cFileName);
                } else {
                    cFileName[0] = '\0';   /* 当前目录 */
                }
                return  (_ftps_cmd_LIST(pFtpSession, cFileName, 1));
                
            } else if (!strcmp("NLST", pcCmd)) {                    /*  打印当前目录列表            */
                if ((pcArg != NULL) && (pcArg[0] != '\0')) {
                    sscanf(pcArg, "%254s", cFileName);
                } else {
                    cFileName[0] = '\0';   /* 当前目录 */
                }
                return  (_ftps_cmd_LIST(pFtpSession, cFileName, 0));/*  简洁打印                    */
                
            } else if (!strcmp("RNFR", pcCmd)) {                    /*  文件改名                    */
                return  (_ftps_cmd_RNFR(pFtpSession, pcArg));
                
            } else if (!strcmp("RETR", pcCmd)) {                    /*  文件发送                    */
                MAKE_ABS_PATH(yaffs_abs_path, pcArg);
                return  (_ftps_cmd_RETR(pFtpSession, yaffs_abs_path));
            
            } else if (!strcmp("STOR", pcCmd)) {                    /*  文件接收                    */
                MAKE_ABS_PATH(yaffs_abs_path, pcArg);
                return  (_ftps_cmd_STOR(pFtpSession, yaffs_abs_path));
            
            } else if (!strcmp("SYST", pcCmd)) {                    /*  询问系统类型                */
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_NAME_SYS_TYPE, 
                                FTP_SERVER_SYSTYPE);
                return  (OS_STATUS_OK);
            
            } else if (!strcmp("MKD", pcCmd)) {                     /*  创建一个目录                */
                MAKE_ABS_PATH(yaffs_abs_path, pcArg);
                if (yaffs_mkdir(yaffs_abs_path, 0) < 0) {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, 
                                    "MKD failed.");
                } else {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_MAKE_DIR_OK, 
                                    "MKD successful.");
                }
                return  (OS_STATUS_OK);
                
            } else if (!strcmp("RMD", pcCmd)) {                     /*  删除一个目录                */
                MAKE_ABS_PATH(yaffs_abs_path, pcArg);
                if (yaffs_rmdir (yaffs_abs_path) < 0) {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, 
                                    "RMD failed.");
                } else {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_MAKE_DIR_OK, 
                                    "RMD successful.");
                }
                return  (OS_STATUS_OK);
            } else if (!strcmp("DELE", pcCmd)) {                    /*  删除一个文件                */
                MAKE_ABS_PATH(yaffs_abs_path, pcArg);
                if (yaffs_unlink(yaffs_abs_path) < 0) {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, 
                                    "DELE failed.");
                } else {
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_MAKE_DIR_OK, 
                                    "DELE successful.");
                }
                return  (OS_STATUS_OK);
                
            } else if (!strcmp("MDTM", pcCmd)) {                    /*  文件时间更新                */
                return  (_ftps_cmd_MDTM(pFtpSession, pcArg));
                
            } else if (!strcmp("SITE", pcCmd)) {                    /*  站点参数                    */
                char *pcOpts;
                _ftps_dispatch_cmd(pcArg, &pcCmd, &pcOpts, &pcArg);
                if (!strcmp("CHMOD", pcCmd)) {                      /*  修改文件 mode               */
                    int     iMask = 0;
                    if (sscanf(pcArg, "%o %254s", &iMask, cFileName) == 2) {

                        MAKE_ABS_PATH(yaffs_abs_path, cFileName);
                        if (yaffs_chmod (yaffs_abs_path, (mode_t)iMask)== 0) {
                            _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_MAKE_DIR_OK, 
                                    "CHMOD successful.");
                            return  (OS_STATUS_OK);
                        }
                    }
                    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_REQ_FAILED, 
                            "CHMOD failed.");
                    return  (OS_STATUS_OK);
                }
                goto command_not_understood;
                
            } else if (!strcmp("NOOP", pcCmd)) {                    /*  什么都不做                  */
                _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_CMD_OK, 
                                "NOOP -- did nothing as requested.");
                return  (OS_STATUS_OK);
            }

command_not_understood:
            _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_CMD_ERROR, 
                            "Command not understood.");             /*  暂不可被识别的命令          */
        }
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - _ftps_close_ctrl_sock()
 *
 * - close a session's ctrl socket
 */
static void  _ftps_close_ctrl_sock (FTP_SESSION *pFtpSession)
{
    close(pFtpSession->ctrl_sock_id);                               /*  关闭控制 socket             */
    pFtpSession->ctrl_sock_id = -1;
}

/*==============================================================================
 * - _ftps_close_data_sock()
 *
 * - close a session's data socket
 */
static void  _ftps_close_data_sock (FTP_SESSION *pFtpSession)
{
    struct linger   linger_close_now = {1, 0};                            /*  立即关闭链接                */

    /*
     *  只可能存在一种数据连接模式.
     */
    if (pFtpSession->data_sock_id > 0) {
        setsockopt(pFtpSession->data_sock_id, SOL_SOCKET, SO_LINGER, 
                   (void *)&linger_close_now, sizeof(linger_close_now));
        close(pFtpSession->data_sock_id);                                 /*  关闭数据连接                */
    } else if (pFtpSession->pasv_sock_id > 0) {
        setsockopt(pFtpSession->pasv_sock_id, SOL_SOCKET, SO_LINGER, 
                   (void *)&linger_close_now, sizeof(linger_close_now));
        close(pFtpSession->pasv_sock_id);                                 /*  关闭 PASV 数据连接          */
    }
    
    pFtpSession->data_sock_id   = -1;
    pFtpSession->pasv_sock_id   = -1;
    pFtpSession->use_default_addr = TRUE;
}

/*==============================================================================
 * - _ftps_skip_opt()
 *
 * - move point after opt segment
 */
static void  _ftps_skip_opt (char **ppcTemp)
{
    char *pcBuf  = *ppcTemp;
    char *pcLast = NULL;
  
    for (;;) {
        while (isspace(*pcBuf)) {                                       /*  忽略                        */
            ++pcBuf;
        }
        
        if (*pcBuf == '-') {
            if (*++pcBuf == '-') {                                      /* `--' should terminate options*/
                if (isspace(*++pcBuf)) {
                    pcLast = pcBuf;
                    do {
                        ++pcBuf;
                    } while (isspace(*pcBuf));
                    break;
                }
            }
            while (*pcBuf && !isspace(*pcBuf)) {
                ++pcBuf;
            }
            pcLast = pcBuf;
        } else {
            break;
        }
    }
    if (pcLast) {
        *pcLast = '\0';
    }
    *ppcTemp = pcBuf;
}

/*==============================================================================
 * - _ftps_dispatch_cmd()
 *
 * - dispatch command line which from client to cmd, opt, arg
 */
static void  _ftps_dispatch_cmd (char *pcBuffer, char **ppcCmd, char **ppcOpts, char **ppcArgs)
{
    char *pcEoc;                        /* end of command */
    char *pcTemp = pcBuffer;
  
    while (isspace(*pcTemp)) {                                          /*  忽略前方无用字符            */
        ++pcTemp;
    }
    
    *ppcCmd = pcTemp;                                                   /*  记录命令起始地址            */
    while (*pcTemp && !isspace(*pcTemp)) {
        *pcTemp = (char)toupper(*pcTemp);                               /*  转换为大写字符              */
        pcTemp++;
    }
    
    pcEoc = pcTemp;
    if (*pcTemp) {
        *pcTemp++ = '\0';                                               /*  命令结束                    */
    }
    
    while (isspace(*pcTemp)) {                                          /*  忽略前方无用字符            */
        ++pcTemp;
    }
    
    *ppcOpts = pcTemp;                                                  /*  记录选项                    */
    _ftps_skip_opt(&pcTemp);
  
    if (*ppcOpts == pcTemp) {
        *ppcOpts = pcEoc;
    }
    
    *ppcArgs = pcTemp;                                                  /*  记录参数                    */
    while (*pcTemp && *pcTemp != '\r' && *pcTemp != '\n') {             /*  命令末尾                    */
        ++pcTemp;
    }
    
    if (*pcTemp) {
        *pcTemp++ = '\0';                                               /*  去掉换行符                  */
    }

    FTP_SERVER_LOG ((COLOR_FG_RED"\"%s\""COLOR_FG_WHITE" CMD will be processed...\n", *ppcCmd));
    FTP_SERVER_LOG (("OPT : %s  ARG : %s\n\n", *ppcOpts, *ppcArgs));
}

/*==============================================================================
 * - T_ftp_server_session()
 *
 * - ftp server session task. receive command, process and reply
 */
static void  T_ftp_server_session (FTP_SESSION *pFtpSession)
{
#ifdef FTP_SERVER_LOG_ON
    char  str_ip[16];
#endif /* FTP_SERVER_LOG_ON */

    char  recv_buf[FTP_SERVER_LOCAL_BUF_SIZE];                      /*  需要占用较大堆栈空间        */
    int   recv_len;
    char *pcCmd;
    char *pcOpts;
    char *pcArgs;

    pFtpSession->server_task = G_p_current_tcb;

    SESSION_LIST_LOCK();
    dlist_add (&_G_session_list, (DL_NODE *)pFtpSession);           /*  链入会话链表                */
    SESSION_LIST_UNLOCK();
    
    FTP_SERVER_LOG(("ftps session create, remote : %s\n", inet_ntoa_r(pFtpSession->remote_ip, str_ip, 16)));
                    
    _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_READY, FTP_SERVER_MESSAGE);/*发送服务器就绪应答*/

    /*
     * ftp server session loop
     */
    FOREVER {
        recv_len = recv (pFtpSession->ctrl_sock_id, recv_buf, FTP_SERVER_LOCAL_BUF_SIZE - 1, 0);
        if (recv_len <= 0) {
            break;
        }
        recv_buf[recv_len] = '\0';

        FTP_SERVER_LOG (("FTP SESSION RECV : %s", recv_buf));
        
        _ftps_dispatch_cmd(recv_buf, &pcCmd, &pcOpts, &pcArgs);     /*  分析命令                    */

        
        if (!strcmp("QUIT", pcCmd)) {                               /*  结束 ftp 会话               */
            _ftps_send_reply(pFtpSession, FTP_SERVER_RETCODE_BYEBYE, "Goodbye.");
            break;
        } else {
            if (_ftps_do_cmd(pFtpSession, pcCmd, pcArgs) != OS_STATUS_OK) { /*  执行命令            */
                break;                                              /*  命令错误直接结束会话        */
            }
        }
    }
                                                                        
    _ftps_close_data_sock(pFtpSession);                             /*  关闭数据连接                */
    _ftps_close_ctrl_sock(pFtpSession);                             /*  关闭控制连接                */
    
    SESSION_LIST_LOCK();
    dlist_remove (&_G_session_list, (DL_NODE *)pFtpSession);        /*  从会话表中解链              */
    SESSION_LIST_UNLOCK();
    
    free(pFtpSession);                                              /*  释放控制块                  */
}

/*==============================================================================
 * - T_ftp_server_listen()
 *
 * - ftp server listen task
 */
static void  T_ftp_server_listen (void)
{
    int           one = 1;
    int           local_sock_id;
    int           accept_sock_id;
    FTP_SESSION  *pNewSession;
    
    struct sockaddr_in  local_sockaddr_in;
    struct sockaddr_in  remote_sockaddr_in;
    socklen_t           sock_len;
    
    local_sockaddr_in.sin_len    = sizeof(struct sockaddr_in);
    local_sockaddr_in.sin_family = AF_INET;
    local_sockaddr_in.sin_port   = htons(21);                       /*  FTP server PORT 21          */
    local_sockaddr_in.sin_addr.s_addr = INADDR_ANY;
    
    local_sock_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (local_sock_id < 0) {
        FTP_SERVER_DEBUG("T_ftp_server_listen() error : can not create socket.\n");
        return;
    }
    
    /* bind local sock to 21 port */
    bind(local_sock_id, (struct sockaddr *)&local_sockaddr_in, sizeof(local_sockaddr_in));
    listen(local_sock_id, FTP_SERVER_LISTEN_BACKLOG);
    
    FOREVER {
        accept_sock_id = accept(local_sock_id, (struct sockaddr *)&remote_sockaddr_in, &sock_len);
        if (accept_sock_id < 0) {
            delayQ_delay(SYS_CLK_RATE);     /*  延迟 1 S        */
            continue;
        }
        if (dlist_count(&_G_session_list) > FTP_SERVER_MAX_LINKS) {
            /*
             *  服务器链接已满, 不能再创建新的连接.
             */
            close(accept_sock_id);
            delayQ_delay(SYS_CLK_RATE);   /*  延迟 1 S (防止攻击)         */
            continue;
        }
        
        /*
         *  FTPD 等待命令时, 控制连接长时间没有收到命令, 将会自动关闭以节省资源.
         */
        setsockopt(accept_sock_id, SOL_SOCKET, SO_RCVTIMEO, (const void *)&_G_IDLE_TIMEOUT, sizeof(int));
        
        /*
         *  创建并初始化会话结构
         */
        pNewSession = (FTP_SESSION *)malloc(sizeof(FTP_SESSION));
        if (pNewSession == NULL) {
            FTP_SERVER_DEBUG("T_ftp_server_listen() error : system low memory.\n");
            goto error_handle;
        }
        memset(pNewSession, 0, sizeof(FTP_SESSION));
        
        pNewSession->ctrl_sock_id         = accept_sock_id;              /*  会话 socket  */
        pNewSession->default_sockaddr_in  = remote_sockaddr_in;          /*  远程地址     */
        pNewSession->remote_ip            = remote_sockaddr_in.sin_addr; /*  远程 IP      */
        
        if (getsockname(accept_sock_id, (struct sockaddr *)&local_sockaddr_in, &sock_len) < 0) {
            FTP_SERVER_DEBUG("T_ftp_server_listen() error : getsockname() failed.\n");
            goto error_handle;
        } else {
            pNewSession->use_default_addr = TRUE;
            pNewSession->ctrl_sockaddr_in = local_sockaddr_in; /*  控制端口本地地址            */
            pNewSession->data_sock_id     = -1;
            pNewSession->pasv_sock_id     = -1;
            pNewSession->trans_mode       = TRANS_MODE_ASCII;
            pNewSession->data_sockaddr_in.sin_port =           /* data port 20 */
                         htons((uint16)(ntohs(pNewSession->ctrl_sockaddr_in.sin_port) - 1));
            pNewSession->start_tick       = tick_get();        /*  记录连接时间                */
        }
        strcpy(pNewSession->current_path, _G_root_path);       /*  初始化为设定的 ftp 根目录   */
        
        /*
         *  设置保鲜参数.
         */
        setsockopt(accept_sock_id, SOL_SOCKET,  SO_KEEPALIVE,  (const void *)&one,               sizeof(int));
        setsockopt(accept_sock_id, IPPROTO_TCP, TCP_KEEPIDLE,  (const void *)&_G_TCP_KEEP_IDLE,  sizeof(int));
        setsockopt(accept_sock_id, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&_G_TCP_KEEP_INTVL, sizeof(int));
        setsockopt(accept_sock_id, IPPROTO_TCP, TCP_KEEPCNT,   (const void *)&_G_TCP_KEEP_CNT,   sizeof(int));
        
        /*
         *  建立服务器会话线程
         */
        if (task_create ("tFtp_session", FTP_SESSION_TASK_STACK_SIZE,
                         FTP_SESSION_TASK_PRIORITY,
                         (FUNC_PTR)T_ftp_server_session,
                         (uint32)pNewSession, 0) == NULL) {
                goto    error_handle;
        }
        continue;                                                  /*  等待新的客户机              */
        
error_handle:
        close(accept_sock_id);                                     /*  关闭临时连接                */
        if (pNewSession) {
            free(pNewSession);                                     /*  释放会话控制块              */
        }
    } /* FOREVER */
}

/*==============================================================================
 * - ftp_server_init()
 *
 * - init the ftp root path, start listen task, add ftp cmd to shell
 */
OS_STATUS  ftp_server_init (const char *path)
{
    static BOOL             is_initialized = FALSE;
           char            *new_path = "/n1/";
    
    if (is_initialized) {                                           /*  已经初始化了                */
        return OS_STATUS_OK;
    } else {
        is_initialized = TRUE;
    }
    
    /*
     * set ftp server root path
     */
    if (path != NULL) {                                             /*  需要设置服务器目录          */
        new_path = (char *)path;
    }
    if (_G_root_path != NULL) {
        free(_G_root_path);
    }
    _G_root_path = (char *)malloc(strlen(new_path) + 1);
    if (_G_root_path == NULL) {
        FTP_SERVER_DEBUG("ftp_server_init() error : system low memory.\n");
        goto error_handle;
    }
    strcpy(_G_root_path, new_path);                                 /*  保存服务器路径              */
    
    /*
     * init session list semM
     */
    if (semM_init(&_G_session_list_semM) == NULL) {                 /*  创建会话链表互斥量          */
        goto error_handle;
    }
    
    /*
     * create ftp server listen task
     */
    if (task_create ("tFtps_listen", FTPS_LISTEN_TASK_STACK_SIZE,
                FTPS_LISTEN_TASK_PRIORITY, (FUNC_PTR)T_ftp_server_listen, 0, 0) == NULL) {
        goto error_handle;
    }
    
    /*
     * add ftp server command
     */
    ftps_cmd_init();
    
    return OS_STATUS_OK;
    
error_handle:
    is_initialized = FALSE;                                         /*  初始化失败                  */
    return OS_STATUS_ERROR;
}

/*==============================================================================
 * - _show_one_session_info()
 *
 * - print a ftp session infomation
 */
static OS_STATUS _show_one_session_info (FTP_SESSION *pFtpSession, int *pCount)
{
    char    str_ip[16];
    uint32  alive_time;

    inet_ntoa_r(pFtpSession->remote_ip, str_ip, 16);             /*  格式化地址字串              */
    alive_time = tick_get() - pFtpSession->start_tick;           /*  计算存活时间                */
    alive_time /= SYS_CLK_RATE;

    serial_printf("%s%s %24u %12u\n", str_ip, "  ", 
                  (pFtpSession->start_tick / SYS_CLK_RATE),
                  alive_time);

    *pCount = (*pCount) + 1;
    return OS_STATUS_OK;
}

/*==============================================================================
 * - ftps_show_sessions()
 *
 * - show current ftp sessions infomation
 */
void  ftps_show_sessions (void)
{
    static const char   session_info_header[] = "\n"
        "    REMOTE             START_TIME          ALIVE(s)\n"
        "--------------- ------------------------ ------------\n";

    int count = 0;
    
    serial_printf(session_info_header);
    
    SESSION_LIST_LOCK();                                           /*  锁定会话链表                */
    dlist_each (&_G_session_list, (EACH_FUNC_PTR)_show_one_session_info, (int)&count);
    SESSION_LIST_UNLOCK();                                         /*  解锁会话链表                */

    serial_printf("\nTotal ftp session : %d", count);              /*  显示会话数量                */
}

/*==============================================================================
 * - ftps_set_root_path()
 *
 * - set ftp server root path, call by shell command
 */
int  ftps_set_root_path (const char *path)
{
    char *new_path;
    char *pcTemp = _G_root_path;
    yaffs_DIR *d;
    
    if (path == NULL) {                                            /*  目录为空                    */
        return  (OS_STATUS_ERROR);
    }
    d = yaffs_opendir(path);
    if (!d) {
        FTP_SERVER_DEBUG("ftps_set_root_path() error : no exsit path.\n");
        return  (OS_STATUS_ERROR);
    } else {
        yaffs_closedir(d);
    }
    
    new_path = (char *)malloc((strlen(path) + 1));
    if (new_path == NULL) {
        FTP_SERVER_DEBUG("ftps_set_root_path() error : system low memory.\n");
        return  (OS_STATUS_ERROR);
    }
    strcpy(new_path, path);                                        /*  拷贝新的路径                */
    
//    __KERNEL_MODE_PROC(
        _G_root_path = new_path;                                   /*  设置新的服务器路径          */
//    );
    
    if (pcTemp) {
        free (pcTemp);
    }
    
    return  (OS_STATUS_OK);
}

/*==============================================================================
 * - C_ftps_show()
 *
 * - show ftp server infomation. A shell command operat function
 */
static int  C_ftps_show (int  argc, char **argv)
{
    serial_printf("ftps show >>\n");
    serial_printf("FTP root path : %s\n", (_G_root_path != NULL) ? (_G_root_path) : "null");

    ftps_show_sessions();
    
    return  (CMD_OK);
}

/*==============================================================================
 * - C_ftps_path()
 *
 * - show or set ftp server root path. A shell command operat function
 */
static int  C_ftps_path (int  argc, char **argv)
{
    if (argc < 2) { /* if no arg, print current ftp server root path */
        serial_printf("FTP root path : %s", (_G_root_path != NULL) ? (_G_root_path) : "null");
        return  (CMD_OK);
    }
    
    if (ftps_set_root_path(argv[1]) == OS_STATUS_OK) {
        return (CMD_OK);
    } else {
        return (CMD_ERROR);
    }
}

/**********************************************************************************************************
  init the ftp server commands, such as "ftps.show"
**********************************************************************************************************/
static CMD_INF _G_ftps_cmds[] = {
    {"ftps.show", "show ftp server sessions", C_ftps_show},
    {"ftps.path", "show or set ftp server root path", C_ftps_path}
};

void ftps_cmd_init()
{
    int i;
    int cmd_num;
    cmd_num = N_ELEMENTS(_G_ftps_cmds);

    for (i = 0; i < cmd_num; i++) {
        cmd_add(&_G_ftps_cmds[i]);
    }
}

/*==============================================================================
 * - _ftps_cd()
 *
 * - change current dir. <cur_path> = "/n1/sfd/ddd/"
 */
static OS_STATUS _ftps_cd (char *cur_path, char *dir)
{
    if (strcmp (dir, ".") == 0) {
        return OS_STATUS_OK;
    } else if (strcmp (dir, "..") == 0) {
        if (strcmp (cur_path, _G_root_path) != 0) {  /* cur is't ftp server root dir */
            char *end = cur_path + strlen(cur_path) - 1;
            while (*--end != '/') ; /* until <end> point to prev '/' */
            *++end = '\0';
            return OS_STATUS_OK;
        }
    } else {
        int  cur_path_len = strlen(cur_path);
        char temp_abs_path[PATH_LEN_MAX];

        if (strncmp (dir, _G_root_path, strlen(_G_root_path)) == 0) {   /* abs path */
            strncpy(temp_abs_path, dir, PATH_LEN_MAX - 2);
        } else {
            strcpy (temp_abs_path, cur_path);
            strncat (temp_abs_path, dir, (PATH_LEN_MAX - cur_path_len - 2));
        }
        temp_abs_path[PATH_LEN_MAX - 2] = '\0';
        
        if (yaffs_access(temp_abs_path, 0) == 0) {  /* 首先探测是否可以访问 */
            strcpy (cur_path, temp_abs_path);

            cur_path_len = strlen(cur_path);
            if (cur_path[cur_path_len - 1] != '/') {   /* append '/' to end if need */
                cur_path[cur_path_len] = '/';
                cur_path[cur_path_len + 1] = '\0';
            }
            return OS_STATUS_OK;
        }
    }

    return OS_STATUS_ERROR;
}

/*==============================================================================
** FILE END
==============================================================================*/

