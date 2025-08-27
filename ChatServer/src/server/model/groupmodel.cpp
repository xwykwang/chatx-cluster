#include "groupmodel.hpp"
//#include "db.hpp"
#include "connectionpool.hpp"

bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        if(mysql->update(sql))
        {
            group.setId(mysql_insert_id(mysql->getMySQLConnection()));
            return true;
        }
    }
    return false;
}

void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values('%d', '%d', '%s')", groupid, userid, role.c_str());

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        mysql->update(sql);
    }
    return;
}

vector<Group> GroupModel::queryGroup(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup inner join\
         on groupuser b on a.id = b.groupid where b.userid = %d)", userid);

    vector<Group> groupvec;

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        MYSQL_RES* res = mysql->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupvec.push_back(group);
            }
            mysql_free_result(res);
        }
        
        for(Group &group : groupvec)
        {
            sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join\
                groupuser b on a.id = b.userid where b.groupid = %d)", group.getId());
            
            MYSQL_RES* res = mysql->query(sql);
            if(res != nullptr)
            {
                MYSQL_ROW row;
                while((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user);
                }
                mysql_free_result(res);
            }
        }
    }
    return groupvec;
}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> uservec;

    auto mysql = ConnectionPool::getConnectionPool()->getConnection();
    if(mysql->connAlive())
    {
        MYSQL_RES* res = mysql->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                uservec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }    
    }
    return uservec;
}
