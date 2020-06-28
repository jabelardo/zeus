#ifndef GLOBAL_H
#define GLOBAL_H

#include <glib.h>
#include <pthread.h>
#include <semaphore.h>
#include <boost/cstdint.hpp>

extern int fd_log_mensajes;
extern sem_t mutex_t_fd_log_mensajes;
extern int fd_log_clientes;
extern sem_t mutex_t_fd_log_clientes;
extern int fd_log_admin;
extern sem_t mutex_t_fd_log_admin;
extern GRand *aleatorio;
extern bool isDaemon;
extern char *archivo_config;

extern GTree *t_preventas_x_taquilla;  /* llave = idAgente + ntaquilla */

/*
 * Cada uno de los nodos del arbol preventas_x_taquilla
 */
struct n_t_preventas_x_taquilla_t
{
	sem_t mutex_preventas_x_taquilla;
	GTree *preventas;         /* llave = renglon */
	boost::uint32_t hora_ultimo_renglon;
	boost::uint32_t hora_ultimo_peticion;
	boost::uint8_t idZona;
};
/*
 * Cada uno de los nodos del arbol preventas
 */
struct Preventa
{
	boost::uint8_t idSorteo;
	boost::uint8_t tipo;
	boost::uint16_t numero;
	boost::int32_t monto_preventa;
	boost::uint32_t monto_pedido;
	boost::uint16_t renglon;
};

struct n_a_renglon_t
{
	boost::uint8_t idSorteo;
	boost::uint8_t tipo;
	boost::uint16_t numero;
	boost::int32_t monto;
	boost::uint8_t idTipoMonto;
};

/*
 * Nodo del arbol t_venta_numero
 */
struct n_t_venta_numero_t
{
	boost::int32_t venta;
	boost::int32_t preventa;
};

/*
 * Nodo del arbol t_tipomonto_terminal y t_tipomonto_triple
 */
struct n_t_tipomonto_t
{
	boost::int32_t valorinicial;
	boost::int32_t montos[3];
	boost::int32_t proporcion;        /*                                    */
	boost::int32_t incremento;        /*      limite_real =                                                  */
	boost::int32_t valoradicional;    /*      ((venta*proporcion/10000+valoradicional)/incremento)*incremento; */
	boost::int32_t venta_global;      /*                                                                   */
	boost::int32_t limite_actual;     /*      limite_actual = (limite_real > valorinicial ? limite_real : valor_inicial);  */
	boost::int32_t valorfinal;
	sem_t sem_t_tipomonto;
	GTree *t_venta_numero;    /* llave = numero */
};

/*
 * Cada uno de los nodos del arbol de numeros restringidos por sorteo por zona
 */
struct n_t_numero_restringido_t
{
	boost::uint32_t venta;
	boost::uint32_t preventa;
	double proporcion;
	boost::uint32_t tope;
};

struct n_t_sorteo_numero_restringido_t
{
	GTree *t_numero_restringido_triple; /* llave = numero */
	GTree *t_numero_restringido_terminal;  /* llave = numero */
};

struct n_t_zona_numero_restringido_t
{
	GTree *t_sorteo_numero_restringido; /* llave = idSorteo */
	sem_t sem_t_sorteo_numero_restringido;
};

extern GTree *t_zona_numero_restringido;  /* llave = idZona */

/*
 * Estructura de comunicacion con el socket interno de cada hilo_socket
 */
struct CabeceraInterna
{
	boost::int8_t peticion;
	int longitud;
};

/*
 * Estructura de Informacion sobre los Parametros de la Aplicacion
 */
struct Configuracion
{
	char dbServer[25];
	char dbZeusName[25];
    char dbSisacName[25];
	char dbUser[15];
	char dbPassword[15];
	boost::uint16_t dbPort;
	boost::uint16_t maximoTiempoVenta;
	boost::uint16_t tiempoVerificarConexion;
	boost::uint16_t tiempoDesconectar;
	char rutaTaquilla[256];
    boost::uint16_t impuesto_ganancia_fortuita;
    char urlSelco[256];
    boost::uint16_t dbConnections;    
    char libreriaOBDCTransax[256];
};

