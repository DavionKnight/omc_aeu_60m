/********************  COPYRIGHT(C)  ***************************************
**--------------文件信息--------------------------------------------------------
**文   件   名: modem.c
**创   建   人: 于宏图
**创 建  日 期: 
**程序开发环境：
**描        述: Modem Gprs数据通讯程序
**--------------历史版本信息----------------------------------------------------
** 创建人: 于宏图
** 版  本: v1.0
** 日　期: 
** 描　述: 原始版本
**--------------当前版本修订----------------------------------------------------
** 修改人:
** 版  本:
** 日　期:
** 描　述:
**------------------------------------------------------------------------------
**
*******************************************************************************/
#include "../../common/commonfun.h"
#include "../../common/druheader.h"
#include "../../protocol/approtocol.h"
#include "../../protocol/apbprotocol.h"
#include "../../protocol/apcprotocol.h"
#include "modem.h"

extern int OpenCommPort(int fd, int comport);
extern int SetCommState(int fd, int nSpeed, int nBits, char nEvent, int nStop);
extern int DbGetThisStrPara(unsigned int objectid, char *buf);
extern INT32U GetSelfIp(char *ethname);
extern void OMCDevParaDeal(void);
extern void ApPackTransimtUnPack(APPack_t *p_packbuf, ComBuf_t *p_combuf);
extern void ApPackTransimtPack(APPack_t *p_packbuf, ComBuf_t *p_combuf);

extern int g_OMCPackNo, g_DevType;
extern DevicePara_t g_DevicePara;
extern ComBuf_t g_OMCCom, g_Ir_OMCTranCom, g_OMC_IrTranCom, rs485_omc_combuf, omc_rs485_combuf;

ComBuf_t g_SmsCom, g_Sms_Net_Buf, g_Net_Sms_Buf;
SelfThread_t g_GprsComThread;

//pppd file gprs 
//判断PPP0已经连接:用ioctl来获取ppp0的地址;
//看的/var/lock/***（有一个是pppd的进程ID锁）问题就是,当我拨号成功拔掉电话线后,所有的这些要一/两分钟后才消失

/*******************************************************************************
*函数名称 : int ModemUartReceiveData(ComBuf_t *p_combuf, int waittime)
*功    能 : 接收的数据转存到p_combuf.Buf数据缓存区中,并处理,超时时间waittime
*输入参数 : ComBuf_t *p_combuf:对应设备参数;int waittime:超时时间ms
*输出参数 : 接收到数据长度或错误标识
*******************************************************************************/
int ModemUartReceiveData(ComBuf_t *p_combuf, int waittime)
{
int res, rcsum;
fd_set readfs;
struct timeval tv;

	rcsum = 0;
_SMSRERECVFLAG:
  tv.tv_sec = waittime/1000;//SECOND
  tv.tv_usec = (waittime%1000)*1000;//USECOND
  FD_ZERO(&readfs);
  FD_SET(p_combuf->Fd, &readfs);

  res = select(p_combuf->Fd + 1, &readfs, NULL, NULL, &tv);
  if (res > 0)
  {
    rcsum = read(p_combuf->Fd, &p_combuf->Buf[p_combuf->RecvLen], (COMBUF_SIZE - p_combuf->RecvLen));
    if (rcsum > 0)
    {
		p_combuf->RecvLen = p_combuf->RecvLen + rcsum;
		waittime = 10;
		goto _SMSRERECVFLAG;//重新接收等待结束
    }
    else if (rcsum < 0)
    {
      perror("UartReceiveData:read() error!");
      return -1;
    }
  }
  else if (res < 0)
  {
    perror("UartReceiveData:select() error!");
    return -1;
  }
  return p_combuf->RecvLen;
}

/*******************************************************************************
*函数名称 : int ModemUartSendData(int fd, char *sbuf, int len)
*功    能 : 串口fd 发送数据
*输入参数 : 串口fd,sbuf,长度
*输出参数 : 读操作成功返回1,否则返回-1
*******************************************************************************/
int ModemUartSendData(int fd, char *sbuf, int len)
{
int sr;

  sr = write(fd, sbuf, len);
  if(sr == -1)
  {
    printf("Write sbuf error!\r\n");
    return -1;
  }
  return 1;
}

