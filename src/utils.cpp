#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <sstream>

#include <mysql/mysql.h>
#include <sys/time.h>
#include <glib.h>
#include <unistd.h>

#include <utils.h>
#include <global.h>
#include <sockets.h>
#include <SocketThread.h>

#include <ProtocoloZeus/Mensajes.h>

using namespace ProtocoloZeus;

#define DEBUG_ENVIAR_MENSAJE 0

std::string 
getString(char const * source, std::size_t bufferSize)
{
	assert(source != NULL);
	assert(bufferSize);
	std::vector<char> dest(bufferSize, '\0');
	strncpy(&dest[0], source, bufferSize - 1);
	return std::string(&dest[0]);
}

boost::int16_t
hora_actual()
{
	time_t t_t;
	struct tm tm_t;

	t_t = time(NULL);
	if (t_t == -1)
		return -1;

	tm_t = *localtime(&t_t);

	/*
	 * ho:mi
	 * ^  ^
	 * |  |
	 * |  +---- mi * 1
	 * +------- ho * 100
	 */
	return tm_t.tm_min + tm_t.tm_hour * 100;
}

struct tm
hora_i2t(boost::uint16_t hora)
{
	struct tm tm_t;

	tm_t.tm_hour = hora / 100;
	tm_t.tm_min = hora - (tm_t.tm_hour * 100);

	return tm_t;
}

int
diadehoy()
{
	time_t tim;
	int dia;
	struct tm *p_tm;

	tim = time(NULL);
	p_tm = localtime(&tim);
	dia = p_tm->tm_wday;
	return dia;
}

struct tm
fecha_i2t(boost::uint32_t fecha)
{
	struct tm tm_t;

	tm_t.tm_year = fecha / 10000;
	tm_t.tm_mon = (fecha - (tm_t.tm_year * 10000)) / 100;
	tm_t.tm_mday = fecha - ((fecha / 100) * 100);

	return tm_t;
}

void
genera_password(boost::uint8_t const* fuente, char destino[33])
{
	memset(destino, 0, sizeof(destino));
	for (int i = 0; i < 16; ++i) {
        char c[3];
		sprintf(c, "%02X", fuente[i]);
		strcat(destino, c);
	}
}

#define ER_FILE_EXIST 1086
#define ER_LOCK_WAIT_TIMEOUT 1205
#define ER_LOCK_DEADLOCK 1213
bool
ejecutar_sql(MYSQL * mysql_con, char const* tsql, bool debug_sql)
{
	if (debug_sql == true) {
		log_mensaje(NivelLog::Bajo, "mensaje", "base de datos", tsql);
	}
    int i = 0;
    while (i < 100) {
        if (mysql_query(mysql_con, tsql) == 0) {
            return true;
        }
        int err = mysql_errno(mysql_con);
        if (err == ER_LOCK_WAIT_TIMEOUT || err == ER_LOCK_DEADLOCK) {
            timespec t;
            t.tv_sec = 0;
            t.tv_nsec = 100000000;
            nanosleep(&t, NULL);
            ++i;
            
        } else if (mysql_errno(mysql_con) == ER_FILE_EXIST) {
            log_mensaje(NivelLog::Bajo, "ERROR", "BASE DE DATOS", mysql_error(mysql_con));
            return false;
            
        } else {
            g_error(" %d %s \n%s", mysql_errno(mysql_con) , tsql, mysql_error(mysql_con));
            return false;
	    }
    }
    return false;
}

bool 
ejecutar_sql(MYSQL * mysql_con, boost::format const& sqlQuery, bool debug)
{
	std::ostringstream sqlStream;
	sqlStream << sqlQuery;
	return ejecutar_sql(mysql_con, sqlStream.str().c_str(), debug);
}

void
limite_tipomonto(n_t_tipomonto_t * n_t_tipomonto, boost::uint8_t tipo)
{
	boost::int32_t nuevo_limite;
	unsigned int proporcion;

	proporcion = tipo == TERMINAL ? 1000 : 10000;

	nuevo_limite =
	    (((n_t_tipomonto->venta_global * n_t_tipomonto->proporcion) / proporcion +
	        n_t_tipomonto->valoradicional) / n_t_tipomonto->incremento) *
	    n_t_tipomonto->incremento;

	n_t_tipomonto->limite_actual =
	    (nuevo_limite > n_t_tipomonto->valorinicial ? nuevo_limite : n_t_tipomonto->valorinicial);

	if (n_t_tipomonto->limite_actual > n_t_tipomonto->valorfinal)
		n_t_tipomonto->limite_actual = n_t_tipomonto->valorfinal;
}

