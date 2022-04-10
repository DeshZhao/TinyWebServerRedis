#ifndef _REDIS_CONNECTION_POOL_
#define _REDIS_CONNECTION_POOL_

#include <hiredis/hiredis.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "../lock/locker.h"
#include "../log/log.h"
#include <map>

using namespace std;

class CacheConn
{
public:
    int Init(string Url, string Port, int LogCtl);

	CacheConn();
	~CacheConn();
public:
    redisContext* m_pContext = NULL;
private:
    int m_last_connect_time = 0;
};

class RedisConnectionPool
{
public:
    CacheConn* GetRedisConnection();  //遍历list<CacheConn*>
	void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log); 
	
	static RedisConnectionPool *RedisPoolInstance();
	int GetFreeRedisConnection();
    bool RedisDisconnection(CacheConn* Conn);
    void DestroyRedisPool();

	friend class CacheConn;
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
	string m_Url;			 //主机地址
	string m_Port;		 //数据库端口号
	string m_User;		 //登陆数据库用户名
	static string m_PassWord;	 //登陆数据库密码
	string m_DatabaseName; //使用数据库名
   // CacheConn* pm_rct;  //redis结构体
	int m_close_log;	//日志开关
};

class RedisConnectionRAII{
public:
	RedisConnectionRAII(CacheConn **con, RedisConnectionPool *connPool);
	~RedisConnectionRAII();
	
private:
	CacheConn *conRAII;
	RedisConnectionPool *poolRAII;
	//map<string, RedisConnectionPool>m_cache_redisPool_Map;
};
#endif