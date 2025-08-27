#pragma once
#include <mysql/mysql.h>
#include <string>
#include <ctime>
using namespace std;

class Connection
{
public:
	Connection();
	~Connection();
	bool connAlive();
	bool connect(string ip, unsigned short port, string username, string password, string dbname);
	bool update(string sql);
	MYSQL_RES* query(string sql);
    MYSQL* getMySQLConnection();
	void refreshTime() { aliveTime_ = clock(); }
	clock_t getAliveTime() const { return clock() - aliveTime_; } //空闲时间
private:
	MYSQL* conn_;
	clock_t aliveTime_;
};