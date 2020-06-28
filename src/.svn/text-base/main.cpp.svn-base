#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <cmath>
#include <sstream>

#include <libgen.h>
#include <glib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <mysql/mysql.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <ace/Thread_Manager.h>

#include <global.h>
#include <utils.h>
#include <sockets.h>
#include <hilo_free_ticket.h>
#include <atender_peticion.h>
#include <sorteo.h>
#include <version.h>
#include <database/DataBaseConnetionPool.h>
#include <ZeusServicioAdmin.h>
#include <SocketThread.h>
#include <TipoDeMontoDeAgencia.h>

#include <ProtocoloZeus/Mensajes.h>
#include <ProtocoloZeus/Protocolo.h>

using namespace ProtocoloZeus;

#include <ProtocoloExterno/Mensajes/MensajeFactory.h>

#include <Selco/Manager/SorteoManager.h>
#include <Selco/Manager/TaquillaManager.h>
#include <Selco/Mensaje.h>

#include <Transax/Manager/SorteoManager.h>
#include <Transax/Manager/ComercializadoraManager.h>
#include <Transax/Transax.h>

#include <Mycrocom/Manager/SorteoManager.h>
#include <Mycrocom/Manager/TaquillaManager.h>
#include <Mycrocom/Mycrocom.h>

#define DEBUG_CARGAR_SORTEOS 0
#define DEBUG_NUMEROS_RESTRINGIDOS 0
#define DEBUG_CARGAR_VENTA_GLOBAL 0
#define DEBUG_CREAR_ARBOL_PREVENTAS_X_TAQUILLAS 0
#define DEBUG_DEPURAR_SISTEMA 0
#define DEBUG_DB_CONFGURACION 0
#define DEBUG_respaldar 0

pthread_t *hilo_escucha_t;
pthread_t *hilo_free_preventa_t;

/*********************************************************************/

void
iniciarProtocolosExternos()
{
    log_mensaje(NivelLog::Detallado, "mensaje", "servidor", "inicianado Mensaje Factory");

    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    try {
        ProtocoloExterno::MensajeFactory::cargarConfiguracion(mysql);
        
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "No se pudo iniciar ProtocoloExterno: <%s>", e.what());
        log_mensaje(NivelLog::Bajo, "advertencia", "base de datos", mensaje);
    }    
    try {            
        Selco::SorteoManager::instance()->crearSorteos(mysql);
        Selco::TaquillaManager::instance()->crearTaquillas(mysql);        
        Selco::Mensaje::url = configuracion.urlSelco;
        
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "No se pudo iniciar Selco: <%s>", e.what());
        log_mensaje(NivelLog::Bajo, "advertencia", "base de datos", mensaje);
    }    
    try {  
        Transax::SorteoManager::instance()->crearSorteos(mysql);
        Transax::ComercializadoraManager::instance()->create(mysql);
        Transax::TransaxManager::instance()->create(mysql, configuracion.libreriaOBDCTransax);
        
   } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "No se pudo iniciar Transax: <%s>", e.what());
        log_mensaje(NivelLog::Bajo, "advertencia", "base de datos", mensaje);
    }    
    try {   
        Mycrocom::MycrocomManager::instance()->create(mysql);
        Mycrocom::SorteoManager::instance()->create(mysql);
        Mycrocom::TaquillaManager::instance()->create(mysql);
                
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "No se pudo iniciar Mycrocom: <%s>", e.what());
        log_mensaje(NivelLog::Bajo, "advertencia", "base de datos", mensaje);
    }    
}

