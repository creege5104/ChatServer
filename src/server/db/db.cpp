#include "db.h"
#include <muduo/base/Logging.h>
using namespace muduo;

// static string server = "127.0.0.1";
// static string user = "root";
// static string password = "54518151";
// static string dbname = "chat";
// static int port = 3306;

MysqlConn::MysqlConn()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MysqlConn::~MysqlConn()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}
// 连接数据库
bool MysqlConn::connect(string user, string password, string dbName, string ip, unsigned short port)
{
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(),
                                  user.c_str(), password.c_str(), dbName.c_str(), port, nullptr, 0);
    if (p != nullptr)
    {
        // 设置字符集
	    mysql_set_character_set(_conn, "utf8");
        LOG_INFO << "连接数据库成功！";
        return true;
    }
    LOG_ERROR << "连接数据库失败！";
    return false;
}
// 更新操作
bool MysqlConn::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << " " << sql << " 更新失败！";
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *MysqlConn::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << " " << sql << " 查询失败！";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

void MysqlConn::refreshAliveTime()
{
	m_alivetime = steady_clock::now();
}

long long MysqlConn::getAliveTime()
{
	milliseconds millsec = duration_cast<milliseconds>(steady_clock::now() - m_alivetime);
	return millsec.count();
}