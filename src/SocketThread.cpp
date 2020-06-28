#include <SocketThread.h>
#include <global.h>
#include <sockets.h>
#include <utils.h>
#include <atender_peticion.h>
#include <database/DataBaseConnetionPool.h>

#include <ProtocoloZeus/Mensajes.h>
#include <ProtocoloZeus/Protocolo.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sstream>

#define DEBUG_LOGOUT 0

SocketThread::SocketThreads SocketThread::socketsThreads;
ACE_Recursive_Thread_Mutex SocketThread::socketsThreadsMutex;
std::vector<pthread_t> SocketThread::idles;
 
SocketThread::SocketThread()
{
    sem_init(&mutex, 0, 0);
}

void
SocketThread::wait()
{
    sem_wait(&mutex);
}

void 
SocketThread::post()
{
    int sem_val;
    sem_getvalue(&mutex, &sem_val);
    if (sem_val == 0) {
        sem_post(&mutex);
    }
}
    
SocketThread::~SocketThread()
{
    sem_destroy(&mutex);
}

std::size_t 
SocketThread::size()
{
    MutexGuard g;
    return socketsThreads.size();
}

std::size_t 
SocketThread::idleSize()
{
    MutexGuard g;
    return idles.size();
}

void 
SocketThread::add(SocketThread* socketsThread)
{
    MutexGuard g;
    socketsThreads.insert(std::make_pair(socketsThread->thread, socketsThread));
}

SocketThread* 
SocketThread::get(pthread_t thread)
{
    MutexGuard g;
    SocketThreads::iterator iter = socketsThreads.find(thread);
    return (iter == socketsThreads.end()) ? NULL : iter->second;
}

SocketThread* 
SocketThread::getIdle()
{
    MutexGuard g;
    if (!idleSize()) return 0;
    pthread_t idleThread = idles.back();
    idles.pop_back();
    return get(idleThread);
}

void 
SocketThread::remove(pthread_t thread)
{
    MutexGuard g;
    socketsThreads.erase(thread);                 
}

void
SocketThread::free(SocketThread * socketsThread)
{   
    MutexGuard g;
    SocketThread::remove(socketsThread->thread);
   
    ConexionActiva& conexionActiva = socketsThread->getConexionTaquilla();

    if (conexionActiva.sd != 0) {
        send2cliente(conexionActiva, ProtocoloZeus::DESCONECCION, NULL, 0);
    }
    
    ::close(socketsThread->conexionInternaIPC->sd);
    ::close(socketsThread->conexionExternaIPC->sd);
    delete socketsThread;
}


SocketThread::MutexGuard::MutexGuard()
{
    socketsThreadsMutex.acquire();
}

SocketThread::MutexGuard::~MutexGuard()
{
    socketsThreadsMutex.release();
}

void
SocketThread::close()
{
    MutexGuard g;
    
    std::auto_ptr<CabeceraInterna> cabecera(new CabeceraInterna);
    cabecera->peticion = ProtocoloZeus::DESCONECCION;
    cabecera->longitud = 0;
    
    for (SocketThreads::iterator iter = socketsThreads.begin(); 
         iter != socketsThreads.end(); ++iter) {
        send2hilo_socket(iter->first, cabecera.get());
     }
}

ConexionActiva& 
SocketThread::getConexionTaquilla()
{
    return *conexionTaquilla.get();
}

ConexionActiva& 
SocketThread::getConexionIPCExterna()
{
    return *conexionExternaIPC.get();
}

ConexionActiva& 
SocketThread::getConexionIPCInterna()
{
    return *conexionInternaIPC.get();
}

void 
SocketThread::setConexionTaquilla(std::auto_ptr<ConexionActiva> conexionActiva)
{
    conexionTaquilla = conexionActiva;
}

void 
SocketThread::setConexionIPCExterna(std::auto_ptr<ConexionActiva> conexionActiva)
{
    conexionExternaIPC = conexionActiva;
}

void 
SocketThread::setConexionIPCInterna(std::auto_ptr<ConexionActiva> conexionActiva)
{
    conexionInternaIPC = conexionActiva;
}

pthread_t 
SocketThread::getThread() const
{
    return thread;
}

