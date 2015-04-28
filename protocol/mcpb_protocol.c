/********************  COPYRIGHT(C) 2007 ***************************************
**                               ����������ͨ�Ƽ����޹�˾
**                                     ���߲�Ʒ�з���
**
**                                 http:// www.aceway.com.cn
**--------------�ļ���Ϣ--------------------------------------------------------
**��   ��   ��: mcpb_protocol.c
**��   ��   ��: �ں�ͼ
**�� ��  �� ��: 2014��4��24��
**���򿪷�����:linux
**��        ��: MCP_BЭ�鴦��
**--------------��ʷ�汾��Ϣ----------------------------------------------------
** ������:
** ��  ��:
** �ա���:
** �衡��:
**--------------��ǰ�汾�޶�----------------------------------------------------
** �޸���:
** ��  ��:
** �ա���:
** �衡��:
**------------------------------------------------------------------------------
**
*******************************************************************************/
#include "../sqlite/drudatabase.h"
#include "approtocol.h"
#include "mcpb_protocol.h"

extern int DbSaveThisIntPara(unsigned int objectid, int val);
extern int g_MCPFlag;
extern DevicePara_t g_DevicePara;

MCPBPara_t SWUpdateData; //�����������̲�������

time_t g_UpdateStartTime, g_UpdateOverTime; //�����������̶�ʱ
extern const int CCITT_CRC16Table[256];

/*******************************************************************************
*�������� : void PreSoftBackup(void)
*��    �� : ��ǰ��������
*������� : None
*������� : None
*******************************************************************************/
void PreSoftBackup(void)
{
char syscmd[200];

	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "mkdir %s", SOFT_BAKDIR);//���������ļ���
	DEBUGOUT(syscmd);
	DEBUGOUT("..............................................\r\n");
	system(syscmd);
	sprintf(syscmd, "cp -r %s/%s %s", SOFT_UPDATEDIR, DEV_VERSION, SOFT_BAKDIR);//���ļ�����
	DEBUGOUT(syscmd);
	DEBUGOUT("..............................................\r\n");
  system(syscmd);
  sprintf(syscmd, "cp -r %s %s/%s", SOFT_UPDATETMP, SOFT_UPDATEDIR, DEV_VERSION);//�����ļ�
	DEBUGOUT(syscmd);
	DEBUGOUT("..............................................\r\n");
  system(syscmd);
}

/*******************************************************************************
*�������� : void TurnToUpdateMode(APPack_t *p_packbuf)
*��    �� : �豸�ļ�������Ӽ��ģʽת������������ģʽ,��ؿ��Ʋ�Э����MAP_A APCЭ��ת��ΪMAP_B APCЭ��
*������� : APPack_t *p_packbuf:ͨѶ���ݰ����ݽṹָ��
*������� : None
*******************************************************************************/
void TurnToUpdateMode(APPack_t *p_packbuf)
{
	//���¸�������ڻظ�ʱ����,��ʼ��������־,AP��Э������,VP:A����Э������,վ����,�豸���,ͨѶ����ʶ�Ų���,MCP��Э���ʶ
	//StartFlag, EndFlag, APType, VPType, StationNo, DeviceNo, PackNo, MCPFlag
	DEBUGOUT("TurnToUpdateMode ...............\n");
	//���·����ݵ�Ԫ�������Э������޸�
	p_packbuf->VPInteractFlag = VP_INTERACT_NORMAL;	//ͨѶ����ִ������,VP�㽻����־,0x00
	p_packbuf->CommandFlag = COMMAND_SW_UPDATE_MOD; //�����ʶ,ת������������ģʽ
	p_packbuf->ResponseFlag = RESPONSE_SUCCESS;			// Ӧ���־:�ɹ�

	//����ģʽ����
	LoadUpdatePara();//�豸�����������ģʽ,����ʱ��һ���ļ����ݿ����
	g_UpdateOverTime = time(NULL);//Զ��ͨ�ų�ʱ��ʱ
	g_UpdateStartTime = time(NULL);//�����������̶�ʱ
	if(SWUpdateData.SWRunMode == SW_UPDATE_MODE)//�ϵ�����
	{
		SWUpdateData.SWRunMode = SW_UPDATE_MODE;
	}
	else//�µ���������
	{
		SWUpdateData.SWRunMode = SW_UPDATE_MODE;
		SWUpdateData.NextFilePackId = 0;
		SWUpdateData.UpdateFileLen = 0;//���������ĳ���
	}
	//����ģʽ��,���Ʋ�������
	SWUpdateData.UpdatePackLen = UPDATEBLOCK_LEN;//0x0203 ���ݿ鳤��
	SWUpdateData.TranSoftFileFlag = 1;//0x0302�ļ��������
	SWUpdateData.SWUpdateResponseFlag = 0;//0x0303	�ļ����ݰ�Ӧ��
	SWUpdateData.UpdateFilePackId = 0;//0x0304	�ļ����ݿ����
	SWUpdateData.UpdateNotificationFlag = 2;//Ԥ��Ϊ�������ϱ�,�ȴ����½�����ϱ�,��־,0:���ϱ�,1:�Ѿ��ϱ�:2:���ϱ�����
	SaveUpdatePara();//�洢��ʼ����ǰ���в���

	g_MCPFlag = MCP_B;
	//���������������豸���������ȹ����޷��������,��������������ݻᰴʵ������޸�
	g_DevicePara.SWUpdateResult = 17;//17:��ʾ�����쳣�ж���������
	DbSaveThisIntPara(SWUPDATERESULT_ID, g_DevicePara.SWUpdateResult);
	g_DevicePara.SWRunMode = SW_UPDATE_MODE;//0x0010(MCP_A)�л�������ģʽ
	DbSaveThisIntPara(SWRUNMODE_ID, g_DevicePara.SWRunMode);
}