char *
str_mes(int mes)
{
	char * str = new char[4];

	if (str == NULL) {
		exit(1);
	}

	switch (mes) {
		case 0: {
				strcpy(str, "Ene");
				break;
			}
		case 1: {
				strcpy(str, "Feb");
				break;
			}
		case 2: {
				strcpy(str, "Mar");
				break;
			}
		case 3: {
				strcpy(str, "Abr");
				break;
			}
		case 4: {
				strcpy(str, "May");
				break;
			}
		case 5: {
				strcpy(str, "Jun");
				break;
			}
		case 6: {
				strcpy(str, "Jul");
				break;
			}
		case 7: {
				strcpy(str, "Ago");
				break;
			}
		case 8: {
				strcpy(str, "Sep");
				break;
			}
		case 9: {
				strcpy(str, "Oct");
				break;
			}
		case 10: {
				strcpy(str, "Nov");
				break;
			}
		case 11: {
				strcpy(str, "Dic");
				break;
			}
	}
	return str;
}

void
log_mensaje(boost::uint8_t nivel, const char * tipo, const char * origen, const char * mensaje)
{

	time_t tim;
	struct tm t;
	char linea_log[1024];

	if (nivel > nivelLogMensajes)
		return ;

	tim = time(NULL);
	if (tim == -1)
		return ;
	t = *localtime(&tim);

	strcpy(linea_log, "[");
	switch (t.tm_wday) {
		case 0: {
				strcat(linea_log, "Domingo");
				break;
			}
		case 1: {
				strcat(linea_log, "Lunes");
				break;
			}
		case 2: {
				strcat(linea_log, "Martes");
				break;
			}
		case 3: {
				strcat(linea_log, "Miercoles");
				break;
			}
		case 4: {
				strcat(linea_log, "Jueves");
				break;
			}
		case 5: {
				strcat(linea_log, "Viernes");
				break;
			}
		case 6: {
				strcat(linea_log, "Sabado");
				break;
			}
	}
	sprintf(linea_log, "%s %02d ", linea_log, t.tm_mday);

	strcat(linea_log, str_mes(t.tm_mon));

	sprintf(linea_log, "%s %02d:%02d:%02d %d] [%s] [%s] ", linea_log, t.tm_hour, t.tm_min,
	         t.tm_sec, t.tm_year + 1900, tipo, origen);
    
    std::ostringstream oss;
    oss << linea_log << mensaje << "\n";
    
    std::string logMensaje = oss.str(); 
    
	sem_wait(&mutex_t_fd_log_mensajes);
	write(fd_log_mensajes, logMensaje.c_str(), logMensaje.length());
	sem_post(&mutex_t_fd_log_mensajes);
}

void
log_acceso(boost::uint8_t nivel, boost::uint8_t NIVEL_DE_LOG, ConexionActiva * conexionActiva,
            const char * mensaje, sem_t * mutex_t_fd_log, int fd_log)
{
	time_t tim;
	struct tm t;
	char linea_log[1024];

	if (nivel > NIVEL_DE_LOG)
		return ;

	tim = time(NULL);
	if (tim == -1)
		return ;
	t = *localtime(&tim);

	if (conexionActiva != NULL) {

		if (conexionActiva->idAgente != 0)
			sprintf(linea_log, "%u", conexionActiva->idAgente);
		else
			sprintf(linea_log, "-");

		if (conexionActiva->idTaquilla != 0)
			sprintf(linea_log, "%s %u", linea_log, conexionActiva->idTaquilla);
		else
			sprintf(linea_log, "%s -", linea_log);

		if (conexionActiva->idUsuario != 0)
			sprintf(linea_log, "%s %u", linea_log, conexionActiva->idUsuario);
		else
			sprintf(linea_log, "%s -", linea_log);

	} else {
		sprintf(linea_log, "- - -");
	}

	sprintf(linea_log, "%s [%02d-%s-%d:%02d:%02d:%02d] \"%s\"\n", linea_log, t.tm_mday,
	         str_mes(t.tm_mon), t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec, mensaje);

	sem_wait(mutex_t_fd_log);
	write(fd_log, linea_log, strlen(linea_log));
	sem_post(mutex_t_fd_log);
}

void
cerrar_archivos_de_registro()
{
	sem_wait(&mutex_t_fd_log_clientes);
	close(fd_log_clientes);
	sem_destroy(&mutex_t_fd_log_clientes);

	sem_wait(&mutex_t_fd_log_admin);
	close(fd_log_admin);
	sem_destroy(&mutex_t_fd_log_admin);

	sem_wait(&mutex_t_fd_log_mensajes);
	close(fd_log_mensajes);
	sem_destroy(&mutex_t_fd_log_mensajes);

}

void
enviarPeticionNoLogeado(ConexionActiva * conexionActiva, boost::int8_t peticion)
{
	char mensaje[BUFFER_SIZE];
	sprintf(mensaje, "Solicitud <%d> sin estar logeado", peticion);
	log_mensaje(NivelLog::Bajo, "advertencia", conexionActiva->ipAddress.c_str(), mensaje);
	send2cliente(*conexionActiva, ERR_OPERNOPERMIT, NULL, 0);
	send2cliente(*conexionActiva, DESCONECCION, NULL, 0);
}