/*******************************************************************************
*函数名称 : int ModemAtChat(ComBuf_t *p_combuf, char *atcmd, char *rc, int timeo)
*功    能 : Modem AT指令对话函数
*输入参数 : ComBuf_t *p_combuf:Modem对应设备参数;char *atcmd:at指令;char *rc:at指令返回值;int timeo:等待超时时间ms
*输出参数 : 接收数据包含at指令返回值,返回1;否则返回-1
*******************************************************************************/
int ModemAtChat(ComBuf_t *p_combuf, char *atcmd, char *rc, int timeo)
{
int rcnum;

  DEBUGOUT("Send:%s\r\n", atcmd);
  memset(p_combuf->Buf, 0, COMBUF_SIZE);
  p_combuf->RecvLen = 0;
  usleep(1000);
  ModemUartSendData(p_combuf->Fd, atcmd, strlen(atcmd));
  ComStrWriteLog(atcmd, strlen(atcmd));//存日志

  rcnum = ModemUartReceiveData(p_combuf, timeo);
  if (rcnum > 0)
  {
    DEBUGOUT("%s\r\n", p_combuf->Buf);
    p_combuf->Timer = 0;
    ComStrWriteLog(p_combuf->Buf, p_combuf->RecvLen);//存日志
    if (strstr(p_combuf->Buf, rc) != 0)//接收到rc所指字符串的应答
    {
      return  1;
    }
    else if (strstr(p_combuf->Buf, "ERROR") != 0)//接收到错误的应答
    {
      return  -1;
    }
  }
  usleep(1000);
  DEBUGOUT("AtChat TimeOut!!\r\n");
  return -2;
}

/*******************************************************************************
*函数名称 : int ModemAutoBaud(ComBuf_t *p_combuf)
*功    能 : 通讯Modem,自动调整Modem串口波特率为115200
*输入参数 : ComBuf_t *p_combuf:Modem对应设备参数
*输出参数 : 成功返回bps;失败返回-1
*******************************************************************************/
int ModemAutoBaud(ComBuf_t *p_combuf)
{
int   i;
int   bps[2] = {115200,9600};
char  sdbuf[100];

  strcpy(sdbuf, "AT+CFUN=1\r");//Modem重启
  ModemAtChat(p_combuf, sdbuf, "OK\r\n", 6000);
  sleep(10);
  for (i = 0 ; i < 2; i++)
  {
    SetCommState(p_combuf->Fd, bps[i], 8, 'N', 1);//设置串口设备
    sleep(1);
    //AT
    memset(sdbuf, 0, sizeof(sdbuf));
    strcpy(sdbuf, "AT\r");//Modem重启
    if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) == 1)
    {
      DEBUGOUT("Modem bps:%d\r\n", bps[i]);
      if (bps[i] != 115200)
      {
        memset(sdbuf, 0, sizeof(sdbuf));
        strcpy(sdbuf, "AT+IPR=115200\r");
        if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) == 1)
        {
          //AT&W保存
          memset(sdbuf, 0, sizeof(sdbuf));
          strcpy(sdbuf, "AT&W\r");
          if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) == 1)
          {
            DEBUGOUT("Set Modem bps:115200!\r\n");
            return  115200;
          }
          else
          {
            DEBUGOUT("AT&W Error!\r\n");
            return  -3;
          }
        }
        else
        {
          DEBUGOUT("AT+IPR=115200 Error!\r\n");
          return  -2;
        }
      }
      else
        return  bps[i];
    }
  }
  DEBUGOUT("Modem bps set Failed!\r\n");
  return  -1;
}

/*******************************************************************************
*函数名称 : int ModemSmsModeInit(ComBuf_t *p_combuf, char *smsctel)
*功    能 : Modem短信模式配置初始化函数
*输入参数 : ComBuf_t *p_combuf:Modem对应设备参数;char *smsctel:短信服务中心号码
*输出参数 : None
*******************************************************************************/
int ModemSmsModeInit(ComBuf_t *p_combuf, char *smsctel)
{
char  sdbuf[100];

  //AT
  memset(sdbuf, 0, sizeof(sdbuf));
  strcpy(sdbuf, "AT\r");
  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
    return -1;
    
	ModemReadCI(p_combuf);
	
  //Characters are not echoed回显功能关闭
  memset(sdbuf, 0, sizeof(sdbuf));
  strcpy(sdbuf, "ATE0\r");
  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
    return -1;

  //短信中心设置
  /*
  memset(sdbuf, 0, sizeof(sdbuf));
  sprintf(sdbuf, "AT+CSCA=+86%s\r", smsctel);
  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
    return -1;
  */
  if(get_net_group() == 0x331){
	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT+CPMS=\"SM\",\"SM\",\"SM\"\r");
	  if (ModemAtChat(p_combuf, sdbuf, "+CPMS:", 3000) < 0)
		  return -1;
	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT+CNMI=1,2,0,1,0\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;

	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT^RSSIREP=0\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;

	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT^PPPCFG=\"CARD\",\"CARD\"\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;

  }else{
	  //Set TEXT mode
	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT+CMGF=1\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;

	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT+CNMI=2,2,0,0,0\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;

	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT+ICF=3,4\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;

	  //AT&W保存
	  memset(sdbuf, 0, sizeof(sdbuf));
	  strcpy(sdbuf, "AT&W\r");
	  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 3000) < 0)
		  return -1;
  }	

  //All messages are deleted
  memset(sdbuf, 0, sizeof(sdbuf));
  strcpy(sdbuf, "AT+CMGD=1,4\r");
  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 10000) < 0)//等待时间较长
    return -1;

  return 1;
}

