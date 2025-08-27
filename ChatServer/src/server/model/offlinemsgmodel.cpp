#include "offlinemsgmodel.hpp"
//#include "db.hpp"
#include "connectionpool.hpp"

void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values('%d', '%s')", userid, msg.c_str());

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        mysql->update(sql);
    }
    return;
}

void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid =%d", userid);

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        mysql->update(sql);
    }
    return;

}

vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid =%d", userid);

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    vector<string> rmsg;
    if(mysql->connAlive())
    {
        MYSQL_RES* rsq = mysql->query(sql);
        if(rsq != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(rsq)) != nullptr)
            {
                rmsg.push_back(row[0]);
            }
            mysql_free_result(rsq);
        }
    }
    return rmsg; 
}