/*********************************************************************/
void
cargar_sorteos()
{
	log_mensaje(NivelLog::Detallado, "mensaje", "servidor", "Cargando sorteos");
    
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();

	/************************************************************/

	GArray * a_tipomonto_triple = g_array_new(false, false, sizeof(data_tipomonto_t));

	std::vector<data_tipomonto_t> tiposDeMontoTriple;
	{
		char const* sql =
		    "SELECT Id, ValorInicial, Proporcion, Incremento, ValorAdicional, "
		    " Monto, ValorFinal FROM TiposMontoTriple";
		ejecutar_sql(mysql, sql, DEBUG_CARGAR_SORTEOS);
		MYSQL_RES *result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			data_tipomonto_t dataTipoMonto;
			dataTipoMonto.id = atoi(row[0]);
			dataTipoMonto.valorInicial = atoi(row[1]);
			dataTipoMonto.proporcion = atoi(row[2]);
			dataTipoMonto.incremento = atoi(row[3]);
			dataTipoMonto.valorAdicional = atoi(row[4]);
			dataTipoMonto.montos[0] = atoi(row[5]);
			dataTipoMonto.valorFinal = atoi(row[6]);
			tiposDeMontoTriple.push_back(dataTipoMonto);
		}
		mysql_free_result(result);
	}

	if (tiposDeMontoTriple.size() < 1) {
		log_mensaje(NivelLog::Bajo, "error", "base de datos", "No se pudo cargar sorteos: cargar_sorteos() "
		             "TiposMontoTriple no tiene registros");
		cerrar_archivos_de_registro();
		exit(1);
	}

	a_tipomonto_triple = g_array_set_size(a_tipomonto_triple, tiposDeMontoTriple.size());
	for (unsigned int i = 0; i < tiposDeMontoTriple.size(); ++i) {
		g_array_index(a_tipomonto_triple, data_tipomonto_t, i) = tiposDeMontoTriple[i];
	}

	/************************************************************/

	GArray * a_tipomonto_terminal = g_array_new(false, false, sizeof(data_tipomonto_t));

	std::vector<data_tipomonto_t> tiposDeMontoTerminal;
	{
		char const* sql =
		    "SELECT Id, ValorInicial, Proporcion, Incremento, ValorAdicional, "
		    "Monto1ro, Monto2do, Monto3ro, ValorFinal FROM TiposMontoTerminal";
		ejecutar_sql(mysql, sql, DEBUG_CARGAR_SORTEOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			data_tipomonto_t dataTipoMonto;
			dataTipoMonto.id = atoi(row[0]);
			dataTipoMonto.valorInicial = atoi(row[1]);
			dataTipoMonto.proporcion = atoi(row[2]);
			dataTipoMonto.incremento = atoi(row[3]);
			dataTipoMonto.valorAdicional = atoi(row[4]);
			dataTipoMonto.montos[0] = atoi(row[5]);
			dataTipoMonto.montos[1] = atoi(row[6]);
			dataTipoMonto.montos[2] = atoi(row[7]);
			dataTipoMonto.valorFinal = atoi(row[8]);
			tiposDeMontoTerminal.push_back(dataTipoMonto);
		}
		mysql_free_result(result);
	}

	if (tiposDeMontoTerminal.size() < 1) {
		log_mensaje(NivelLog::Bajo, "error", "base de datos", "No se pudo cargar sorteos: cargar_sorteos() "
		             "TiposMontoTerminal no tiene registros");
		
		cerrar_archivos_de_registro();
		exit(1);
	}

	a_tipomonto_terminal = g_array_set_size(a_tipomonto_terminal, tiposDeMontoTerminal.size());
	for (unsigned int i = 0; i < tiposDeMontoTerminal.size(); ++i) {
		g_array_index(a_tipomonto_terminal, data_tipomonto_t, i) = tiposDeMontoTerminal[i];
	}

	/************************************************************************/
	// FIXME: esto funciona, pero el codigo es muy dificil de entender.
	//       Por otra parte es muy dificil de separar asi que lo dejo para
	//       cuando tenga la mente mas fresca.
	char const* sqlSorteos =
	    "SELECT Id, Dia, HoraCierre+0, Estado, UNIX_TIMESTAMP(FechaSuspHasta), "
	    "UNIX_TIMESTAMP(FechaModHoraHasta), ModHora +0, TipoJugada FROM Sorteos";

	ejecutar_sql(mysql, sqlSorteos, DEBUG_CARGAR_SORTEOS);
	MYSQL_RES * result = mysql_store_result(mysql);

	boost::uint32_t numrows = mysql_num_rows(result);

	G_sorteos = g_tree_new(uint8_t_comp_func);

	for (unsigned int i = 0; i < numrows; ++i) {
        MYSQL_ROW row = mysql_fetch_row(result);
        boost::uint8_t * idSorteo = new boost::uint8_t;
        *idSorteo = atoi(row[0]);
		Sorteo * sorteo = new Sorteo(*idSorteo);
		sorteo->horaCierre_ = atoi(row[2]) / 100;
		int dia = atoi(row[1]);
		int rightShift = 128;
		rightShift = rightShift >> diadehoy();
		sorteo->estadoActivo_ = (atoi(row[3]) != 0);
		sorteo->estadoActivo_ = ((dia & rightShift) > 0) 
                             ? sorteo->estadoActivo_ 
                             : false;

		time_t fechasusp; // FIXME: y si row[4] == NULL que pasa con fechasusp?
		if (row[4] != NULL) {
			fechasusp = atoi(row[4]);
			if (fechasusp >= time(NULL))
				sorteo->estadoActivo_ = false;
		}

		time_t fechamodhora; // FIXME: y si row[5] == NULL que pasa con fechamodhora?
		if (row[5] != NULL) {
			fechamodhora = atoi(row[5]);
			if (fechamodhora >= time(NULL))
				sorteo->horaCierre_ =
				    row[6] != NULL ? atoi(row[6]) / 100 : sorteo->horaCierre_;
		}

		sorteo->conSigno_ = atoi(row[7]) != 0;
		if (hora_actual() > sorteo->horaCierre_) {
			sorteo->estadoActivo_ = false;
		}
		sorteo->estadoInicialActivo_ = sorteo->estadoActivo_;

		g_tree_insert(G_sorteos, idSorteo, sorteo);

		for (unsigned int j = 0; j < a_tipomonto_triple->len; ++j) {
			data_tipomonto_t data_tipomonto = g_array_index(a_tipomonto_triple, data_tipomonto_t, j);

			boost::uint8_t * idMontoTriple = new boost::uint8_t;
			*idMontoTriple = data_tipomonto.id;

			n_t_tipomonto_t * n_t_tipomonto = new n_t_tipomonto_t;
			n_t_tipomonto->valorinicial = data_tipomonto.valorInicial;
			n_t_tipomonto->limite_actual = data_tipomonto.valorInicial;
			n_t_tipomonto->proporcion = data_tipomonto.proporcion;
			n_t_tipomonto->incremento = data_tipomonto.incremento;
			n_t_tipomonto->valoradicional = data_tipomonto.valorAdicional;
			n_t_tipomonto->montos[0] = data_tipomonto.montos[0];
			n_t_tipomonto->montos[1] = 0;
			n_t_tipomonto->montos[2] = 0;
			n_t_tipomonto->venta_global = 0;
			n_t_tipomonto->valorfinal = data_tipomonto.valorFinal;
			n_t_tipomonto->t_venta_numero = g_tree_new(uint16_t_comp_func);

			sem_init(&n_t_tipomonto->sem_t_tipomonto, 0, 1);

			boost::uint16_t maximo = sorteo->isConSigno() ? MAXTRIPLETAZO : MAXTRIPLE;
			boost::uint16_t inicio = sorteo->isConSigno() ? MAXTRIPLE + 1 : 0;

			for (unsigned int k = inicio; k <= maximo; ++k) {
				boost::uint16_t * numero = new boost::uint16_t;
				*numero = k;

				n_t_venta_numero_t * n_t_venta_numero = new n_t_venta_numero_t;
				n_t_venta_numero->venta = 0;
				n_t_venta_numero->preventa = 0;

				g_tree_insert(n_t_tipomonto->t_venta_numero, numero, n_t_venta_numero);

				g_tree_insert(sorteo->getTiposMontoTriple(), idMontoTriple, n_t_tipomonto);
			}
		}

		/***************************************************************************/

		for (unsigned int j = 0; j < a_tipomonto_terminal->len; ++j) {
			data_tipomonto_t data_tipomonto = g_array_index(a_tipomonto_terminal, data_tipomonto_t, j);

			boost::uint8_t * idMontoTerminal = new boost::uint8_t;
			*idMontoTerminal = data_tipomonto.id;

			n_t_tipomonto_t * n_t_tipomonto = new n_t_tipomonto_t;
			n_t_tipomonto->valorinicial = data_tipomonto.valorInicial;
			n_t_tipomonto->limite_actual = data_tipomonto.valorInicial;
			n_t_tipomonto->proporcion = data_tipomonto.proporcion;
			n_t_tipomonto->incremento = data_tipomonto.incremento;
			n_t_tipomonto->valoradicional = data_tipomonto.valorAdicional;
			n_t_tipomonto->montos[0] = data_tipomonto.montos[0];
			n_t_tipomonto->montos[1] = data_tipomonto.montos[1];
			n_t_tipomonto->montos[2] = data_tipomonto.montos[2];
			n_t_tipomonto->venta_global = 0;
			n_t_tipomonto->valorfinal = data_tipomonto.valorFinal;
			n_t_tipomonto->t_venta_numero = g_tree_new(uint16_t_comp_func);

			sem_init(&n_t_tipomonto->sem_t_tipomonto, 0, 1);

			boost::uint16_t maximo = sorteo->isConSigno() ? MAXTERMINALAZO : MAXTERMINAL;
			boost::uint16_t inicio = sorteo->isConSigno() ? MAXTERMINAL + 1 : 0;
			for (unsigned int k = inicio; k <= maximo; ++k) {
				boost::uint16_t * numero = new boost::uint16_t;
				*numero = k;

				n_t_venta_numero_t * n_t_venta_numero = new n_t_venta_numero_t;
				n_t_venta_numero->venta = 0;
				n_t_venta_numero->preventa = 0;

				g_tree_insert(n_t_tipomonto->t_venta_numero, numero, n_t_venta_numero);
			}
			g_tree_insert(sorteo->getTiposMontoTerminal(), idMontoTerminal, n_t_tipomonto);

		}
	}
	mysql_free_result(result);
	g_array_free(a_tipomonto_triple, true);
	g_array_free(a_tipomonto_terminal, true);
	
}

/*-------------------------------------------------------------------------*/
struct cargar_renglones_t
{
	boost::uint32_t idAgente;
	boost::uint16_t numero;
	boost::uint32_t monto;
	boost::uint8_t tipo;
	boost::uint8_t idSorteo;
	boost::uint8_t idZona;
};

