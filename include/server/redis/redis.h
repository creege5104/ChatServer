#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <string>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    //连接服务器
    bool connect();
    //向指定通道channel发布消息
    bool publish(int channel, string message);
    //订阅消息
    bool subscribe(int channel);
    //取消订阅
    bool unsubscribe(int channel);
    //在独立线程中接收订阅消息
    void observer_channel_message();
    //初始化回调对象
    void init_notify_handler(function<void(int, string)>fn);

private:
    //同步上下文，负责发布
    redisContext *_publish_context;
    //同步上下文，复杂订阅
    redisContext *_subscribe_context;
    //回调操作，收到订阅消息上报service层
    function<void(int, string)> _notify_handler;
};


#endif