/*******************************************************************************
*�������� : void SWVerisonSwitch(APPack_t *p_packbuf)
*��    �� : �л���������汾
*������� : APPack_t *p_packbuf:ͨѶ���ݰ����ݽṹָ��
*������� : None
*******************************************************************************/
void SWVerisonSwitch(APPack_t *p_packbuf)
{
	char syscmd[200];
	FILE *fp = NULL;

	//���¸�������ڻظ�ʱ����,��ʼ��������־,AP��Э������,VP:A����Э������,վ����,�豸���,ͨѶ����ʶ�Ų���,MCP��Э���ʶ
	//StartFlag, EndFlag, APType, VPType, StationNo, DeviceNo, PackNo, MCPFlag
	DEBUGOUT("TurnToUpdateMode ...............\n");
	//���·����ݵ�Ԫ�������Э������޸�
	p_packbuf->VPInteractFlag = VP_INTERACT_NORMAL;		//ͨѶ����ִ������,VP�㽻����־,0x00
	p_packbuf->CommandFlag = COMMAND_SWVERISONSWITCH; 	//�����ʶ,ת������������ģʽ

	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "%s/%s", SOFT_BAKDIR, DEV_VERSION);	//���ļ�����
	fp = fopen(syscmd, "r");
	if (fp == NULL)//û�б��ݰ汾,�л�
	{
		p_packbuf->ResponseFlag = RESPONSE_SWVERISONSWITCH_ERR;			// Ӧ���־:�ɹ�
	}
	else
	{
		p_packbuf->ResponseFlag = RESPONSE_SUCCESS;			// Ӧ���־:�ɹ�
		fclose(fp);
		memset(syscmd, 0, sizeof(syscmd));
		sprintf(syscmd, "cp -r %s/%s /tmp", SOFT_UPDATEDIR, DEV_VERSION);//�ļ�����
	  	system(syscmd);
	  
	  	memset(syscmd, 0, sizeof(syscmd));
	  	sprintf(syscmd, "cp -r %s/%s %s/%s", SOFT_BAKDIR, DEV_VERSION, SOFT_UPDATEDIR, DEV_VERSION);
	  	system(syscmd);
	  
	  	memset(syscmd, 0, sizeof(syscmd));
	  	sprintf(syscmd, "cp -r /tmp/%s %s/%s", DEV_VERSION, SOFT_BAKDIR, DEV_VERSION);
	  	system(syscmd);	  
	}
	g_MCPFlag = SW_VERISONSWITCH_MODE;
}

/*******************************************************************************
*�������� : int	MCP_B_QueryCommand(APPack_t *p_packbuf)
*��    �� : ��ؿ��Ʋ�Э��MAP_B APCЭ������������ѯ���ƺ���
*������� : APPack_t *p_packbuf:ͨѶ���ݰ����ݽṹָ��
*������� : ��ѯ������ȷ���ݰ�����,���󷵻�-1
*******************************************************************************/
int	MCP_B_QueryCommand(APPack_t *p_packbuf)
{
int objectlen, objectid, pdustart, result, i, j;
DevInfo_t devinfo;

	//g_UpdateStartTime = time(NULL);//�����������̶�ʱ
  //���¸�������ڻظ�ʱ����,��ʼ��������־,AP��Э������,VP:A����Э������,վ����,�豸���,ͨѶ����ʶ�Ų���,MCP��Э���ʶ
  //StartFlag, EndFlag, APType, VPType, StationNo, DeviceNo, PackNo, MCPFlag
  //���·����ݵ�Ԫ�������Э������޸�
  p_packbuf->VPInteractFlag = VP_INTERACT_NORMAL;// ͨѶ����ִ������,VP�㽻����־,0x00
  p_packbuf->CommandFlag = COMMAND_QUERY;// �����ʶ:��ѯ
  p_packbuf->ResponseFlag = RESPONSE_SUCCESS;// Ӧ���־:�ɹ�

	g_UpdateOverTime = time(NULL);//Զ��ͨ�ų�ʱ��ʱ
  pdustart = GetDevInfo(&devinfo, p_packbuf);

	//����������ѯ
  result = 0;
  //���ݵ�Ԫ����Э������ID�Ž����޸�
  for (i = 0; i < (p_packbuf->PackLen - AP_MSG_HEAD_TAIL_LEN); )//AP_MSG_HEAD_TAIL_LEN:17,���ݰ�����Э���ֽ���(�����ݵ�Ԫ����������)
  {
		if (p_packbuf->PackValue[i] == 0)//���ݳ��ȴ���
	  {
	  	DEBUGOUT("MCP_B QueryCommand:PackValue Len Error...");
	  	goto MCP_B_QUERYFAILURE;
	  }
    //MCP_BЭ�����ݳ���2�ֽ�,��ʶ2�ֽ�,�ܼ�-4�ֽ�
    objectlen = p_packbuf->PackValue[i]+(p_packbuf->PackValue[i+1] * 0x100) - 4;
    // ���ݵ�Ԫ���ݱ�ʶ,ID
    objectid = p_packbuf->PackValue[i+2]+(p_packbuf->PackValue[i+3] * 0x100);
		switch(objectid)
    {
      case  0x0201://�豸�����������ģʽ
        p_packbuf->PackValue[i+4] = SWUpdateData.SWRunMode;//����ģʽ
        result = 1;
      break;
      case  0x0202://��һ���ļ����ݿ����
        p_packbuf->PackValue[i+4] = (INT8U)(SWUpdateData.NextFilePackId & 0xFF);
        p_packbuf->PackValue[i+5] = (INT8U)((SWUpdateData.NextFilePackId >> 8) & 0xFF);
        p_packbuf->PackValue[i+6] = (INT8U)((SWUpdateData.NextFilePackId >> 16) & 0xFF);
        p_packbuf->PackValue[i+7] = (INT8U)((SWUpdateData.NextFilePackId >> 24) & 0xFF);
        result = 1;
      break;
      case  0x0203://���ݿ鳤��
        p_packbuf->PackValue[i+4] = (INT8U)(SWUpdateData.UpdatePackLen & 0xFF);
        p_packbuf->PackValue[i+5] = (INT8U)((SWUpdateData.UpdatePackLen >> 8) & 0xFF);
        result = 1;
      break;
      case  0x0301://�ļ���ʶ��,�ж��Ƿ�����Ч�����ݰ�
        for(j = 0; j < 20; j++)
        {
          p_packbuf->PackValue[i+j+4] = SWUpdateData.SWUpdateFileID[j];
        }
        result = 1;
      break;
      case  0x0302://�ļ��������
        p_packbuf->PackValue[i+4] = SWUpdateData.TranSoftFileFlag;
        result = 1;
      break;
      case  0x0303://�ļ����ݰ�Ӧ��
        p_packbuf->PackValue[i+4] = SWUpdateData.SWUpdateResponseFlag;//��ʾ�ɹ�����,���Լ������պ������ݰ�
        result = 1;
      break;
      case  0x0304://�ļ����ݿ����
        p_packbuf->PackValue[i+4] = (INT8U)(SWUpdateData.UpdateFilePackId & 0xFF);
        p_packbuf->PackValue[i+5] = (INT8U)((SWUpdateData.UpdateFilePackId >> 8) & 0xFF);
        p_packbuf->PackValue[i+6] = (INT8U)((SWUpdateData.UpdateFilePackId >> 16) & 0xFF);
        p_packbuf->PackValue[i+7] = (INT8U)((SWUpdateData.UpdateFilePackId >> 24) & 0xFF);
        result = 1;
      break;
      case  0x0305://�����ļ�����
//        Flash_Address = UPDATEFILE_STARTADDR+SWUpdateData.UpdateFilePackId*SWUpdateData.UpdatePackLen;
//        for(j = 0; j < SWUpdateData.UpdatePackLen; j++)
//        {
//          p_packbuf->PackValue[i+j+4] = F29040ByteRead(Flash_Address);
//        }
        result = 1;
      break;
    }
    i = i+4+objectlen;//���ݳ���+ID+����
  }
  if (result == 1)//��ѯ���ݳɹ�
  {
    return  p_packbuf->PackLen;
  }
  else
  {
MCP_B_QUERYFAILURE:
    DEBUGOUT("MCP_B Query Command Failure!\r\n");
    ClearAPPackBuf(p_packbuf);
    return -1;
  }
}

