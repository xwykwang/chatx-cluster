#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
    :_publish_context(nullptr),
    _subscribe_context(nullptr)
{

}

Redis::~Redis()
{
    if(_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if(_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(_publish_context == nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }
    else
    {
        // 2. 认证
        redisReply *reply = (redisReply *)redisCommand(
            _publish_context,
            "AUTH %s",
            "299404" // 密码
        );

        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
            cerr << "Authentication failed: "<< (reply ? reply->str : "No reply")<< endl;
            freeReplyObject(reply);
            redisFree(_publish_context);
            return false;
        }
        else
        {
            cout<<reply->str<<endl;
        }

        freeReplyObject(reply); // 释放认证响应
        cout << "Redis authenticated!" << endl;
    }

    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if(_subscribe_context == nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }
    else
    {
        // 2. 认证
        redisReply *reply = (redisReply *)redisCommand(
            _subscribe_context,
            "AUTH %s",
            "299404" // 密码
        );

        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
            cerr << "Authentication failed: "<< (reply ? reply->str : "No reply")<< endl;
            freeReplyObject(reply);
            redisFree(_subscribe_context);
            return false;
        }
        else
        {
            cout<<reply->str<<endl;
        }

        freeReplyObject(reply); // 释放认证响应
        cout << "Redis authenticated!" << endl;
    }

    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout<<"connect redis-server success!"<<endl;
    return true;
}

bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr)
    {
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    else
    {
        cout<<"publish message "<<message<< " to id "<< channel<<endl;
    }

    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
    cout << "subscribe id:" << channel << endl;
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr<<"subscribe command failed!"<<endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr<<"subscribe command failed!"<<endl;
            return false;
        }
    }
    //redisGetReply
    return true;
}

bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }
    return true;
}

void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        cout<<"reply type: "<<reply->type<<endl;
        if(reply != nullptr && reply->type == REDIS_REPLY_ERROR) {
            cerr << "Redis error: " << reply->str << endl;
        }
        else if(reply != nullptr && reply->type == REDIS_REPLY_ARRAY&& reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>> observer_channel_message quit<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
    _notify_message_handler = fn;
}