void 
SocketThread::addNewIdle()
{
    if (SocketThread::size() > maxThreadNumber) {
        return;
    }
    
    SocketThread* socketsThread = new SocketThread;
    
    if (pthread_create(&socketsThread->thread, NULL, run, socketsThread) != 0)
        g_error("pthread_create()");

    char mensaje[BUFFER_SIZE];
    sprintf(mensaje, "Creado hilo_socket <%u>", (unsigned int)socketsThread->thread);
    log_mensaje(NivelLog::Detallado, "mensaje", "servidor", mensaje);
        
    int socketsInternos[2];
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socketsInternos) == -1)
        g_error("socketpair()");
    
    
    socketsThread->setConexionIPCExterna(ConexionActiva::create(socketsInternos[0]));
    socketsThread->setConexionIPCInterna(ConexionActiva::create(socketsInternos[1]));
    socketsThread->setConexionTaquilla(ConexionActiva::create(0));
    SocketThread::add(socketsThread);
    idles.push_back(socketsThread->thread);
}

void
SocketThread::readIPCMessage(SocketThread * socketsThread, boost::uint8_t * message)
{
    CabeceraInterna cabeza;
    memcpy(&cabeza, message, sizeof(CabeceraInterna));

    switch (cabeza.peticion) {
        case ProtocoloZeus::DESCONECCION: 
            /*
             * si no llega otra cosa se desconecta a todos los sd del hilo
             * si no, se desconectan solo los sockets que se pasen
             */
            if (cabeza.longitud == 0) {
                free(socketsThread);
            } else {
                ConexionActiva& conexionActiva = socketsThread->getConexionTaquilla();
                if (conexionActiva.sd != 0) {
                    send2cliente(conexionActiva, ProtocoloZeus::DESCONECCION, NULL, 0);
                }
            }
            break;
            
        default:
            break;          
    }
}