/*******************************************************************************
*�������� : void SaveUpdatePara(void)
*��    �� : ��ؿ��Ʋ�Э��MAP_B APCЭ�����������������ݴ洢����
*������� : None
*������� : None
*******************************************************************************/
void SaveUpdatePara(void)
{
char syscmd[200];
FILE *fp = NULL;

	sprintf(syscmd, "%s", SOFT_UPDATEPARA);
	fp = fopen(syscmd, "w");
	//0x0201�豸�����������ģʽ,uint1�� 0:���ģʽ,1:��������ģʽ,����ֵΪϵͳ����
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "SWRunMode:%d\r\n", SWUpdateData.SWRunMode);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //0x0202 ��һ���ļ����ݿ����	uint4��
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "NextFilePackId:%d\r\n", SWUpdateData.NextFilePackId);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //0x0203  ���ݿ鳤��	uint2��,��λΪByte
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "UpdatePackLen:%d\r\n", SWUpdateData.UpdatePackLen);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //���������ĳ���(�Զ���)
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "UpdateFileLen:%d\r\n", SWUpdateData.UpdateFileLen);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //0x0301 �ļ���ʶ��,���ִ�,��󳤶�20���ֽ���16λCRC�㷨
  //��AP��У�鵥Ԫʹ����ͬ���㷨����.����ʱ,�������ִ�����ǰ�����ֽ�,���ҵ�1���ֽڷ�CRC����ĵ�8bit
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "SWUpdateFileID:%.20s\r\n", SWUpdateData.SWUpdateFileID);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //0x0302 �ļ�������� uint1��1:��ʾ�ļ����俪ʼ,
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "TranSoftFileFlag:%d\r\n", SWUpdateData.TranSoftFileFlag);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //0x0303�ļ����ݰ�Ӧ��;2:��ʾ�ļ��������,3:��ʾOMCȡ����������,4:��ʾ����������������
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "SWUpdateResponseFlag:%d\r\n", SWUpdateData.SWUpdateResponseFlag);
  //0x0304	�ļ����ݿ����	uint4��,����Ŵ�0��ʼ˳����б��
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "UpdateFilePackId:%d\r\n", SWUpdateData.UpdateFilePackId);
  fwrite(syscmd, 1, strlen(syscmd), fp);
  //0x0305	(�˴�Ϊ���ݵ�ַ)�ļ����ݿ� ���ִ�,���Ƚ���ͨ�Ű�����󳤶�����
	memset(syscmd, 0, sizeof(syscmd));
	sprintf(syscmd, "UpdateFilePackAddr:%d\r\n", SWUpdateData.UpdateFilePackAddr);
  fwrite(syscmd, 1, strlen(syscmd), fp);
	fclose(fp);
}

