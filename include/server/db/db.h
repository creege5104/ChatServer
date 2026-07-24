#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

class MysqlConn
{
public:
    // 初始化数据库连接
    MysqlConn();
    // 释放数据库连接资源
    ~MysqlConn();
    // 获取连接
    MYSQL *getConnection() { return _conn; }
    // 连接数据库
    bool connect(string user, string password, string dbName, string ip, unsigned short port = 3306);
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
	//刷新时间
	void refreshAliveTime();
	//计算存活时长
	long long getAliveTime();

private:
    MYSQL *_conn;
    steady_clock::time_point m_alivetime;
};

#endif