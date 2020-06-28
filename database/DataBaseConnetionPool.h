#ifndef DATABASECONNETIONPOOL_H_
#define DATABASECONNETIONPOOL_H_

#include <ace/Singleton.h>
#include <ace/Condition_T.h>
#include <mysql/mysql.h>
#include <queue>

class Configuracion;

class DataBaseConnetionPoolImpl
{
    friend class ACE_Singleton<DataBaseConnetionPoolImpl, ACE_Recursive_Thread_Mutex>;
    
    DataBaseConnetionPoolImpl();
    
    ACE_Thread_Mutex mutex;
    ACE_Condition<ACE_Thread_Mutex> condition; 
    
    typedef std::queue<MYSQL*> MySqlQueue;
    MySqlQueue queue;
        
    MYSQL* get();
    
    MYSQL* put(MYSQL* mysql);
    
    friend class DataBaseConnetion;
        
public:
	
	virtual ~DataBaseConnetionPoolImpl();
    
    void init(Configuracion const& config);
};

typedef ACE_Singleton<DataBaseConnetionPoolImpl, ACE_Recursive_Thread_Mutex> DataBaseConnetionPool;

class DataBaseConnetion
{
    MYSQL* mysql;
    DataBaseConnetionPoolImpl* dbPool;
    
public:
    explicit DataBaseConnetion(DataBaseConnetionPoolImpl* dbPool_);
    DataBaseConnetion();
    ~DataBaseConnetion();
    
    MYSQL* get();
};


#endif /*DATABASECONNETIONPOOL_H_*/
