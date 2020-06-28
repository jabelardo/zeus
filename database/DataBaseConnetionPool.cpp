#include <database/DataBaseConnetionPool.h>
#include <global.h>
#include <stdexcept>
    
DataBaseConnetion::DataBaseConnetion()
    : mysql(0)
    , dbPool(DataBaseConnetionPool::instance())
{
}

DataBaseConnetion::DataBaseConnetion(DataBaseConnetionPoolImpl* dbPool_)
    : mysql(0)
    , dbPool(dbPool_)
{
}

DataBaseConnetion::~DataBaseConnetion()
{
    if (mysql) dbPool->put(mysql);
}
    
MYSQL* 
DataBaseConnetion::get()
{
    if (!mysql) mysql = dbPool->get();
    return mysql;
}

DataBaseConnetionPoolImpl::DataBaseConnetionPoolImpl()
    : mutex()    
    , condition(mutex)
    , queue()
{
}

DataBaseConnetionPoolImpl::~DataBaseConnetionPoolImpl()
{
    while (!queue.empty()) {
        MYSQL* mysql = get();
        mysql_close(mysql);
        delete mysql;
    }
}

void
DataBaseConnetionPoolImpl::init(Configuracion const& config)
{
    ACE_Guard<ACE_Thread_Mutex> mon(mutex);
    
    for (size_t i = 0; i < config.dbConnections; ++i) {
        MYSQL* mysql = new MYSQL;
        mysql_init(mysql);
        if (!mysql_real_connect(mysql, config.dbServer, config.dbUser, config.dbPassword,
                                config.dbZeusName, config.dbPort, NULL, 0)) {
            throw std::runtime_error(mysql_error(mysql));      
        }        
        queue.push(mysql);
    } 
}

MYSQL* 
DataBaseConnetionPoolImpl::get()
{
    ACE_Guard<ACE_Thread_Mutex> mon(mutex);
    MYSQL* result = NULL;
    while (queue.empty()) {
        condition.wait();
    }
    result = queue.front();
    queue.pop();
    return result;
}
    
MYSQL*
DataBaseConnetionPoolImpl::put(MYSQL* mysql)
{
    ACE_Guard<ACE_Thread_Mutex> mon(mutex);
    queue.push(mysql);    
    condition.signal();
    return NULL;
}
