#ifndef OFFLINEMESSAGE_MODEL_H
#define OFFLINEMESSAGE_MODEL_H
#include <string>
#include <vector>
using namespace std;
class OffMsgModel
{
public:
    //存储用户的离线消息
    bool insert(int userid, string msg);

    //删除用户的离线消息
    bool remove(int userid);

    //查询用户的离线消息
    vector<string> query(int userid);
};
#endif