void
cargar_venta_global()
{
	log_mensaje(NivelLog::Detallado, "mensaje", "servidor", "Cargando venta global");

	DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
	{
		char const * sqlTempTable =
		    "CREATE TEMPORARY TABLE tmp (idTicket INT UNSIGNED, idAgente INT UNSIGNED, idZona TINYINT UNSIGNED)";
		ejecutar_sql(mysql, sqlTempTable, DEBUG_CARGAR_VENTA_GLOBAL);
	}
	{
		time_t t = time(NULL);
		struct tm* fecha = localtime(&t);
		boost::format sqlQuery;
		sqlQuery.parse("INSERT INTO tmp SELECT Tickets.Id, Agentes.Id, Agentes.IdZona FROM Tickets, Agentes "
		                "WHERE Tickets.Fecha >= '%d-%02d-%02d 00:00:00' "
                        "AND   Tickets.Fecha <= '%d-%02d-%02d 23:59:59' "
                        "AND Tickets.Estado IN (%u,%u) "
		                "AND Agentes.Id=Tickets.IdAgente")
		% (fecha->tm_year + 1900) % (fecha->tm_mon + 1) % fecha->tm_mday
        % (fecha->tm_year + 1900) % (fecha->tm_mon + 1) % fecha->tm_mday
		% unsigned(NORMAL) % unsigned(PAGADO);
		ejecutar_sql(mysql, sqlQuery, DEBUG_CARGAR_VENTA_GLOBAL);
	}

	std::vector<cargar_renglones_t> renglonesParaCargar;
	{
		char const* sqlSelectTemp =
		    "SELECT tmp.idAgente, Numero, Monto, Tipo, idSorteo, tmp.idZona "
		    "FROM tmp, Renglones WHERE Renglones.idTicket = tmp.idTicket "
		    "ORDER BY tmp.idAgente, idSorteo";
		ejecutar_sql(mysql, sqlSelectTemp, DEBUG_CARGAR_VENTA_GLOBAL);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			cargar_renglones_t renglonParaCargar;
			renglonParaCargar.idAgente = atoi(row[0]);
			renglonParaCargar.numero = atoi(row[1]);
			renglonParaCargar.monto = atoi(row[2]);
			renglonParaCargar.tipo = atoi(row[3]);
			renglonParaCargar.idSorteo = atoi(row[4]);
			renglonParaCargar.idZona = atoi(row[5]);

			renglonesParaCargar.push_back(renglonParaCargar);
		}
		mysql_free_result(result);
	}

	n_t_zona_numero_restringido_t * zona_num_restringido = NULL;
	TipoDeMontoPorAgencia * tipomonto_agencia = NULL;
	n_t_tipomonto_t * tipomonto_terminal = NULL;
	n_t_tipomonto_t * tipomonto_triple = NULL;
	n_t_sorteo_numero_restringido_t * sorteo_num_restringido = NULL;

	boost::uint32_t idAgente = 0;
	boost::uint8_t idSorteo = 0;

	for (std::size_t i = 0, size = renglonesParaCargar.size(); i < size ; ++i) {
		boost::uint32_t tempIdAgente = renglonesParaCargar[i].idAgente;
		boost::uint16_t numero = renglonesParaCargar[i].numero;
		boost::uint32_t monto = renglonesParaCargar[i].monto;
		boost::uint8_t tipo = renglonesParaCargar[i].tipo;
		boost::uint8_t tempIdSorteo = renglonesParaCargar[i].idSorteo;
		boost::uint8_t idZona = renglonesParaCargar[i].idZona;

		if (tempIdAgente != idAgente) {
			idAgente = tempIdAgente;
			idSorteo = tempIdSorteo;

			do {                
				tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
				if (tipomonto_agencia == NULL) {
					cargar_tipomonto_x_sorteo(idAgente, mysql);
				}
			} while (tipomonto_agencia == NULL);
			zona_num_restringido =
			    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido,
			                                                       &idZona);
			sorteo_num_restringido =
			    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
			                                                         t_sorteo_numero_restringido,
			                                                         &idSorteo);
			Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
			TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
			if (tipomonto_sorteo == NULL) {
				g_error("La agencia [%u] No tiene Tipos de Monto para el sorteo [%u]",
				         idAgente,
				         idSorteo);
			}
			boost::uint8_t tipoDeMonto = tipomonto_sorteo->tipomonto_terminal;
			tipomonto_terminal =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipoDeMonto);
			tipoDeMonto = tipomonto_sorteo->tipomonto_triple;
			tipomonto_triple =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipoDeMonto);
		}
		if (idSorteo != tempIdSorteo) {
			idSorteo = tempIdSorteo;
			sorteo_num_restringido =
			    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
			                                                         t_sorteo_numero_restringido,
			                                                         &idSorteo);
			Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
			TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
			boost::uint8_t tipoDeMonto = tipomonto_sorteo->tipomonto_terminal;
			tipomonto_terminal =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipoDeMonto);
			tipoDeMonto = tipomonto_sorteo->tipomonto_triple;
			tipomonto_triple =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipoDeMonto);
		}
		if (tipo == TERMINAL || tipo == TERMINALAZO) {
			n_t_venta_numero_t * venta_numero =
			    (n_t_venta_numero_t *) g_tree_lookup(tipomonto_terminal->t_venta_numero, &numero);

			if (venta_numero == NULL) {
				g_error("Terminal [%u] no existe tipo [%u] sorteo [%d]", numero, tipo, tempIdSorteo);
			}
			/*-------------------------------------------------------------------------------------------*/
			venta_numero->venta += monto;
			tipomonto_terminal->venta_global += monto;
			limite_tipomonto(tipomonto_terminal, tipo);

			/*-------------------------------------------------------------------------------------------*/

			n_t_numero_restringido_t * num_restringido =
			    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
			                                                  t_numero_restringido_terminal,
			                                                  &numero);
			if (num_restringido != NULL) {
				num_restringido->venta += monto;
			}
		} else if (tipo == TRIPLE || tipo == TRIPLETAZO || tipo == BONO || tipo == BONOTRIPLETAZO) {
			n_t_venta_numero_t * venta_numero =
			    (n_t_venta_numero_t *) g_tree_lookup(tipomonto_triple->t_venta_numero, &numero);

			if (venta_numero == NULL) {
				g_error("Triple [%u] no existe", numero);
			}
			/*-------------------------------------------------------------------------------------------*/
			venta_numero->venta += monto;
			tipomonto_triple->venta_global += monto;
			limite_tipomonto(tipomonto_triple, tipo);

			/*-------------------------------------------------------------------------------------------*/

			n_t_numero_restringido_t * num_restringido =
			    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
			                                                  t_numero_restringido_triple, &numero);
			if (num_restringido != NULL) {
				num_restringido->venta += monto;
			}
		}
	}

	{
		char const* sqlDropTemp = "DROP TABLE tmp";
		ejecutar_sql(mysql, sqlDropTemp, DEBUG_CARGAR_VENTA_GLOBAL);
	}
	log_mensaje(NivelLog::Detallado, "mensaje", "servidor", "Fin de cargar la venta global");
	
}

/*-------------------------------------------------------------------------*/
struct Restringido
{
	boost::uint16_t numero;
	double proporcion;
	boost::int32_t fechaHasta;
	boost::uint32_t tope;
};

