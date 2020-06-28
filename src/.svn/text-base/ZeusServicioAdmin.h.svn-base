#ifndef ZEUSSERVICIOADMIN_ZEUSSERVICIOADMIN_H_
#define ZEUSSERVICIOADMIN_ZEUSSERVICIOADMIN_H_

#include <ace/Singleton.h>
#include <list>

namespace ZeusServicioAdmin
{

class ConexionAdmin;

class ZeusServicioAdminImpl
{
    std::size_t conexionesAdminMax;    
    int socket;    
    pthread_t thread; 
    bool* finalizar;
    
    typedef std::list<ConexionAdmin*> Conexiones;
    Conexiones conexiones;
    
    void desconectar();
    
    static void* startThread(void* selfPtr);
    
public:
	ZeusServicioAdminImpl();    
	~ZeusServicioAdminImpl();
    
    bool open(int puerto);
    void close();
    void join();
    
    
    void startService();
    
    void setFinalizarPtr(bool* finalizar);
    void setConexionesMax(std::size_t conexionesMax);
};

typedef ACE_Singleton<ZeusServicioAdminImpl, ACE_Recursive_Thread_Mutex> ZeusServicioAdmin;

}

#endif /*ZEUSSERVICIOADMIN_ZEUSSERVICIOADMIN_H_*/
