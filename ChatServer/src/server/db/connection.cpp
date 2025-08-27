#include <muduo/base/Logging.h>
#include "connection.hpp"

Connection::Connection()
{
	conn_ = mysql_init(nullptr);
}

Connection::~Connection()
{
	if (conn_ != nullptr)
	{
		mysql_close(conn_);
	}

}

MYSQL* Connection::getMySQLConnection()
{
    return conn_;
}

bool Connection::connAlive()
{
	if (conn_ != nullptr)
	{
		int flag = mysql_ping(conn_);
		if(flag != 0)
		{
			LOG_INFO << "connection invalid mysql return :" << flag;
			return false;
		}
		return true;
	}
	LOG_INFO << "empty mysql connection !";
	return false;
}

bool Connection::connect(string ip, unsigned short port, string username, string password, string dbname)
{
	MYSQL* p = mysql_real_connect(conn_, ip.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
	if (p == nullptr)
	{
		LOG_ERROR << "连接数据库失败";
		return false;
	}
    else
    {
        mysql_query(conn_, "set name gbk");
        LOG_INFO << "连接数据库成功!";
    }
    
	return true;
}

bool Connection::update(string sql)
{
	if (mysql_query(conn_, sql.c_str()))
	{
		LOG_ERROR << "更新失败: " + sql;
		return false;
	}
	return true;
}

MYSQL_RES* Connection::query(string sql)
{
	if (mysql_query(conn_, sql.c_str()))
	{
		LOG_ERROR << "查询失败：" + sql;
		return nullptr;
	}
	return mysql_use_result(conn_);
}