void
cargar_numeros_restringidos()
{
	log_mensaje(NivelLog::Detallado, "mensaje", "servidor", "Cargando numeros restringidos");

	DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();

	std::vector<boost::uint8_t> idZonas;
	{
		char const* sqlIdZonas = "SELECT Id FROM Zonas";
		ejecutar_sql(mysql, sqlIdZonas, DEBUG_NUMEROS_RESTRINGIDOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result);
		        NULL != row;
		        row = mysql_fetch_row(result)) {
			idZonas.push_back(strtoul(row[0], NULL, 10));
		}
		mysql_free_result(result);
	}

#if 0
	if (idZonas.size() < 1) {
		log_mensaje(NivelLog::Bajo, "error", "base de datos",
		             "No se pudo cargar numeros restringidos: "
		             "cargar_numeros_restringidos() Zonas no tiene registros");
		
		cerrar_archivos_de_registro();
		exit(1);
	}
#endif

	std::vector<boost::uint8_t> idSorteos;
	{
		char const* sqlIdSorteos = "SELECT Id FROM Sorteos";
		ejecutar_sql(mysql, sqlIdSorteos, DEBUG_NUMEROS_RESTRINGIDOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result);
		        NULL != row;
		        row = mysql_fetch_row(result)) {
			idSorteos.push_back(strtoul(row[0], NULL, 10));
		}
		mysql_free_result(result);
	}

#if 0
	if (idSorteos.size() < 1) {
		log_mensaje(NivelLog::Bajo, "error", "base de datos",
		             "No se pudo cargar numeros restringidos: "
		             "cargar_numeros_restringidos() Sorteos no tiene registros");
		
		cerrar_archivos_de_registro();
		exit(1);
	}
#endif

	t_zona_numero_restringido = g_tree_new(uint8_t_comp_func);

	for (std::vector<boost::uint8_t>::const_iterator idZona = idZonas.begin();
	        idZona != idZonas.end();
	        ++idZona) {

		boost::uint8_t * id = new boost::uint8_t;
		*id = *idZona;
		n_t_zona_numero_restringido_t * zona = new n_t_zona_numero_restringido_t;
		sem_init(&zona->sem_t_sorteo_numero_restringido, 0, 1);
		zona->t_sorteo_numero_restringido = g_tree_new(uint8_t_comp_func);
		g_tree_insert(t_zona_numero_restringido, id, zona);

		for (std::vector<boost::uint8_t>::const_iterator idSorteo = idSorteos.begin();
		        idSorteo != idSorteos.end();
		        ++idSorteo) {

			id = new boost::uint8_t;
			*id = *idSorteo;
			n_t_sorteo_numero_restringido_t * sorteo = new n_t_sorteo_numero_restringido_t;
			sorteo->t_numero_restringido_triple = g_tree_new(uint16_t_comp_func);
			sorteo->t_numero_restringido_terminal = g_tree_new(uint16_t_comp_func);
			g_tree_insert(zona->t_sorteo_numero_restringido, id, sorteo);

			std::vector<Restringido> terminalesRestringidos;
			{
				boost::format sqlQuery;
				sqlQuery.parse("SELECT Numero, PorcVenta, UNIX_TIMESTAMP(FechaHasta), Tope FROM Restringidos "
				                "WHERE IdSorteo=%1% AND IdZona=%2% AND Tipo IN (%3%, %4%)")
				% unsigned(*idSorteo) % unsigned(*idZona) % unsigned(TERMINAL) % unsigned(TERMINALAZO);
				ejecutar_sql(mysql, sqlQuery, DEBUG_NUMEROS_RESTRINGIDOS);
				MYSQL_RES * result = mysql_store_result(mysql);
				for (MYSQL_ROW row = mysql_fetch_row(result);
				        NULL != row;
				        row = mysql_fetch_row(result)) {
					Restringido numeroRestringido;
					numeroRestringido.numero = strtoul(row[0], NULL, 10);
					numeroRestringido.proporcion = atoi(row[1]) / 10000;
					numeroRestringido.fechaHasta = atoi(row[2]);
					numeroRestringido.tope = strtoul(row[3], NULL, 10);
					terminalesRestringidos.push_back(numeroRestringido);
				}
				mysql_free_result(result);
			}

			for (std::vector<Restringido>::const_iterator restringido = terminalesRestringidos.begin();
			        restringido != terminalesRestringidos.end();
			        ++restringido) {

				if (restringido->fechaHasta + 86399 > time(NULL)) {
					boost::uint16_t * num = new boost::uint16_t;
					*num = restringido->numero;
					n_t_numero_restringido_t * numeroRestringido = new n_t_numero_restringido_t;
					numeroRestringido->proporcion = restringido->proporcion;
					numeroRestringido->venta = 0;
					numeroRestringido->preventa = 0;
					numeroRestringido->tope = restringido->tope;
					g_tree_insert(sorteo->t_numero_restringido_terminal, num, numeroRestringido);
				}
			}

			std::vector<Restringido> triplesRestringidos;
			{
				boost::format sqlQuery;
				sqlQuery.parse("SELECT Numero, PorcVenta, UNIX_TIMESTAMP(FechaHasta), Tope FROM Restringidos "
				                "WHERE IdSorteo=%1% AND IdZona=%2% AND Tipo IN (%3%, %4%)")
				% unsigned(*idSorteo) % unsigned(*idZona) % unsigned(TRIPLE) % unsigned(TRIPLETAZO);
				ejecutar_sql(mysql, sqlQuery, DEBUG_NUMEROS_RESTRINGIDOS);
				MYSQL_RES * result = mysql_store_result(mysql);
				for (MYSQL_ROW row = mysql_fetch_row(result);
				        NULL != row;
				        row = mysql_fetch_row(result)) {
					Restringido numeroRestringido;
					numeroRestringido.numero = strtoul(row[0], NULL, 10);
					numeroRestringido.proporcion = atoi(row[1]) / 10000;
					numeroRestringido.fechaHasta = atoi(row[2]);
					numeroRestringido.tope = strtoul(row[3], NULL, 10);
					triplesRestringidos.push_back(numeroRestringido);
				}
				mysql_free_result(result);
			}

			for (std::vector<Restringido>::const_iterator restringido = triplesRestringidos.begin();
			        restringido != triplesRestringidos.end();
			        ++restringido) {

				if (restringido->fechaHasta + 86399 > time(NULL)) {
					boost::uint16_t * num = new boost::uint16_t;
					*num = restringido->numero;
					n_t_numero_restringido_t *numeroRestringido = new n_t_numero_restringido_t;
					numeroRestringido->proporcion = restringido->proporcion;
					numeroRestringido->venta = 0;
					numeroRestringido->preventa = 0;
					numeroRestringido->tope = restringido->tope;
					g_tree_insert(sorteo->t_numero_restringido_triple, num, numeroRestringido);
				}
			}
		}
	}
	
}

