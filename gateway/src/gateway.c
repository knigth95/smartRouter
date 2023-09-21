#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "config.h"
#include "comport.h"
#include "qcloud_iot_export.h"
#include "data_template_client_json.h"
#include "lite-utils.h"

void HandleControlMsgFromCloud(int fd, void *pClient);
void ReportDataToCloud(void *pClient);
void OnReportReplyCallback(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument, void *pUserdata);
void HandleDataFromComport(int fd);
static void event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg);
static void ConfigInitParams(TemplateInitParams *initParams);
static void OnControlMsgCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength,
                                 DeviceProperty *pProperty);
static void InitDataTemplate(void);
static int ConfigDataTemplate(void *client);
static void GetStatusReplyAckCb(void *pClient, Method method, ReplyAck replyAck, const char *pReceivedJsonDocument, void *pUserdata);

//#define REGION "china"
//#define DEVICE_NAME "router_knight"
//#define PRODUCT_ID "FV2AM0BG3T"
//#define DEVICE_SECRET "YBI8B29XB07box4XbOlhnQ=="
#define BANDRATE 115200
#define COMPORT_NUMBER "/dev/ttyS1"
#define cCHANGED 2
#define MAX_LINE_LENGTH 200
char DEVICE_NAME[20];
char PRODUCT_ID[20];
char DEVICE_SECRET[20];
char REGION[20];

typedef struct _ProductDataDefine {
    TYPE_DEF_TEMPLATE_FLOAT   m_temperature;
    TYPE_DEF_TEMPLATE_FLOAT   m_humidity;
    TYPE_DEF_TEMPLATE_FLOAT   m_illumination;
    TYPE_DEF_TEMPLATE_BOOL    m_door;
    TYPE_DEF_TEMPLATE_BOOL    m_light;
    TYPE_DEF_TEMPLATE_BOOL    m_ac;
    TYPE_DEF_TEMPLATE_FLOAT   m_curtain;

} ProductDataDefine;

#define TOTAL_PROPERTY_COUNT 7
static MQTTEventType sg_subscribe_event_result = MQTT_EVENT_UNDEF;
static sDataPoint dataTemplates[TOTAL_PROPERTY_COUNT];
static ProductDataDefine productData;
static char dataReportBuffer[2048];
static size_t dataReportBufferSize = sizeof(dataReportBuffer)/sizeof(dataReportBuffer[0]);


void remove_single_quote(char str[]) {
    char *ptr;

    while ((ptr = strchr(str, '\'')) != NULL) {
        strcpy(ptr, ptr + 1);
    }
}

