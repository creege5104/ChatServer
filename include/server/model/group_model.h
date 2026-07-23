#ifndef GROUP_MODEL_H
#define GROUP_MODEL_H

#include "group.hpp"

class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    void addGroup(int uid, int gid, string role);
    //查询用户所在群组信息
    vector<Group>queryGroups(int uid);
    //根据指定的groupid查询群用户（排除userid）
    vector<int>queryGroupUsers(int uid, int gid);
};

#endif