/***************************************************************************/
void
escribir_pidfile()
{
	char directorio_actual[BUFFER_SIZE];
	getcwd(&directorio_actual[0], sizeof(directorio_actual));
	if (chdir(directorioDePidof) < 0) {
		g_error("No se puedo tener acceso al directorio <%s>: chdir()", directorioDePidof);
	}

	char pidfile[BUFFER_SIZE];
	sprintf(pidfile, "%s.pid", nombreDelPrograma);

	int fd_pidfile = open(pidfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd_pidfile < 0) {
		g_error("No se pudo abrir el archivo <%s%s>: open()", directorioDePidof, pidfile);
	}

	pid_t pid = getpid();

	char strpid[BUFFER_SIZE];
	sprintf(strpid, "%d\n", pid);

	write(fd_pidfile, strpid, strlen(strpid));
	close(fd_pidfile);
	chdir(directorio_actual);
}

void
abrir_archivos_de_registro()
{
	char mensaje[BUFFER_SIZE];
	char directorio_actual[BUFFER_SIZE];

	umask(022);

	getcwd(directorio_actual, sizeof(directorio_actual));
	if (chdir(directorioDeLogs) < 0) {
		g_error("No se puedo tener acceso al directorio <%s>: chdir()", directorioDeLogs);
	}
	fd_log_mensajes = open(REGISTRO_MENSAJES, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd_log_mensajes == -1) {
		perror("");
		g_error("No se pudo abrir el registro '%s%s': open()", directorioDeLogs,
		         REGISTRO_MENSAJES);
	}
	sem_init(&mutex_t_fd_log_mensajes, 0, 1);

	log_mensaje(NivelLog::Bajo, "mensaje", "servidor", "Servidor iniciado");

	sprintf(mensaje, "Archivo de registro <%s> abierto con exito", REGISTRO_MENSAJES);
	log_mensaje(NivelLog::Normal, "mensaje", "servidor", mensaje);

	fd_log_clientes = open(REGISTRO_CLIENTES, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd_log_clientes == -1) {
		g_error("No se pudo abrir el registro '%s%s': open()", directorioDeLogs,
		         REGISTRO_CLIENTES);
	}
	sem_init(&mutex_t_fd_log_clientes, 0, 1);

	sprintf(mensaje, "Archivo de registro <%s> abierto con exito", REGISTRO_CLIENTES);
	log_mensaje(NivelLog::Normal, "mensaje", "servidor", mensaje);

	fd_log_admin = open(REGISTRO_ADMIN, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd_log_admin == -1) {
		g_error("No se pudo abrir el registro '%s%s': open()", directorioDeLogs,
		         REGISTRO_ADMIN);
	}
	sem_init(&mutex_t_fd_log_admin, 0, 1);

	sprintf(mensaje, "Archivo de registro <%s> abierto con exito", REGISTRO_ADMIN);
	log_mensaje(NivelLog::Normal, "mensaje", "servidor", mensaje);
	chdir(directorio_actual);
}

void
eliminar_pidfile()
{
	char pidfile[BUFFER_SIZE];
	sprintf(pidfile, "%s/%s.pid", directorioDePidof, nombreDelPrograma);
	unlink(pidfile);
}

void
salir(int)
{
	minThreadNumber = 0;
	finalizar = true;

	SocketThread::close();

    ZeusServicioAdmin::ZeusServicioAdmin::instance()->close();
    
	pthread_cancel(*hilo_free_preventa_t);

	boost::uint8_t cont = 1;
	while ((SocketThread::size() > 0) && (++cont < 60)) {
		struct timespec t;
		t.tv_sec = 1;
		t.tv_nsec = 0;
		nanosleep(&t, NULL);
	}
	close(sck_serv);	

	log_mensaje(NivelLog::Bajo, "mensaje", "servidor", "Servidor finalizado");

	cerrar_archivos_de_registro();
	eliminar_pidfile();
	exit(0);
}


void *
hilo_escucha(void *)
{
	for (int z = 0; z < minThreadNumber; ++z) {		
    	SocketThread::addNewIdle();
    }
    
    if (finalizar) {
        return NULL;
    }

	/*
	 * Se avisa al sistema que atenda llamadas de clientes
	 */
	if (listen(sck_serv, colaDeEspera) == -1) {
		char mensaje[BUFFER_SIZE];
		close(sck_serv);
		sprintf(mensaje, "No se puede escuchar el socket <%d>: listen() coderror:%d", sck_serv, errno);
		log_mensaje(NivelLog::Bajo, "error", "servidor", mensaje);
		cerrar_archivos_de_registro();
		exit(1);
	}

	/*
	 * Espera a que llegue una solicitud de coneccion y la asigna al
	 * primer sd disponible
	 */
	while (!finalizar) {
        sockaddr_in sockaddrIn;
		memset(&sockaddrIn, 0, sizeof(sockaddr_in));
		socklen_t sockaddrInLen = sizeof(sockaddr_in);
		int sdCliente = accept(sck_serv, (struct sockaddr *)&sockaddrIn, &sockaddrInLen);
		if (sdCliente == -1) {
			char mensaje[BUFFER_SIZE];
			sprintf(mensaje, "No se pudo aceptar conexion del cliente: accept() coderror:%d", errno);
			log_mensaje(NivelLog::Bajo, "advertencia", "servidor", mensaje);
			continue;
		}
        int so = 1;
		setsockopt(sdCliente, SOL_SOCKET, SO_REUSEADDR, &so, sizeof(so));

		if (SocketThread::size() >= maxThreadNumber && SocketThread::idleSize() == 0) {
			close(sdCliente);
			log_mensaje(NivelLog::Bajo, "advertencia", "servidor",
			             "No se pudo aceptar conexion del cliente: "
			             "numero maximo de conexiones alcanzado");
			continue;
		} else {
            if (SocketThread::idleSize() == 0) {
    			for (int z = 0; z < deltaThreadNumber; ++z) {
    				if (SocketThread::size() <= maxThreadNumber) {
    					SocketThread::addNewIdle();
    				} 
    			}
			}

			/*
			 * escribe en el arreglo del hilo correspondiente el nuevo sd
			 */
			SocketThread* socketsThread = SocketThread::getIdle();

			if (socketsThread == NULL) {
				close(sck_serv);
				log_mensaje(NivelLog::Bajo, "error", "servidor", "El hilo no existe");
				cerrar_archivos_de_registro();
				exit(1);
			}
            {
                SocketThread::MutexGuard g;
                std::auto_ptr<ConexionActiva> 
                    conexionTaquilla = ConexionActiva::create(sdCliente, inet_ntoa(sockaddrIn.sin_addr));
			
            
                char mensaje[BUFFER_SIZE];
                sprintf(mensaje, "Conexion del cliente abierta - socket <%d>", sdCliente);
                log_mensaje(NivelLog::Detallado, "mensaje", conexionTaquilla->ipAddress.c_str(), mensaje);
            
            
                socketsThread->setConexionTaquilla(conexionTaquilla);            			
            }
			socketsThread->post();
			/*
			 * Se libera el SELECT() del hilo de la nueva coneccion
			 */
			CabeceraInterna* cabecera = new CabeceraInterna;
			cabecera->longitud = 0;
			cabecera->peticion = OK;

			if (send2hilo_socket(socketsThread->getThread(), cabecera) == -1)
				g_error("send2hilo_socket()");

			delete cabecera;
		}
	}
	return NULL;
}

bool strBeginWith(char const* variable, char const* subString)
{
	return strncmp(variable, subString, strlen(subString)) == 0;
}

bool readStrValue(char const* variable, char const* variableName, char* dest, char const* value)
{
	if (strBeginWith(variable, variableName)) {
		strcpy(dest, value);
		return true;
	}
	return false;
}

