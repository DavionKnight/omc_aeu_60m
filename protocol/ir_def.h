#ifndef _IR_DEF_H
#define _IR_DEF_H
#include "../common/type_def.h"
//定义消息编号
#define CREATE_CHANNEL_REQ 1
#define CREATE_CHANNEL_CFG 2
#define CREATE_CHANNEL_CFG_RSP 3
#define UPDATE_RESULT_DIS 4
#define UPDATE_RESULT_RSP 5
#define RRU_VERSION_REQ 11
#define RRU_VERSION_RSP 12
#define VERSION_DOWNLOARD_REQ 21
#define VERSION_DOWNLOARD_RSP 22
#define VERSION_DOWNLOARD_RESULT_DIS 23
#define RRU_VERSION_ALIVE_DIS 31
#define RRU_VERSION_ALIVE_RSP 32
#define RRU_STATUS_REQ 41
#define RRU_STATUS_RSP 42
#define RRU_PARAMETER_REQ 51
#define RRU_PARAMETER_RSP 52
#define RRU_PARAMETER_CFG 61
#define RRU_PARAMETER_CFG_RSP 62
#define INIT_ADJUST_RESULT_REP 71
#define ADJUST_PARAMETER_CFG_REQ 81
#define ADJUST_PARAMETER_CFG_RSP 82
#define TIME_DELAY_MEASURE_REQ 101
#define TIME_DELAY_MEASURE_RSP 102
#define TIME_DELAY_CFG 103
#define TIME_DELAY_CFG_RSP 104
#define WARNING_ALARM_REP 111
#define WARNING_ALARM_REQ 121
#define WARNING_ALARM_RSP 122
#define UPDATE_LOG_REQ 131
#define UPDATE_LOG_RSP 132
#define UPDATE_LOG_RESULT_DIS 133
#define RESTART_DIS 141
#define REMOTE_RESTART_DIS 151
#define RRU_ALIVE_MSG 171
#define BBU_ALIVE_MSG 181
#define RRU_TO_BBU 230
#define BBU_TO_RRU 240
//ie最大同时查询数量
#define MAX_IE_BUFFER 20
#pragma pack(1)
//时延测量和设置定义
typedef struct delay_control_t{
U8 enable_flag;
U8 delay_flag;
U8 up_link_port;
U16 max_delay;
U16 path_delay;
U16 next_delay[8];
}delay_control;
//ie地址存储
typedef struct ie_addr_t{
	U8 *paddr;
	U16 length;
}ie_addr;
//协议与函数对照函数
typedef struct function_t{
U32 number;
void (* deal)(U8 *inbuf);
}function;
//消息头结构定义
typedef struct msg_head_t{
U32 msg_number;
U32 msg_length;
U8 RRU_ID;
U8 BBU_ID;
U8 light_port_number;
U32 serial_number;
}msg_head;
/**************************************************/
//通道建立请求ie结构定义

//rru产品标识ie
typedef struct rru_mark_t{
CU16 ie_symbol;
CU16 ie_length;
U8 made_name[16];
U8 publisher_name[16];
U8 serial_number[16];
U8 production_date[16];
U8 modify_date[16];
U8 other_info[16];
}rru_mark;
//通道建立原因ie
typedef struct create_channel_reason_t{
CU16 ie_symbol;
CU16 ie_length;
U8 reason;
U32 alarm;
}create_channel_reason;
//RRU级数ie
typedef struct rru_series_t{
CU16 ie_symbol;
CU16 ie_length;
U8 series;
U64 port_number[8];
}rru_series;
//RRU硬件信息ie
typedef struct rru_hardware_info_t{
CU16 ie_symbol;
CU16 ie_length;
U8 device_type[32];
U8 device_version[16];
}rru_hardware_info;
//RRU软件信息ie
typedef struct rru_software_info_t{
CU16 ie_symbol;
CU16 ie_length;
U8 software_version[40];
U8 firmware_version[40];
}rru_software_info;
//RRU基本信息，通道建立时由bbu存储
typedef struct rru_info_t{
rru_mark r_mark;
create_channel_reason c_reason;
rru_series r_series;
rru_hardware_info r_hardware;
rru_software_info r_software;
}rru_info;
/*******************************************************
			通道建立及核对版本
*******************************************************/
/******************************************************/
//通道建立配置包含的ie

