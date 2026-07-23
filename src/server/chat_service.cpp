#include "chat_service.h"
#include "public.h"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <map>

using namespace muduo;
using namespace std;

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

void ChatService::reset()
{
    //将online状态的用户，设置为offline
    _userModel.resetState();
}


//注册消息与回调handler
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribe, this, _1, _2));
    }
}


//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        return [=](auto a, auto b, auto c){
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
    
}

//登录业务
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int id = js["id"];
    //string name = js["name"];
    string password = js["password"];
    User user = _userModel.query(id);
    if (user.getId() != -1 && user.getPassword() == password)
    {
        if(user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump() + "\n");
            
        }
        else
        {
            //登录成功
            //记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({user.getId(), conn});
            }
            
            //向redis订阅用户id对应的channel
            _redis.subscribe(id);

            //更新用户在线状态
            user.setState("online");
            _userModel.updateState(user);
            
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            response["msg"] = "login success";
            //conn->send(response.dump() + "\n");

            //检查是否有离线消息
            vector<string> vec = _offMsgModel.query(user.getId());
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offMsgModel.remove(user.getId());
                //conn->send(response.dump() + "\n");
            }

            //返回好友信息
            vector<User>friendList = _friendModel.query(user.getId());
            if(!friendList.empty())
            {
                vector<string>friends;
                for(User &user : friendList)
                {
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    friends.push_back(js.dump());
                }
                response["friends"] = friends;
            }

            //返回群组信息
            vector<Group>groupList = _groupModel.queryGroups(user.getId());
            if(!groupList.empty())
            {
                vector<string>groups;
                for(Group &group : groupList)
                {
                    json gjs;
                    gjs["id"]=group.getId();
                    gjs["groupname"]=group.getName();
                    gjs["groupdesc"]=group.getDesc();

                    vector<string>users;
                    for(GroupUser &user : group.getUsers())
                    {
                        json ujs;
                        ujs["id"] = user.getId();
                        ujs["name"] = user.getName();
                        ujs["state"] = user.getState();
                        ujs["role"] = user.getRole();
                        users.push_back(ujs.dump());
                    }
                    gjs["users"] = users;
                    groups.push_back(gjs.dump());
                }
                response["groups"] = groups;
            }

            conn->send(response.dump() + "\n");

        }
        
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or passwprd is invalid!";
        conn->send(response.dump() + "\n");
    }
    return;
}

//注册业务
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];
    User user(-1, name, password);
    bool state = _userModel.insert(user);
    if (state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump() + "\n");
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump() + "\n");
    }
}

void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //toid在线，转发消息
            it->second->send(js.dump() + "\n");
            return;
        }
    }

    User user = _userModel.query(toid);
    if(user.getState()=="online")
    {
        //toid在线，非同一服务器
        _redis.publish(toid, js.dump());
    }
    else
    {
        //toid不在线，存储离线消息
        _offMsgModel.insert(toid, js.dump());
    }
    
    return;
}

void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid, friendid);
}

//创建群组方法
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int uid = js["id"].get<int>();
    string name = js["name"];
    string desc = js["desc"];
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group))
    {
        _groupModel.addGroup(uid, group.getId(), "creator");
    } 
}
//加入群组方法
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int uid = js["id"].get<int>();
    int gid = js["groupid"].get<int>();
    _groupModel.addGroup(uid, gid, "normal");
}
//群组聊天方法
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int uid = js["id"].get<int>();
    int gid = js["groupid"].get<int>();
    vector<int>vec = _groupModel.queryGroupUsers(uid, gid);
    
    lock_guard<mutex> lock(_connMutex);
    for(int toid : vec)
    {
        js["to"] = toid;
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //toid在线，转发消息
            it->second->send(js.dump() + "\n");
        }
        else
        {
            User user = _userModel.query(toid);
            if(user.getState()=="online")
            {
                //toid在线，非同一服务器
                _redis.publish(toid, js.dump());
            }
            else
            {
                //toid不在线，存储离线消息
                _offMsgModel.insert(toid, js.dump());
            }
        }      
    }
    
    return;
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                //删除用户连接信息，更新用户状态
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    _redis.unsubscribe(user.getId());

    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::loginout(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int id = js["id"].get<int>();
    
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //向redis取消订阅用户id对应的channel
    _redis.unsubscribe(id);

    User user(id,"","","offline");
    _userModel.updateState(user);

}

void ChatService::handlerRedisSubscribe(int uid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(uid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
    }
    else
    {
        _offMsgModel.insert(uid, msg);
    }

}
