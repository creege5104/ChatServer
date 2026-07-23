#include "redis.h"
#include <iostream>
using namespace std;

Redis::Redis():_publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if(_publish_context != nullptr)redisFree(_publish_context);
    if(_subscribe_context != nullptr)redisFree(_subscribe_context);

}
//连接服务器
bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(_publish_context == nullptr)
    {
        cerr<<"connect redis failed"<<endl;
        return false;
    }
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if(_subscribe_context == nullptr)
    {
        cerr<<"connect redis failed"<<endl;
        return false;
    }

    thread t(
        [&](){
            observer_channel_message();
        }
    );
    t.detach();
    cout<<"connect redis-server success!"<<endl;
    
    return true;

}
//向指定通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *rep = (redisReply*)redisCommand(_publish_context, 
        "PUBLISH %d %s", channel, message.c_str());
    if(rep == nullptr)
    {
        cerr<<"publish command failed"<<endl;
        return false;
    }
    freeReplyObject(rep);
    return true;
}
//订阅消息
bool Redis::subscribe(int channel)
{
    //防止阻塞，将命令写入缓存并发送
    if(redisAppendCommand(_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr<<"subscribe command failed"<<endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(redisBufferWrite(_subscribe_context, &done) == REDIS_ERR)
        {
            cerr<<"subscribe command failed"<<endl;
            return false;
        }
    }
    return true;
}
//取消订阅
bool Redis::unsubscribe(int channel)
{
    //防止阻塞，将命令写入缓存并发送
    if(redisAppendCommand(_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr<<"unsubscribe command failed"<<endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(redisBufferWrite(_subscribe_context, &done) == REDIS_ERR)
        {
            cerr<<"unsubscribe command failed"<<endl;
            return false;
        }
    }
    return true;
}
//在独立线程中接收订阅消息
void Redis::observer_channel_message()
{
    redisReply *rep = nullptr;
    while(redisGetReply(_subscribe_context, (void**)&rep)==REDIS_OK)
    {
        if(rep != nullptr && rep->element[2] != nullptr && rep->element[2]->str != nullptr)
        {
            _notify_handler(atoi(rep->element[1]->str), rep->element[2]->str);
        }
        freeReplyObject(rep);
    }
    cerr<<"observer_channel_message quit"<<endl;
}
//初始化回调对象
void Redis::init_notify_handler(function<void(int, string)>fn)
{
    _notify_handler = fn;
}