int main(void)
{
	int fd, rc;	
	char *filename = "/etc/config/gateway";
	
	char line[100];
	char *dev_name_prefix = "option DEVICE_NAME ";
	char *prod_id_prefix = "option PRODUCT_ID ";
	char *dev_secret_prefix = "option DEVICE_SECRET ";
	char *dev_REGION_prefix = "option REGION ";
	char *dev_name = NULL;
	char *prod_id = NULL;
	char *dev_secret = NULL;
	char *prod_REGION=NULL;
	FILE *fp;
	
	char DEVICE_NAME_cp[40];
	char PRODUCT_ID_cp[40];
	char DEVICE_SECRET_cp[40];
	char REGION_cp[40];
	/* Open file for reading */
	fp = fopen(filename, "r");
	if (fp == NULL) {
	printf("Could not open file %s\n", filename);
	exit(EXIT_FAILURE);
	}

	/* Read each line of the file */
	while (fgets(line, MAX_LINE_LENGTH, fp)) {
		/* Check if the line contains DEVICE_NAME */
		if (strstr(line, dev_name_prefix) != NULL) {
		    /* Extract the value of DEVICE_NAME */
		    dev_name = strtok(line, " ");
		    dev_name = strtok(NULL, " ");
		    dev_name=strtok(NULL, " ");
		    
		    strcpy(DEVICE_NAME_cp, dev_name);
		}

		/* Check if the line contains PRODUCT_ID */
		if (strstr(line, prod_id_prefix) != NULL) {
		    /* Extract the value of PRODUCT_ID */
		    prod_id = strtok(line, " ");
		    prod_id = strtok(NULL, " ");
		    prod_id = strtok(NULL, " ");
		    
		    strcpy(PRODUCT_ID_cp, prod_id);
		}

		/* Check if the line contains DEVICE_SECRET */
		if (strstr(line, dev_secret_prefix) != NULL) {
		    /* Extract the value of DEVICE_SECRET */
		    dev_secret = strtok(line, " ");
		    dev_secret = strtok(NULL, " ");
		    dev_secret = strtok(NULL, " ");
		    
		    strcpy(DEVICE_SECRET_cp, dev_secret);
		}
		if (strstr(line, dev_REGION_prefix) != NULL) {
		    
		    prod_REGION = strtok(line, " ");
		    prod_REGION = strtok(NULL, " ");
		    prod_REGION = strtok(NULL, " ");
		    
		    strcpy(REGION_cp, prod_REGION);
		}
	}


	fclose(fp);
	remove_single_quote(DEVICE_NAME_cp);
	remove_single_quote(PRODUCT_ID_cp);
	remove_single_quote(DEVICE_SECRET_cp);
	remove_single_quote(REGION_cp);
	fp = fopen("/etc/config/gateway.txt", "w");
	if (fp == NULL) {
	printf("Failed to open file.\n");
	return -1;
	}
	fprintf(fp, "%s %s %s %s", DEVICE_NAME_cp, PRODUCT_ID_cp, DEVICE_SECRET_cp,REGION_cp);
	fclose(fp);

	fp = fopen("/etc/config/gateway.txt", "r");
	if (fp == NULL) {
	printf("Failed to open file.\n");
	return -1;
	}
	fscanf(fp, "%s %s %s %s",DEVICE_NAME,PRODUCT_ID,DEVICE_SECRET,REGION);
	fclose(fp);


	printf("The value of DEVICE_NAME is %s\n", DEVICE_NAME);
	printf("The value of PRODUCT_ID is %s\n", PRODUCT_ID);
	printf("The value of DEVICE_SECRET is %s\n", DEVICE_SECRET);
	printf("The value of REGION is %s\n", REGION);

	ReplyAck ackRequest = ACK_NONE;
	struct CONFIG config;
	TemplateInitParams initParams = DEFAULT_TEMPLATE_INIT_PARAMS;	

	IOT_Log_Set_Level(eLOG_DEBUG);
	//配置连接参数，连接IoT云
	int z;	
	void *client;
	for(z=0;z<10;z++){	
		ConfigInitParams(&initParams);
		client = IOT_Template_Construct(&initParams, NULL);
		if (client != NULL) {
			Log_i("Cloud Device Construct Success");
			break;
	    	} else {
			Log_e("Cloud Device Construct Failed");
			return QCLOUD_ERR_FAILURE;
	    	}
	}
	//配置属性
	InitDataTemplate();//初始化属性数据
	rc = ConfigDataTemplate(client);
	if (rc == QCLOUD_RET_SUCCESS) {
		Log_i("Register data template propertys Success");
	} else {
		Log_e("Register data template propertys Failed: %d", rc);
		goto exit;
	}
	
	//从云端读取信息
	rc = IOT_Template_GetStatus(client, GetStatusReplyAckCb, &ackRequest, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	if (rc != QCLOUD_RET_SUCCESS) {
		Log_e("Get data status fail, err: %d", rc);
	}
	while(ACK_NONE == ackRequest)
	{
		IOT_Template_Yield(client, 200);
	}
	printf("%f, %f, %f\n",productData.m_temperature, productData.m_humidity, productData.m_illumination);
	//以非阻塞读和写的方式打开并设置串口
	if((fd = open(COMPORT_NUMBER, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
	{//打开窗口失败
		printf("Open %s failed\n", COMPORT_NUMBER);
		return 0;
	}
	if(fcntl(fd, F_SETFL, 0) < 0)
	{
		printf("fcntl failed!\n");
		return 0;
	}
	SetComPort(fd, 115200, 8, 'N', 1);//设置成8N1 115200bps
	
	//循环发送数据
	
	while(1)
	{	
		//进行MQTT报文读取，消息处理，超时请求，心跳包及重连状态管理等任务		
		if(!IOT_Template_IsConnected(client))//连接断开，重连
		{
			Log_e("Attemping to reconnect...");			
			rc = IOT_Template_Destroy(client);
			client = IOT_Template_Construct(&initParams, NULL);
			if(client != NULL)
			{
				Log_d("Connected!");
				rc = ConfigDataTemplate(client);
			}		
		}
		IOT_Template_Yield(client, 200);
		
		//对云端下发控制命令进行处理
		HandleControlMsgFromCloud(fd, client);

		//接收串口数据
		HandleDataFromComport(fd);

		//向云端上报数据
		ReportDataToCloud(client);	
		
	}
exit:
	IOT_Template_Destroy(client);
	return 0;
}
//对云端下发控制命令进行处理
void HandleControlMsgFromCloud(int fd, void *pClient)
{
	int i,len, j,rc;
	unsigned char buf[10] = {'X', 'Y'}, recvd = 0;
	sReplyPara replyPara;
	
	for(i = 0; i < TOTAL_PROPERTY_COUNT; i++)
	{
		if(dataTemplates[i].state == cCHANGED)
		{
			recvd = 1;
			dataTemplates[i].state = eNOCHANGE;		
			buf[3] = i;
			switch(i)
			{
				case 0:
				case 1:
				case 2:
				case 6:
					len = 4;
					break;
				case 3:
				case 4:
				case 5:
					len = 1;
					break;
			}
			memcpy(buf+4, dataTemplates[i].data_property.data, len);
			buf[2] = len + 1;
			buf[buf[2]+3] = 0;
			for(j = 2; j < buf[2]+3; j++)
				buf[buf[2]+3] += buf[j];
			write(fd, buf, buf[2]+4);
			printf("Done.");	
		}
	}
	if(recvd)
	{
		printf("Replying control msg.");		
		memset((char *)&replyPara, 0, sizeof(sReplyPara));
		replyPara.code = eDEAL_SUCCESS;
		replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
		replyPara.status_msg[0] = '\0';
		rc = IOT_Template_ControlReply(pClient, dataReportBuffer, dataReportBufferSize, &replyPara);
		if(rc==QCLOUD_RET_SUCCESS)
			Log_d("Control msg reply success.");
		else
			Log_e("Control msg reply failed, err: %d.", rc);
	}
}
//向云端上报数据
void ReportDataToCloud(void *pClient)
{
	int i, j = 0, rc;
	DeviceProperty *pReportDataList[TOTAL_PROPERTY_COUNT];
	//查找要上报的数据	
	for(i = 0; i < TOTAL_PROPERTY_COUNT; i++)
	{
		if(dataTemplates[i].state == eCHANGED)
		{
			pReportDataList[j++] = &(dataTemplates[i].data_property);
			dataTemplates[i].state = eNOCHANGE;
		}
	}
	//没有数据上报，返回
	if(j == 0)
		return;
	
	rc = IOT_Template_JSON_ConstructReportArray(pClient, dataReportBuffer, dataReportBufferSize, j, pReportDataList);
	if(rc == QCLOUD_RET_SUCCESS)
	{
		rc = IOT_Template_Report(pClient, dataReportBuffer, dataReportBufferSize, OnReportReplyCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		if(rc == QCLOUD_RET_SUCCESS)
			Log_i("Data template report success");
		else
			Log_e("Data template report failed, err: %d", rc);
	}else{
		Log_e("Data template construct failed, err: %d", rc);
	}
}	

void OnReportReplyCallback(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument, void *pUserdata)
{
	Log_i("Receive report reply(ack=%d): %s", replyAck, pJsonDocument);
}
//接收串口数据
void HandleDataFromComport(int fd)
{
	#define MAX_DATA_LENGTH 5
	unsigned int cnt, i;
	unsigned char buf[4096];
	static unsigned char recvData[MAX_DATA_LENGTH], *pData, checkSum, recvFlag = 0, recvLength;
	
	cnt = read(fd, buf, 4096);

	if (cnt == 0)
		return;

	for (i = 0; i < cnt; i++)
	{
		switch(recvFlag)
		{
			case 0:	
				if(buf[i]=='X')
					recvFlag = 1;
				break;
			case 1: 
				if(buf[i]=='Y')
					recvFlag = 2;
				else
					recvFlag = 0;
				break;
			case 2:
				recvLength = buf[i];
				if((recvLength > MAX_DATA_LENGTH) || (recvLength == 0))
					recvFlag = 0;
				else
				{
					recvFlag = 3;
					pData = recvData;
					checkSum = recvLength;
				}
				break;
			case 3:
				*pData++ = buf[i];
				checkSum += buf[i];
				if(--recvLength == 0)
					recvFlag = 4;
				break;
			case 4:
				if(checkSum == buf[i])
				{
					if(recvData[0] < TOTAL_PROPERTY_COUNT)
					{
						dataTemplates[recvData[0]].state = eCHANGED;
						if(dataTemplates[recvData[0]].data_property.type == TYPE_TEMPLATE_FLOAT)
						{
							*((float *)(dataTemplates[recvData[0]].data_property.data)) = *((float *)(recvData+1));
						} 
						else if (dataTemplates[recvData[0]].data_property.type == TYPE_TEMPLATE_BOOL)
						{
							*((unsigned char *)(dataTemplates[recvData[0]].data_property.data)) = (recvData[1] > 0) ? 1 : 0;
						}
					}				
				}
				recvFlag = 0;
				break;
			}
		}
}

static void ConfigInitParams(TemplateInitParams *initParams)
{
	initParams->region = REGION;
	initParams->device_name = DEVICE_NAME;
	initParams->product_id = PRODUCT_ID;
	initParams->device_secret = DEVICE_SECRET;
	initParams->command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    	initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
    	initParams->auto_connect_enable = 1;
    	initParams->event_handle.h_fp = event_handler;
    	initParams->usr_control_handle = NULL;
}

static void OnControlMsgCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength,
                                 DeviceProperty *pProperty)
{
	int i = 0;
	for (i = 0; i < TOTAL_PROPERTY_COUNT; i++)
	{
		if (strcmp(dataTemplates[i].data_property.key, pProperty->key) == 0)
		{
			dataTemplates[i].state = cCHANGED;
			Log_i("Property=%s changed: %d", pProperty->key, *((int *)dataTemplates[i].data_property.data));
			return;
		}
	}
	Log_e("Property=%s no match", pProperty->key);
}

static void InitDataTemplate(void)
{
	memset((void *)&productData, 0, sizeof(productData));
	
	dataTemplates[0].data_property.key = "temperature";
	dataTemplates[0].data_property.data = &productData.m_temperature;
	dataTemplates[0].data_property.type = TYPE_TEMPLATE_FLOAT;
	dataTemplates[0].state = eNOCHANGE;
	
	dataTemplates[1].data_property.key = "humidity";
	dataTemplates[1].data_property.data = &productData.m_humidity;
	dataTemplates[1].data_property.type = TYPE_TEMPLATE_FLOAT;
	dataTemplates[1].state = eNOCHANGE;

	dataTemplates[2].data_property.key = "illumination";
	dataTemplates[2].data_property.data = &productData.m_illumination;
	dataTemplates[2].data_property.type = TYPE_TEMPLATE_FLOAT;
	dataTemplates[2].state = eNOCHANGE;

	dataTemplates[3].data_property.key = "door";
	dataTemplates[3].data_property.data = &productData.m_door;
	dataTemplates[3].data_property.type = TYPE_TEMPLATE_BOOL;
	dataTemplates[3].state = eNOCHANGE;

	dataTemplates[4].data_property.key = "light";
	dataTemplates[4].data_property.data = &productData.m_light;
	dataTemplates[4].data_property.type = TYPE_TEMPLATE_BOOL;
	dataTemplates[4].state = eNOCHANGE;

	dataTemplates[5].data_property.key = "ac";
	dataTemplates[5].data_property.data = &productData.m_ac;
	dataTemplates[5].data_property.type = TYPE_TEMPLATE_BOOL;
	dataTemplates[5].state = eNOCHANGE;

	dataTemplates[6].data_property.key = "curtain";
	dataTemplates[6].data_property.data = &productData.m_curtain;
	dataTemplates[6].data_property.type = TYPE_TEMPLATE_FLOAT;
	dataTemplates[6].state = eNOCHANGE;
}

static int ConfigDataTemplate(void *client)
{
	int i, rc;
	for(i = 0; i < TOTAL_PROPERTY_COUNT; i++)
	{
		rc = IOT_Template_Register_Property(client, &dataTemplates[i].data_property, OnControlMsgCallback);
		if (rc != QCLOUD_RET_SUCCESS) {
            	    rc = IOT_Template_Destroy(client);
		    Log_e("register device data template property failed, err: %d", rc);
		    return rc;
		} else {
		    Log_i("data template property=%s registered.", dataTemplates[i].data_property.key);
		}
	}
	return QCLOUD_RET_SUCCESS;
}



static void GetStatusReplyAckCb(void *pClient, Method method, ReplyAck replyAck, const char *pReceivedJsonDocument, void *pUserdata)
{
	Request *request = (Request *)pUserdata;
	if (*((ReplyAck *)request->user_context) == ACK_ACCEPTED)
		IOT_Template_ClearControl(pClient, request->client_token, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	else
		*((ReplyAck *)request->user_context) = replyAck;
	printf("Received Json Document = %s", pReceivedJsonDocument);
	char *data = LITE_json_value_of("data",	pReceivedJsonDocument);
	if (data != NULL)
	{
		char *reported = LITE_json_value_of("reported", data);
		if(reported != NULL)
		{
			if(reported != NULL)
			{
				char *tmpr = LITE_json_value_of("temperature", reported);
				if(tmpr != NULL)
				productData.m_temperature = atof(tmpr);
			}	
		}
	}
}

static void event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;

    switch (msg->event_type) {
        case MQTT_EVENT_UNDEF:
            Log_i("undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            Log_i("MQTT disconnect.");
            break;

        case MQTT_EVENT_RECONNECT:
            Log_i("MQTT reconnect.");
            break;

        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            sg_subscribe_event_result = msg->event_type;
            Log_i("subscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            sg_subscribe_event_result = msg->event_type;
            Log_i("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            sg_subscribe_event_result = msg->event_type;
            Log_i("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
            break;
        default:
            Log_i("Should NOT arrive here.");
            break;
    }
}