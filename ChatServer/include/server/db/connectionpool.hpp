#pragma once
#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include "connection.hpp"

class ConnectionPool
{
public:
	static ConnectionPool* getConnectionPool();
	~ConnectionPool();
	shared_ptr<Connection> getConnection();
	
private:
	ConnectionPool();
	bool loadConfigFile();
	void produceConnectionTask();
	void scanConnectionTask();
	string ip_;
	unsigned short port_;
	string userName_;
	string password_;
	string dbName_;

	int initSize_;
	int maxSize_;
	int maxIdleTime_;
	int connectionTimeout_;

	queue<Connection*> connectionQue_;
	mutex queMtx_;
	atomic_int connectionSize_;
	condition_variable notEmpty_;
};