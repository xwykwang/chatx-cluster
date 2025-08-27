#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <atomic>
using namespace std;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <semaphore.h>

#include "json.hpp"
using namespace nlohmann;

#include "user.hpp"
#include "public.hpp"
#include "group.hpp"

User g_currentUser;

bool onMainMenu = false;

sem_t rwsem;

atomic_bool g_isLoginSucess(false);

vector<User> g_currentUserFriendList;

vector<Group> g_currentUserGroupList;

void help(int = 0, string = "");
void chat(int, string);
void addFriend(int, string);
void createGroup(int, string);
void addGroup(int, string);
void groupChat(int, string);
void loginOut(int, string);


unordered_map<string,string> commandMap = {
    {"help", "show all command, form::help"},
    {"chat", "chat with another id, form::chat:friendid:message"},
    {"addfriend", "add friend with userid, form::addfriend:friendid"},
    {"creategroup", "create a chat group, form::creategroup:groupname:groupdesc"},
    {"addgroup", "join in a chat group, form::addgroup:groupid"},
    {"groupchat", "chat in a group, form::groupchat:groupid:message"},
    {"loginout", "login out, form::loginout"},
};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addFriend},
    {"creategroup", createGroup},
    {"addgroup", addGroup},
    {"groupchat", groupChat},
    {"loginout", loginOut},
};

void showCurrentUserData();

void readTaskHandler(int clientfd);

string getCurrentTime();

void mainMenu(int clientfd);

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1)
    {
        cerr<<"socker create error"<<endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if(connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr<<"connected server error"<<endl;
        close(clientfd);    
        exit(-1);
    }

    sem_init(&rwsem, 0, 0);

    thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    for(;;)
    {
        cout<<"==============================="<<endl;
        cout<<"1, login"<<endl;
        cout<<"2, register"<<endl;
        cout<<"3, quit"<<endl;
        cout<<"==============================="<<endl;
        cout<<"choice:";
        int choice = 0;
        cin>>choice;
        cin.get();

        switch(choice)
        {
            case 1://login
            {
                int id = 0;
                char pwd[50] = {0};
                cout<<"userid:";
                cin>>id;
                cin.get();
                cout<<"password:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                g_isLoginSucess = false;

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1)
                {
                    cerr<<"send login msg error"<<endl;
                }
                else
                {
                    sem_wait(&rwsem);
                }

                if(g_isLoginSucess)
                {
                    mainMenu(clientfd);
                }
                else
                {
                    cout<<"login failed"<<endl;
                }

            }
            break;

            case 2:
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout<<"username:";
                cin.getline(name, 50);
                cout<<"password:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1)
                {
                    cerr<<"send register msg error"<< endl;
                }
                else
                {
                    sem_wait(&rwsem);
                }
            }
            break;
            
            case 3:
            {
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
            }
            break;

            default:
            {
                cout<<"invalid input!"<<endl;
            }
            break;
        }
    }
}


void showCurrentUserData()
{
    cout<<"==============login=============="<<endl;
    cout<<"User:"<<g_currentUser.getName()<<endl;
}

void doRegRes(json &response)
{
    if (response["errno"].get<int>() != 0)
    {
        cerr << response["errmsg"] << endl;
    }
    else
    {
        cout << "register success, userid is " << response["id"] << ", do not forget it!" << endl;
    }
}

void doLoginRes(json &response)
{

    //json response = json::parse(buffer);
    if (response["errno"].get<int>() != 0)
    {
        cerr << response["errmsg"] << endl;
        g_isLoginSucess = false;
    }
    else
    {
        g_currentUser.setId(response["id"].get<int>());
        g_currentUser.setName(response["name"]);

        if (response.contains("friends"))
        {
            g_currentUserFriendList.clear();
            vector<string> fvec = response["friends"];
            for (string &str : fvec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        if (response.contains("groups"))
        {
            g_currentUserGroupList.clear();
            vector<string> gvec = response["groups"];
            for (string &gstr : gvec)
            {
                json gpjs = json::parse(gstr);
                Group group;
                group.setId(gpjs["id"].get<int>());
                group.setName(gpjs["groupname"]);
                group.setDesc(gpjs["groupdesc"]);

                vector<string> guvec = gpjs["users"];
                for (string &ustr : guvec)
                {
                    GroupUser user;
                    json ujs = json::parse(ustr);
                    user.setId(ujs["id"].get<int>());
                    user.setName(ujs["name"]);
                    user.setState(ujs["state"]);
                    user.setRole(ujs["role"]);
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        showCurrentUserData();

        if (response.contains("offlinemsg"))
        {
            vector<string> omvec = response["offlinemsg"];
            cout << "There are " << omvec.size() << " offline messages" << endl;
            for (string &str : omvec)
            {
                json js = json::parse(str);
                if (js["msgid"].get<int>() == ONE_CHAT_MSG)
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                    continue;
                }
                else
                {
                    cout << "Group[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                    continue;
                }
            }
        }
        g_isLoginSucess = true;
    }
}
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if(len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgid = js["msgid"].get<int>();
        if(msgid == ONE_CHAT_MSG)
        {
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(msgid == GROUP_CHAT_MSG)
        {
            cout<<"Group["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(msgid == LOGIN_ACK_MSG)
        {
            doLoginRes(js);
            sem_post(&rwsem);
            continue;
        }
        if(msgid == REG_ACK_MSG)
        {
            doRegRes(js);
            sem_post(&rwsem);
            continue;
        }
    }
}

string getCurrentTime()
{
    return "";
}

void mainMenu(int clientfd)
{
    help();
    onMainMenu = true;
    char buffer[1024] = {0};
    while(onMainMenu)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(':');
        if(idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }

        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));

    }

}

void help(int, string)
{
    cout<<"show command list: "<<endl;
    for(auto &it : commandMap)
    {
        cout<<it.first<<" : "<<it.second<<endl;
    }
}

void addFriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr<<"send addfriend msg error"<<endl;
    }
}

void chat(int clientfd, string str)
{
    int idx = str.find(':');
    if(idx == -1)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send chat message error -> " << buffer << endl;
    }
}

void createGroup(int clientfd, string str)
{
    int idx = str.find(':');
    if(idx == -1)
    {
        cerr << "create group command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send create group message error -> " << buffer << endl;
    }
}

void addGroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send add group message error -> " << buffer <<endl;
    }
}

void groupChat(int clientfd, string str)
{
    int idx = str.find(':');
    if(idx == -1)
    {
        cerr << "group chat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send group chat message error -> " << buffer <<endl;
    }
}

void loginOut(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send login out message error -> " << buffer <<endl;
    }
    else
    {
        onMainMenu = false;
    }

}