void
tarea_no_permitida(ConexionActiva * conexionActiva, boost::int8_t peticion)
{
	char mensaje[BUFFER_SIZE];

	sprintf(mensaje, "Tarea <%d> no permitida para el usuario", peticion);

	log_admin(NivelLog::Bajo, conexionActiva, mensaje);
	send2cliente(*conexionActiva, ERR_OPERNOPERMIT, NULL, 0);
}

void
enviar_mensaje(mensajeria_t mensajeria, MYSQL * mysql_con, ConexionActiva& conexionActiva)
{
	boost::uint16_t idGrupo = 0;
	bool destinoEsGrupo = false;
	{
		std::vector<char> usuario(strlen(mensajeria.login) * 2 + 1, '\0');
		mysql_real_escape_string(mysql_con, &usuario[0], mensajeria.login, strlen(mensajeria.login));
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Id FROM Grupos WHERE Nombre='%1%'") % &usuario[0];
		ejecutar_sql(mysql_con, sqlQuery, DEBUG_ENVIAR_MENSAJE);
		MYSQL_RES * result = mysql_store_result(mysql_con);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			idGrupo = atoi(row[0]);
			destinoEsGrupo = true;
		}
		mysql_free_result(result);
	}

	std::vector<std::pair<pthread_t, boost::uint16_t> > hilosDestino;
	{
		boost::format sqlQuery;
		if (destinoEsGrupo) {
			sqlQuery.parse("SELECT U.Hilo, U.Id FROM Usuarios AS U, Usuarios_Grupos_R AS UG "
			                "WHERE U.Id = UG.IdUsuario AND UG.idGrupo=%1%") % idGrupo;

		} else {
			std::vector<char> usuario(strlen(mensajeria.login) * 2 + 1, '\0');
			mysql_real_escape_string(mysql_con, &usuario[0], mensajeria.login, strlen(mensajeria.login));
			sqlQuery.parse("SELECT Hilo, Id FROM Usuarios WHERE Login='%1%'") % &usuario[0];
		}

		ejecutar_sql(mysql_con, sqlQuery, DEBUG_ENVIAR_MENSAJE);
		MYSQL_RES * result = mysql_store_result(mysql_con);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			pthread_t hilo = (NULL != row[0]) ? pthread_t(strtoul(row[0], NULL, 10)) : 0;
			hilosDestino.push_back(std::make_pair(hilo, atoi(row[1])));
		}
		mysql_free_result(result);
	}

	strncpy(mensajeria.login, conexionActiva.login, LOGIN_LENGTH - 1);

	boost::int32_t lon = MENSAJERIA_LON + mensajeria.longitud;
	mensajeria.fechahora = time(NULL);
	boost::uint8_t * buffer =  mensajeria_t2b(mensajeria);

	for (std::size_t i = 0 , size = hilosDestino.size(); i < size ; ++i) {
        
        SocketThread::MutexGuard g;
		SocketThread * socketsThread = SocketThread::get(hilosDestino[i].first);

		if (NULL != socketsThread) {
			ConexionActiva& conexion_usuario = socketsThread->getConexionTaquilla();
			if (conexion_usuario.idUsuario == hilosDestino[i].second) {
				if (mensajeria.tipo == MSJURGENTE) {
					send2cliente(conexion_usuario, MENSAJERIA, buffer, lon);
				} else {
					send2cliente(conexion_usuario, MENSAJERIA, NULL, 0);
				}
				break;
			}
		}
		{
			std::vector<char> mensaje(strlen(mensajeria.mensaje) * 2 + 1, '\0');
			mysql_real_escape_string(mysql_con, 
			                          &mensaje[0], 
			                          mensajeria.mensaje, 
			                          strlen(mensajeria.mensaje));
			boost::format sqlQuery;
			sqlQuery.parse("INSERT INTO Mensajeria VALUES(NULL, %1%, %2%, FROM_UNIXTIME(%3%), '%4%', %5%, %6%)")
			% conexionActiva.idUsuario 
			% hilosDestino[i].second 
			% time(NULL) 
			% &mensaje[0]
			% boost::int32_t(false) 
			% unsigned(mensajeria.tipo);

			ejecutar_sql(mysql_con, "BEGIN", DEBUG_ENVIAR_MENSAJE);
			ejecutar_sql(mysql_con, sqlQuery, DEBUG_ENVIAR_MENSAJE);
			ejecutar_sql(mysql_con, "COMMIT", DEBUG_ENVIAR_MENSAJE);
		}		
	}
	delete [] buffer;
}
