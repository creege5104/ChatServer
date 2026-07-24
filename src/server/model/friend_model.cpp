#include "friend_model.h"
#include "ConnPool.h"
#include <string>

void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend (userid, friendid) values (%d, %d)", userid, friendid);
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    conn->update(sql);
}

vector<User> FriendModel::query(int userid)
{
    vector<User> friends;
    char sql[1024] = {0};
    sprintf(sql, "select u.id, u.name, u.state from User u inner join Friend f on u.id = f.friendid where f.userid = %d", userid);
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if (conn != nullptr)
    {
        MYSQL_RES *res = conn->query(sql);
        if (res)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                friends.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return friends;
}
