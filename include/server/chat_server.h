#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:  
    //初始化服务器对象
    ChatServer(EventLoop *loop,   
            const InetAddress &listenAddr,
            const string &nameArg);
    //启动服务
    void start();

private:
    TcpServer _server;
    EventLoop *_loop;

    //传递连接回调函数
    void onConnection(const TcpConnectionPtr& conn);
    //传递读写回调函数
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
};

#endif