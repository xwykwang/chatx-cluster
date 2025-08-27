#ifndef USER_H
#define USER_H

#include <string>
using namespace std;


class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    int getId() { return id; }
    string getName() { return name; }
    string getPassWord() { return password; }
    string getState() { return state; }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPassWord(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

protected:
    int id;
    string name;
    string password;
    string state;
};

#endif