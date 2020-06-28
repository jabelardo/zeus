#include <ZeusServicioAdmin.h>
#include <ConexionAdmin.h>
#include <global.h>
#include <utils.h>
#include <sockets.h>

#include <sstream>

namespace ZeusServicioAdmin
{

ZeusServicioAdminImpl::ZeusServicioAdminImpl()
    : conexionesAdminMax(0)
    , socket(-1)
    , thread(0)
    , finalizar(NULL)
{
}

ZeusServicioAdminImpl::~ZeusServicioAdminImpl()
{
}

void 
ZeusServicioAdminImpl::setFinalizarPtr(bool* finalizar_)
{
    finalizar = finalizar_;
}

void 
ZeusServicioAdminImpl::setConexionesMax(std::size_t conexionesMax)
{
    conexionesAdminMax = conexionesMax;
}
    
void* 
ZeusServicioAdminImpl::startThread(void* selfPtr)
{
    ZeusServicioAdminImpl* self = reinterpret_cast<ZeusServicioAdminImpl*>(selfPtr);
            
    std::ostringstream oss;
    oss << "Iniciando ZeusServicioAdmin : " << pthread_self();
    log_mensaje(NivelLog::Detallado, "mensaje", "servidor/admin", oss.str().c_str());
    
    self->startService();
    
    return NULL;
}

void
ZeusServicioAdminImpl::startService()
{
    if (listen(socket, conexionesAdminMax) == -1) {            
        shutdown(socket, SHUT_RDWR);        
        std::ostringstream oss;
        oss << "No se puede escuchar el socket: " << socket;
        log_mensaje(NivelLog::Bajo, "error", "servidor/admin ", oss.str().c_str());
        cerrar_archivos_de_registro();
        exit(1);
    }

    while (!*finalizar) {
        fd_set fdSet;
        FD_ZERO(&fdSet);
        FD_SET(socket, &fdSet);
        int maxSd = socket;

        for (Conexiones::const_iterator conexion = conexiones.begin();
             conexion != conexiones.end(); ++conexion) {                
            if (*finalizar) {
                return;
            }
            if ((*conexion)->getSd() > maxSd) {
                maxSd = (*conexion)->getSd();
            }
            if ((*conexion)->getSd() != 0) {
                FD_SET((*conexion)->getSd(), &fdSet);
            }
        }

        select(maxSd + 1, &fdSet, NULL, NULL, NULL);

        for (Conexiones::iterator conexion = conexiones.begin();
             conexion != conexiones.end(); ++conexion) {
                
            if (*finalizar) {
                return;
            }
            
            if ((*conexion)->isSet(&fdSet)) {
                std::vector<boost::uint8_t> mensaje = (*conexion)->getMessage();
                {
                    std::ostringstream oss;
                    oss << "(*conexion)->getMessage().size() = " << mensaje.size();
                    log_mensaje(NivelLog::Debug, "debug", "servidor/admin", oss.str().c_str());
                }
                /*
                 * Se lee lo enviado por el cliente
                 */
                if (!mensaje.empty()) {

                    /*
                     * se atiende la peticion que llego
                     */
                    (*conexion)->dipatch(mensaje);
                }

                /*
                 * desconeccion
                 */
                else {
                    (*conexion)->close();
                    log_mensaje(NivelLog::Detallado, 
                                "mensaje",
                                (*conexion)->getIpAddress().c_str(),
                                "Conexion cerrada - socket admin");
                    
                    std::auto_ptr<ConexionAdmin> conexionPtr(*conexion);
                    conexion = conexiones.erase(conexion);
                }
            }
        }

        /*
         * Se comprueba si algun cliente nuevo desea conectarse y se le
         * admite
         */
        if (FD_ISSET(socket, &fdSet)) {
            
            std::auto_ptr<ConexionAdmin> conexion = ConexionAdmin::accept(socket);
            
            if (conexion.get()) {
                if (conexiones.size() >= conexionesAdminMax) {
                    conexion->close();           
                    log_mensaje(NivelLog::Bajo, 
                                "advertencia", 
                                "servidor/admin",
                                 "No se pudo aceptar conexion: numero maximo de conexiones alcanzado");                              
                } else {
                    /*
                     * Coneccion con exito
                     */
                    log_mensaje(NivelLog::Debug, 
                                "advertencia", 
                                "servidor/admin",
                                "Coneccion con exito");
                    conexiones.push_back(conexion.get());
                    conexion.release();
                }
            }
        }
    }
}

void 
ZeusServicioAdminImpl::desconectar()
{
    for (Conexiones::iterator conexion = conexiones.begin();
         conexion != conexiones.end(); ++conexion) {
        (*conexion)->sendClose();
    }
}

bool 
ZeusServicioAdminImpl::open(int puerto)
{
    if (finalizar == NULL || conexionesAdminMax == 0) {
        return false;
    }
    socket = abrir_socket(puerto);
    pthread_create(&thread, NULL, &startThread, this);
    return true;
}

void 
ZeusServicioAdminImpl::close()
{
    desconectar();
    pthread_cancel(thread);
    ::close(socket);
}

void 
ZeusServicioAdminImpl::join()
{
    pthread_join(thread, NULL);
}

}