extern Configuracion configuracion;

struct data_tipomonto_t
{
	boost::uint16_t id;
	boost::int32_t valorInicial;
	boost::int32_t proporcion;
	boost::int32_t incremento;
	boost::int32_t valorAdicional;
	boost::int32_t montos[3];
	boost::int32_t valorFinal;
};

/*
 * Otras Variables Globales
 */
extern char *nombreDelPrograma;
extern int sck_serv;
extern boost::uint16_t minThreadNumber;
extern boost::uint16_t maxThreadNumber;
extern boost::uint16_t deltaThreadNumber;
extern int colaDeEspera;
extern int puertoTaquillas;
extern int puertoAdmin;
extern int tiempoDeEliminacion;
extern boost::uint8_t conexionesAdminMax;
extern long tiempoDeEspera;
extern bool finalizar;
extern char directorioDeLogs[256];
extern char directorioDePidof[256];
extern char directorioDeRespaldos[256];
extern boost::uint8_t nivelLogMensajes;
extern boost::uint8_t nivelLogAccesoTaquillas;
extern boost::uint8_t nivelLogAccesoAdmin;
extern boost::uint16_t tiempoChequeoPreventa;
extern char autentificacion[50];
extern boost::uint8_t diasDeInformacion;

#define BUFFER_SIZE 1024

#define BANCA 3
#define RECOGEDOR 2
#define AGENCIA 1

#define REGISTRO_MENSAJES "mensajes.log"
#define REGISTRO_CLIENTES "acceso_clientes.log"
#define REGISTRO_ADMIN "acceso_admin.log"

struct NivelLog
{
	enum Niveles {
	    Desabilitado = 0,
        Bajo,
	    Normal,
	    Detallado,
	    Debug        
	};
};

/*Estados de un ticket*/
boost::uint8_t const NORMAL = 1;
//boost::uint8_t const MODIFICADO = 2;
boost::uint8_t const ANULADO = 3;          /* Eliminado por el Cliente */
//boost::uint8_t const GENERADO = 4;
boost::uint8_t const PAGADO = 5;
boost::uint8_t const TACHADO = 6;     // Eliminado por el Servidor
boost::uint8_t const ELIMINADO = 7;  /* Eliminado por el Administrativo */
boost::uint8_t const BANQUEADO = 8;  // no pudo eliminarse por lo que va jugando por la banca

/*Tipos de Renglones*/
boost::uint8_t const TERMINAL       = 2;
boost::uint8_t const TRIPLE         = 3;
boost::uint8_t const TERMINALAZO    = 4;
boost::uint8_t const TRIPLETAZO     = 5;
boost::uint8_t const BONO           = 7;          /* triple */
boost::uint8_t const BONOTRIPLETAZO = 8;          /* tripletazo */
boost::uint8_t const MONTO_TERMINAL_PRIMERO = 10;
boost::uint8_t const MONTO_TERMINAL_SEGUNDO = 11;
boost::uint8_t const MONTO_TERMINAL_TERCERO = 12;

#define MAXTERMINAL      99
#define MAXTRIPLE       999
#define MAXTERMINALAZO 1299
#define MAXTRIPLETAZO 12999

/* Tipos de nivel de  mensajes*/
#define MSJERROR      0
#define MSJBAJO       1
#define MSJNORMAL     2
#define MSJURGENTE    3

#define JUGADANORMAL  0
#define JUGADASIGNO   1

/*
 * Funciones de compracion de GTree
 */
int uint64_t_comp_func(gconstpointer a, gconstpointer b);
int uint16_t_comp_func(gconstpointer a, gconstpointer b);
int uint16_t_gcomp_func(gconstpointer a, gconstpointer b, void* user_data);

int uint8_t_comp_func(gconstpointer a, gconstpointer b);
int uint8_t_comp_data_func(gconstpointer a, gconstpointer b, void* data);
void uint8_t_destroy_key(void* data);
void uint16_t_destroy_key(void* data);
void t_n_t_preventas_t_destroy(void* data);

#endif // GLOBAL_H
