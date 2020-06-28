#ifndef ATENDER_PETICION_H
#define ATENDER_PETICION_H

#include <global.h>
#include <ConexionActiva.h>

class SocketThread;

void mensajes(ConexionActiva& conexionActiva, int cual);

void cargar_tipomonto_x_sorteo(boost::uint32_t idAgente, MYSQL * dbConnection);

void atender_peticion(SocketThread * socketsThread, boost::uint8_t * buffer);

#endif //ATENDER_PETICION_H
