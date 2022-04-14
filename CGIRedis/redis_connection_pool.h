#ifndef _REDIS_POOL_
#define _REDIS_POOL_

#include <hiredis/hiredis.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <iostream>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class CacheConn
{
public:
    int Init(string Url, int Port, int LogCtl, string r_PassWord);

	CacheConn();
	~CacheConn();
public:
    redisContext* m_pContext;
	int RedisLogCtl;
private:
    int m_last_connect_time;
	string R_password;
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
    friend class CacheConn;
	string m_Url;			 //主机地址
	string m_Port;		 //数据库端口号
	string m_User;		 //登陆数据库用户名
	string m_PassWord;	 //登陆数据库密码
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