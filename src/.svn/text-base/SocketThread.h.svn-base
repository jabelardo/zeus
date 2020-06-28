#ifndef ZEUS_SOCKETTHREAD_H_
#define ZEUS_SOCKETTHREAD_H_

#include <boost/noncopyable.hpp>
#include <cstdlib>
#include <map>
#include <vector>
#include <ace/Recursive_Thread_Mutex.h>
#include <ConexionActiva.h>

class SocketThread : boost::noncopyable
{
public:
    SocketThread();
    virtual ~SocketThread();
       
    void wait();
    void post();  

    static void remove(pthread_t thread);
    static void free(SocketThread * socketsThread);
    static SocketThread* get(pthread_t thread);
    static SocketThread* getIdle();
    static std::size_t size();
    static std::size_t idleSize(); 
    static void add(SocketThread* socketsThread);
    static void close();        
    
    static void* run(void *arg);
    static void addNewIdle();
    
    static void readIPCMessage(SocketThread * socketsThread, boost::uint8_t * message);
    
    struct MutexGuard
    {
        MutexGuard();
        ~MutexGuard();
    };
    
    ConexionActiva& getConexionTaquilla();
    ConexionActiva& getConexionIPCExterna();
    ConexionActiva& getConexionIPCInterna();
    
    void setConexionTaquilla(std::auto_ptr<ConexionActiva> conexionActiva);
    void setConexionIPCExterna(std::auto_ptr<ConexionActiva> conexionActiva);
    void setConexionIPCInterna(std::auto_ptr<ConexionActiva> conexionActiva);
    
    pthread_t getThread() const;
    
private:
    std::auto_ptr<ConexionActiva> conexionTaquilla;  
    std::auto_ptr<ConexionActiva> conexionExternaIPC;
    std::auto_ptr<ConexionActiva> conexionInternaIPC;
    sem_t mutex;
    pthread_t thread;
    
    typedef std::map<pthread_t, SocketThread*> SocketThreads;
    static SocketThreads socketsThreads;
    static ACE_Recursive_Thread_Mutex socketsThreadsMutex;
    
    static std::vector<pthread_t> idles;
};

#endif /*ZEUS_SOCKETTHREAD_H_*/