/*******************************************************************************
*函数名称 : void ModemReadCI(ComBuf_t *p_combuf)
*功    能 : Modem读取信源小区CI
*输入参数 : ComBuf_t *p_combuf:Modem对应设备参数
*输出参数 : None
*******************************************************************************/
void ModemReadCI(ComBuf_t *p_combuf)
{
char  atcmd[100], *ptr, *ptr1;
int i, rcnum, ci;

  memset(atcmd, 0, sizeof(atcmd));
  sprintf(atcmd, "AT+CCED=0,1\r");
  ModemUartSendData(p_combuf->Fd, atcmd, strlen(atcmd));
  ComStrWriteLog(atcmd, strlen(atcmd));//存日志
  sleep(3);

  rcnum = ModemUartReceiveData(p_combuf, 6000);
  if (rcnum > 0)
  {
    DEBUGOUT("%s\r\n", p_combuf->Buf);
    ComStrWriteLog(p_combuf->Buf, p_combuf->RecvLen);//存日志
    ptr = strstr(p_combuf->Buf, "+CCED:");
    if (ptr != NULL)
    {
    	ptr = strchr(ptr, ',');
    	ptr = strchr((ptr+1), ',');
    	ptr = strchr((ptr+1), ',');
    	ptr1 = strchr((ptr+1), ',');
    	memset(atcmd, 0, sizeof(atcmd));
    	memcpy(atcmd, (ptr+1), (ptr1-ptr-1));
	    ci = atoi(atcmd);
	    DEBUGOUT("信源小区识别码:%d\r\n", ci);
	    DbSaveThisIntPara(SOURCECI_ID, ci);//存储升级过程中的升级过程中,产生的参数
    }
  }
}

/*******************************************************************************
*函数名称 : int ModemInit(void)
*功    能 : 通讯Modem初始化配置函数
*输入参数 : none
*输出参数 : 成功p_combuf->Fd;失败返回-1
*******************************************************************************/
int ModemInit(void)
{
ComBuf_t *p_combuf;

	p_combuf = &g_SmsCom;
  p_combuf->Fd = -1;
  p_combuf->Timer = 0;
  p_combuf->Status = 0;
  p_combuf->RecvLen = 0;
  memset(p_combuf->Buf, 0, COMBUF_SIZE);
  
	//串口设置
  p_combuf->Fd = OpenCommPort(p_combuf->Fd, MODEMUART);
  if (p_combuf->Fd < 0)//打开设备失败
  	return -1;
  //自动检测Modem串口波特率
  system(GPRS_DIALOFF);
  ModemAutoBaud(p_combuf);
  sleep(1);
  SetCommState(p_combuf->Fd, 115200, 8, 'N', 1);//设置串口设备

  if (ModemSmsModeInit(p_combuf, g_DevicePara.SmscTel) < 0)
	{
    printf("SMS Init Error!\r\n");
    p_combuf->Status = MODEM_ERROR;
  }
  else
  {
    printf("SMS Init OK!\r\n");
    p_combuf->Timer = 0;
    p_combuf->RecvLen = 0;
    p_combuf->Status = MODEM_SMS;
    memset(p_combuf->Buf, 0, COMBUF_SIZE);
  }

	GprsComThreadStart();
  sleep(1);
  return p_combuf->Fd;
}

/*******************************************************************************
*函数名称 : int GprsModeSendSms(ComBuf_t *p_combuf, char *tel, APPack_t *p_packbuf)
*功    能 : gprs模式下发送短信
*输入参数 : ComBuf_t *p_combuf:Modem对应设备参数;char *tel:电话号码,APPack_t *p_packbuf:待发送数据
*输出参数 : 成功返回1,错误返回-1
*******************************************************************************/
int GprsModeSendSms(ComBuf_t *p_combuf, char *tel, APPack_t *p_packbuf)
{
char  sdbuf[100];

  //发送+++,Switch from online to offline mode
  memset(sdbuf, 0, sizeof(sdbuf));
  strcpy(sdbuf, "+++\r");
  if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 1000) == 1)
  {
    ModemSendPackSms(p_combuf, tel, p_packbuf);
    sleep(10);
    //发送ATO,Switch from offline to online mode
    strcpy(sdbuf, "ATO\r");
    if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 1000) == 1)
      return  1;//切换到online mode模式
    else
      return  2;//未切换回到online mode模式,offline mode模式
  }
  else
    return  -1;
}

