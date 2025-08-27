#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <iostream>

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    _msgHandlerMap.insert({DEFAULT_MSG, bind(&ChatService::defultHanlder, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginOut, this, placeholders::_1, placeholders::_2, placeholders::_3)});

    _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage, this, placeholders::_1, placeholders::_2));
    if(!_redis.connect())
    {
        exit(1);
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR<<"msgid:"<<msgid<<" not exist in msgHandlerMap!"; 
        };
    }
    else
    {
        return it->second;
    }
}

void ChatService::defultHanlder(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"do default service";
}

void ChatService::reset()
{
    _userModel.resetState();
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"do login service";
    int id = js["id"];
    string pwd = js["password"];
    User user = _userModel.query(id);

    json response;
    if(user.getId() != -1 && pwd == user.getPassWord())
    {
        if(user.getState() == "online")
        {
            response["msgid"] = LOGIN_ACK_MSG;
            response["id"] = id;
            response["errno"] = 1;
            response["errmsg"] = "user is online";
        }
        else
        {
            user.setState("online");
            _userModel.updateState(user);

            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            
            _redis.subscribe(id);

            response["msgid"] = LOGIN_ACK_MSG;
            response["id"] = id;
            response["errno"] = 0;
            response["name"] = user.getName();

            vector<string> offlineMsg = _offlineMsgModel.query(id);
            if(!offlineMsg.empty())
            {
                response["offlinemsg"] = offlineMsg;
                _offlineMsgModel.remove(id);
            }
            
            vector<User> allfriends = _friendModel.query(id);
            if(!allfriends.empty())
            {
                vector<string> friendmsg;
                for(auto &fuser : allfriends)
                {
                    json js;
                    js["id"] = fuser.getId();
                    js["name"] = fuser.getName();
                    js["state"] = fuser.getState();
                    friendmsg.push_back(js.dump());
                } 
                response["friends"] = friendmsg;
            }
        }
    }
    else
    {
        response["msgid"] = LOGIN_ACK_MSG;
        response["id"] = id;
        response["errno"] = 2;
        response["errmsg"] = "user id or password error";
    }
    conn->send(response.dump());
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"do reg service";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassWord(pwd);

    bool state = _userModel.insert(user);
    json response;
    if(state)
    {
        response["msgid"] = REG_ACK_MSG;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        response["msgid"] = REG_ACK_MSG;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"];
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }

    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    _offlineMsgModel.insert(toid, js.dump());
    
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);

}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    Group group(-1, name, desc);

    if(_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> users = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int uid : users)
    {
        auto it = _userConnMap.find(uid);
        if(it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(uid);
            if(user.getState() == "online")
            {
                _redis.publish(uid, js.dump());
            }
            else
            {
                _offlineMsgModel.insert(uid, js.dump());
            }
        }
    }
}

void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    _redis.unsubscribe(userid);

    User user(userid, "", "offline");
    _userModel.updateState(user);
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it : _userConnMap)
        {
            if(it.second == conn)
            {
                user.setId(it.first);
                _userConnMap.erase(it.first);
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

void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{

    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    _offlineMsgModel.insert(userid, msg);
}

