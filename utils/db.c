#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include "db.h"


static int g_db_debug_switch = 1;
#define G_DB_DEBUG(format,...) \
if (g_db_debug_switch ){                                    \
	printf(format, ##__VA_ARGS__);\
}


MYSQL * OpenDB( const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket)
{
	MYSQL * m_pDBAccess =mysql_init(NULL);
	if (NULL == m_pDBAccess) {  
G_DB_DEBUG("%d | open db mysql_init fail %s %s %s\n",__LINE__, host, user, db);
		return NULL;
	}  
/* 	//开启mysql多线程时mysql_library_init必须执行对应在关闭时执行mysql_library_end
	if (mysql_thread_init()) {  
		G_DB_DEBUG( (char *) "连接数据库could not initialize MySQL library 失败");
G_DB_DEBUG("%d | %d | open db mysql_library_init fail %s %s %s\n",(unsigned int)pthread_self(), __LINE__, host, user, db);
		return -1;
	}  
 */
	if(!mysql_real_connect(m_pDBAccess, host, user, passwd, db, port, NULL, 0))
		return NULL;
	return m_pDBAccess;
/* 	if(!mysql_real_connect((MYSQL *)m_pDBAccess, host, user, passwd, db, port, NULL, CLIENT_MULTI_STATEMENTS)){ 
		G_DB_DEBUG( (char *) "Failed to connect to mysql database %s on %s.\n", db, host); 
G_DB_DEBUG("%d | %d | open db mysql_real_connect fail %s %s %s\n",(unsigned int)pthread_self(), __LINE__, host, user, db);
		return -1;
	} else { 
		mysql_autocommit((MYSQL *)m_pDBAccess, 0);
		if (mysql_query_get_res((MYSQL *)m_pDBAccess, "set NAMES gbk")) // 如果失败
		{
			G_DB_DEBUG( (char *)"连接数据库失败，错误原因是:%s", mysql_error((MYSQL *)m_pDBAccess));
			return -1;
		}
		char value = 1; 
		mysql_options((MYSQL *)m_pDBAccess, MYSQL_OPT_RECONNECT, (char*)&value); 
G_DB_DEBUG("%d | %d | open db ok %s %s %s\n",(unsigned int)pthread_self(), __LINE__, host, user, db);
		return 0; 
	} */
}


int CloseDB(MYSQL * m_pDBAccess)
{
	if(m_pDBAccess){
		if(	mysql_commit((MYSQL *)m_pDBAccess)){
G_DB_DEBUG("%d | close db commit erro\n",__LINE__);
		}
		mysql_close((MYSQL *)m_pDBAccess);
		m_pDBAccess = NULL ;
		//mysql_thread_end();
	}
	return 0;
}

/*
*	入参:DB 数据指针
*			sql执行语句
*	出参:res返回结果集
*
*	返回值:-1　sql语句执行失败
*				>=0语句执行成功
*
**/

int mysql_query_get_res(MYSQL *DB,const  char *sql, MYSQL_RES **res)
{
	if(NULL == DB){
		//LogTransMsg((char *)__FILE__, __LINE__, (char *) "Failed to connect to mysql database.\n"); 
		return -1;
	}

	if(mysql_query(DB, sql)){
		mysql_rollback(DB);
		return -1;
	}
	*res = mysql_store_result(DB);

	return 0;
}

int mysql_query_free_res(MYSQL *DB, MYSQL_RES *res)
{
	if(NULL == DB){
		return -1;
	}

	MYSQL_RES *new_res; 
	if(res)
		mysql_free_result(res);

	// 释放当前查询的所有结果集. 否则下次查询将会出错.
	while (!mysql_next_result(DB))
	{
		new_res = mysql_store_result(DB);
		if(new_res)
			mysql_free_result(new_res);
	}

	return 0;
}

