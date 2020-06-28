#include <glib.h>
#include <pthread.h>
#include <semaphore.h>

#include <global.h>
#include <sockets.h>
#include <ProtocoloZeus/Mensajes.h>
    
int sck_serv;

GRand *aleatorio;
int fd_log_mensajes;
sem_t mutex_t_fd_log_mensajes;
int fd_log_clientes;
sem_t mutex_t_fd_log_clientes;
int fd_log_admin;
sem_t mutex_t_fd_log_admin;

bool isDaemon = true;
char *archivo_config = NULL;

char *nombreDelPrograma = NULL;

boost::uint16_t minThreadNumber = 1;
boost::uint16_t maxThreadNumber = 21;
boost::uint16_t deltaThreadNumber = 1;
int colaDeEspera = 16;
int puertoTaquillas = 5000;
int puertoAdmin = 5800;
boost::uint8_t conexionesAdminMax = 8;
long tiempoDeEspera = 250000;
Configuracion configuracion;
char directorioDeLogs[ 256 ];
char directorioDePidof[ 256 ];
char directorioDeRespaldos[ 256 ] = "/tmp/";
boost::uint8_t nivelLogMensajes = NivelLog::Bajo;
boost::uint8_t nivelLogAccesoTaquillas = NivelLog::Bajo;
boost::uint8_t nivelLogAccesoAdmin = NivelLog::Bajo;
int tiempoDeEliminacion = 5;
boost::uint16_t tiempoChequeoPreventa = 3;
char autentificacion[ 50 ];
boost::uint8_t diasDeInformacion = 0;

GTree *t_preventas_x_taquilla = NULL;
sem_t mutex_t_preventas_x_taquilla;

GTree *t_zona_numero_restringido = NULL;

bool finalizar = false;


int
uint64_t_comp_func( gconstpointer a, gconstpointer b )
{
	boost::uint64_t u_a = *( boost::uint64_t * ) a;
	boost::uint64_t u_b = *( boost::uint64_t * ) b;
	boost::int64_t s_a = ( u_a ) >> 32;
	boost::int64_t s_b = ( u_b ) >> 32;
	boost::int64_t comp = s_a - s_b;

	if ( comp > 0 ) {
		return 1;
	} else if ( comp < 0 ) {
		return -1;
	}

	u_a = ( *( boost::uint64_t * ) a ) << 32;
	u_b = ( *( boost::uint64_t * ) b ) << 32;
	s_a = u_a >> 32;
	s_b = ( u_b ) >> 32;

	comp = s_a - s_b;

	if ( comp == 0 ) {
		return 0;
	} else if ( comp > 0 ) {
		return 1;
	} else {
		return -1;
	}
}

int
uint16_t_comp_func( gconstpointer a, gconstpointer b )
{
	boost::int32_t s_a = *( boost::uint16_t * ) a;
	boost::int32_t s_b = *( boost::uint16_t * ) b;
	boost::int32_t comp = s_a - s_b;

	if ( comp == 0 ) {
		return 0;
	} else if ( comp > 0 ) {
		return 1;
	} else {
		return -1;
	}
}

int
uint8_t_comp_func( gconstpointer a, gconstpointer b )
{
	boost::int16_t s_a = *( boost::uint8_t * ) a;
	boost::int16_t s_b = *( boost::uint8_t * ) b;
	boost::int16_t comp = s_a - s_b;

	if ( comp == 0 ) {
		return 0;
	} else if ( comp > 0 ) {
		return 1;
	} else {
		return -1;
	}
}

void
t_n_t_preventas_t_destroy( void* data )
{
	Preventa * preventa = (Preventa*) data;
	delete preventa;
}

void
uint16_t_destroy_key( void* data )
{
	boost::uint16_t * dato = ( boost::uint16_t * ) data;
	delete dato;
}

int
uint16_t_gcomp_func( gconstpointer a, gconstpointer b, void* )
{
	boost::int32_t s_a = *( boost::uint16_t * ) a;
	boost::int32_t s_b = *( boost::uint16_t * ) b;
	boost::int32_t comp = s_a - s_b;

	if ( comp == 0 ) {
		return 0;
	} else if ( comp > 0 ) {
		return 1;
	} else {
		return -1;
	}
}

void
uint8_t_destroy_key( void* data )
{
	boost::uint8_t * dato = ( boost::uint8_t * ) data;
	delete dato;
}

int
uint8_t_comp_data_func( gconstpointer a, gconstpointer b, void* )
{
	boost::int16_t s_a = *( boost::uint8_t * ) a;
	boost::int16_t s_b = *( boost::uint8_t * ) b;
	boost::int16_t comp = s_a - s_b;

	if ( comp == 0 ) {
		return 0;
	} else if ( comp > 0 ) {
		return 1;
	} else {
		return -1;
	}
}
