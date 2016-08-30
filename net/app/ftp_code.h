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
  ��������Ϣ
======================================================================*/
#define FTP_SERVER_MESSAGE  "Dfew FTP server ready."DFEW_KERNEL_VERSION
#define FTP_SERVER_SYSTYPE  "UNIX Type: L8"

/*======================================================================
  ��������Ӧ��
======================================================================*/
#define FTP_SERVER_RETCODE_RESTART_ACK       110             /*  �����������Ӧ��            */
#define FTP_SERVER_RETCODE_READY_MIN         120             /*  ������nnn������׼����       */
#define FTP_SERVER_RETCODE_DATALINK_READY    125             /*  ����������׼����            */
#define FTP_SERVER_RETCODE_FILE_OK           150             /*  �ļ�״̬���ã�����������  */
#define FTP_SERVER_RETCODE_CMD_OK            200             /*  ����ɹ�                    */
#define FTP_SERVER_RETCODE_CMD_UNSUPPORT     202             /*  ����δʵ��                  */
#define FTP_SERVER_RETCODE_STATUS_HELP       211             /*  ϵͳ״̬��ϵͳ������Ӧ      */
#define FTP_SERVER_RETCODE_DIR_STATUS        212             /*  Ŀ¼״̬                    */
#define FTP_SERVER_RETCODE_FILE_STATUS       213             /*  �ļ�״̬                    */
#define FTP_SERVER_RETCODE_HELP_INFO         214             /*  ������Ϣ, ���������û�����  */
#define FTP_SERVER_RETCODE_NAME_SYS_TYPE     215             /*  ����ϵͳ����                */
#define FTP_SERVER_RETCODE_READY             220             /*  ����������                  */
#define FTP_SERVER_RETCODE_BYEBYE            221             /*  ����رտ������ӿ����˳���¼*/
#define FTP_SERVER_RETCODE_DATALINK_NODATA   225             /*  �������Ӵ򿪣��޴������ڽ���*/
#define FTP_SERVER_RETCODE_DATACLOSE_NOERR   226             /*  �ر���������, �ļ������ɹ�  */
#define FTP_SERVER_RETCODE_INTO_PASV         227             /*  ���뱻��ģʽ                */
#define FTP_SERVER_RETCODE_USER_LOGIN        230             /*  �û���¼                    */
#define FTP_SERVER_RETCODE_FILE_OP_OK        250             /*  ������ļ��������          */
#define FTP_SERVER_RETCODE_MAKE_DIR_OK       257             /*  ����Ŀ¼�ɹ�                */
#define FTP_SERVER_RETCODE_PW_REQ            331             /*  �û�����ȷ����Ҫ����        */
#define FTP_SERVER_RETCODE_NEED_INFO         350             /*  ������Ҫ��һ������Ϣ        */
#define FTP_SERVER_RETCODE_DATALINK_FAILED   425             /*  ���ܴ���������            */
#define FTP_SERVER_RETCODE_DATALINK_ABORT    426             /*  �ر����ӣ���ֹ����          */
#define FTP_SERVER_RETCODE_REQ_NOT_RUN       450             /*  ��������δִ��              */
#define FTP_SERVER_RETCODE_REQ_ABORT         451             /*  ��ֹ����Ĳ��� �����д�     */
#define FTP_SERVER_RETCODE_DONOT_RUN_REQ     452             /*  δִ������Ĳ���,�ռ䲻��   */
#define FTP_SERVER_RETCODE_CMD_ERROR         500             /*  �����ʶ��                */
#define FTP_SERVER_RETCODE_SYNTAX_ERR        501             /*  �����﷨����                */
#define FTP_SERVER_RETCODE_UNSUP_WITH_ARG    504             /*  �˲����µ������δʵ��    */
#define FTP_SERVER_RETCODE_LOGIN_FAILED      530             /*  �û���¼ʧ��                */
#define FTP_SERVER_RETCODE_REQ_FAILED        550             /*  δִ������Ĳ���            */
#define FTP_SERVER_RETCODE_DREQ_ABORT        551             /*  ����������ֹ                */
#define FTP_SERVER_RETCODE_FILE_NAME_ERROR   553             /*  �ļ������Ϸ�                */

#endif /* __FTP_CODE_H__ */

/*==============================================================================
** FILE END
==============================================================================*/

