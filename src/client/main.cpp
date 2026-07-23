#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <map>
#include <iomanip>
#include <sstream>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nlohmann/json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.h"

using namespace std;
using json = nlohmann::json;

//当前系统登录用户
User currentUser;
//当前用户好友信息
vector<User>currentUserFriends;
//当前用户群组信息
vector<Group>currentUserGroups;
//控制主菜单聊天页面
bool isLogin = false;
//显示用户信息
void showCurrentUserData();
//接受线程
void readTaskHandler(int clientfd);
//获取系统时间
string getCurrentTime();
//主聊天页面程序
void mainMenu(int clientfd);

//主线程用于发送，子线程用于接收
int main(int argc, char **argv)
{
    if(argc < 3)
    {
        cerr <<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1)
    {
        cerr<<"socket create error"<<endl;
        exit(-1);
    }

    sockaddr_in saddr;
    memset(&saddr, 0, sizeof(sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &saddr.sin_addr);

    if(connect(clientfd, (sockaddr*)&saddr, sizeof(sockaddr_in))==-1)
    {
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }

    while(true)
    {
        cout<<"====================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"====================="<<endl;
        cout<<"choice: ";
        int choice = 0;
        myInput<int>(choice);

        switch (choice)
        {
            case 1:  //登录业务
            {
                int id = 0;
                string pwd;
                cout<<"user id: ";
                myInput<int>(id);
                cout<<"user password: ";
                myInput<string>(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(len == -1)
                {
                    cerr<<"send login msg error: "<<request<<endl;
                }
                else
                {
                    char buffer[10240]={0};
                    len = recv(clientfd, buffer, 10240, 0);
                    if(len == -1 || len == 0)
                    {
                        cerr<<"recv login response error"<<endl;
                    }
                    else
                    {
                        json response = json::parse(buffer);
                        if(response["errno"].get<int>()!=0)
                        {
                            cerr<<response["errmsg"]<<endl;
                        }
                        else
                        {
                            currentUser.setId(response["id"].get<int>());
                            currentUser.setName(response["name"]);

                            //好友列表
                            if(response.contains("friends"))
                            {
                                //初始化好友列表
                                currentUserFriends.clear();
                                
                                vector<string>freinds = response["friends"];
                                for(string &f : freinds)
                                {
                                    json js = json::parse(f);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    currentUserFriends.push_back(user);
                                }
                            }
                            
                            //群组列表
                            if(response.contains("groups"))
                            {
                                //初始化群组列表
                                currentUserGroups.clear();

                                vector<string>groups = response["groups"];
                                for(string &g : groups)
                                {
                                    json gjs = json::parse(g);
                                    Group group;
                                    group.setId(gjs["id"].get<int>());
                                    group.setName(gjs["groupname"]);
                                    group.setDesc(gjs["groupdesc"]);

                                    vector<string>users = gjs["users"];
                                    for(string &u : users)
                                    {
                                        json ujs = json::parse(u);
                                        GroupUser user;
                                        user.setId(ujs["id"].get<int>());
                                        user.setName(ujs["name"]);
                                        user.setState(ujs["state"]);
                                        user.setRole(ujs["role"]);
                                        group.getUsers().push_back(user);
                                    }

                                    currentUserGroups.push_back(group);
                                }
                            }

                            showCurrentUserData();

                            //离线消息
                            if(response.contains("offlinemsg"))
                            {
                                vector<string>msgs = response["offlinemsg"];
                                cout<<"here has "<<msgs.size()<<" offline messages!"<<endl;
                                for(string &msg : msgs)
                                {
                                    json js = json::parse(msg);
                                    int msgtype = js["msgid"].get<int>();
                                    //聊天消息
                                    if(msgtype == ONE_CHAT_MSG)
                                    {
                                        cout<<"UserMsg: "<<js["time"] << " ["<< js["id"]<<"] "
                                            <<js["name"]<<" said: "<<js["msg"]<<endl;
                                    }
                                    else if(msgtype == GROUP_CHAT_MSG)
                                    {
                                        cout<<"GroupMsg: ["<<js["groupid"]<<"] "<<js["time"]
                                        << " ["<< js["id"]<<"] "<<js["name"]<<" said: "<<js["msg"]<<endl;
                                    }
                                }
                            }

                            //登录成功，启动接收线程，仅一次
                            static int threadnum = 0;
                            if(threadnum == 0)
                            {
                                thread readTask(readTaskHandler, clientfd);
                                readTask.detach();
                                threadnum++;
                            }
                            
                            isLogin = true;
                            mainMenu(clientfd);
                            
                        }
                    }

                    
                }
            }
            break;
            case 2:  //注册业务
            {
                char name[50] = {};
                string pwd;
                cout<<"user name: ";
                cin.getline(name, 50);
                cout<<"user password: ";
                myInput<string>(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(len == -1)
                {
                    cerr<<"send reg msg error: "<<request<<endl;
                }
                else
                {
                    char buffer[1024]={0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if(len == -1)
                    {
                        cerr<<"recv reg response error"<<endl;
                    }
                    else
                    {
                        json response = json::parse(buffer);
                        if(response["errno"].get<int>()!=0)
                        {
                            cerr<<name<<"is already exist, register error!"<<endl;
                        }
                        else
                        {
                            cout<<name<<" register success, userid is "<<response["id"]
                                <<", do not forget it!"<<endl;
                        }
                    }
                }
            } 
            break;
            case 3:
            {
                cout<<"thank you for using."<<endl;
                close(clientfd);
                exit(0);
            } 
            default:
                cerr<<"invalid input!"<<endl;
                break;
        }

    }

    return 0;
}


void showCurrentUserData()
{
    cout<<"=========================login user========================="<<endl;
    cout<<"current login user => id:"<< currentUser.getId()<<" name:"<<currentUser.getName()<<endl;
    cout<<"-------------------------friend list-------------------------"<<endl;
    if(!currentUserFriends.empty())
    {
        for(User &user : currentUserFriends)
        {
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    else
    {
        cout<<"null"<<endl;
    }
    cout<<"-------------------------group list-------------------------"<<endl;
    if(!currentUserGroups.empty())
    {
        for(Group &group : currentUserGroups)
        {
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user : group.getUsers())
            {
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
            }
        }
    }
    else
    {
        cout<<"null"<<endl;
    }
    cout<<"============================================================"<<endl;
}


void readTaskHandler(int clientfd)
{
    while(true)
    {
        char buf[10240]={0};
        int len = recv(clientfd, buf, 10240, 0);
        if(len == -1 || len ==0)
        {
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buf);
        int msgtype = js["msgid"].get<int>();
        
        if(msgtype == ONE_CHAT_MSG)  //聊天消息
        {
            cout<<"UserMsg: "<<js["time"] << " ["<< js["id"]<<"] "
                <<js["name"]<<" said: "<<js["msg"]<<endl;
            continue;
        }
        else if(msgtype == GROUP_CHAT_MSG)  //群消息
        {
            cout<<"GroupMsg: ["<<js["groupid"]<<"] "<<js["time"]
            << " ["<< js["id"]<<"] "<<js["name"]<<" said: "<<js["msg"]<<endl;
            continue;
        }
    }
    
}

//系统支持的客户端命令列表
unordered_map<string, string>commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"logout", "注销，格式logout"}
};

void help(int fd = 0, string str = "")
{
    cout << "show command list >>>" <<endl;
    for(auto p :commandMap)
    {
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}

void chat(int fd, string str)
{
    //"chat", "一对一聊天，格式chat:friendid:message"
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr<<"chat format invalid!"<<endl;
        return;
    }
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = currentUser.getId();
    js["name"] = currentUser.getName();
    js["to"] = atoi(str.substr(0,idx).c_str());
    js["msg"] = str.substr(idx+1, str.size()-idx);
    js["time"] = getCurrentTime();
    string buf = js.dump();

    int len = send(fd, buf.c_str(), buf.size()+1, 0);
    if(len == -1)
    {
        cerr<<"send chat msg error ->"<<buf<<endl;
    }
}

void addfriend(int fd, string str)
{
    //"addfriend", "添加好友，格式addfriend:friendid"
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser.getId();
    js["friendid"] = atoi(str.c_str());
    string buf = js.dump();

    int len = send(fd, buf.c_str(), buf.size()+1, 0);
    if(len == -1)
    {
        cerr<<"send addfriend msg error ->"<<buf<<endl;
    }
}

void creategroup(int fd, string str)
{
    //"creategroup", "创建群组，格式creategroup:groupname:groupdesc"
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr<<"creategroup format invalid"<<endl;
        return;
    }
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["name"] = str.substr(0,idx);
    js["desc"] = str.substr(idx+1, str.size()-idx);
    string buf = js.dump();

    int len = send(fd, buf.c_str(), buf.size()+1, 0);
    if(len == -1)
    {
        cerr<<"send creategroup msg error ->"<<buf<<endl;
    }
}

void addgroup(int fd, string str)
{
    //"加入群组，格式addgroup:groupid"
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupid"] = atoi(str.c_str());
    string buf = js.dump();

    int len = send(fd, buf.c_str(), buf.size()+1, 0);
    if(len == -1)
    {
        cerr<<"send addgroup msg error ->"<<buf<<endl;
    }
}

void groupchat(int fd, string str)
{
    //{"groupchat", "群聊，格式groupchat:groupid:message"},
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr<<"groupchat format invalid"<<endl;
        return;
    }
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = currentUser.getId();
    js["name"] = currentUser.getName();
    js["groupid"] = atoi(str.substr(0,idx).c_str());
    js["msg"] = str.substr(idx+1, str.size()-idx);
    js["time"] = getCurrentTime();
    string buf = js.dump();

    int len = send(fd, buf.c_str(), buf.size()+1, 0);
    if(len == -1)
    {
        cerr<<"send groupchat msg error ->"<<buf<<endl;
    }
}

void logout(int fd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = currentUser.getId();
    string buf = js.dump();

    int len = send(fd, buf.c_str(), buf.size()+1, 0);
    if(len == -1)
    {
        cerr<<"send loginout msg error ->"<<buf<<endl;
    }
    else
    {
        isLogin = false;
    }
    
}



//命令对应的函数
unordered_map<string, function<void(int,string)>>commnandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}
};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024]={0};
    while(isLogin)
    {
        cin.getline(buffer, 1024);
        string commandBuf(buffer);
        string command;
        int idx = commandBuf.find(":");
        if(idx == -1)command = commandBuf;
        else command = commandBuf.substr(0,idx);
        auto it = commnandHandlerMap.find(command);
        if(it == commnandHandlerMap.end())
        {
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        it->second(clientfd, commandBuf.substr(idx+1, commandBuf.size()-idx));
    }
}


string getCurrentTime()
{
    auto now = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(now);
    
    stringstream ss;
    ss << put_time(localtime(&tt), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}