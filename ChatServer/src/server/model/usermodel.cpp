#include "usermodel.hpp"
//#include "db.hpp"
#include "connectionpool.hpp"
#include <muduo/base/Logging.h>


bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPassWord().c_str(), user.getState().c_str());

    //MySQL mysql;
    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        if(mysql->update(sql))
        {
            user.setId(mysql_insert_id(mysql->getMySQLConnection()));
            return true;
        }
    }

    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);
    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive()) 
    {
        MYSQL_RES* res = mysql->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassWord(row[2]);
                user.setState(row[3]);

                mysql_free_result(res);
                //LOG_INFO << user.getId() << user.getName() << user.getPassWord()<< user.getState();
                return user;
            }
        }
        else
        {
            LOG_INFO << "empty query : " << sql;
        }
    }

    return User();
}

bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",user.getState().c_str(), user.getId());
    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        if(mysql->update(sql))
        {
            return true;
        }
    }
    return false;
}

void UserModel::resetState()
{
    char sql[1024] = {"update user set state = 'offline' where state = 'online'"};
    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        mysql->update(sql);
    }
}