/*******************************************************************************
*�������� : int LoadUpdatePara(void)
*��    �� : ��ؿ��Ʋ�Э��MAP_B APCЭ�����������������ݶ�ȡ����
*������� : None
*������� : ���󷵻�-1
*******************************************************************************/
int LoadUpdatePara(void)
{
char *pbuf, buf[200];
INT32U i;
FILE  *fp = NULL;

	printf("LoadUpdatePara ...\n");
  //�ж���ز����ļ��Ƿ����
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "mkdir %s", SOFT_UPDATEDIR);//���������ļ���
	DEBUGOUT(buf);
	system(buf);
	
  sprintf(buf, "%s", SOFT_UPDATEPARA);
  if (access(buf, F_OK) != 0)
  {
  	//�ļ�������
		SWUpdateData.SWRunMode = SW_MONITOR_MODE;  		//0x0010,0x0201 �豸�����������ģʽ uint1�� 0:���ģʽ,1:��������ģʽ,����ֵΪϵͳ����
		SWUpdateData.NextFilePackId = 0;         			//0x0202 ��һ���ļ����ݿ����	uint4��
		SWUpdateData.UpdatePackLen = UPDATEBLOCK_LEN;	//0x0203  ���ݿ鳤��	uint2��,��λΪByte
		SWUpdateData.UpdateFileLen = 0;          			//���������ĳ���(�Զ���)
		//(��AP��У�鵥Ԫʹ����ͬ���㷨)����.����ʱ,�������ִ�����ǰ�����ֽ�,���ҵ�1���ֽڷ�CRC����ĵ�8bit.
		memset(SWUpdateData.SWUpdateFileID, 0, sizeof(SWUpdateData.SWUpdateFileID));//0x0301 �ļ���ʶ��,���ִ�,��󳤶�20���ֽ���16λCRC�㷨
		SWUpdateData.TranSoftFileFlag = 1;       	//0x0302 �ļ�������� uint1��1:��ʾ�ļ����俪ʼ,2:��ʾ�ļ��������,3:��ʾOMCȡ����������,4:��ʾ����������������
		SWUpdateData.SWUpdateResponseFlag = 0; 		//0x0303	�ļ����ݰ�Ӧ��
		SWUpdateData.UpdateFilePackId = 0;       	//0x0304	�ļ����ݿ����	uint4��,����Ŵ�0��ʼ˳����б��
		SWUpdateData.UpdateFilePackAddr = 0;     	//0x0305	(�˴�Ϊ���ݵ�ַ)�ļ����ݿ� ���ִ�,���Ƚ���ͨ�Ű�����󳤶�����
		SWUpdateData.UpdateNotificationFlag = 2; 	//�������½���ϱ���־,1:�Ѿ��ϱ�,0:δ�ϱ�
		SaveUpdatePara();//�洢Ĭ�ϲ���
  }
	fp = fopen(SOFT_UPDATEPARA, "r");
  if (fp == NULL) return -1;
  while(!feof(fp))
  {
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), fp);//���ж�ȡ�ļ�

    pbuf = strstr(buf, "SWRunMode:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.SWRunMode = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "NextFilePackId:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.NextFilePackId = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "UpdatePackLen:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.UpdatePackLen = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "UpdateFileLen:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.UpdateFileLen = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "SWUpdateFileID:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(pbuf, ':');
    	sscanf((pbuf+1), "%s", SWUpdateData.SWUpdateFileID);
    }
    pbuf = strstr(buf, "TranSoftFileFlag:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.TranSoftFileFlag = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "SWUpdateResponseFlag:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.SWUpdateResponseFlag = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "UpdateFilePackId:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.UpdateFilePackId = atoi(pbuf+1);//����
    }
    pbuf = strstr(buf, "UpdateFilePackAddr:");
    if (pbuf != NULL)
    {
    	pbuf = strchr(buf, ':');
    	SWUpdateData.UpdateFilePackAddr = atoi(pbuf+1);//����
    }
  }
  fclose(fp);
  printf("LoadUpdatePara END ...\n");
}

/*******************************************************************************
*�������� : void  NewSoftUpdate(void)
*��    �� : �µ���������
*������� : None
*������� : None
*******************************************************************************/
void  NewSoftUpdate(void)
{
  SWUpdateData.SWRunMode = SW_UPDATE_MODE;
  SWUpdateData.NextFilePackId = 0x00;

  SWUpdateData.UpdateFileLen = 0;//���µ���������
  g_UpdateStartTime = time(NULL);//�����������̶�ʱ
  SaveUpdatePara();
}

/*******************************************************************************
*�������� : int SWUpdateResponse(void)
*��    �� : ���������ļ����ݰ�Ӧ����ƺ���
*������� : None
*������� : ���������ļ����ݰ�Ӧ���־����
*******************************************************************************/
int SWUpdateResponse(void)
{
	DevicePara_t *p_devpara;
	FILE  *fp = NULL;
	char *pbuf, rbuf[250];

	//�������������ɹ�,����������ж�����������и���
	p_devpara = &g_DevicePara;
	//0����ʾ�ɹ�����,���Լ������պ������ݰ�,
	//1����ʾ�����������ط����ݰ���ǰ��,֮ǰ�İ��ɹ����գ�,
	//2����ʾ������������ʱTP������������ݰ���ǰ��,�˰��ɹ����գ�,
	//3����ʾ����������ȡ����������
	//4����ʾ�����ļ��еĳ��ұ�ʶ����,�豸��ֹ��������,
	//5����ʾ�����ļ��е��豸��ʶ����,�豸��ֹ��������,
	//�����������������豸�����������
	if(SWUpdateData.UpdateFilePackId > MAXUPDATEFILEPACKSUM)
	{
		p_devpara->SWUpdateResult = 1; //1:��ʾ�豸��ֹ��������
		DbSaveThisIntPara(SWUPDATERESULT_ID, p_devpara->SWUpdateResult);
		return  6;//6:��ʾ�����ļ��е���������,�豸��ֹ��������
	}
	//�ڶϵ�����ʱ,OMC��0����ʼ�����ļ�
	if(SWUpdateData.UpdateFilePackId != SWUpdateData.NextFilePackId)
	{
		//���͵����ݰ���洢��Ҫ��������ݰ�������,OMC��0����ʼ����
		if(SWUpdateData.UpdateFilePackId == 0x00)//0x00��ʾ�ɹ�����,���Լ������պ������ݰ�
		{
		  	NewSoftUpdate();//OMC���Ӷϵ㴦��������,��0����ʼ
		  	return  0;//2:��ʾ����OMC��ʱTP������������ݰ�(ǰ��,�˰��ɹ�����)
		}
		else
		{
		  	return  1;//1:��ʾ�����������ط����ݰ�(ǰ��,֮ǰ�İ��ɹ�����)
		}
	}

	fp = fopen(SOFT_UPDATETMP, "r");
	if(fp == NULL) //��Ҫ�򿪵��ļ�������,���½���
		return 0;
	rewind(fp);//ָ���ļ�ͷ
	memset(rbuf, 0, sizeof(rbuf));
	fgets(rbuf, sizeof(rbuf), fp);//���ж�ȡ�ļ�

	pbuf = strstr(rbuf, MANUFACTURER_INFO);
	if (pbuf == NULL)
	{
		p_devpara->SWUpdateResult = 3; //1:��ʾ�豸��ֹ��������
		DbSaveThisIntPara(SWUPDATERESULT_ID, p_devpara->SWUpdateResult);
		return  4;//4:��ʾ�����ļ��еĳ��ұ�ʶ����,�豸��ֹ��������
	}
	memset(rbuf, 0, sizeof(rbuf));
	fgets(rbuf, sizeof(rbuf), fp);//���ж�ȡ�ļ�

	pbuf = strstr(rbuf, DEV_VERSION);
	if (pbuf == NULL)
	{
		p_devpara->SWUpdateResult = 3; //1:��ʾ�豸��ֹ��������
		DbSaveThisIntPara(SWUPDATERESULT_ID, p_devpara->SWUpdateResult);
		return  5;//5:��ʾ�����ļ��е��豸��ʶ����,�豸��ֹ��������
	}
	fclose(fp);
	return  0;//0:��ʾ�ɹ�����,���Լ������պ������ݰ�
}