template <typename DEST_TYPE>
bool readNumValue(char const* variable, char const* variableName, DEST_TYPE& dest, char const* value)
{
	if (strBeginWith(variable, variableName)) {
		dest = strtol(value, NULL, 10);
		return true;
	}
	return false;
}

template <bool>
bool readNumValue(char const* variable, char const* variableName, bool& dest, char const* value)
{
	if (strBeginWith(variable, variableName)) {
		dest = (strtol(value, NULL, 10) == 0) ? false : true;
		return true;
	}
	return false;
}

void
cargar_archivo_config(char *fn)
{
#define READ_STR_VALUE(VAR_NAME, DEST) \
    readStrValue(variable, VAR_NAME, DEST, valor)

#define READ_NUM_VALUE(VAR_NAME, DEST) \
    readNumValue(variable, VAR_NAME, DEST, valor)

	strcpy(configuracion.dbZeusName,  "zeus");
    strcpy(configuracion.dbSisacName, "Sisac");
	strcpy(configuracion.dbServer, "127.0.0.1");
	strcpy(configuracion.dbUser, "root");
	strcpy(configuracion.dbPassword, "");
	configuracion.dbPort = 1308;
	configuracion.maximoTiempoVenta = 1;
    configuracion.dbConnections = 1;
	strcpy(directorioDeLogs, "./");

	if (access(fn, R_OK) != 0) {
		g_error("No se pudo abrir el archivo de configuracion %s.  Verifique "
		         "que el Archivo exista y este en la ruta especificada.", fn);
		exit(1);
	}

	FILE * fparam = fopen(fn, "r");
	char variable[250], *valor;
	while (not feof(fparam)) {
		do {
			fgets(variable, 250, fparam);
		} while (not feof(fparam) and (strlen(variable) == 1 or variable[0] == '#'));

		if (feof(fparam)) {
			break;
		}

		valor = strchr(variable, '=');
		if (valor != NULL) {
			*valor = '\0';
			++valor;
			char *ultimoCaracter = strchr(valor, '\n');
			*ultimoCaracter = '\0';
		}

		if (not (READ_STR_VALUE("DB_ZEUS_NAME", configuracion.dbZeusName)
                   or READ_STR_VALUE("DB_SISAC_NAME", configuracion.dbSisacName)
		           or READ_STR_VALUE("SERV_DB", configuracion.dbServer)
		           or READ_STR_VALUE("USER", configuracion.dbUser)
		           or READ_STR_VALUE("PASSWORD", configuracion.dbPassword)
		           or READ_NUM_VALUE("PUERTO", configuracion.dbPort)
                   or READ_NUM_VALUE("DB_NUMERO_CONEXIONES", configuracion.dbConnections)
		           or READ_NUM_VALUE("MAX_TIEMPO_VENTA", configuracion.maximoTiempoVenta)
		           or READ_NUM_VALUE("MIN_HILOS", minThreadNumber)
		           or READ_NUM_VALUE("MAX_HILOS", maxThreadNumber)
		           or READ_NUM_VALUE("MAX_ADMIN_CON", conexionesAdminMax)
		           or READ_NUM_VALUE("DELTA_HILOS", deltaThreadNumber)
		           or READ_NUM_VALUE("SERVIDOR_PUERTO_ADMIN", puertoAdmin)
		           or READ_NUM_VALUE("SERVIDOR_PUERTO", puertoTaquillas)
		           or READ_NUM_VALUE("TIEMPO_ESPERA", tiempoDeEspera)
		           or READ_NUM_VALUE("COLA_DE_ESPERA", colaDeEspera)
		           or READ_STR_VALUE("DIRECTORIO_REGISTROS", directorioDeLogs)
		           or READ_STR_VALUE("DIRECTORIO_PIDOF", directorioDePidof)
		           or READ_STR_VALUE("DIRECTORIO_RESPALDOS", directorioDeRespaldos)
		           or READ_NUM_VALUE("NIVEL_REGISTRO_MENSAJES", nivelLogMensajes)
		           or READ_NUM_VALUE("NIVEL_REGISTRO_CLIENTES", nivelLogAccesoTaquillas)
		           or READ_NUM_VALUE("NIVEL_REGISTRO_ADMIN", nivelLogAccesoAdmin)
		           or READ_NUM_VALUE("TIEMPO_ELIMINACION", tiempoDeEliminacion)
		           or READ_NUM_VALUE("TIEMPO_CHEQUEO_PREVENTA", tiempoChequeoPreventa)
		           or READ_STR_VALUE("AUTENTIFICACION", autentificacion)
		           or READ_NUM_VALUE("TIEMPO_VERIFICAR_CONEXION", configuracion.tiempoVerificarConexion)
		           or READ_NUM_VALUE("TIEMPO_DESCONECTAR", configuracion.tiempoDesconectar)
		           or READ_NUM_VALUE("DIASINFORMACION", diasDeInformacion)
		           or READ_STR_VALUE("RUTA_ACTUALIZACION_TAQUILLA", configuracion.rutaTaquilla)
                   or READ_STR_VALUE("URL_SELCO", configuracion.urlSelco) 
                   or READ_STR_VALUE("ODBC_TRANSAX", configuracion.libreriaOBDCTransax))) {

			g_error("Variable <%s> desconocida verifique su configuracion y \n "
			        "ejecute '%s' de nuevo.", variable, nombreDelPrograma);
		}
	}                            /* while */
	fclose(fparam);
}

struct Taquilla
{
	boost::uint32_t idAgente;
	boost::uint8_t idTaquilla;
	boost::uint8_t idZona;
};

void
crear_arbol_preventas_x_taquillas()
{
	log_mensaje(NivelLog::Detallado,
	            "mensaje",
	            "servidor",
	            "Creando estructura de preventas por taquilla");

	DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();

	t_preventas_x_taquilla = g_tree_new(uint64_t_comp_func);

	std::vector<Taquilla> taquillas;
	{
		char const* sqlQuery =
		    "SELECT IdAgente, Numero, IdZona FROM Agentes, Taquillas WHERE Taquillas.IdAgente=Agentes.Id";
		ejecutar_sql(mysql, sqlQuery, DEBUG_CREAR_ARBOL_PREVENTAS_X_TAQUILLAS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			Taquilla taquilla;
			taquilla.idAgente = atoi(row[0]);
			taquilla.idTaquilla = atoi(row[1]);
			taquilla.idZona = atoi(row[2]);

			taquillas.push_back(taquilla);
		}
		mysql_free_result(result);
	}

	for (std::size_t i = 0, size = taquillas.size(); i < size ; ++i) {
		n_t_preventas_x_taquilla_t * n_t_p_x_t = new n_t_preventas_x_taquilla_t;

		boost::uint64_t *p_agente_taquilla = new boost::uint64_t;

		sem_init(&n_t_p_x_t->mutex_preventas_x_taquilla, 0, 1);
		n_t_p_x_t->preventas =
		    g_tree_new_full(uint16_t_gcomp_func, NULL, uint16_t_destroy_key,
		                     t_n_t_preventas_t_destroy);
		n_t_p_x_t->hora_ultimo_renglon = time(NULL);

		n_t_p_x_t->idZona = taquillas[i].idZona;
		boost::uint64_t agente_taquilla = taquillas[i].idAgente;
		agente_taquilla = agente_taquilla << 32;
		agente_taquilla += taquillas[i].idTaquilla;
		*p_agente_taquilla = agente_taquilla;
		g_tree_insert(t_preventas_x_taquilla, p_agente_taquilla, n_t_p_x_t);
	}

	/*Desconectar todas las taquillas del sistema*/
	{
		char const* sqlTaquillasHiloNull = "UPDATE Taquillas SET Hilo=NULL";
		ejecutar_sql(mysql, sqlTaquillasHiloNull, DEBUG_CREAR_ARBOL_PREVENTAS_X_TAQUILLAS);
	}
	{
		char const* sqlUsuariosHiloNull = "UPDATE Usuarios SET Hilo=NULL";
		ejecutar_sql(mysql, sqlUsuariosHiloNull, DEBUG_CREAR_ARBOL_PREVENTAS_X_TAQUILLAS);
	}

	
}

