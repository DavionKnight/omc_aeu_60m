/*******************************************************************************
********************************************************************************
* 文件名称:  dru_lmx2531.h
* 功能描述:  有关于lmx2531的配置操作的宏定义及函数声明
* 使用说明: 	 所有和lmx2531直接相关的定义都在这个文件中定义
* 文件作者:	H4
* 编写日期: （2012/06/18）
* 修改历史:
* 修改日期    修改人       修改内容
*-------------------------------------------------------------------------------

*******************************************************************************/
#ifndef _DRU_LMX2531_H
#define _DRU_LMX2531_H

#define SELECT_LMX2531 3
#define LMX2531_REG_LENGTH 24

int dru_lmx2531_init(unsigned int);
int dru_lmx2531_gsm_init();
int dru_lmx2531_gsm1_init();
int dru_lmx2531_td_init();
int dru_lmx2531_lte1_init();
int dru_lmx2531_lte2_init();
int dru_lmx2531_lte1_config(unsigned int freq);
int dru_lmx2531_lte2_config(unsigned int freq);
int dru_lmx2531_fdd_lte1_config(unsigned int freq);
int dru_lmx2531_fdd_lte2_config(unsigned int freq);
int dru_lmx2531_fdd_wcdma_config(unsigned int freq);
int dru_lmx2531_fdd_lte2_init();
int dru_lmx2531_fdd_lte1_init();
int dru_lmx2531_wcdma_init();
#endif
