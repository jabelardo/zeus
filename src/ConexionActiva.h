#ifndef CONEXIONACTIVA_H_
#define CONEXIONACTIVA_H_

#include <ace/Recursive_Thread_Mutex.h>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>

/* 
 * Cada uno de los nodos del arreglo de conexiones encontrado en cada nodo de t_hilo_socket
 */
struct ConexionActiva : boost::noncopyable
{ 
    int sd;
    boost::uint8_t idTaquilla;
    boost::uint32_t idAgente;
    boost::uint32_t idUsuario;
    boost::uint8_t idZona;
    ACE_Recursive_Thread_Mutex mutex;  
    char login[12];
    bool usarProductos;
    std::string ipAddress;
    
    virtual ~ConexionActiva();

    static std::auto_ptr<ConexionActiva> create(int sd);
    
    static std::auto_ptr<ConexionActiva> create(int sd, char const* ipAddress_);    
    
protected:
    ConexionActiva();
};

#endif /*CONEXIONACTIVA_H_*/
