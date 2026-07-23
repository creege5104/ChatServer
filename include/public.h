#ifndef PUBLIC_H
#define PUBLIC_H
#include<iostream>
#include<string>
enum EnMsgType
{
    LOGIN_MSG = 1, //1登录消息
    REG_MSG,       //2注册消息
    LOGIN_MSG_ACK, //3登录响应
    REG_MSG_ACK,    //4注册响应
    ONE_CHAT_MSG,    //5聊天消息
    ADD_FRIEND_MSG,  //6添加好友消息
    CREATE_GROUP_MSG, //7创建群组消息
    ADD_GROUP_MSG,    //8加入群组消息
    GROUP_CHAT_MSG,   //9群组聊天消息
    LOGINOUT_MSG,     //10注销消息

    FRIEND_ONLINE_MSG, //好友上线消息
    FRIEND_OFFLINE_MSG, //好友下线消息
    FRIEND_LIST_MSG,    //好友列表消息
    GROUP_LIST_MSG,     //群组列表消息
    FRIEND_CHAT_MSG,    //好友聊天消息
    GROUP_CHAT_ACK_MSG, //群组聊天响应消息
    FRIEND_CHAT_ACK_MSG, //好友聊天响应消息
    HEART_BEAT_MSG,      //心跳消息
    FILE_MSG,            //文件消息
    FILE_ACK_MSG,        //文件响应消息
    FILE_FINISH_MSG,     //文件传输完成消息
    FILE_FINISH_ACK_MSG, //文件传输完成响应消息
    FILE_REFUSE_MSG,     //文件拒绝消息
    FILE_REFUSE_ACK_MSG //文件拒绝响应消息
};

template<class T>
struct LengthChecker {
    static bool tooLong(const T&, int) { return false; }
};

template<>
struct LengthChecker<std::string> {
    static bool tooLong(const std::string& s, int maxlen) {
        return s.size() > maxlen;
    }
};

template<class T>
void myInput(T& i, int maxlen = 0)
{
    while(std::cin>>i, !std::cin.eof())
    {
        if(std::cin.bad())
        {
            throw std::runtime_error("cin is corrupted");
        }
        if(std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
            std::cout<<"data format error, please try again"<<std::endl;
            continue;
        }
        if(LengthChecker<T>::tooLong(i, maxlen))
        {
            std::cout<<"input exceeds the limit, please try again"<<std::endl;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        break;
    }
    return ;
}




#endif