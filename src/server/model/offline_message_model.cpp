#include "offline_message_model.h"
#include <muduo/base/Logging.h>
#include "ConnPool.h"

//存储用户的离线消息
bool OffMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage(userid, message) values(%d, '%s')",
         userid, msg.c_str());
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if (conn!=nullptr)
    {
        if(conn->update(sql))
        {
            return true;
        }
    } 
    return false;
}

//删除用户的离线消息
bool OffMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid = %d", userid);
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if (conn!=nullptr)
    {
        if(conn->update(sql))
        {
            return true;
        }
    } 
    return false;
}

//查询用户的离线消息
vector<string> OffMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid = %d", userid);
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if (conn!=nullptr)
    {
        MYSQL_RES *res = conn->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            vector<string> vec;
            while ((row = mysql_fetch_row(res))!= nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vector<string>();
}
              