void
uso(const char *prog)
{
	fprintf(stderr, 
        std::string(
            "zeus " + VERSION + "Uso: %s [<opciones>]\n" 
            "Opciones Posibles:\n"
	       "\t-c <archivo de configuracion>   Archivo de configuracion alternativo.\n"
	       "\t-f                              Se ejecuta en primer plano.\n"
	       "\t-h o -?                         Imprime esta mensaje.\n"
        ).c_str(), 
        prog);
	exit(1);
}

void
procesar_argumentos(int argc, char *argv[])
{
	extern char * optarg;
	int opciones;

	nombreDelPrograma = basename(argv[0]);

	while ((opciones = getopt(argc, argv, "h?fc:")) != EOF) {
		switch (opciones) {
			case 'f':
				isDaemon = false;
				break;
			case 'c':
				archivo_config = new char[strlen(optarg) + 1];
				strcpy(archivo_config, optarg);
				break;
			case 'h':
			case '?':
				uso(argv[0]);
		}
	}

	if (getuid() == 0)
		g_error("%s no puede ejecutarse como super usuario: getuid()", argv[0]);
}

void
iniciar_daemon()
{
	pid_t pid;

	/*
	 * Haga forking
	 */
	if ((pid = fork()) < 0) {
		g_error("fork()");
	}
	if (pid > 0) {
		exit(0);                /* padre */
	}
	/*
	 * Primer proceso hijo
	 */
	setsid();                  /* sea el lider de sesion */

	if ((pid = fork()) < 0) {
		g_error("fork()");
	}
	if (pid > 0) {
		/* primer hijo */
		exit(0);
	}
}

void
mensaje_error(char const*, GLogLevelFlags, const char * mensaje, void* )
{
	std::string error("*ERROR: ");
	error += mensaje;
	error += ": ";
	error += strerror(errno);
	log_mensaje(NivelLog::Bajo, "error", "servidor", error.c_str());
	g_print("%s\n", error.c_str());
	cerrar_archivos_de_registro();
	eliminar_pidfile();
	exit(1);
}

/*-----------------------------------------------------------------------------*/

void
respaldar(MYSQL *mysql_con, boost::format const& sqlQueryTail, std::string const& tablaHistorico)
{
    {
        boost::format sqlQueryHead;
        sqlQueryHead.parse("INSERT INTO %1% SELECT * ") % tablaHistorico;

        std::ostringstream sqlStream;
        sqlStream << sqlQueryHead << sqlQueryTail;

        ejecutar_sql(mysql_con, sqlStream.str().c_str(), DEBUG_respaldar);
    }
    {
        boost::format sqlQueryHead;
        std::ostringstream sqlStream;
        sqlStream << "DELETE " << sqlQueryTail;

        ejecutar_sql(mysql_con, sqlStream.str().c_str(), DEBUG_respaldar);
    }
}

void
depurar(MYSQL *mysql_con, boost::format const& sqlQueryTail, char *archivo)
{
	{
		boost::format sqlQueryHead;
		sqlQueryHead.parse("SELECT * INTO OUTFILE '%1%' ") % archivo;

		std::ostringstream sqlStream;
		sqlStream << sqlQueryHead << sqlQueryTail;

		ejecutar_sql(mysql_con, sqlStream.str().c_str(), DEBUG_DEPURAR_SISTEMA);
	}
	{
		boost::format sqlQueryHead;
		std::ostringstream sqlStream;
		sqlStream << "DELETE " << sqlQueryTail;

		ejecutar_sql(mysql_con, sqlStream.str().c_str(), DEBUG_DEPURAR_SISTEMA);
	}
}

/*-----------------------------------------------------------------------------*/

void
cargar_db_configuracion()
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    char const* sql = "select valor from Configuracion where llave='IGF'";
    ejecutar_sql(mysql, sql, DEBUG_DB_CONFGURACION);
    MYSQL_RES * result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (NULL != row) {
        configuracion.impuesto_ganancia_fortuita = atoi(row[0]);
    } else {
        configuracion.impuesto_ganancia_fortuita = 1600;
    }
    mysql_free_result(result);
}

