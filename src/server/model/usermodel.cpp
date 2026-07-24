#include "user_model.h"
#include "ConnPool.h"
#include <iostream>
using namespace std;

bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')",
         user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn!=nullptr)
    {
        if(conn->update(sql))
        {
            user.setId(mysql_insert_id(conn->getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id=%d", id);
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn!=nullptr)
    {
        MYSQL_RES *res = conn->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
            mysql_free_result(res);
        }
    }
    return User();
}

bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", 
        user.getState().c_str(), user.getId());
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn != nullptr)
    {
        return conn->update(sql);
    }    
    return false;
}

void UserModel::resetState()
{
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn != nullptr) conn->update(sql);
}

bool UserModel::delete_(int id)
{
    // Implementation for deleting user
    return false; // Placeholder return value
}