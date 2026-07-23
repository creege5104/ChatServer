#include "chat_server.h"
#include "chat_service.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json =  nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : _server(loop, listenAddr, nameArg),_loop(loop)
{
    //注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    //注册读写回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    
    //设置线程数量
    _server.setThreadNum(2);
}
void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        cout << conn->peerAddress().toIpPort() << " -> "
                << conn->localAddress().toIpPort() << " is online "
                << endl;
    }
    else
    {
        ChatService::instance()->clientCloseException(conn);
        
        cout << conn->peerAddress().toIpPort() << " -> "
                << conn->localAddress().toIpPort() << " is offline"
                << endl;
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    string msg = buf->retrieveAllAsString();
    json js = json::parse(msg);
    
    MsgHandler msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}