void
depurar_sistema()
{
	if (diasDeInformacion == 0) {
		return ;
	}

	DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();

	bool hayTicketsMasAntiguos = false;
	time_t fechahasta;
	{
		char const* sqlTicketsAntiguos = "SELECT MIN(unix_timestamp(Fecha)) FROM Tickets";
		ejecutar_sql(mysql, sqlTicketsAntiguos, DEBUG_DEPURAR_SISTEMA);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			fechahasta = atoi(row[0]);
			hayTicketsMasAntiguos = true;
		}
		mysql_free_result(result);
	}

	if (not hayTicketsMasAntiguos) {
		
		return ;
	}

	time_t const unDia = 86400;
	time_t fecha = time(NULL) - unDia;
	struct tm * TM = localtime(&fecha);
	
	{
		boost::format sqlQuery;
		sqlQuery.parse("DELETE FROM Restringidos WHERE FechaHasta < '%d-%02d-%02d'")
		% (TM->tm_year + 1900) % (TM->tm_mon + 1) % TM->tm_mday;
		ejecutar_sql(mysql, sqlQuery, DEBUG_DEPURAR_SISTEMA);
	}
	{
		time_t const lapso = time(0) - (unDia*30);
		struct tm * lapsoTm = localtime(&lapso);
		boost::format sqlQuery;
		sqlQuery.parse("DELETE FROM Mensajeria WHERE FechaHora < '%d-%02d-%02d 00:00:00' and Leido=1")
		% (lapsoTm->tm_year + 1900) % (lapsoTm->tm_mon + 1) % lapsoTm->tm_mday;
		ejecutar_sql(mysql, sqlQuery, DEBUG_DEPURAR_SISTEMA);
	}

	int numeroDeDias = 0;
	while (numeroDeDias < diasDeInformacion && fecha > fechahasta) {
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT * FROM Tickets "
                           "WHERE Fecha >= '%d-%02d-%02d 00:00:00' "
                           "WHERE Fecha <= '%d-%02d-%02d 23:59:59' "
                           "LIMIT 1")
            % (TM->tm_year + 1900) % (TM->tm_mon + 1) % TM->tm_mday
			% (TM->tm_year + 1900) % (TM->tm_mon + 1) % TM->tm_mday;
			ejecutar_sql(mysql, sqlQuery, DEBUG_DEPURAR_SISTEMA);
			MYSQL_RES * result = mysql_store_result(mysql);
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row != NULL) {
				numeroDeDias ++;
			}
			mysql_free_result(result);
		}
		fecha -= unDia;
	}

	if (numeroDeDias == diasDeInformacion) {
		time_t aux = fechahasta;
		fechahasta = fecha;
		fecha = aux;

		char path[255];
		sprintf(path, "%s/", directorioDeRespaldos);

		char FechaHasta[15];
		TM = localtime(&fechahasta);
		sprintf(FechaHasta, "%d-%02d-%02d", TM->tm_year + 1900, TM->tm_mon + 1, TM->tm_mday);

		while (true) {
			char Fecha[15];
			TM = localtime(&fecha);
			sprintf(Fecha, "%d-%02d-%02d", TM->tm_year + 1900, TM->tm_mon + 1, TM->tm_mday);
			if (strcmp(Fecha, FechaHasta) > 0) {
				break;
			}

			int min = 0;
			{
				boost::format sqlQuery;
				sqlQuery.parse("SELECT MIN(Id) FROM Tickets "
                               "WHERE Fecha >= '%s 00:00:00' "
                               "AND   Fecha <= '%s 23:59:59'")
				% Fecha % Fecha;
				ejecutar_sql(mysql, sqlQuery, DEBUG_DEPURAR_SISTEMA);
				MYSQL_RES * result = mysql_store_result(mysql);
				MYSQL_ROW row = mysql_fetch_row(result);
				if (NULL != row)
				{
					min = atoi(row[0]);
				}
				mysql_free_result(result);
			}

			int max = 0;
			{
				boost::format sqlQuery;
				sqlQuery.parse("SELECT MAX(Id) FROM Tickets "
                               "WHERE Fecha >= '%s 00:00:00' "
                               "AND   Fecha <= '%s 23:59:59'")
				% Fecha % Fecha;
				ejecutar_sql(mysql, sqlQuery, DEBUG_DEPURAR_SISTEMA);
				MYSQL_RES * result = mysql_store_result(mysql);
				MYSQL_ROW row = mysql_fetch_row(result);
				if (NULL != row)
				{
					max = atoi(row[0]);
				}
				mysql_free_result(result);
			}

			if (0 != min and 0 != max) {
				ejecutar_sql(mysql, "BEGIN", DEBUG_DEPURAR_SISTEMA);
				ejecutar_sql(mysql, "SET AUTOCOMMIT=0", DEBUG_DEPURAR_SISTEMA);
                ejecutar_sql(mysql, "SET FOREIGN_KEY_CHECKS=0", DEBUG_DEPURAR_SISTEMA);
				{
					boost::format sqlQuery;
					sqlQuery.parse("FROM Cambio_Ticket "
					                "WHERE (IdTicket_Old >= %1% AND IdTicket_Old <= %2%) "
					                "OR (IdTicket_New >= %1% AND IdTicket_New <= %2%)")
					% min % max;
					char nombre[255];
					sprintf(nombre, "%sCambio_Ticket-%s.txt", path, Fecha);
					depurar(mysql, sqlQuery, nombre);
				}
				{
					boost::format sqlQuery;
					sqlQuery.parse("FROM Info_Ticket WHERE IdTicket >= %1% AND IdTicket <= %2%")
					% min % max;
					char nombre[255];
					sprintf(nombre, "%sInfo_Ticket-%s.txt", path, Fecha);
					depurar(mysql, sqlQuery, nombre);
				}
				{
					boost::format sqlQuery;
					sqlQuery.parse("FROM Resumen_Renglones WHERE IdTicket >= %1% AND IdTicket <= %2%")
					% min % max;
                    char nombre[255];
                    sprintf(nombre, "%sResumen_Renglones-%s.txt", path, Fecha);
                    depurar(mysql, sqlQuery, nombre);
				}
				{
					boost::format sqlQuery;
					sqlQuery.parse("FROM Tickets_Premiados WHERE IdTicket >= %1% AND IdTicket <= %2%")
					% min % max;
					respaldar(mysql, sqlQuery, "Historico_Tickets_Premiados");
				}
				{
					boost::format sqlQuery;
					sqlQuery.parse("FROM Renglones WHERE IdTicket >= %1% AND IdTicket <= %2%")
					% min % max;
					respaldar(mysql, sqlQuery, "Historico_Renglones");
				}
				{
					boost::format sqlQuery;
					sqlQuery.parse("FROM Tickets WHERE Id >= %1% AND Id <= %2%")
					% min % max;
					respaldar(mysql, sqlQuery, "Historico_Tickets");
				}
                ejecutar_sql(mysql, "SET FOREIGN_KEY_CHECKS=1", DEBUG_DEPURAR_SISTEMA);
				ejecutar_sql(mysql, "SET AUTOCOMMIT=1", DEBUG_DEPURAR_SISTEMA);
				ejecutar_sql(mysql, "COMMIT", DEBUG_DEPURAR_SISTEMA);
			}
			fecha += unDia;
		}
	}
	
}

/*-----------------------------------------------------------------------------*/

int
main(int argc, char * argv[])
{
	g_thread_init(NULL);
	aleatorio = g_rand_new();

	signal(SIGKILL, salir);
	signal(SIGQUIT, salir);
	signal(SIGINT, salir);
	signal(SIGTERM, salir);

	procesar_argumentos(argc, argv);

	if (archivo_config == NULL) {
		archivo_config = new char[strlen(nombreDelPrograma) + 13];

		sprintf(archivo_config, "../etc/%s.conf", nombreDelPrograma);
	}

	cargar_archivo_config(archivo_config);
	abrir_archivos_de_registro();

	g_log_set_handler(NULL,
	                   (GLogLevelFlags) (G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL |
	                                        G_LOG_FLAG_RECURSION), mensaje_error, NULL);
	
    DataBaseConnetionPool::instance()->init(configuracion);
    
    cargar_db_configuracion();
    
    if (isDaemon) {
		iniciar_daemon();
	}
	escribir_pidfile();
	depurar_sistema();
    iniciarProtocolosExternos();
	cargar_sorteos();
	cargar_numeros_restringidos();
	crear_arbol_preventas_x_taquillas();
	cargar_venta_global();	
	sck_serv = abrir_socket(puertoTaquillas);
    
	/*
	 * llama al hilo que realiza el listen y las asignaciones de los
	 * sockets por hilo
	 */
	hilo_escucha_t = new pthread_t;
	pthread_create(hilo_escucha_t, NULL, &hilo_escucha, NULL);
    
    ZeusServicioAdmin::ZeusServicioAdmin::instance()->setFinalizarPtr(&finalizar);
    ZeusServicioAdmin::ZeusServicioAdmin::instance()->setConexionesMax(conexionesAdminMax);
    ZeusServicioAdmin::ZeusServicioAdmin::instance()->open(puertoAdmin);

	hilo_free_preventa_t = new pthread_t;
	pthread_create(hilo_free_preventa_t, NULL, &hilo_free_tickets, NULL);

	pthread_join(*hilo_escucha_t, NULL);
    
    ZeusServicioAdmin::ZeusServicioAdmin::instance()->join();
    
    ACE_Thread_Manager::instance()->wait();

	return 0;
}
