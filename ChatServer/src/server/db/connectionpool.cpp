#include <muduo/base/Logging.h>
#include "connectionpool.hpp"

ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

ConnectionPool::ConnectionPool()
	:connectionSize_(0)
{
	if (!loadConfigFile())
	{
		return;
	}

	for (int i = 0; i < initSize_; i++)
	{
		Connection* p = new Connection();
		p->connect(ip_, port_, userName_, password_, dbName_);
		p->refreshTime();
		connectionQue_.push(p);
		connectionSize_++;

	}

	thread produce(bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();
	thread scaner(bind(&ConnectionPool::scanConnectionTask, this));
	scaner.detach();
}

ConnectionPool::~ConnectionPool()
{

}

bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("./bin/mysql.cnf", "r");
	if (pf == nullptr)
	{
		LOG_ERROR << "加载配置文件 mysql.cnf 失败！errno:" << errno;
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1)
		{
			continue;
		}

		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
		{
			ip_ = value;
		}
		else if (key == "port")
		{
			port_ = atoi(value.c_str());
		}
		else if (key == "username")
		{
			userName_ = value;
		}
		else if (key == "password")
		{
			password_ = value;
		}
		else if (key == "dbname")
		{
			dbName_ = value;
		}
		else if (key == "initsize")
		{
			initSize_ = atoi(value.c_str());
		}
		else if (key == "maxsize")
		{
			maxSize_ = atoi(value.c_str());
		}
		else if (key == "maxidletime")
		{
			maxIdleTime_ = atoi(value.c_str());
		}
		else if (key == "connnectiontimeout")
		{
			connectionTimeout_ = atoi(value.c_str());
		}
	}

	return true;
}

void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(queMtx_);
		while (!connectionQue_.empty())
		{
			notEmpty_.wait(lock);
		}
		if (connectionSize_ < maxSize_)
		{
			Connection* p = new Connection();
			p->connect(ip_, port_, userName_, password_, dbName_);
			p->refreshTime();
			connectionQue_.push(p);
			connectionSize_++;
		}
		notEmpty_.notify_all();
	}
}

void ConnectionPool::scanConnectionTask()
{
	for (;;)
	{
		this_thread::sleep_for(chrono::seconds(maxIdleTime_));
		{
			unique_lock<mutex> lock(queMtx_);
			while (connectionSize_ > initSize_)
			{
				Connection* p = connectionQue_.front();
				if (p->getAliveTime() >= maxIdleTime_ * 1000)
				{
					connectionQue_.pop();
					connectionSize_--;
					delete p;
				}
				else
				{
					break;
				}
			}
		}
	}

}

shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(queMtx_);
	while (connectionQue_.empty())
	{
		if (notEmpty_.wait_for(lock, chrono::milliseconds(connectionTimeout_)) == cv_status::timeout)
		{
			if (connectionQue_.empty())
			{
				LOG_INFO << "获取连接超时...";
				return nullptr;
			}
		}
	}

	shared_ptr<Connection> sp(connectionQue_.front(), [&](Connection *pcon) {
		unique_lock<mutex> lock(queMtx_);
		pcon->refreshTime();
		connectionQue_.push(pcon);
		});
	connectionQue_.pop();

	notEmpty_.notify_all();
	return sp;
	
}