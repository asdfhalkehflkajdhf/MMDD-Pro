
#ifndef __DB_H__
#define __DB_H__


#include <mysql/mysql.h>


MYSQL * OpenDB( const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket);
int CloseDB(MYSQL * m_pDBAccess);

int mysql_query_get_res(MYSQL *DB,const  char *sql, MYSQL_RES **res);
int mysql_query_free_res(MYSQL *DB, MYSQL_RES *res);



#endif /* !__DB_H__ */
