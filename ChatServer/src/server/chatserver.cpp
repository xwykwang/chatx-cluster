#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
using namespace std;
using namespace nlohmann;

ChatServer::ChatServer(EventLoop* loop, const InetAddress &listenAddr, const string &nameArg)
    :_server(loop, listenAddr, nameArg),
    _loop(loop)
{
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, placeholders::_1));

    _server.setMessageCallback(bind(&ChatServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));

    _server.setThreadNum(3);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    handler(conn, js, time);
}