void *
SocketThread::run(void *arg)
{
    {
        char gmensaje[BUFFER_SIZE];
        sprintf(gmensaje, "Iniciando hilo_socket <%u>", (unsigned int) pthread_self());
        log_mensaje(NivelLog::Detallado, "mensaje", "servidor", gmensaje);
    }
    
    SocketThread* socketsThread = (SocketThread *) arg;
    socketsThread->wait();
    time_t controlTime = time(NULL);
    bool terminar_hilo = false;
    
    while (!terminar_hilo) {
        fd_set conjunto_sd;
        timeval timeout;
        int max_sd = 0;
        
        std::time_t const MAX_WAIT_TIME = 20;
        
        do {
            
            if (terminar_hilo) {
                break;
            }
                
            FD_ZERO(&conjunto_sd);                                                  
            
            ConexionActiva& conexionTaquilla = socketsThread->getConexionTaquilla();
            if (conexionTaquilla.sd > max_sd)
                max_sd = conexionTaquilla.sd;

            if (conexionTaquilla.sd != 0) {
                FD_SET(conexionTaquilla.sd, &conjunto_sd);
            }
            
            ConexionActiva& conexionIPCInterna = socketsThread->getConexionIPCInterna();
            if (conexionIPCInterna.sd > max_sd)
                max_sd = conexionIPCInterna.sd;

            if (conexionIPCInterna.sd != 0) {
                FD_SET(conexionIPCInterna.sd, &conjunto_sd);
            }

            if (conexionTaquilla.sd != 0 && conexionTaquilla.idAgente == 0) {
                time_t now = time(NULL);
                if (now - controlTime >= MAX_WAIT_TIME) {
                    shutdown(conexionTaquilla.sd, SHUT_RDWR);
                }
            }
            
            timeout.tv_sec = MAX_WAIT_TIME;
            timeout.tv_usec = 0;
        } while (select(max_sd + 1, &conjunto_sd, NULL, NULL, &timeout) == 0);
                
        /***************************************************************************/


        if (!terminar_hilo) {
            ConexionActiva& conexionIPCInterna = socketsThread->getConexionIPCInterna();
            if (FD_ISSET(conexionIPCInterna.sd, &conjunto_sd)) {
                std::vector<boost::uint8_t> mensaje = readInternalSocket(conexionIPCInterna.sd);
                
                if (!mensaje.empty()) {                
                    readIPCMessage(socketsThread, &mensaje[0]);
                    if (finalizar) {
                        terminar_hilo = true;
                    }
                }
            }
        }
        /*
         * Se comprueba si algun cliente conectado ha enviado algo
         */
        if (!terminar_hilo) {
            ConexionActiva& conexionTaquilla = socketsThread->getConexionTaquilla();
            if (FD_ISSET(conexionTaquilla.sd, &conjunto_sd)) {
                std::vector<boost::uint8_t> mensaje = readSocket(conexionTaquilla);
                {
                    std::ostringstream oss;
                    oss << "readSocket(conexionTaquilla).size() = " << mensaje.size();
                    log_mensaje(NivelLog::Debug, "debug", "servidor/cliente", oss.str().c_str());
                }
                if (!mensaje.empty()) {
                    atender_peticion(socketsThread, &mensaje[0]);
                
                } else {
                    
                    if (conexionTaquilla.idAgente != 0) {
                        
                        DataBaseConnetion dbConnetion;
                        MYSQL* mysql = dbConnetion.get();
                                                
                        pthread_t hilo = 0;
                        bool hiloExiste = false;
                        {
                            boost::format sqlQuery;
                            sqlQuery.parse("SELECT Hilo FROM Taquillas WHERE IdAgente=%1% AND Numero=%2% AND "
                                            "Hilo IS NOT NULL")
                            % conexionTaquilla.idAgente % unsigned(conexionTaquilla.idTaquilla);
                            ejecutar_sql(mysql, sqlQuery, DEBUG_LOGOUT);
                            MYSQL_RES * result = mysql_store_result(mysql);
                            MYSQL_ROW row = mysql_fetch_row(result);
                            if (row != NULL) {
                                hilo = pthread_t(strtoul(row[0], NULL, 10));
                                hiloExiste = true;
                            }
                            mysql_free_result(result);
                        }
                        if (hiloExiste) {
                            if (hilo == pthread_self()) {
                                boost::format sqlQuery1;
                                sqlQuery1.parse("UPDATE Taquillas SET Hilo=NULL "
                                                 "WHERE IdAgente=%1% AND Numero=%2%")
                                % conexionTaquilla.idAgente % unsigned(conexionTaquilla.idTaquilla);

                                boost::format sqlQuery2;
                                sqlQuery2.parse("UPDATE Usuarios SET Hilo=NULL WHERE Id=%1%")
                                % conexionTaquilla.idUsuario;

                                ejecutar_sql(mysql, sqlQuery1, DEBUG_LOGOUT);
                                ejecutar_sql(mysql, sqlQuery2, DEBUG_LOGOUT);                               
                            }
                        }
                        {
                            boost::format sqlQuery;
                            sqlQuery.parse("UPDATE Taquillas SET Hora_Ult_Desconexion=FROM_UNIXTIME(%1%) "
                                            "WHERE IdAgente=%2% AND Numero=%3%")
                            % int(time(NULL) - 1) %
                            conexionTaquilla.idAgente % unsigned(conexionTaquilla.idTaquilla);

                            ejecutar_sql(mysql, sqlQuery, DEBUG_LOGOUT);
                        }
                    }
                    free(socketsThread);
                    shutdown(conexionTaquilla.sd, SHUT_RDWR);
                    ::close(conexionTaquilla.sd);
                    
                    char gmensaje[BUFFER_SIZE];
                    
                    sprintf(gmensaje, "[%d] [%d]", conexionTaquilla.idAgente, conexionTaquilla.idTaquilla);
                    log_mensaje(NivelLog::Normal, "desconexion", conexionTaquilla.ipAddress.c_str(), gmensaje);
                    
                    sprintf(gmensaje, "Finalizando hilo_socket <%u>", unsigned(pthread_self()));
                    log_mensaje(NivelLog::Debug, "mensaje", "servidor", gmensaje);
                    
                    return NULL;
                }
            }
        }
    }
    {
        char gmensaje[BUFFER_SIZE];

        sprintf(gmensaje, "Finalizando hilo_socket <%u>", (unsigned int) pthread_self());
        log_mensaje(NivelLog::Detallado, "mensaje", "servidor", gmensaje);
    }
    return NULL;
}