/*******************************************************************************
*函数名称 : int ModemSendPackSms(ComBuf_t *p_combuf, char *tel, APPack_t *p_packbuf)
*功    能 : 短信发送
*输入参数 : ComBuf_t *p_combuf:Modem对应设备参数;char *tel:电话号码;APPack_t *p_packbuf:待发送数据
*输出参数 : 成功返回1,错误返回-1
*******************************************************************************/
int ModemSendPackSms(ComBuf_t *p_combuf, char *tel, APPack_t *p_packbuf)
{
char  cmdbuf[100], sdbuf[COMBUF_SIZE];
int   sdsum, rcsum, j;
  
  if (GetSelfIp("ppp0") != 0)//gprs模式下
 	{
 		DEBUGOUT("退出 ppp0 状态\r\n");
 		GprsDisconnet(&g_OMCCom);
//  	//发送+++,Switch from online to offline mode
//  	memset(sdbuf, 0, sizeof(sdbuf));
//  	strcpy(sdbuf, "+++\r");
//  	if (ModemAtChat(p_combuf, sdbuf, "OK\r\n", 1000) < 0)
//  	{
//  		DEBUGOUT("GprsMode Send +++ Error!\r\n");
//			return -1;
//		}
 	}
  sdsum = 0;
  memset(sdbuf, 0, sizeof(sdbuf));
  if (p_packbuf->APType == AP_B)
  {
  	sdsum = APBPack(p_packbuf, sdbuf);
  }
  else
  {
    DEBUGOUT("APType Error!\r\n");
    ClearAPPackBuf(p_packbuf);//清除包
    return  -1;
  }
  if (sdsum > 0)
  {
	  if(get_net_group() != 0x331){
		  for (j = 0; j < 50; j++)
		  {
			  sprintf(cmdbuf, "AT+CPAS\r", tel);
			  if (ModemAtChat(p_combuf, cmdbuf, "0", 1000) == 1)//接收到正确的应答,等待
				  break;
			  sleep(1);
		  }
	  }
	  if(get_net_group() == 0x331){ // 电信
		  sprintf(cmdbuf, "AT^HCMGS=\"%s\"\r", tel);
	  }else{
		  sprintf(cmdbuf, "AT+CMGS=\"%s\"\r", tel);
	  }
    if (ModemAtChat(p_combuf, cmdbuf, ">", 1000) == 1)//接收到正确的应答,等待
    {
      sdbuf[sdsum] = 0x1A; //发送短信标识
      sdsum++;
      ComDataWriteLog(sdbuf, sdsum);//存日志
      DEBUGOUT("Send:%s", sdbuf);
      //通过串口向modem发送数据
      ClearComBuf(p_combuf);
      ModemUartSendData(p_combuf->Fd, sdbuf, sdsum);
      sleep(4);
	  	for (j = 0; j < 5; j++)
	  	{
	  		rcsum = ModemUartReceiveData(p_combuf, 5000);
  			if (rcsum > 0)//判断是否有发送短信后的正确应答
  			{
				DEBUGOUT("Modem Send Result:%s\r\n", p_combuf->Buf);//接收到正确的应答,等待
				if(get_net_group() == 0x331){
					if (strstr(p_combuf->Buf, "^HCMGSS:") != NULL)
						break;
					if (strstr(p_combuf->Buf, "^HCMGSF:") != NULL)
						return -1;
				}else{
					if (strstr(p_combuf->Buf, "+CMGS:") != NULL)
					{
						if (strstr(p_combuf->Buf, "OK") != NULL)
							break;
					}
				}
  			}
  			sdbuf[0] = 0x1A; //发送短信结束标识
  			ModemUartSendData(p_combuf->Fd, sdbuf, 1);
	  		sleep(1);
	  	}
      return 1;
    }
  }
  DEBUGOUT("Send Data Error!\r\n");
  return -1;
}

void send_cnma(ComBuf_t * p_combuf)
{
	ComBuf_t st_tmp;
	memcpy(&st_tmp, p_combuf, sizeof(ComBuf_t));
	ModemAtChat(&st_tmp, "AT+CNMA\r", "OK\r\n", 3000);
}
/*******************************************************************************
*函数名称 : int ModemSmsReceiveData(char *tel)
*功    能 : 主单元通过短信猫方式与OMC通讯
*输入参数 : 根据接受短信返回短信电话
*输出参数 : 有短信返回短信内容,否则返回0
*******************************************************************************/ 
int ModemSmsReceiveData(ComBuf_t *p_combuf, char *tel)
{
char *ptr, *ptr1;
int i, rcsum;
DevicePara_t *p_devpara;



  p_devpara = &g_DevicePara;

  //本地监控接收
  //p_combuf->Status = MODEM_SMS;
  rcsum = ModemUartReceiveData(p_combuf, 5000);
  if (rcsum > 0)//判断是否有短信接入
	{
	  //+CMT: "+8615110003614",,"14/04/15,14:36:01+32"
    //内容,AT+CMGS="13391601848"
    //提取电话号码,进行电话号码鉴权
    DEBUGOUT("Modem Sms:%s\r\n", p_combuf->Buf);//经过鉴权短信号码错误
    if(strstr(p_combuf->Buf, "+CMTI:") != NULL)
    {
		if(get_net_group() == 0x331){
			send_cnma(p_combuf);
			ModemAtChat(p_combuf, "AT+CNMI=1,2,0,1,0\r", "OK\r\n", 3000);
			DEBUGOUT("Modem Set Error!\r\n");//设置错误,重新设置
			ClearComBuf(p_combuf);
		}else{
			ModemAtChat(p_combuf, "AT+CNMI=2,2,0,0,0\r", "OK\r\n", 3000);
			DEBUGOUT("Modem Set Error!\r\n");//设置错误,重新设置
			ClearComBuf(p_combuf);
		}
		  return 0;
  	}

	if(get_net_group() == 0x331){
		ptr = strstr(p_combuf->Buf, "^HCMT:");
		send_cnma(p_combuf);
	}else{
		ptr = strstr(p_combuf->Buf, "+CMT:");
	}
    if (ptr != NULL)
    {
		if(get_net_group() == 0x331){
			ptr = strchr(ptr, ':');
			ptr1 = strchr((ptr+1), ',');
		}else{
			ptr = strchr(ptr, '\"');
			ptr1 = strchr((ptr+1), '\"');
		}
		memcpy(tel, (ptr+1), (ptr1-ptr-1));
		DEBUGOUT("TEL_NUM: %s\n", tel);	
		//电话号码鉴权
	    //if (ptr == NULL)
	    {
			ptr = NULL;
		    for (i = 0; i < 5; i++)
			{
				if(p_devpara->QueryTel[i][0] != 0){
					ptr = strstr(tel, p_devpara->QueryTel[i]);
					if (ptr != NULL)
						return rcsum;
				}
		    }
		  }
		  //无效电话号码
		  if (ptr == NULL)
		  {
				DEBUGOUT("Modem Sms Tel Error!\r\n");//经过鉴权短信号码错误
				ClearComBuf(p_combuf);
		  	return 0;
		  }
    }
    else
    {
    	ClearComBuf(p_combuf);
    	return 0;
    }
	}
	return rcsum;
}