//系统时间ie
typedef struct sys_time_t{
CU16 ie_symbol;
CU16 ie_length;
U8 sec;
U8 min;
U8 hour;
U8 date;
U8 mon;
U16 year;
}sys_time;
//接入应答地址ie
typedef struct ftp_addr_t{
CU16 ie_symbol;
CU16 ie_length;
U8 ip[4];
}ftp_addr;
//RRU操作模式ie
typedef struct rru_mode_t{
CU16 ie_symbol;
CU16 ie_length;
U32 mode;
}rru_mode;
//软件版本核对结果ie
typedef struct software_version_result_t{
CU16 ie_symbol;
CU16 ie_length;
U8 type;
U32 result;
U8 path[200];
U8 name[16];
U32 length;
U8 time[20];
U8 version[40];
}software_version_result;
//ir口工作模式配置
typedef struct ir_mode_config_t{
CU16 ie_symbol;
CU16 ie_length;
U32 mode;
}ir_mode_config;
/************************************************/
//通道建立配置应答包含的ie
//通道建立配置应答ie
typedef struct create_channel_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 status;
}create_channel_rsp;
/*********************************************************/
//版本更新结果指示包含的ie
//版本更新结果ie
typedef struct update_version_result_t{
CU16 ie_symbol;
CU16 ie_length;
U8 type;
U32 status;
}update_version_result;
/*******************************************************
			版本查询
*******************************************************/
////版本查询应答ie
//rru_hardware_info
//rru_software_info
/*******************************************************
			版本下载
*******************************************************/
////版本下载请求包含的ie
//软件版本核对结果ie
//software_version_result
////版本下载应答包含的ie
//版本下载响应ie
typedef struct downloard_version_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 type;
U32 status;
}downloard_version_rsp;
////版本下载结果指示包含的ie
//版本下载传输完成指示ie
typedef struct downloard_result_t{
CU16 ie_symbol;
CU16 ie_length;
U8 type;
U32 status;
}downloard_result;
/*******************************************************
			版本激活
*******************************************************/
////RRu版本激活指示包含的ie
//RRU软件版本信息ie
//rru_software_info
////RRU版本激活应答包含的ie
//RRU版本激活应答ie
typedef struct software_activate_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 type;
U32 status;
}software_activate_rsp;
/*******************************************************
			状态查询
*******************************************************/
////RRU状态查询包含的ie
//射频通道状态ie
typedef struct fchannel_state_req_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
}fchannel_state_req;
//载波状态ie
typedef struct carrier_wave_req_t{
CU16 ie_symbol;
CU16 ie_length;
}carrier_wave_req;
//本振状态ie
typedef struct local_osc_req_t{
CU16 ie_symbol;
CU16 ie_length;
}local_osc_req;
//时钟状态ie
typedef struct clock_req_t{
CU16 ie_symbol;
CU16 ie_length;
}clock_req;
//RRU运行状态ie
typedef struct run_state_req_t{
CU16 ie_symbol;
CU16 ie_length;
}run_state_req;
//ir口工作模式查询ie
typedef struct ir_mode_req_t{
CU16 ie_symbol;
CU16 ie_length;
}ir_mode_req;
//初始化校准结果ie
typedef struct init_result_req_t{
CU16 ie_symbol;
CU16 ie_length;
}init_result_req;
//光口信息查询ie
typedef struct light_info_req_t{
CU16 ie_symbol;
CU16 ie_length;
U8 port;
}light_info_req;
////RRU状态查询响应包含的ie
//射频通路状态响应ie
typedef struct fchannel_state_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
U32 up;
U32 down;
}fchannel_state_rsp;
//载波状态响应ie
typedef struct carrier_wave_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
U32 status;
}carrier_wave_rsp;
//本振状态响应ie
typedef struct local_osc_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 hz;
U32 status;
}local_osc_rsp;
//时钟状态响应ie
typedef struct clock_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 status;
}clock_rsp;
//RRU运行状态响应ie
typedef struct run_state_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 status;
}run_state_rsp;
//ir口工作模式查询响应ie
typedef struct ir_mode_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 result;
}ir_mode_rsp;
//初始化校准结果响应ie
typedef struct init_result_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 tx_channel_number;
U32 tx_status;
U32 rx_channel_number;
U32 rx_status;
}init_result_rsp;
//光口信息查询响应ie
typedef struct light_info_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 port;
U16 rx_capy;
U16 tx_capy;
U8 is_being;
U8 company[16];
U16 speed;
S8 temp;
U16 mv;
U16 ma;
U8 length_9ums_km;
U8 length_9ums_100m;
U8 length_50umm_10m;
U8 length_625umm_10m;
}light_info_rsp;
/*******************************************************
			参数查询
*******************************************************/
////参数查询包含的ie
//系统时间查询ie
typedef struct sys_time_req_t{
CU16 ie_symbol;
CU16 ie_length;
}sys_time_req;
//cpu占用率ie
typedef struct cpu_occ_req_t{
CU16 ie_symbol;
CU16 ie_length;
}cpu_occ_req;
//cpu占用率查询周期ie
typedef struct cpu_cycle_req_t{
CU16 ie_symbol;
CU16 ie_length;
}cpu_cycle_req;
//rru温度ie
typedef struct rru_temp_req_t{
CU16 ie_symbol;
CU16 ie_length;
U32 type;
}rru_temp_req;
//驻波比状态查询ie
typedef struct rru_swr_req_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
}rru_swr_req;
//驻波比门限查询ie
typedef struct swr_limit_req_t{
CU16 ie_symbol;
CU16 ie_length;

}swr_limit_req;
//过温门限查询ie
typedef struct temp_limit_req_t{
CU16 ie_symbol;
CU16 ie_length;
}temp_limit_req;
//输出功率查询ie
typedef struct out_capy_req_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
}out_capy_req;
//状态机查询ie
typedef struct state_machine_req_t{
CU16 ie_symbol;
CU16 ie_length;
}state_machine_req;
////rru参数查询响应ie
//系统时间查询响应ie
//sys_time

