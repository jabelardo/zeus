#ifndef SOCKETS_H
#define SOCKETS_H

#include <ConexionActiva.h>
#include <vector>

#include <ProtocoloZeus/Mensajes.h>

template<class T>
int 
send2cliente(ConexionActiva& conexionActiva, T const& t)
{
    return send2cliente(conexionActiva, ProtocoloZeus::ERR_EXCEPTION, t.toRawBuffer());
}

template<class T>
int 
send2cliente(ConexionActiva& conexionActiva, boost::int8_t peticion, T const& t)
{
    return send2cliente(conexionActiva, peticion, t.toRawBuffer());
}

int send2cliente(ConexionActiva& conexionActiva, boost::int8_t peticion, std::vector<boost::uint8_t> const& buffer);

int send2cliente(ConexionActiva& conexionActiva, boost::int8_t peticion, boost::uint8_t const* buffer, boost::uint16_t buffer_len);

int send2hilo_socket(pthread_t hilo, void* msg);

boost::int32_t readbyte(int sd, boost::uint8_t * buffer, int lon);

std::vector<boost::uint8_t> readSocket(ConexionActiva& conexionActiva);

std::vector<boost::uint8_t> readInternalSocket(int sd);

int abrir_socket(int puerto);

#endif // SOCKETS_H