/*******************************************************************************
*�������� : void MCP_B_SetCommand(APPack_t *p_packbuf)
*��    �� : ��ؿ��Ʋ�Э��MAP_B APCЭ����������������ƺ���
*������� : APPack_t *p_packbuf:ͨѶ���ݰ����ݽṹָ��
*������� : ��ѯ������ȷ���ݰ�����,���󷵻�-1
*******************************************************************************/
int MCP_B_SetCommand(APPack_t *p_packbuf)
{
int objectlen, objectid, pdustart, result, i, j;
DevInfo_t devinfo;
FILE  *fp = NULL;

	//g_UpdateStartTime = time(NULL);//�����������̶�ʱ
  //���¸�������ڻظ�ʱ����,��ʼ��������־,AP��Э������,VP:A����Э������,վ����,�豸���,ͨѶ����ʶ�Ų���,MCP��Э���ʶ
  //StartFlag, EndFlag, APType, VPType, StationNo, DeviceNo, PackNo, MCPFlag
  //���·����ݵ�Ԫ�������Э������޸�
  p_packbuf->VPInteractFlag = VP_INTERACT_NORMAL;// ͨѶ����ִ������,VP�㽻����־,0x00
  p_packbuf->CommandFlag = COMMAND_SET;// �����ʶ:��ѯ
  p_packbuf->ResponseFlag = RESPONSE_SUCCESS;// Ӧ���־:�ɹ�
  
  g_UpdateOverTime = time(NULL);//Զ��ͨ�ų�ʱ��ʱ

  pdustart = GetDevInfo(&devinfo, p_packbuf);

	//����������ѯ
  result = 0;
  //���ݵ�Ԫ����Э������ID�Ž����޸�
  for (i = 0; i < (p_packbuf->PackLen - AP_MSG_HEAD_TAIL_LEN); )//AP_MSG_HEAD_TAIL_LEN:17,���ݰ�����Э���ֽ���(�����ݵ�Ԫ����������)
  {
		if (p_packbuf->PackValue[i] == 0)//���ݳ��ȴ���
	  {
	  	DEBUGOUT("MCP_B QueryCommand:PackValue Len Error...");
	  	goto MCP_B_SETFAILURE;
	  }
    //MCP_BЭ�����ݳ���2�ֽ�,��ʶ2�ֽ�,�ܼ�-4�ֽ�
    objectlen = p_packbuf->PackValue[i]+(p_packbuf->PackValue[i+1] * 0x100) - 4;
    // ���ݵ�Ԫ���ݱ�ʶ,ID
    objectid = p_packbuf->PackValue[i+2]+(p_packbuf->PackValue[i+3] * 0x100);
		switch(objectid)
    {
      case  0x0202://��һ���ļ����ݿ����
        p_packbuf->PackValue[i+4] = (INT8U)(SWUpdateData.NextFilePackId & 0xFF);
        p_packbuf->PackValue[i+5] = (INT8U)((SWUpdateData.NextFilePackId >> 8) & 0xFF);
        p_packbuf->PackValue[i+6] = (INT8U)((SWUpdateData.NextFilePackId >> 16) & 0xFF);
        p_packbuf->PackValue[i+7] = (INT8U)((SWUpdateData.NextFilePackId >> 24) & 0xFF);
        result = 1;
      break;
      case  0x0203://���ݿ鳤��
        p_packbuf->PackValue[i+4] = (INT8U)(SWUpdateData.UpdatePackLen & 0xFF);
        p_packbuf->PackValue[i+5] = (INT8U)((SWUpdateData.UpdatePackLen >> 8) & 0xFF);
        result = 1;
      break;
      case  0x0301://�ļ���ʶ��,�ж��Ƿ�����Ч�����ݰ�
        pdustart = 0;//�ж��ļ���ʶ����ԭ���洢�Ƿ���ͬ
        for(j = 0; j < objectlen; j++)
        {
          //�ļ���ʶ����ԭ���洢�ļ���ʶ�벻ͬ,��Ϊ�Ǵ����µ�����
          if(SWUpdateData.SWUpdateFileID[j] != p_packbuf->PackValue[i+4+j])
          {
            SWUpdateData.SWUpdateFileID[j] = p_packbuf->PackValue[i+4+j];
            pdustart = 1;
          }
        }
        if(pdustart != 0)//�ļ���ʶ����ԭ���洢�ļ���ʶ�벻ͬ,��Ϊ�Ǵ����µ�����
        {
          NewSoftUpdate();
        }
        result = 1;
      break;
      case  0x0302://�ļ��������
        SWUpdateData.TranSoftFileFlag = p_packbuf->PackValue[i+4];
        result = 1;
      break;
      case  0x0303://�ļ����ݰ�Ӧ��,Ϊ�豸�������ʵ��ֵ,��0x0305�и������ݿ�Ĵ������з���
        SWUpdateData.SWUpdateResponseFlag = p_packbuf->PackValue[i+4];
        result = 1;
      break;
      case  0x0304://�ļ����ݿ����
      	SWUpdateData.UpdateFilePackId = p_packbuf->PackValue[i+4]	+ p_packbuf->PackValue[i+5] * 0x100
                                      + p_packbuf->PackValue[i+6] * 0x10000 + p_packbuf->PackValue[i+7] * 0x1000000;
        result = 1;
      break;
      case  0x0305://�����ļ����ݿ�,���ݴ洢�������ļ�
      	//���͸��豸ʱ���밴��0x0303��0x0304��0x0305������˳��
      	//0x0303:�������������ļ����ݰ�Ӧ����,Ϊ�豸�������ʵ��ֵ,0:��ʾ�ɹ�����,���Լ������պ������ݰ�
      	//ִ�гɹ�ʱ,0x0304Ϊԭֵ����,0x0305Ϊԭֵ����,��0x0303��Ϊ�豸�������ʵ��ֵ
      	if (SWUpdateData.UpdateFilePackId > 0)
        {
        	SWUpdateData.SWUpdateResponseFlag = SWUpdateResponse();//���������ļ����ݰ�Ӧ��
        	if ((p_packbuf->PackValue[2] + p_packbuf->PackValue[3] * 0x100) == 0x0303)
        	{
          	p_packbuf->PackValue[4] = SWUpdateData.SWUpdateResponseFlag;
          }
        }
        else
        {
        	SWUpdateData.SWUpdateResponseFlag = 0;
        }
printf("\r\nSWUpdateData.SWUpdateResponseFlag:%d\r\n", SWUpdateData.SWUpdateResponseFlag);
        if((SWUpdateData.SWUpdateResponseFlag== 0)//0:��ʾ�ɹ�����,���Լ������պ������ݰ�
        || (SWUpdateData.SWUpdateResponseFlag == 2))//2:��ʾ����OMC��ʱTP������������ݰ�(�ϵ�����ʱOMC��0��ʼ����,�豸��Ҫ����FLASH)
        {
        	if (SWUpdateData.UpdateFilePackId == 0)
        		fp = fopen(SOFT_UPDATETMP, "wb");
        	else
						fp = fopen(SOFT_UPDATETMP, "ab");
  				if (fp == NULL) return -1;
  				//fseek(fp, (SWUpdateData.UpdateFilePackId*UPDATEBLOCK_LEN), SEEK_SET);
ComDataHexDis(&p_packbuf->PackValue[i+4], objectlen);
printf("\r\n");
 					fwrite(&p_packbuf->PackValue[i+4], 1, objectlen, fp);
					fclose(fp);
	        if (SWUpdateData.UpdateFilePackId == 0)//�ж��Ƿ��ǳ��ұ�ʶ�����豸��ʶ����
	        {
	        	SWUpdateData.SWUpdateResponseFlag = SWUpdateResponse();//���������ļ����ݰ�Ӧ��
	        	if ((p_packbuf->PackValue[2] + p_packbuf->PackValue[3] * 0x100) == 0x0303)
	        	{
	          	p_packbuf->PackValue[4] = SWUpdateData.SWUpdateResponseFlag;
	        	}
	        	if((SWUpdateData.SWUpdateResponseFlag== 4)//4����ʾ�����ļ��еĳ��ұ�ʶ����,�豸��ֹ��������
	          || (SWUpdateData.SWUpdateResponseFlag == 5))//5����ʾ�����ļ��е��豸��ʶ����,�豸��ֹ��������
	          {
							fp = fopen(SOFT_UPDATETMP,"w");
	  					if (fp == NULL) return -1;
	  					fclose(fp);
	  				}
	        }
          SWUpdateData.UpdateFileLen = SWUpdateData.UpdateFileLen + objectlen;//���������ĳ���
          SWUpdateData.NextFilePackId = SWUpdateData.UpdateFilePackId + 1;//�洢��һ�����
        }

        else if(SWUpdateData.SWUpdateResponseFlag == 1)//1:��ʾ�����������ط����ݰ���ǰ�ᣬ֮ǰ�İ��ɹ�����
        {
        	if (SWUpdateData.UpdateFilePackId > SWUpdateData.NextFilePackId)//�м��ж���,�ط���һ�����
        	{
        		p_packbuf->PackValue[4] = 1;
        	}
        	else//������ǰ��,������
        	{
        		p_packbuf->PackValue[4] = 0;
        	}
        }
        else//�豸��ֹ��������
        {
					fp = fopen(SOFT_UPDATETMP,"w");
  				if (fp == NULL) return -1;
  				fclose(fp);
        	SWUpdateData.UpdateFileLen = 0x00;
          SWUpdateData.NextFilePackId = 0x00;
        }
        //0305�����ݲ�ȫ����,ֻ����һ������,�Լ���ͨѶѹ��
        p_packbuf->PackValue[i] = 5;
        p_packbuf->PackValue[i+1] = 0;
        p_packbuf->PackValue[i+4] = 0;
        p_packbuf->PackLen = AP_MSG_HEAD_TAIL_LEN+5+8+5;//0303,0304,0305�������
        SaveUpdatePara();//�洢��������
        result = 1;
      break;
    }
    i = i+4+objectlen;//���ݳ���+ID+����
  }
  if (result == 1)//��ѯ���ݳɹ�
  {
    return  p_packbuf->PackLen;
  }
  else
  {
MCP_B_SETFAILURE:
    DEBUGOUT("MCP_B Set Command Failure!\r\n");
    ClearAPPackBuf(p_packbuf);
    return -1;
  }
}