/*******************************************************************************
*函数名称 : void GprsDialOff(void)
*功    能 : Modem Gprs通讯拨号函数
*输入参数 : 参数表名称
*输出参数 : 成功返回1
*******************************************************************************/
void GprsDialOff(void)
{
  printf("Gprs Dial Off ...\n");
	system(GPRS_DIALOFF);
	sleep(1);
}

/*******************************************************************************
*函数名称 : int GprsDial(void)
*功    能 : Modem Gprs通讯拨号函数
*输入参数 : 参数表名称
*输出参数 : 成功返回1
*******************************************************************************/
int GprsDial(void)
{
int   setflag;
char  *pbuf, buf[100], var[100], syscmd[150];
FILE  *fp = NULL, *tmfp = NULL;

	GprsDialOff();
	printf("Start Gprs Dial ...\n");
  //判断相关拨号文件是否存在
  sprintf(syscmd, "%s", GPRS_PPPSCRIPT);
  if (access(syscmd, F_OK) != 0)
  {
    perror(GPRS_PPPSCRIPT);
    return -1;
  }
  usleep(1000);
  sprintf(syscmd, "%s", GPRS_CONNECT);
  if (access(syscmd, F_OK) != 0)
  {
    perror(GPRS_CONNECT);
    return -1;
  }
  usleep(1000);
  sprintf(syscmd, "%s", GPRS_DISCONNECT);
  if (access(syscmd, F_OK) != 0)
  {
    perror("gprs-disconnect");
    return -1;
  }
  usleep(1000);
	//根据设置,修改gprs-connect文件中的APN参数
  fp = fopen(GPRS_CONNECT, "r");
  if (fp == NULL) return -1;
  sprintf(syscmd, "%s", GPRS_TMPFILE);
  tmfp = fopen(syscmd, "w");
  if (tmfp == NULL)
  {
    fclose(fp);
    return -1;
  }

  setflag = 0;
  while(!feof(fp))
  {
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), fp);//逐行读取文件
    pbuf = strstr(buf, "AT+CGDCONT=1");//是否有字符串"AT+CGDCONT=1"
    if (pbuf != NULL)
    {
		  //修改gprs-connect文件对应APN参数
		  memset(var, 0, sizeof(var));
		  DbGetThisStrPara(GPRSAPN_ID, var);
      if (strstr(buf, var) == NULL)//文件中未设置
      {
      	setflag = 1;
      }
      memset(buf, 0, sizeof(buf));
      sprintf(buf, "OK   'AT+CGDCONT=1,\"IP\",\"%s\"\' \\\n", var);
    }
    fwrite(buf, 1, strlen(buf), tmfp);//写到临时文件中
  }
  fclose(fp);
  fclose(tmfp);
  if (setflag == 1)
  {
  	sprintf(syscmd, "cp %s %s", GPRS_TMPFILE, GPRS_CONNECT);//用新文件替换老文件
  	system(syscmd);
  }
  sprintf(syscmd, "rm %s", GPRS_TMPFILE);
  system(syscmd);//删除临时文件
  
  //根据设置,修改gprs-gsm文件中的用户名和密码参数
	fp = fopen(GPRS_PPPSCRIPT, "r");
  if (fp == NULL) return -1;
  sprintf(syscmd, "%s", GPRS_TMPFILE);
  tmfp = fopen(syscmd, "w");
  if (tmfp == NULL)
  {
    fclose(fp);
    return -1;
  }
  setflag = 0;
  while(!feof(fp))
  {
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), fp);//逐行读取文件
    //user设置
    if (str1nstr2(buf, "user", sizeof("user")) == 0)
    {
		  memset(var, 0, sizeof(var));
		  DbGetThisStrPara(GPRSUSER_ID, var);
      if (strstr(buf, var) == NULL)//文件中未设置
      {
      	setflag = 1;
      }
      memset(buf, 0, sizeof(buf));
      sprintf(buf, "user %s \n", var);
    }
    //password设置
    if (str1nstr2(buf, "password", sizeof("password")) == 0)
    {
			memset(var, 0, sizeof(var));
			DbGetThisStrPara(GPRSPASSWORD_ID, var);
	    if (strstr(buf, var) == NULL)//文件中未设置
	    {	
	      setflag = 1;
	    }
	    memset(buf, 0, sizeof(buf));
	    sprintf(buf, "password %s \n", var);
    }
    fwrite(buf, 1, strlen(buf), tmfp);//写到临时文件中
  }
  fclose(fp);
  fclose(tmfp);
  if (setflag == 1)
  {
  	sprintf(syscmd, "cp %s %s", GPRS_TMPFILE, GPRS_PPPSCRIPT);//用新文件替换老文件
  	system(syscmd);
  }
  sprintf(syscmd, "rm %s", GPRS_TMPFILE);
  system(syscmd);//删除临时文件
  
  sprintf(syscmd, "chmod +x -R %s", GPRS_PPPDIR);
  system(syscmd);
  sleep(3);

	system(ROUTE_OFF);
  system(GPRS_DIALON);
  return 1;
}

