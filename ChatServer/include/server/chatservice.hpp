#ifndef CHAT_SERVICE_H
#define CHAT_SERVICE_H

#include <unordered_map>
#include <functional>
#include <mutex>
#include <muduo/net/TcpConnection.h>
#include "redis.hpp"
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;

using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

class ChatService
{
public:
    static ChatService* instance();
    
    void defultHanlder(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void clientCloseException(const TcpConnectionPtr &conn);

    void reset();

    void handleRedisSubscribeMessage(int channel, string message);
    
    MsgHandler getHandler(int);

private:
    ChatService();

    unordered_map<int, MsgHandler> _msgHandlerMap;
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel; 

    mutex _connMutex;
    Redis _redis;
};

#endif