/*******************************************************************************
*�������� : int SWUpdateNotification(void)
*��    �� : �������½���ϱ�
*������� : None
*������� : None
*******************************************************************************/
int SWUpdateNotification(void)
{
//  if(SWUpdateData.SWRunMode == UPDATEMODE)//����һ����������,��������ϱ�
//  {
//    if(SWUpdateData.UpdateNotificationFlag == 2)
//    {
//      RptNotification(UpdateResultNotify);//����������Ϣ�ϱ�
//
//      SWUpdateData.UpdateNotificationFlag = 1;//�������½���ϱ���־,0:���ϱ�,1:�Ѿ��ϱ�,2:���ϱ�����
//      if(g_RptGeneralInfo.UpdateResult == 0)//���������ɹ�
//      {
//        SWUpdateData.SWRunMode = MONITORMODE;//��������ģʽ�޸�Ϊ���ģʽ
//      }
//      SaveUpdateData();
//    }
//  }
}

/*******************************************************************************
*�������� : void  CheckUpdateResult(void)
*��    �� : �ж��豸ִ�����������Ľ��
*������� : None
*������� : None
*******************************************************************************/
void CheckUpdateResult(void)
{
DevicePara_t *p_devpara;
INT8U   data;
INT16U  crc;
FILE  *fp = NULL;

  //�������������ɹ�,����������ж�����������и���
	p_devpara = &g_DevicePara;
  p_devpara->SWUpdateResult = 0;//0:��ʾ�Ѿ��ɹ��������
  crc = 0;
	fp = fopen(SOFT_UPDATETMP,"rb");
 	if (fp == NULL)
 	{
 		return -1;
 	}
  while(SWUpdateData.UpdateFileLen)
  {
    fread(&data, 1, 1, fp);
    crc = (crc << 8) ^ CCITT_CRC16Table[((crc >> 8) ^ data) & 0xFF];
    SWUpdateData.UpdateFileLen--;
  }
	fclose(fp);
  if((SWUpdateData.SWUpdateFileID[0] != (INT8U)(crc ))
  || (SWUpdateData.SWUpdateFileID[1] != (INT8U)(crc >> 8)))
  {
  	DEBUGOUT("CRCУ��ʧ��:%4X\r\n", crc);
    p_devpara->SWUpdateResult = 3;//3:��ʾ�ļ����ʧ��
  }
	//�ж��豸ִ�����������Ľ��,p_devpara->SWUpdateResult
  switch(SWUpdateData.SWUpdateResponseFlag)//0x0303,SWUpdateData.SWUpdateResponseFlag:�ļ����ݰ�Ӧ��
  {
    case  0://0:��ʾ�ɹ�����,���Լ������պ������ݰ�
    break;
    case  1://1:��ʾ����OMC�ط����ݰ�(ǰ��,֮ǰ�İ��ɹ�����)
    break;
    case  2://2:��ʾ����OMC��ʱTP������������ݰ�(ǰ��,�˰��ɹ�����)
    break;
    case  3://3:��ʾ����OMCȡ����������
      p_devpara->SWUpdateResult = 2;//2:��ʾOMCȡ����������
    break;
    case  4://4:��ʾ�����ļ��еĳ��ұ�ʶ����,�豸��ֹ��������
      p_devpara->SWUpdateResult = 3;//3:��ʾ�ļ����ʧ��
    break;
    case  5://5:��ʾ�����ļ��е��豸��ʶ����,�豸��ֹ��������
      p_devpara->SWUpdateResult = 3;//3:��ʾ�ļ����ʧ��
    break;
    case  6://6:��ʾ�����ļ��е���������,�豸��ֹ��������
      p_devpara->SWUpdateResult = 17;//17:��ʾ�����쳣�ж���������
    break;
  }
  DbSaveThisIntPara(SWUPDATERESULT_ID, p_devpara->SWUpdateResult);
}
/*******************************************************************************
*�������� : void UpdateModeApp(void)
*��    �� : ϵͳ����������ģʽ�����е�Ӧ�ó���
*������� : None
*������� : None
*******************************************************************************/
void UpdateModeApp(void)
{
int updatetime;
char syscmd[150];

	if (g_MCPFlag == MCP_B)
	{
    switch(SWUpdateData.TranSoftFileFlag)//0x0302:�ļ��������
    {
      case  1://1:��ʾ�ļ����俪ʼ
      break;
      case  2://2:��ʾ�ļ��������
        SWUpdateData.NextFilePackId = 0;//0x0202: ��һ���ļ����ݿ����
      break;
      case  3://3:��ʾ�������ȡ����������
      	DEBUGOUT("ID0x0018:3,OMCȡ����������!\r\n");
      	g_MCPFlag = MCP_C;//������������������,�л���MAP_A APC
				//���������������豸���������ȹ����޷��������,��������������ݻᰴʵ������޸�
  			g_DevicePara.SWUpdateResult = 2;//2:��ʾOMCȡ����������
  			DbSaveThisIntPara(SWUPDATERESULT_ID, g_DevicePara.SWUpdateResult);
				g_DevicePara.SWRunMode = SW_MONITOR_MODE;//0x0010(MCP_A)�л���0:���ģʽ
				DbSaveThisIntPara(SWRUNMODE_ID, g_DevicePara.SWRunMode);
				//����ģʽ��,���Ʋ�������
				//SWUpdateData.NextFilePackId = 0;//0x0202: ��һ���ļ����ݿ����
				SWUpdateData.SWRunMode = SW_UPDATE_MODE;//0x0201(MCP_B)�豸�����������ģʽ ��������ģʽ
        SaveUpdatePara();//�豸�����������ģʽ,����ʱ��һ���ļ����ݿ����
        sleep(3);
          system("reboot");//��λ���¿�ʼ���г���
//        UpdateNotification(2);//�����ϱ�,2:��ʾOMCȡ����������
      break;
      case  4://4:��ʾ����������������
      	DEBUGOUT("ID0x0018:4,����������������!\r\n");
        CheckUpdateResult();//0:��ʾ�Ѿ��ɹ��������
        g_DevicePara.SWRunMode = SW_MONITOR_MODE;//0x0010(MCP_A)�л���0:���ģʽ
        DbSaveThisIntPara(SWRUNMODE_ID, g_DevicePara.SWRunMode);//�洢���������е�����������,�����Ĳ���
        g_MCPFlag = MCP_C;//������������������,�л���MAP_A APC
        
        if(g_DevicePara.SWUpdateResult == 0)//�����ɹ�
        {
        	DEBUGOUT("���������ɹ�!\r\n");
					//����ģʽ��,���Ʋ�������
					SWUpdateData.NextFilePackId = 0;//0x0202: ��һ���ļ����ݿ����
					SWUpdateData.SWRunMode = SW_UPDATE_MODE;//0x0201(MCP_B)�豸�����������ģʽ ��������ģʽ
        	SaveUpdatePara();//�豸�����������ģʽ,����ʱ��һ���ļ����ݿ����
        	PreSoftBackup();
					sleep(3);
          //11.18
          //����ǰ��������,�³������·ŵ�����λ��
          //FirmwareUpdate(0);//��������,����������������ģʽ��������ģʽ
          //���������,������������ڼ��ģʽ,��������������������ģʽΪ����ģʽ,���ϱ����������ɹ�
          system("reboot");//��λ���¿�ʼ���г���
        }
        else//��������ʧ��
        {
        	DEBUGOUT("��������ʧ������!\r\n");
  				SWUpdateData.SWRunMode = SW_UPDATE_MODE;
  				SWUpdateData.NextFilePackId = 0;
  				SWUpdateData.UpdateFileLen = 0;//���µ���������
  				SWUpdateData.UpdateFilePackId = 0;
  				SaveUpdatePara();
          system("reboot");//��λ���¿�ʼ���г���
        }
      break;
    }
    
    updatetime = (int)(time(NULL) - g_UpdateStartTime);//�����������̶�ʱ
    //���ն�ʱ��(T5):λ��ֱ��վ�豸,�����ж��Ƿ���OMC��ͨ���ж�.
    //���豸ÿ���յ�һ��ͨ�Ű���Ӽ��ģʽ�л���Զ������ģʽʱ�����¿�ʼ��ʱ,
    //����ʱ����TOT3ʱ���ж�Ϊͨ���ж�,��ʱ�豸Ӧ����������������ģʽ.
    DEBUGOUT("�����������̶�ʱ:%d!\r\n", updatetime);
    //if(updatetime > (5*60))//����Ҫ��ʱ��,ת�����ģʽ����������,Ҫ��15�������������
    if(updatetime > (15*60))//����Ҫ��ʱ��,ת�����ģʽ����������,Ҫ��15�������������
    {
    	DEBUGOUT("��������Զ��ͨ�ų�ʱʧ������!\r\n");
      g_DevicePara.SWUpdateResult = 6;//6:��ʾԶ��ͨ�ų�ʱ
      DbSaveThisIntPara(SWUPDATERESULT_ID, g_DevicePara.SWUpdateResult);
      g_DevicePara.SWRunMode = SW_MONITOR_MODE;//0x0010(MCP_A)�л���0:���ģʽ
      DbSaveThisIntPara(SWRUNMODE_ID, g_DevicePara.SWRunMode);//�洢���������е�����������,�����Ĳ���
      sleep(3);
      system("reboot");//��λ���¿�ʼ���г���
    }

    updatetime = (int)(time(NULL) - g_UpdateOverTime);//Զ��ͨ�ų�ʱ
	DEBUGOUT("22222 updatetime=%d\n", updatetime);
    //if(updatetime > 9)//���ͼ��ʱ��300ms*30���
    if(updatetime > 30)//���ͼ��ʱ��300ms*30���
    {
    	DEBUGOUT("��������Զ��ͨ�ų�ʱʧ������!\r\n");
      g_DevicePara.SWUpdateResult = 6;//6:��ʾԶ��ͨ�ų�ʱ
      DbSaveThisIntPara(SWUPDATERESULT_ID, g_DevicePara.SWUpdateResult);
      g_DevicePara.SWRunMode = SW_MONITOR_MODE;//0x0010(MCP_A)�л���0:���ģʽ
      DbSaveThisIntPara(SWRUNMODE_ID, g_DevicePara.SWRunMode);//�洢���������е�����������,�����Ĳ���
      sleep(3);
      system("reboot");//��λ���¿�ʼ���г���
    }

    if ((SWUpdateData.SWUpdateResponseFlag == 3) || (SWUpdateData.SWUpdateResponseFlag == 4)
    	||(SWUpdateData.SWUpdateResponseFlag == 5) || (SWUpdateData.SWUpdateResponseFlag == 6))
    {
    	DEBUGOUT("�豸��ֹ��������,����!\r\n");
      g_DevicePara.SWRunMode = SW_MONITOR_MODE;//0x0010(MCP_A)�л���0:���ģʽ
      DbSaveThisIntPara(SWRUNMODE_ID, g_DevicePara.SWRunMode);//�洢���������е�����������,�����Ĳ���
      sleep(3);
      system("reboot");//��λ���¿�ʼ���г���
    }
	}
	if (g_MCPFlag == SW_VERISONSWITCH_MODE)
	{
  	DEBUGOUT("�����汾�л�,����................\r\n");
    sleep(1);
    system("reboot");//��λ���¿�ʼ���г���
	}
}