/*******************************************************************************
*函数名称 : void GprsDisconnet(ComBuf_t *pcombuf)
*功    能 : 断开Gprs连接函数
*输入参数 : ComBuf_t *pcombuf
*输出参数 : none
*******************************************************************************/
void GprsDisconnet(ComBuf_t *pcombuf)
{
  //运行过程中断开连接
  if (pcombuf->Fd > 0)
  {
    printf("断开与服务器的连接!\r\n");
    close(pcombuf->Fd);
  }
  pcombuf->Fd = -1;
  pcombuf->Timer = 0;
  pcombuf->Status = GPRS_DISCONNET;
  pcombuf->RecvLen = 0;
	memset(pcombuf->Buf, 0, COMBUF_SIZE);
	
	if (GetSelfIp("ppp0") != 0)//判断是否有gprs ppp连接
  {
    printf("断开Gprs连接!\r\n");
    GprsDialOff();
    sleep(3);
    system(RM_PPP0_PID);	//move ppp0.pid file
    system(RM_LCK_TTY);		//move LCK..TTY file
    sleep(3);
  }
}

/*******************************************************************************
*函数名称 : void GprsComThreadInit(void)
*功    能 : Gprs网络通讯线程初始化
*输入参数 : none
*输出参数 : none
*******************************************************************************/ 
void GprsComThreadInit(void)
{
  g_GprsComThread.Tid = 0;
  g_GprsComThread.ThreadStatus = THREAD_STATUS_EXIT;
}

/*******************************************************************************
*函数名称 : void GprsComThreadStart(void)
*功    能 : 创建Gprs网络通讯线程
*输入参数 : none
*输出参数 : none
*******************************************************************************/ 
void GprsComThreadStart(void)
{
  if (g_GprsComThread.ThreadStatus != THREAD_STATUS_RUNNING)
  {
    pthread_create(&g_GprsComThread.Tid, NULL, GprsCom_Thread, NULL);
    g_GprsComThread.ThreadStatus = THREAD_STATUS_RUNNING;
    printf("GprsCom_Thread ID: %lu.\n", g_GprsComThread.Tid);
  }
}

/*******************************************************************************
*函数名称 : void GprsComThreadStop(void)
*功    能 : 停止Gprs网络通讯线程
*输入参数 : none
*输出参数 : none
*******************************************************************************/ 
void GprsComThreadStop(void)
{
  if (g_GprsComThread.ThreadStatus != THREAD_STATUS_EXIT)
  {
    GprsDisconnet(&g_OMCCom);
    pthread_cancel(g_GprsComThread.Tid);
    g_GprsComThread.Tid = 0;
    g_GprsComThread.ThreadStatus = THREAD_STATUS_EXIT;
    printf("GprsCom_Thread Cancel!\r\n");
  }
}

