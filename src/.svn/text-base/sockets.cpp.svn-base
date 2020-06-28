#include <ace/ACE.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cstdio>

#include <glib.h>
#include <zlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

#include <global.h>
#include <sockets.h>
#include <utils.h>
#include <SocketThread.h>

#include <ace/Guard_T.h>

#include <ProtocoloZeus/Protocolo.h>

using namespace ProtocoloZeus;

int 
send2cliente(ConexionActiva& conexionActiva, boost::int8_t peticion, std::vector<boost::uint8_t> const& buffer)
{
    return send2cliente(conexionActiva, peticion, &buffer[0], buffer.size());
}

int
send2cliente(ConexionActiva& conexionActiva, boost::int8_t peticion, boost::uint8_t const* payload, boost::uint16_t payloadLength)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> mon(conexionActiva.mutex);
    
    std::vector<boost::uint8_t> buffer(payload, payload + payloadLength);
	Cabecera cabecera(peticion, conexionActiva.idTaquilla, buffer);
	std::vector<boost::uint8_t> outputBuffer = cabecera.toRawBuffer();	
	{
		char buffer[10];
        sprintf(buffer, "%d", int(peticion));
        log_mensaje (NivelLog::Normal, "send2cliente peticion", "", buffer);
	}    

	int enviado = ACE::send_n(conexionActiva.sd,
	                          &outputBuffer[0],
	                          outputBuffer.size(),
	                          MSG_NOSIGNAL,
	                          NULL,
	                          NULL);

	if (enviado == -1) {
		log_mensaje(NivelLog::Bajo, "error", conexionActiva.ipAddress.c_str(),
		             "No se pudo enviar paquete: send()");
	} else {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Paquete enviado (%u Bytes)", unsigned(outputBuffer.size()));
		log_mensaje(NivelLog::Normal, "mensaje", conexionActiva.ipAddress.c_str(), mensaje);
                
        std::string form = hexDump(&outputBuffer[0], outputBuffer.size());        
        log_mensaje(NivelLog::Debug, "mensaje", conexionActiva.ipAddress.c_str(), form.c_str());
	}
	return enviado;
}

/**************************************************************************/

int
send2hilo_socket(pthread_t thread, void* msg)
{
	CabeceraInterna * cabecera = (CabeceraInterna *) msg;

	SocketThread::MutexGuard g;
    SocketThread * socketsThread = SocketThread::get(thread);

	ConexionActiva& conexionActiva = socketsThread->getConexionIPCExterna();

	socketsThread->post();
	
	return send(conexionActiva.sd, (boost::uint8_t *) msg, 
                sizeof(CabeceraInterna) + cabecera->longitud, 0);
}

/**************************************************************************/

std::vector<boost::uint8_t>
readInternalSocket(int sd)
{
	CabeceraInterna cabecera;
	ACE_Time_Value timeout(0, tiempoDeEspera);

	boost::int32_t bytes = ACE::recv_n(sd, &cabecera, sizeof(CabeceraInterna), &timeout, NULL);

	if (bytes != sizeof(CabeceraInterna)) {
		return std::vector<boost::uint8_t>();
	}
	std::vector<boost::uint8_t> buffer(sizeof(CabeceraInterna) + cabecera.longitud);

	memcpy(&buffer[0], &cabecera, sizeof(CabeceraInterna));
	if (cabecera.longitud == 0) {
        return std::vector<boost::uint8_t>();
	}
	boost::uint8_t * tbuff = new boost::uint8_t[cabecera.longitud];

	bytes = ACE::recv_n(sd, tbuff, cabecera.longitud, &timeout, NULL);

	if (bytes != cabecera.longitud) {
		delete [] tbuff;
        return std::vector<boost::uint8_t>();
	}
	memcpy(&buffer[sizeof(CabeceraInterna)], tbuff, cabecera.longitud);
	delete [] tbuff;
	return buffer;
}

std::vector<boost::uint8_t>
readSocket(ConexionActiva& conexionActiva)
{
	std::vector<boost::uint8_t> buffer(Cabecera::CONST_RAW_BUFFER_SIZE);
	ACE_Time_Value timeout(0, tiempoDeEspera);

	boost::int32_t bytes = ACE::recv_n(conexionActiva.sd, &buffer[0], Cabecera::CONST_RAW_BUFFER_SIZE, 
                                       &timeout, NULL);

	if (bytes == 0) {
		log_mensaje(NivelLog::Bajo, "mensaje", conexionActiva.ipAddress.c_str(), "desconexion");
		return std::vector<boost::uint8_t>();
	}

	if (bytes != boost::int32_t(Cabecera::CONST_RAW_BUFFER_SIZE)) {
		char gmensaje[BUFFER_SIZE];
		sprintf(gmensaje, "cabecera corrompida de %u bytes", bytes);
		log_mensaje(NivelLog::Bajo, "error", conexionActiva.ipAddress.c_str(), gmensaje);
		return std::vector<boost::uint8_t>();
	}

	Cabecera cabecera(&buffer[0]);
    bytes = 0;
    if (cabecera.getPayloadSize() > 0) {        
	    cabecera.payload.resize(cabecera.getPayloadSize());
	    bytes = ACE::recv_n(conexionActiva.sd, &cabecera.payload[0], cabecera.getPayloadSize(), &timeout, NULL);        
	    if (bytes == 0) {
            log_mensaje(NivelLog::Bajo, "mensaje", conexionActiva.ipAddress.c_str(), "desconexion");
            return std::vector<boost::uint8_t>();
	    }
    }

	if (bytes != cabecera.getPayloadSize()) {
		char mensaje[BUFFER_SIZE];  // paquete corrompido de 8 bytes esperado 0
		sprintf(mensaje, "paquete corrompido de %u bytes esperado %u ", bytes,
		         cabecera.getPayloadSize());
		log_mensaje(NivelLog::Bajo, "error", conexionActiva.ipAddress.c_str(), mensaje);
        return std::vector<boost::uint8_t>();
	}
	std::vector<boost::uint8_t> returnBuffer = cabecera.toRawBuffer();
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Paquete recibido (%d Bytes)", cabecera.getPayloadSize() + Cabecera::CONST_RAW_BUFFER_SIZE);
		log_mensaje(NivelLog::Normal, "mensaje", conexionActiva.ipAddress.c_str(), mensaje);
						
		std::string form = hexDump(&returnBuffer[0], cabecera.getPayloadSize() + Cabecera::CONST_RAW_BUFFER_SIZE);        
        log_mensaje(NivelLog::Debug, "mensaje", conexionActiva.ipAddress.c_str(), form.c_str());        
	}
	
	if (cabecera.getCrc() != cabecera.calcularCrc()) {
		log_mensaje(NivelLog::Bajo, "error", conexionActiva.ipAddress.c_str(), "CRC incorrecto");
        return std::vector<boost::uint8_t>();
	}
	return returnBuffer;
}

int
abrir_socket(int puerto)
{
	int sck = socket(AF_INET, SOCK_STREAM, 0);
	if (sck == -1) {
		g_error("socket()");
	}
	int val = 1;
	setsockopt(sck, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));

	/*
	 * Se rellenan los campos de la estructura Direccion, necesaria
	 * para la llamada a la funcion bind()
	 */
	struct sockaddr_in direccion;
	memset(&direccion, 0, sizeof(struct sockaddr_in));
	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(puerto);
	direccion.sin_addr.s_addr = INADDR_ANY;

	if (bind(sck,
	           (struct sockaddr *) & direccion,
	           sizeof(struct sockaddr_in)) == -1) {
		close(sck);
		g_error("bind()");
	}
	return sck;
}
