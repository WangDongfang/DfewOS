/*==============================================================================
** ftp_code.h -- ftp server reply code.
**
** MODIFY HISTORY:
**
** 2011-08-30 wdf Create.
==============================================================================*/

#ifndef __FTP_CODE_H__
#define __FTP_CODE_H__

/*======================================================================
  服务器信息
======================================================================*/
#define FTP_SERVER_MESSAGE  "Dfew FTP server ready."DFEW_KERNEL_VERSION
#define FTP_SERVER_SYSTYPE  "UNIX Type: L8"

/*======================================================================
  服务器响应码
======================================================================*/
#define FTP_SERVER_RETCODE_RESTART_ACK       110             /*  重新启动标记应答            */
#define FTP_SERVER_RETCODE_READY_MIN         120             /*  服务在nnn分钟内准备好       */
#define FTP_SERVER_RETCODE_DATALINK_READY    125             /*  数据连接已准备好            */
#define FTP_SERVER_RETCODE_FILE_OK           150             /*  文件状态良好，打开数据连接  */
#define FTP_SERVER_RETCODE_CMD_OK            200             /*  命令成功                    */
#define FTP_SERVER_RETCODE_CMD_UNSUPPORT     202             /*  命令未实现                  */
#define FTP_SERVER_RETCODE_STATUS_HELP       211             /*  系统状态或系统帮助响应      */
#define FTP_SERVER_RETCODE_DIR_STATUS        212             /*  目录状态                    */
#define FTP_SERVER_RETCODE_FILE_STATUS       213             /*  文件状态                    */
#define FTP_SERVER_RETCODE_HELP_INFO         214             /*  帮助信息, 仅对人类用户有用  */
#define FTP_SERVER_RETCODE_NAME_SYS_TYPE     215             /*  名字系统类型                */
#define FTP_SERVER_RETCODE_READY             220             /*  服务器就绪                  */
#define FTP_SERVER_RETCODE_BYEBYE            221             /*  服务关闭控制连接可以退出登录*/
#define FTP_SERVER_RETCODE_DATALINK_NODATA   225             /*  数据连接打开，无传输正在进行*/
#define FTP_SERVER_RETCODE_DATACLOSE_NOERR   226             /*  关闭数据连接, 文件操作成功  */
#define FTP_SERVER_RETCODE_INTO_PASV         227             /*  进入被动模式                */
#define FTP_SERVER_RETCODE_USER_LOGIN        230             /*  用户登录                    */
#define FTP_SERVER_RETCODE_FILE_OP_OK        250             /*  请求的文件操作完成          */
#define FTP_SERVER_RETCODE_MAKE_DIR_OK       257             /*  创建目录成功                */
#define FTP_SERVER_RETCODE_PW_REQ            331             /*  用户名正确，需要口令        */
#define FTP_SERVER_RETCODE_NEED_INFO         350             /*  请求需要进一步的信息        */
#define FTP_SERVER_RETCODE_DATALINK_FAILED   425             /*  不能打开数据连接            */
#define FTP_SERVER_RETCODE_DATALINK_ABORT    426             /*  关闭连接，中止传输          */
#define FTP_SERVER_RETCODE_REQ_NOT_RUN       450             /*  请求命令未执行              */
#define FTP_SERVER_RETCODE_REQ_ABORT         451             /*  中止请求的操作 本地有错     */
#define FTP_SERVER_RETCODE_DONOT_RUN_REQ     452             /*  未执行请求的操作,空间不足   */
#define FTP_SERVER_RETCODE_CMD_ERROR         500             /*  命令不可识别                */
#define FTP_SERVER_RETCODE_SYNTAX_ERR        501             /*  参数语法错误                */
#define FTP_SERVER_RETCODE_UNSUP_WITH_ARG    504             /*  此参数下的命令功能未实现    */
#define FTP_SERVER_RETCODE_LOGIN_FAILED      530             /*  用户登录失败                */
#define FTP_SERVER_RETCODE_REQ_FAILED        550             /*  未执行请求的操作            */
#define FTP_SERVER_RETCODE_DREQ_ABORT        551             /*  数据请求中止                */
#define FTP_SERVER_RETCODE_FILE_NAME_ERROR   553             /*  文件名不合法                */

#endif /* __FTP_CODE_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

