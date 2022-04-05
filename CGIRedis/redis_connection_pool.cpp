#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include <hiredis/hiredis.h>
#include "redis_connection_pool.h"

using namespace std;

RedisConnectionPool::RedisConnectionPool()
{
    this->m_FreeConn=0;
    this->m_CurConn=0;
}

int CacheConn::Init()
{
	//重连
	time_t cur_time = time(NULL);
	if(cur_time < m_last_connect_time +4){
		return 1;
	}

	m_last_connect_time = cur_time;

	struct timeval timeout
	{
		0,200000
	};
	
	Cachetemp.m_pContext = redisConnectWithTimeout(m_url.c_str(), m_port.c_str(), timeout);

	if(!m_pContext || m_pContext->err)
	{
		if(m_pContext)
		{
			redisFree(m_pContext);
			m_pContext = NULL;
		}
		LOG_ERR("redis connect failed");
		return 1;
	}
	
	redisReply* reply;
	//登陆验证：
	if(!m_password.empty())
	{
		reply = (redisReply *)redisCommand(m_pContext, "AUTH %s", "");
		if(!reply || redis->type == REDIS_REPLY_ERROR)
		{
			if(reply){
				freeReplyObject(reply);
			}
			return -1;
		}
		freeReplyObject(reply);
	}

	reply = (redisReply *)redisCommand(m_pContext, "SELECT %d", 0);
	if (reply && (reply->type == REDIS_REPLY_STATUS) && (strncmp(reply->str, "OK", 2) == 0))
	{
		freeReplyObject(reply);
		return 0;
	}
	else
	{
		if (reply)
			log_error("select cache db failed:%s\n", reply->str);
		return 2;
	}

}

RedisConnectionPool::~RedisConnectionPool()
{
	DestroyRedisPoll();
}

RedisConnectionPool *RedisConnectionPool::RedisPoolInstance()
{
    static RedisConnectionPool ConPool;
    return &ConPool;
}

void RedisConnectionPool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
    m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;
	m_close_log = close_log;

	for (int i = 0; i < MaxConn; i++)
	{
		CacheConn *con = NULL;
		con = new CacheConn();

        if(con->Init() != 0 || con == NULL)
		{
			if(con->Init() == 1)
			{
				delete con;
			}
			LOG_ERROR("Redis Error");
			exit(1);
		}
		
		connList.push_back(con);
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);
	m_MaxConn = m_FreeConn;

	LOG_ERROR("cache pool: %s, list size: %lu", ConPool.c_str(), connList.size());
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
CacheConn *RedisConnectionPool::GetRedisConnection()
{
	CacheConn *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	lock.lock();

	con = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool RedisConnectionPool::RedisDisconnection(CacheConn *con)
{
	if (NULL == con)
		return false;

	lock.lock();

    list<CacheConn *>::iterator it;
	for (it = connList.begin(); it != connList.end(); ++it)
	{
		if(*it == con){
			break;
		}
	}
	if(it == connList.end())
	{
		connList.push_back(con);
	}
	++m_FreeConn;
	--m_CurConn;

	lock.unlock();

	reserve.post();
	return true;
}

//销毁数据库连接池
void RedisConnectionPool::DestroyRedisPoll()
{
	lock.lock();
	if (connList.size() > 0)
	{
		list<CacheConn *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			CacheConn *con = *it;
			redisFree(con);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		connList.clear();
	}

	lock.unlock();
}

//Redis当前空闲的连接数
int RedisConnectionPool::GetFreeRedisConnection()
{
	return this->m_FreeConn;
}

RedisConnectionPool::~RedisConnectionPool()
{
	DestroyRedisPool();
}

RedisConnectionRAII::RedisConnectionRAII(CacheConn **Con, RedisConnectionPool *connPool){
	*Con = connPool->GetRedisConnection();
	
	conRAII = *Con;
	poolRAII = connPool;
}

RedisConnectionRAII::~RedisConnectionRAII(){
	poolRAII->RedisDisconnection(conRAII);
}