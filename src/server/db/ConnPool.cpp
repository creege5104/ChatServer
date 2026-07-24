#include "ConnPool.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <limits.h>
#include <filesystem>

using json = nlohmann::json;
using std::thread;
using std::mutex;
using std::condition_variable;
using namespace std::chrono;

static std::string getConfigPath()
{
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        std::filesystem::path exePath(buf);
        return (exePath.parent_path() / ".." / "config" / "dbconf.json").lexically_normal().string();
    }
    return "config/dbconf.json";
}


bool ConnPool::parseJsonFile()
{
	std::ifstream ifs(getConfigPath());
    if (!ifs.is_open()) {
        return false;
    }
	try	{
		json js;
		ifs>>js;
		m_ip = js["ip"];
		m_port = js["port"];
		m_user = js["userName"];
		m_password = js["passWord"];
		m_dbName = js["dbName"];
		m_minSize = js["minSize"];
		m_maxSize = js["maxSize"];
		m_timeOut = js["timeOut"];
		m_maxIdleTime = js["maxIdleTime"];
		return true;
	}
	catch (const json::exception& e) {
        return false;
    }
}

void ConnPool::produceConn()
{
	while (true)
	{
		std::unique_lock<mutex>locker(m_mutexQ);
		while (m_connQ.size() >= m_minSize)
		{
			m_cond.wait(locker);
		}
		addConn();
		m_cond.notify_all();
	}
}

void ConnPool::recycleConn()
{
	while (true)
	{
		std::this_thread::sleep_for(seconds(1));
		std::lock_guard<mutex>locker(m_mutexQ);
		while (m_connQ.size() > m_minSize)
		{
			MysqlConn* conn = m_connQ.front();
			if (conn->getAliveTime() >= m_maxIdleTime)
			{
				m_connQ.pop();
				delete conn;
			}
			else
			{
				break;
			}
		}
	}
}

void ConnPool::addConn()
{
	MysqlConn* conn = new MysqlConn;
	conn->connect(m_user, m_password, m_dbName, m_ip, m_port);
	conn->refreshAliveTime();
	m_connQ.push(conn);
}

ConnPool* ConnPool::getPool()
{
	static ConnPool pool;
	return &pool;
}

shared_ptr<MysqlConn> ConnPool::getConn()
{
	std::unique_lock<mutex>locker(m_mutexQ);
	while (m_connQ.empty())
	{
		if (m_cond.wait_for(locker, milliseconds(m_timeOut)) == std::cv_status::timeout)
		{
			if (m_connQ.empty())
			{
				//return nullptr;
				continue;
			}
		}
	}
	shared_ptr<MysqlConn> conn(m_connQ.front(), [this](MysqlConn* conn) {
		std::lock_guard<mutex>locker(m_mutexQ);
		conn->refreshAliveTime();
		m_connQ.push(conn);
		});
	m_connQ.pop();
	m_cond.notify_all();
	return conn;
}

ConnPool::~ConnPool()
{
	while (!m_connQ.empty())
	{
		MysqlConn* conn = m_connQ.front();
		m_connQ.pop();
		delete conn;
	}
}

ConnPool::ConnPool()
{
	if (parseJsonFile())
	{
		for (int i = 0; i < m_minSize; ++i)
		{
			addConn();
		}
		thread producer(&ConnPool::produceConn, this);
		thread recycler(&ConnPool::recycleConn, this);
		producer.detach();
		recycler.detach();
	}
}

