#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    ChatServer(EventLoop * loop, const InetAddress &listenAddr, const string& nameArg)
        :_server(loop, listenAddr, nameArg),
        _loop(loop)
    {
        _server.setConnectionCallback(bind(&ChatServer::onConnection,this, placeholders::_1));

        _server.setMessageCallback(bind(&ChatServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));

        _server.setThreadNum(3);
    }

    void start()
    {
        _server.start();
    }


private:

    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<" state:online"<< endl;
        }
        else
        {
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<" state:offline"<< endl;
            conn->shutdown();

        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer *buffer, Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        cout<<"recv data:"<<buf<<" time:"<< time.toString()<<endl;
        conn->send(buf);
    }

    TcpServer _server;
    EventLoop* _loop;

};

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 8000);
    string name = "ChatServer";
    ChatServer server(&loop, addr, name);
    
    server.start();
    loop.loop();
}