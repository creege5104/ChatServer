#include "group_model.h"
#include "ConnPool.h"

using namespace std;

//创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname,groupdesc) values('%s','%s')",
        group.getName().c_str(), group.getDesc().c_str());
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if (conn != nullptr)
    {
        if(conn->update(sql))
        {
            group.setId(mysql_insert_id(conn->getConnection()));
            return true;
        }
    }
    return false;
}
//加入群组
void GroupModel::addGroup(int uid, int gid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser(groupid, userid, grouprole) values(%d, %d,'%s')",
            gid, uid, role.c_str());
    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn!=nullptr)
    {
        conn->update(sql);
    }

}
//查询用户所在群组信息
vector<Group>GroupModel::queryGroups(int uid)
{
    char sql[1024] = {0};
    sprintf(sql, "select g.id, g.groupname, g.groupdesc from AllGroup g \
            inner join GroupUser u on g.id = u.groupid where u.userid=%d", uid);

    vector<Group>groups;

    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn!=nullptr)
    {
        MYSQL_RES *res = conn->query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groups.push_back(group);
            }
            mysql_free_result(res);   
        }
    }
    for(Group &group : groups)
    {
        sprintf(sql, "select u.id, u.name, u.state, g.grouprole from User u \
            inner join GroupUser g on g.userid = u.id where g.groupid=%d", group.getId());

        MYSQL_RES *res = conn->query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);   
        }
    }
    return groups;
}
//根据指定的groupid查询群用户（排除userid）
vector<int>GroupModel::queryGroupUsers(int uid, int gid)
{
    char sql[1024]={0};
    sprintf(sql, "select userid from GroupUser where groupid=%d and userid!=%d", gid, uid);

    vector<int>users;

    shared_ptr<MysqlConn> conn = ConnPool::getPool()->getConn();
    if(conn!=nullptr)
    {
        MYSQL_RES *res = conn->query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr)
            {
                users.push_back(atoi(row[0]));
            }
            mysql_free_result(res);   
        }
    }  
    return users;
}