/*******************************************************************************
*函数名称 : void *GprsCom_Thread(void *pvoid)
*功    能 : Gprs网络通讯线程
*输入参数 : none
*输出参数 : none
*******************************************************************************/ 
void *GprsCom_Thread(void *pvoid)
{
DevicePara_t *p_devpara;
ComBuf_t *p_combuf;
APPack_t PackBuf, *p_packbuf;
DevInfo_t DevInfo, *p_devinfo;
int dialsum, resum, res;
time_t starttime, dialtime;
char smstel[20], smssettel[20];
DevInfo_t devinfo;

  pthread_detach(pthread_self());
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);//线程设置为立即取消

  printf("GprsCom_Thread Run!\r\n");
  dialsum = 0;
  starttime = time(NULL)-DIALINTERVAL+3;
  memset(&devinfo, 0, sizeof(DevInfo_t));
  LoadDevicePara(&devinfo, &g_DevicePara);
  p_devpara = &g_DevicePara;
	//gprs模式下的拨号连接维护
	if ((p_devpara->ComModemType == GSM_MODEM) && (p_devpara->DeviceCommType == PS_MODE))
	{
		g_SmsCom.Status = GPRS_DIAL;
		g_OMCCom.Status = NET_NULL;
	}
	memset(smstel, 0, sizeof(smstel));
	memset(smssettel, 0, sizeof(smssettel));
  
  while(1)
  {
  	memset(&devinfo, 0, sizeof(DevInfo_t));
  	LoadDevicePara(&devinfo, &g_DevicePara);
    p_devpara = &g_DevicePara;
    p_combuf = &g_SmsCom;
    p_packbuf = &PackBuf;
    p_devinfo = &DevInfo;
    dialtime = (int)(time(NULL) - starttime);

	  if ((p_devpara->ComModemType == GSM_MODEM) && (p_devpara->DeviceCommType == SMS_MODE))
    {
    	SocketClientDisconnet(&g_OMCCom);
    	g_OMCCom.Status = NET_NULL;
    	dialsum = 0;
    	starttime = time(NULL)-DIALINTERVAL+3;
    	if (GetSelfIp("ppp0") != 0)//ppp已经拨号
    	{
    		GprsDialOff();
    	}
//    	if (g_Ir_OMCTranCom.RecvLen > 0)//IR协议转发透传
//      {
//        DEBUGOUT("MODEM Send SMS: Ir->OMC TransimtPack!\r\n");
//        ApPackTransimtUnPack(p_packbuf, &g_Ir_OMCTranCom);
//        //从机上报命令
//        if (p_packbuf->CommandFlag == COMMAND_REPORT)
//        {
//    			memcpy(smstel, p_devpara->NotifyTel, 20);
//        }
//				ModemSendPackSms(p_combuf, smstel, p_packbuf);
//				memset(smstel, 0, sizeof(smstel));
//				ClearComBuf(p_combuf);
//      }
      if (rs485_omc_combuf.RecvLen > 0)//RS485通讯透传
      {
      	DEBUGOUT("MODEM Send SMS: RS485->OMC TransimtPack!\r\n");
        ComDataHexDis(rs485_omc_combuf.Buf, rs485_omc_combuf.RecvLen);
        ApPackTransimtUnPack(p_packbuf, &rs485_omc_combuf);
        //从机上报命令
        if (p_packbuf->CommandFlag == COMMAND_REPORT)
        {
    			memcpy(smstel, p_devpara->NotifyTel, 20);
        }
				ModemSendPackSms(p_combuf, smstel, p_packbuf);
				memset(smstel, 0, sizeof(smstel));
				ClearComBuf(p_combuf);
      }
    }
    
    if (g_Net_Sms_Buf.RecvLen > 0)//网络通讯模式下的告警
    {
    	DEBUGOUT("MODEM Send SMS: Ir->OMC TransimtPack!\r\n");
        ApPackTransimtUnPack(p_packbuf, &g_Net_Sms_Buf);
     	if ((p_packbuf->APType == AP_A) || (p_packbuf->APType == AP_C))
     	{
			p_packbuf->StartFlag = '!';
     		p_packbuf->APType = AP_B;//转换为APB
			p_packbuf->EndFlag = '!';
     	}
      if (p_packbuf->CommandFlag == COMMAND_REPORT)
      {
    		memcpy(smstel, p_devpara->NotifyTel, 20);
      }
			else
			{
				memcpy(smstel, smssettel, 20);
			}
			ModemSendPackSms(p_combuf, smstel, p_packbuf);
			memset(smstel, 0, sizeof(smstel));
			ClearComBuf(p_combuf);
    }

		//短信通讯处理
		resum = 0;
 		resum = ModemSmsReceiveData(p_combuf, smstel);
    if (resum > 0)
    {
    //if (strstr(smstel, p_devpara->NotifyTel) == NULL)
    //	{
    		memcpy(smssettel, smstel, 20);//使用的设置查询电话号码
    //	}
			res = APBUnpack(p_combuf->Buf, p_combuf->RecvLen, p_packbuf);
      DEBUGOUT("GSM MODEM Receive SMS WriteLogBook...................................\r\n");
      ComStrWriteLog(p_combuf->Buf, p_combuf->RecvLen);//存日志
      if (res > 0)
      {
        GetDevInfo(p_devinfo, p_packbuf);
        if (p_devinfo->DeviceNo == p_devpara->DeviceNo)//本设备数据
        {
        	if (p_devinfo->ModuleType != 0x00)//透传,转RS485通讯
        	{
          	DEBUGOUT("OMC->RS485 TransimtPack!\r\n");
          	ApPackTransimtPack(p_packbuf, &omc_rs485_combuf);
        	}
	        else
	        {
	          if (APProcess(p_packbuf, p_devpara) > 0)
	          {
	            if (p_packbuf->PackLen > 0)
	            {
	            	p_combuf->Timer = 0;
							  ModemSendPackSms(p_combuf, smstel, p_packbuf);
							  memset(smstel, 0, sizeof(smstel));
	            }
	          }
	        }
        }
        else//通过IR协议转发
        {
          if (g_DevType == MAIN_UNIT)
          {
            DEBUGOUT("OMC->Ir TransimtPack!\r\n");
            ApPackTransimtPack(p_packbuf, &g_OMC_IrTranCom);
          }
        }
        ClearComBuf(p_combuf);
      }
      else
      {
        DEBUGOUT("OMC_MODEM_Com APB Unpacked Error!\r\n");//存错误数据包标识
        ClearComBuf(p_combuf);
      }
	  }
	  //gprs模式下的拨号连接维护

    if ((p_devpara->ComModemType == GSM_MODEM) && (p_devpara->DeviceCommType == PS_MODE))
   	{
      if (GetSelfIp("ppp0") == 0)//判断拨号是否成功,不成功
      {
      	g_OMCCom.Status = NET_NULL;
      	if (dialtime > DIALINTERVAL)
      	{
      		starttime = time(NULL);
         	//dialsum++;
          if (g_SmsCom.Status == GPRS_SMS)
          {
          	g_SmsCom.Status = GPRS_DIAL;
          }
          else
          {
          	dialsum++;
	          if (dialsum == CONNETFAILSUM)//“拨号失败”上报是一次性的上报,当上报失败后,设备不需要自动重发
	          {
	            //短信发送,拨号连接失败
	            memset(p_devinfo, 0, sizeof(DevInfo_t));
		          p_devinfo->StationNo = p_devpara->StationNo;
		          p_devinfo->DeviceNo = p_devpara->DeviceNo;
		          ReportParaPack(AP_B, p_devinfo, REPORT_PSLOGIN_FAIL, g_OMCPackNo++, p_packbuf);
		          ModemSendPackSms(&g_SmsCom, p_devpara->NotifyTel, p_packbuf);
		          DEBUGOUT("Gprs Dial:Fail...\r\n");
		          ClearAPPackBuf(p_packbuf);
		          g_SmsCom.Status = GPRS_SMS;
	          }
	          else if (dialsum == (CONNETFAILSUM+2))//“拨号失败”上报是一次性的上报,当上报失败后,设备不需要自动重发
	          {
	          	dialsum = CONNETFAILSUM;
	          	g_SmsCom.Status = GPRS_SMS;
	          }
	          else
	          {
	          	GprsDial();
	          	g_SmsCom.Status = GPRS_DIAL;
	          }
          }
        }
      }
      else if(g_SmsCom.Status == GPRS_DIAL)
      {
      	//判断ppp0是否为默认路由
				system(ROUTE_ON);
				SocketClientDisconnet(&g_OMCCom);
				dialsum = CONNETFAILSUM;
				g_SmsCom.Status = GPRS_DIALOK;
			}
			else if(g_SmsCom.Status == GPRS_SMS)
			{
				g_OMCCom.Status = NET_NULL;
			  if (GetSelfIp("ppp0") != 0)//ppp已经拨号
		    {
		    	GprsDialOff();
		    }
		    sleep(1);
			if(get_net_group() != 0x331){
				ModemUartSendData(p_combuf->Fd, "AT+CMGL=\"REC UNREAD\"\r", strlen("AT+CMGL=\"REC UNREAD\""));
			}
		    starttime = time(NULL);
		    g_SmsCom.Status = GPRS_DIAL;
		  }
    }
   	sleep(3);
  }
  g_GprsComThread.ThreadStatus = THREAD_STATUS_EXIT;
  g_GprsComThread.Tid = 0;
  pthread_exit(NULL);
}