/*******************************************************************************
*�������� : void  UpdateTest(void)
*��    �� : ϵͳ���������Ե�Ӧ�ó���
*������� : None
*������� : None
*******************************************************************************/
void  UpdateTest(void)
{
//INT8U   i;
//INT32U  j, Flash_Destination;
//
//  if(g_SelfSetData.UpLinkGainOffset == 12)
//  {
//   //��Ƭ��Flash 29F040�洢�ĳ�����д��0x8000 0000~0x8001 FFFFƬ��Flash��
//    Flash_Destination = 0x80000000;
//    j = 0;
//    if((F29040SectorErase(PROGRAM_SA0) == 1)
//     &&(F29040SectorErase(PROGRAM_SA1) ==1))
//    {
//      while(Flash_Destination < 0x80020000)
//      {
//        i =  *(u8*)Flash_Destination;
//        F29040ByteWrite((PROGRAM_SA0 * 0x10000 +j), i);
//        Flash_Destination++;
//        j++;
//      }
//    }
//    g_SelfSetData.UpLinkGainOffset++;
//  }
//  if(g_SelfSetData.UpLinkGainOffset == 16)//��Ƭ��Flash��������
//  {
//    FirmwareUpdate(0);
//  }
//
//  if(g_SelfSetData.UpLinkGainOffset == 22)//��ǰ�汾��������
//  {
//    PreFirmwareBackup();//��ǰ�汾��������
//    g_SelfSetData.UpLinkGainOffset++;
//  }
//  if(g_SelfSetData.UpLinkGainOffset == 26)//��ǰ�汾��������
//  {
//    FirmwareUpdate(1);
//  }
}
/********************  COPYRIGHT(C) 2014 ***************************************/