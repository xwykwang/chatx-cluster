#include "friendmodel.hpp"
//#include "db.hpp"
#include "connectionpool.hpp"

void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values('%d', '%d')", userid, friendid);

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        mysql->update(sql);
    }
    return;
}

vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid =%d", userid);

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    vector<User> allfriends;
    if(mysql->connAlive())
    {
        MYSQL_RES* rsq = mysql->query(sql);
        if(rsq != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(rsq)) != nullptr)
            {
                User fuser;
                fuser.setId(atoi(row[0]));
                fuser.setName(row[1]);
                fuser.setState(row[2]);
                allfriends.push_back(fuser);
            }
            mysql_free_result(rsq);
        }
    }
    return allfriends; 
}