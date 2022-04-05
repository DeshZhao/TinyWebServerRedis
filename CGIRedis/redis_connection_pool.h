#ifndef _REDIS_CONNECTION_POOL_
#define _REDIS_CONNECTION_POOL_

#include <../hiredis/hiredis.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "../lock/locker.h"
#include "redis_connection_pool.h"

using namespace std;

class CacheConn
{
public:
    int Init();
public:
    redisContext* m_pContext = NULL;
private:
    int m_last_connect_time = 0;
private:
    CacheConn();
	~CacheConn();
}

class RedisConnectionPool
{
public:
    static CacheConn* GetRedisConnection();  //遍历list<CacheConn*>,Singleton
	void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log); 
	
	static RedisConnectionPool *RedisPoolInstance();
	int GetFreeRedisConnection();
    bool RedisDisconnection(CacheConn* Conn);
    void DestroyRedisPoll();
private:
    RedisConnectionPool();
    ~RedisConnectionPool();

	int m_MaxConn;  //最大连接数
	int m_CurConn;  //当前已使用的连接数
	int m_FreeConn; //当前空闲的连接数
	locker lock;
	list<CacheConn *> connList; //连接池
	sem reserve;

public:
	string m_url;			 //主机地址
	string m_Port;		 //数据库端口号
	string m_User;		 //登陆数据库用户名
	string m_PassWord;	 //登陆数据库密码
	string m_DatabaseName; //使用数据库名
   // CacheConn* pm_rct;  //redis结构体
	int m_close_log;	//日志开关
}

class RedisConnectionRAII{

public:
	RedisConnectionRAII(CacheConn **con, RedisConnectionPool *connPool);
	~RedisConnectionRAII();
	
private:
	CacheConn *conRAII;
	RedisConnectionPool *poolRAII;
	std::unordered_map<string, RedisConnectionPool>m_cache_redisPool_Map;
};
#endif