/*******************************************************************************
*函数名称 : int str1nstr2(char *s1, char *s2, int n) 
*功    能 : 在字符串s1第一个非空字符开始查找s2的n个字符是否存在
*输入参数 : char *s1, char *s2, int n
*输出参数 : 当s1<s2时,返回值<0  当s1=s2时,返回值=0  当s1>s2时,返回值>0
*******************************************************************************/ 
int str1nstr2(char *s1, char *s2, int n) 
{
  if(n==0)//n为无符号整形变量;如果n为0,则返回0
  	return(0);
	//第一个循环条件:--n,如果比较到前n个字符则退出循环
	//第二个循环条件:*s1,如果s1指向的字符串末尾退出循环
  //第三个循环条件:*s1,如果s1指向的字符串是空格
  while(--n && *s1 && *s1 == ' ') 
  {
  	s1++;//S1指针自加1,指向下一个字符
  }
  //第一个循环条件：--n,如果比较到前n个字符则退出循环
  //第二个循环条件：*s1,如果s1指向的字符串末尾退出循环
  //第二个循环条件：*s1 == *s2,如果两字符比较不等则退出循环
  while (--n && *s1 && *s1 == *s2) 
  { 
    s1++;//S1指针自加1,指向下一个字符
    s2++;//S2指针自加1,指向下一个字符
  }
  return( *s1 - *s2 );//返回比较结果
}
