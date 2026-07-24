#ifndef _CONNPOOL_H
#define _CONNPOOL_H

#include "db.h"
#include <queue>
#include <mutex>
#include <condition_variable>
using std::queue;
using std::mutex;
using std::condition_variable;
using std::shared_ptr;

class ConnPool
{
public:
	static ConnPool* getPool();
	ConnPool(const ConnPool& pool) = delete;
	ConnPool& operator=(const ConnPool& pool) = delete;
	shared_ptr<MysqlConn> getConn();
	~ConnPool();

private:
	ConnPool();
	bool parseJsonFile();
	void produceConn();
	void recycleConn();
	void addConn();

	string m_ip;
	string m_user;
	string m_password;
	string m_dbName;
	unsigned short m_port;
	int m_minSize, m_maxSize;
	int m_timeOut;
	int m_maxIdleTime;
	queue<MysqlConn*>m_connQ;
	mutex m_mutexQ;
	condition_variable m_cond;

};

#endif