//cpu占用率响应ie
typedef struct cpu_occ_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 occ;
}cpu_occ_rsp;
//cpu占用率查询周期响应ie
typedef struct cpu_cycle_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 sec;
}cpu_cycle_rsp;
//rru温度响应ie
typedef struct rru_temp_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 type;
U8 number;
U32 temp;
}rru_temp_rsp;
//驻波比状态查询响应ie
typedef struct rru_swr_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
U32 value;
}rru_swr_rsp;
//驻波比门限查询响应ie
typedef struct swr_limit_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 one_level;
U32 two_level;
}swr_limit_rsp;
//过温门限查询响应ie
typedef struct temp_limit_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
S32 up_limit;
S32 down_limit;
}temp_limit_rsp;
//输出功率查询响应ie
typedef struct out_capy_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
S16 value;
}out_capy_rsp;
//状态机查询响应ie
typedef struct state_machine_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 state;
}state_machine_rsp;
/*******************************************************
			参数配置
*******************************************************/
////参数配置包含的ie
//系统时间
//sys_time

//iq 数据通道配置ie
typedef struct iq_config_t{
CU16 ie_symbol;
CU16 ie_length;
U8 carrier_number;
U8 antenna_number;
U8 axc_number;
U8 light_number;
}iq_config;
//cpu占用率统计周期配置ie
typedef struct cpu_scycle_config_t{
CU16 ie_symbol;
CU16 ie_length;
U32 sec;
}cpu_scycle_config;
//驻波比门限配置ie
typedef struct swr_limit_config_t{
CU16 ie_symbol;
CU16 ie_length;
U32 one_level;
U32 two_level;
}swr_limit_config;
//ir工作模式配置ie
//typedef struct ir_mode_config_t
//ir_mode_config

