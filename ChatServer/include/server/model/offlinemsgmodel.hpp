#ifndef OFFLINEMSGMODEL_H
#define OFFLINEMSGMODEL_H

#include <string>
#include <vector>
using namespace std;

class OfflineMsgModel
{
public:
    void insert(int userid, string msg);
    void remove(int userid);

    vector<string> query(int userid);

private:



};



#endif