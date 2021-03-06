/********************  COPYRIGHT(C)***************************************
**--------------文件信息--------------------------------------------------------
**文   件   名: sqliteops.h
**创   建   人: 于宏图
**创 建  日 期: 
**程序开发环境：
**描        述: sqlite数据库操作头文件
**--------------历史版本信息----------------------------------------------------
** 创建人: 于宏图
** 版  本:
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
#ifndef _SQLITEOPS_H_
#define _SQLITEOPS_H_

#include "../common/druheader.h"

void SqliteInit(void);
void SqliteExit(void);
int SqliteOpen(const char *dbName);
void SqliteClose(void);
int SqliteSelect(const char *sql, SqlResult_t *prs);
int  SqliteUpdate(const char *sql);
int SqliteInsert(const char *sql);
int SqliteDelete(const char *sql);
int SqliteCreate(const char *sql);
int SqliteTransaction(const char *sql);

#endif /*_SQLITEOPS_H_*/
/*******************************************************************************/