//过温门限配置ie
typedef struct temp_limit_config_t{
CU16 ie_symbol;
CU16 ie_length;
U32 type;
S32 up_limit;
S32 down_limit;
}temp_limit_config;
//射频通道状态ie
//fchannel_state_rsp

//rru级联功能配置ie
typedef struct connection_config_t{
CU16 ie_symbol;
CU16 ie_length;
U32 value;
}connection_config;
////参数配置响应ie
//系统时间配置响应ie
typedef struct sys_time_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 result;
}sys_time_rsp;
//iq数据通道配置响应ie
typedef struct iq_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 carrier_number;
U8 antenna_number;
U8 light_number;
U8 result;
}iq_config_rsp;
//cpu占用率周期配置响应ie
typedef struct cpu_scycle_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 result;
}cpu_scycle_config_rsp;
//驻波比门限配置响应ie
typedef struct swr_limit_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 result;
}swr_limit_config_rsp;
//ir工作模式配置响应ie
typedef struct ir_mode_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 m_port;
U8 s_port;
U32 result;
}ir_mode_config_rsp;
//过温门限配置响应ie
typedef struct temp_limit_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 type;
U32 result;
}temp_limit_config_rsp;
//射频通道状态配置响应ie
typedef struct fchannel_state_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 number;
U32 result;
}fchannel_state_config_rsp;
//RRU级联功能配置响应ie
typedef struct connection_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U32 value;
}connection_config_rsp;

/*******************************************************
			时延测量
*******************************************************/
////时延测量请求包含的ie
//周期性时延测量请求ie
typedef struct time_delay_req_t{
CU16 ie_symbol;
CU16 ie_length;
U8 port;
}time_delay_req;
////时延测量响应包含的ie
//光纤时延测量响应ie
typedef struct time_delay_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 port;
U16 read_delay[8];
}time_delay_rsp;
////时延配置命令包含的ie
//时延配置命令ie
typedef struct time_delay_config_t{
CU16 ie_symbol;
CU16 ie_length;
U8 port;
U32 delay12;
U32 delay34;
U32 offset;
}time_delay_config;
////时延配置命令应答包含的ie
//时延配置结果响应ie
typedef struct time_delay_config_rsp_t{
CU16 ie_symbol;
CU16 ie_length;
U8 port;
U8 result;
}time_delay_config_rsp;

/*******************************************************
			告警上报
*******************************************************/
////告警上报请求包含的ie
//告警上报ie
typedef struct alarm_rep_t{
CU16 ie_symbol;
CU16 ie_length;
U16 status;
U32 word;
U32 sub_word;
U32 flag;
U8 time[20];
U8 other[100];
}alarm_rep;

/*******************************************************
			告警查询
*******************************************************/
////告警查询请求包含的ie
//告警查询请求ie
typedef struct alarm_req_t{
CU16 ie_symbol;
CU16 ie_length;
U32 word;
U32 sub_word;
}alarm_req;
////告警查询应答包含的ie
//告警查询应答ie
//alarm_rep



/*******************************************************
			透传
*******************************************************/
//透传目标ie
typedef struct transmit_aim_t{
CU16 ie_symbol;
CU16 ie_length;
U8 aim;
}transimt_aim;
//透传内容ie
typedef struct transmit_content_t{
CU16 ie_symbol;
CU16 ie_length;
U8 buffer[1024];
}transmit_content;








#endif
