#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <glib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <mysql/mysql.h>
#include <zlib.h>
#include <sstream>
#include <md5.hh>
#include <boost/format.hpp>

#include <global.h>
#include <ventas.h>
#include <utils.h>
#include <sockets.h>
#include <database/DataBaseConnetionPool.h>
#include <SocketThread.h>
#include <TipoDeMontoDeAgencia.h>

#include <ProtocoloZeus/Mensajes.h>
#include <ProtocoloZeus/Protocolo.h>

using namespace ProtocoloZeus;

#define __DEBUG_B__
#define DEBUG_LOGIN_SOL 0
#define DEBUG_SORTEOS_SOL 0
#define DEBUG_SORTEOS_SOL_BA 0
#define DEBUG_SORTEOS_SOL_AL 0
#define DEBUG_DATOS_BASICOS_SOL 0
#define DEBUG_CARGAR_TIPOMONTO_X_SORTEO 0
#define DEBUG_ATENDER_PETICION 0
#define DEBUG_CAMBIAR_PASSWORD 0
#define DEBUG_CONSULTAR_MENSAJES 0
#define DEBUG_TODOS_SORTEOS 0
#define DEBUG_TODOS_LIMITES 0

/*--------------------------------------------------------------------------*/


void
mensajes(ConexionActiva& conexionActiva, int cual)
{
	char mensaje[BUFFER_SIZE];

	switch (cual) {
		case ERR_TICKETINCONSS: {
				sprintf(mensaje,
				         "ADVERTENCIA: login - la taquilla <%u> del agente <%u> "
				         "tiene problemas en la base de datos", conexionActiva.idTaquilla,
				         conexionActiva.idAgente);
			}
			break;

		case ERR_TICKETINCONSA: {
				sprintf(mensaje,
				         "ADVERTENCIA: login - la taquilla <%u> del agente <%u> "
				         "posee mas tickets que el servidor", conexionActiva.idTaquilla,
				         conexionActiva.idAgente);
			}
	}
	log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
	send2cliente(conexionActiva, cual, 0, 0);
}

bool
atenderLoginV0(ConexionActiva& conexionActiva,SolicitudLoginV0 login,
          RespuestaLoginV0* respuestaLogin, boost::uint8_t* idZona,
          bool* usarProductos)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	log_clientes(NivelLog::Bajo, &conexionActiva, "Solicitud de Login");

	boost::uint32_t llave = crc32(crc32(0L, Z_NULL, 0), (boost::uint8_t *) autentificacion,
	                          strlen(autentificacion));
	if (llave != login.autentificacion) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		        "ADVERTENCIA: La taquilla <%u> del Agente <%u> "
		        "Falla en la autentificacion de login <%u>/<%u>", 
                 conexionActiva.idTaquilla, login.idAgente, 
                 llave, login.autentificacion);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_AUTENTIFICACION, 0, 0);
		return false;
	}

	char password[33];
	genera_password(login.password, password);
	if (login.fechaHora - time(NULL) > 60) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		         "ADVERTENCIA: login - la taquilla <%u> del agente <%u> "
		         "tiene la hora adelantada por mas de %0.2f minutos.", conexionActiva.idTaquilla,
		         login.idAgente, (double) (login.fechaHora - time(NULL)) / 60);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
	} else if ((time(NULL) - login.fechaHora) > 60) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		         "ADVERTENCIA: login - la taquilla <%u> del agente <%u> "
		         "tiene la hora atrazada por mas de %0.2f minutos.", conexionActiva.idTaquilla,
		         login.idAgente, (double) (time(NULL) - login.fechaHora) / 60);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
	}

	// obtiene el estado de la Agencia y su zona
	bool agenteActivo = false;
	bool agenteExiste = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Estado, IdZona, UsarProductos FROM Agentes WHERE Id=%1%") 
        % login.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
		MYSQL_RES *result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			agenteExiste = true;
			agenteActivo = (atoi(row[0]) == 1);
			*idZona = atoi(row[1]);
            *usarProductos = (atoi(row[2]) != 0);
		}
		mysql_free_result(result);
	}

	if (agenteExiste) {
		if (not agenteActivo) {
			char mensaje[BUFFER_SIZE];
			sprintf(mensaje,
			         "ADVERTENCIA: login - agente <%u> desactivado por razones administrativas.",
			         login.idAgente);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
			return false;
		}
	} else {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: login - el agente <%u> no existe.", login.idAgente);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
		return false;
	}

	// login del usuario
	SolicitudLoginV0 testLogin = login;
	boost::uint8_t estado = 0;
	boost::uint32_t idUsuario = 0;
	boost::uint8_t prioridad = 0;
	do {
		// obtiene estado del usuario verificando que no está asignado a un agenta banca
		bool usuarioActivo = false;
		bool usuarioExiste = false;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT Usuarios.Estado, Usuarios.Id, Prioridad FROM Usuarios, Agentes "
			                "WHERE Login='%1%' AND Password='%2%' AND IdAgente=%3% "
			                "AND Agentes.Id=IdAgente AND Agentes.IdRecogedor IS NOT NULL")
			% testLogin.login % password % testLogin.idAgente;
			ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
			MYSQL_RES * result = mysql_store_result(mysql);
			MYSQL_ROW row = mysql_fetch_row(result);
			if (NULL != row) {
				usuarioExiste = true;
				usuarioActivo = (atoi(row[0]) == 0) ? false : true;
				estado = atoi(row[0]);
				idUsuario = atoi(row[1]);
				prioridad = atoi(row[2]);
			}
			mysql_free_result(result);
		}

		if (usuarioExiste) {
			if (not usuarioActivo) {
				char mensaje[BUFFER_SIZE];
				sprintf(mensaje,
				         "ADVERTENCIA: login - el usuario con login <%s>@<%u> no esta activo.",
				         testLogin.login, testLogin.idAgente);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				send2cliente(conexionActiva, ERR_USUARIONOACT, 0, 0);
				return false;
			}
			break;
		} else {
			// obtiene el id del recogedor del agente
			bool agenteExiste = false;
			{
				boost::format sqlQuery;
				sqlQuery.parse("SELECT IdRecogedor FROM Agentes "
				                "WHERE IdRecogedor IS NOT NULL AND Id = %1%")
				% testLogin.idAgente;
				ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
				MYSQL_RES * result = mysql_store_result(mysql);
				MYSQL_ROW row = mysql_fetch_row(result);
				if (NULL != row) {
					agenteExiste = true;
					testLogin.idAgente = atoi(row[0]);
				}
				mysql_free_result(result);
			}
			if (not agenteExiste) {
				char mensaje[BUFFER_SIZE];
				sprintf(mensaje,
				         "ADVERTENCIA: login - login <%s>@<%u>  no existe o clave incorrecta.",
				         testLogin.login, login.idAgente);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				send2cliente(conexionActiva, ERR_LOGINPASSWORD, 0, 0);
				return false;
			}
		}
	} while (true);

	// Reconocimiento de inconsistencia de Tickets

	if (login.ultimoTicket == 0) {

		// obtiene el numero de los tickets no tachados de la taquilla
		unsigned cantidadTickets = 0;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT COUNT(Id) FROM Tickets WHERE IdAgente=%1% "
			                "AND NumTaquilla=%2% AND Estado <> %3%")
			% login.idAgente % unsigned(conexionActiva.idTaquilla) % unsigned(TACHADO);
			ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
			MYSQL_RES * result = mysql_store_result(mysql);
			MYSQL_ROW row = mysql_fetch_row(result);
			if (NULL != row) {
				cantidadTickets = atoi(row[0]);
			}
			mysql_free_result(result);
		}

		if (cantidadTickets > 1) {
				conexionActiva.idAgente = login.idAgente;
				mensajes(conexionActiva, ERR_TICKETINCONSS);
				conexionActiva.idAgente = 0;
				return false;
		}
	} else {
		// obtiene los 2 Tickets mas recientes para la Taquilla dada que no esten
		// tachados y su id sea igual o mayor que el id dado
		std::vector<boost::uint32_t> idTickets;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT Id FROM Tickets WHERE IdAgente=%1% "
			                "AND NumTaquilla=%2% AND Estado <> %3% AND Id >=%4% ORDER BY Id DESC LIMIT 2")
			% login.idAgente % unsigned(conexionActiva.idTaquilla) % unsigned(TACHADO)
			% login.ultimoTicket;
			ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
			MYSQL_RES * result = mysql_store_result(mysql);
			for (MYSQL_ROW row = mysql_fetch_row(result);
			        NULL != row;
			        row = mysql_fetch_row(result)) {
				idTickets.push_back(atoi(row[0]));
			}
			mysql_free_result(result);
		}

		//FIXME: esto deberia ser un query a la base de datos
		bool ticketEncontrado = false;
		for (std::size_t i = 0, size = idTickets.size(); i < size ; ++i) {
			if (login.ultimoTicket == idTickets[i]) {
				ticketEncontrado = true;
				break;
			}
		}

		if (not ticketEncontrado) {
			conexionActiva.idAgente = login.idAgente;
			mensajes(conexionActiva, ERR_TICKETINCONSS);
			conexionActiva.idAgente = 0;
			return false;
		}
	}

	// validar el serial del disco duro

	// obtiene el serial de la taquilla
	boost::uint32_t serial = 0;
	bool taquillaExiste = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Serial FROM Taquillas WHERE Numero=%1% AND IdAgente=%2%")
		% unsigned(conexionActiva.idTaquilla) % login.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			taquillaExiste = true;
			serial = strtoul(row[0], NULL, 10);
		}
		mysql_free_result(result);
	}

	if (not taquillaExiste) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: No existe la taquilla <%u> agencia <%u>.",
		         conexionActiva.idTaquilla, login.idAgente);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_TAQUILLANOEXIS, 0, 0);
		return false;
	}

	if (serial == 0) {
		// actualizar el serial
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Taquillas SET Serial=%1% WHERE Numero=%2% AND IdAgente=%3%")
		% login.serialDD % unsigned(conexionActiva.idTaquilla) % login.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
	} else {
		if (serial != login.serialDD) {
			char mensaje[BUFFER_SIZE];
			sprintf(mensaje, "ADVERTENCIA: login - serial de disco duro incorrecto.");
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_SERIALDDINCO, 0, 0);
			return false;
		}
	}

	pthread_t hilo = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Hilo FROM Taquillas "
		                "WHERE IdAgente=%1% AND Numero=%2% AND Hilo IS NOT NULL")
		% login.idAgente % unsigned(conexionActiva.idTaquilla);
		ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			hilo = (pthread_t) strtoul(row[0], NULL, 10);
		}
		mysql_free_result(result);
	}
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Taquillas SET Hilo=%1%, Hora_Ult_Conexion=FROM_UNIXTIME(%2%) "
                       "WHERE IdAgente=%3% AND Numero=%4%")
		% (boost::uint32_t) pthread_self() % time(NULL) % login.idAgente % unsigned(conexionActiva.idTaquilla);
		ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
	}
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Usuarios SET Hilo=%1% WHERE Id=%2%")
		% unsigned(pthread_self())
		% idUsuario;
		ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
	}
	if (hilo != 0) {
		SocketThread::MutexGuard g;
        SocketThread * socketsThread = SocketThread::get(hilo);
		if (socketsThread != 0) {
			ConexionActiva& conexion_activa2 = socketsThread->getConexionTaquilla();
			if (conexion_activa2.idAgente == login.idAgente
			        and conexion_activa2.idTaquilla == conexionActiva.idTaquilla
			        and conexion_activa2.sd != conexionActiva.sd) {
				shutdown(conexion_activa2.sd, SHUT_RDWR);
			}
		}
	}

	respuestaLogin->estado = estado;
	respuestaLogin->prioridad = prioridad;
	respuestaLogin->idUsuario = idUsuario;
	respuestaLogin->fechaHora = time(NULL) - 1800;
    
	std::vector<boost::uint8_t> buff = respuestaLogin->toRawBuffer();
	send2cliente(conexionActiva, LOGIN_V_0, &buff[0], buff.size());
	return true;
}

bool
atenderLoginV1(ConexionActiva& conexionActiva, SolicitudLoginV0 login,
          RespuestaLoginV1* respuestaLogin, boost::uint8_t* idZona,
          bool* usarProductos)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Bajo, &conexionActiva, "Solicitud de Login");

    boost::uint32_t llave = crc32(crc32(0L, Z_NULL, 0), (boost::uint8_t *) autentificacion,
                              strlen(autentificacion));
    if (llave != login.autentificacion) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: La taquilla <%u> del Agente <%u> "
                 "Falla en la autentificacion de login <%u>/<%u>", conexionActiva.idTaquilla,
                 login.idAgente, llave, login.autentificacion);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AUTENTIFICACION, 0, 0);
        return false;
    }
    char password[33];
    genera_password(login.password, password);
    if (login.fechaHora + 1800 - time(NULL) > 60) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: login - la taquilla <%u> del agente <%u> "
                 "tiene la hora adelantada por mas de %0.2f minutos.", conexionActiva.idTaquilla,
                 login.idAgente, (double) (login.fechaHora - time(NULL)) / 60);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    } else if ((time(NULL) - login.fechaHora + 1800) > 60) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: login - la taquilla <%u> del agente <%u> "
                 "tiene la hora atrazada por mas de %0.2f minutos.", conexionActiva.idTaquilla,
                 login.idAgente, (double) (time(NULL) - login.fechaHora) / 60);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    }

    // obtiene el estado de la Agencia y su zona
    bool agenteActivo = false;
    bool agenteExiste = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Estado, IdZona, UsarProductos FROM Agentes WHERE Id=%1%") 
        % login.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            agenteExiste = true;
            agenteActivo = (atoi(row[0]) == 1);
            *idZona = atoi(row[1]);
            *usarProductos = (atoi(row[2]) != 0);
        }
        mysql_free_result(result);
    }

    if (agenteExiste) {
        if (not agenteActivo) {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje,
                     "ADVERTENCIA: login - agente <%u> desactivado por razones administrativas.",
                     login.idAgente);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
            return false;
        }
    } else {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: login - el agente <%u> no existe.", login.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
        return false;
    }

    // login del usuario
    SolicitudLoginV0 testLogin = login;
    boost::uint8_t estado = 0;
    boost::uint32_t idUsuario = 0;
    boost::uint8_t prioridad = 0;
    do {
        // obtiene estado del usuario verificando que no está asignado a un agenta banca
        bool usuarioActivo = false;
        bool usuarioExiste = false;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Usuarios.Estado, Usuarios.Id, Prioridad FROM Usuarios, Agentes "
                            "WHERE Login='%1%' AND Password='%2%' AND IdAgente=%3% "
                            "AND Agentes.Id=IdAgente AND Agentes.IdRecogedor IS NOT NULL")
            % testLogin.login % password % testLogin.idAgente;
            ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
            MYSQL_RES * result = mysql_store_result(mysql);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (NULL != row) {
                usuarioExiste = true;
                usuarioActivo = (atoi(row[0]) == 0) ? false : true;
                estado = atoi(row[0]);
                idUsuario = atoi(row[1]);
                prioridad = atoi(row[2]);
            }
            mysql_free_result(result);
        }

        if (usuarioExiste) {
            if (not usuarioActivo) {
                char mensaje[BUFFER_SIZE];
                sprintf(mensaje,
                         "ADVERTENCIA: login - el usuario con login <%s>@<%u> no esta activo.",
                         testLogin.login, testLogin.idAgente);
                log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
                send2cliente(conexionActiva, ERR_USUARIONOACT, 0, 0);
                return false;
            }
            break;
        } else {
            // obtiene el id del recogedor del agente
            bool agenteExiste = false;
            {
                boost::format sqlQuery;
                sqlQuery.parse("SELECT IdRecogedor FROM Agentes "
                                "WHERE IdRecogedor IS NOT NULL AND Id = %1%")
                % testLogin.idAgente;
                ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
                MYSQL_RES * result = mysql_store_result(mysql);
                MYSQL_ROW row = mysql_fetch_row(result);
                if (NULL != row) {
                    agenteExiste = true;
                    testLogin.idAgente = atoi(row[0]);
                }
                mysql_free_result(result);
            }
            if (not agenteExiste) {
                char mensaje[BUFFER_SIZE];
                sprintf(mensaje,
                         "ADVERTENCIA: login - login <%s>@<%u>  no existe o clave incorrecta.",
                         testLogin.login, login.idAgente);
                log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
                send2cliente(conexionActiva, ERR_LOGINPASSWORD, 0, 0);
                return false;
            }
        }
    } while (true);

    // Reconocimiento de inconsistencia de Tickets

    if (login.ultimoTicket == 0) {

        // obtiene el numero de los tickets no tachados de la taquilla
        unsigned cantidadTickets = 0;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT COUNT(Id) FROM Tickets WHERE IdAgente=%1% "
                            "AND NumTaquilla=%2% AND Estado <> %3%")
            % login.idAgente % unsigned(conexionActiva.idTaquilla) % unsigned(TACHADO);
            ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
            MYSQL_RES * result = mysql_store_result(mysql);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (NULL != row) {
                cantidadTickets = atoi(row[0]);
            }
            mysql_free_result(result);
        }

        if (cantidadTickets > 1) {
                conexionActiva.idAgente = login.idAgente;
                mensajes(conexionActiva, ERR_TICKETINCONSS);
                conexionActiva.idAgente = 0;
                return false;
        }
    } else {
        // obtiene los 2 Tickets mas recientes para la Taquilla dada que no esten
        // tachados y su id sea igual o mayor que el id dado
        std::vector<boost::uint32_t> idTickets;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Id FROM Tickets WHERE IdAgente=%1% "
                            "AND NumTaquilla=%2% AND Estado <> %3% AND Id >=%4% ORDER BY Id DESC LIMIT 2")
            % login.idAgente % unsigned(conexionActiva.idTaquilla) % unsigned(TACHADO)
            % login.ultimoTicket;
            ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
            MYSQL_RES * result = mysql_store_result(mysql);
            for (MYSQL_ROW row = mysql_fetch_row(result);
                    NULL != row;
                    row = mysql_fetch_row(result)) {
                idTickets.push_back(atoi(row[0]));
            }
            mysql_free_result(result);
        }

        //FIXME: esto deberia ser un query a la base de datos
        bool ticketEncontrado = false;
        for (std::size_t i = 0, size = idTickets.size(); i < size ; ++i) {
            if (login.ultimoTicket == idTickets[i]) {
                ticketEncontrado = true;
                break;
            }
        }

        if (not ticketEncontrado) {
            conexionActiva.idAgente = login.idAgente;
            mensajes(conexionActiva, ERR_TICKETINCONSS);
            conexionActiva.idAgente = 0;
            return false;
        }
    }

    // validar el serial del disco duro

    // obtiene el serial de la taquilla
    boost::uint32_t serial = 0;
    bool taquillaExiste = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Serial FROM Taquillas WHERE Numero=%1% AND IdAgente=%2%")
        % unsigned(conexionActiva.idTaquilla) % login.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            taquillaExiste = true;
            serial = strtoul(row[0], NULL, 10);
        }
        mysql_free_result(result);
    }

    if (not taquillaExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: No existe la taquilla <%u> agencia <%u>.",
                 conexionActiva.idTaquilla, login.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TAQUILLANOEXIS, 0, 0);
        return false;
    }

    if (serial == 0) {
        // actualizar el serial
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Taquillas SET Serial=%1% WHERE Numero=%2% AND IdAgente=%3%")
        % login.serialDD % unsigned(conexionActiva.idTaquilla) % login.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    } else {
        if (serial != login.serialDD) {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ADVERTENCIA: login - serial de disco duro incorrecto.");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ERR_SERIALDDINCO, 0, 0);
            return false;
        }
    }

    pthread_t hilo = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Hilo FROM Taquillas "
                        "WHERE IdAgente=%1% AND Numero=%2% AND Hilo IS NOT NULL")
        % login.idAgente % unsigned(conexionActiva.idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            hilo = (pthread_t) strtoul(row[0], NULL, 10);
        }
        mysql_free_result(result);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Taquillas SET Hilo=%1%, Hora_Ult_Conexion=FROM_UNIXTIME(%2%) "
                       "WHERE IdAgente=%3% AND Numero=%4%")
        % (boost::uint32_t) pthread_self() % time(NULL) % login.idAgente % unsigned(conexionActiva.idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Usuarios SET Hilo=%1% WHERE Id=%2%")
        % unsigned(pthread_self())
        % idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    }
    if (hilo != 0) {
        SocketThread::MutexGuard g;
        SocketThread * socketsThread = SocketThread::get(hilo);
        if (socketsThread != 0) {
            ConexionActiva& conexion_activa2 = socketsThread->getConexionTaquilla();
            if (conexion_activa2.idAgente == login.idAgente
                && conexion_activa2.idTaquilla == conexionActiva.idTaquilla
                && conexion_activa2.sd != conexionActiva.sd) {
                shutdown(conexion_activa2.sd, SHUT_RDWR);
            }
        }
    }

    respuestaLogin->estado = estado;
    respuestaLogin->prioridad = prioridad;
    respuestaLogin->idUsuario = idUsuario;
    respuestaLogin->fechaHora = time(NULL) - 1800;
    
    respuestaLogin->impuesto_ganancia_fortuita = configuracion.impuesto_ganancia_fortuita;
    respuestaLogin->impuesto_ventas = 0;
    respuestaLogin->usar_productos = *usarProductos;
    
    std::vector<boost::uint8_t> buff = respuestaLogin->toRawBuffer();
    send2cliente(conexionActiva, LOGIN_V_1, &buff[0], buff.size());    
    
    return true;
}

void
atenderPeticionTodosLimites(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Todos los Limites");    
    
    boost::format sqlQuery;
    sqlQuery.parse("SELECT IdSorteo, LimDiscTerm, LimDiscTrip, PorcTerm, PorcTrip "
                   "FROM Sorteos, LimitesPorc WHERE Sorteos.Estado=1 AND "
                   "Sorteos.Id=LimitesPorc.IdSorteo AND IdAgente=%1%")
    % conexionActiva.idAgente;   
     
    ejecutar_sql(mysql, sqlQuery, DEBUG_TODOS_LIMITES);
    MYSQL_RES * result = mysql_store_result(mysql);
    todos_limites_t respuesta;
    for (MYSQL_ROW row = mysql_fetch_row(result); row != NULL; row = mysql_fetch_row(result)) {
        limites_t limites;        
        limites.idSorteo = atoi(row[0]);
        limites.lim_ter  = atoi(row[1]);
        limites.lim_tri  = atoi(row[2]);
        limites.porc_ter = atoi(row[3]);
        limites.porc_tri = atoi(row[4]);
        respuesta.limites.push_back(limites);        
    }
    mysql_free_result(result);
    
    respuesta.numeroLimites = respuesta.limites.size();
    
    boost::uint8_t * buffer = todos_limites_t2b(respuesta);
    std::size_t bufferSize = TODOS_LIMITES_LON + (LIMITES_LON * respuesta.numeroLimites);   
    send2cliente(conexionActiva, TODOS_LIMITES, buffer, bufferSize);   
    delete [] buffer;
}

/**************************************************************************/

void
atenderPeticionLimites(ConexionActiva& conexionActiva, limites_sol_t limitesSol)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de limites");

	bool sorteoExiste = false;
	limites_t limites;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Id FROM Sorteos WHERE Id=%1% AND Estado=1")
		% unsigned(limitesSol.idSorteo);
		ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_BA);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			sorteoExiste = true;
			limites.idSorteo = atoi(row[0]);
		}
		mysql_free_result(result);
	}
	if (not sorteoExiste) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: no existe el sorteo <%u>.", limitesSol.idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
		return ;
	}

	bool limitesExisten = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT LimDiscTrip, LimDiscTerm, PorcTrip, PorcTerm "
		                "FROM LimitesPorc WHERE IdSorteo=%1% AND IdAgente=%2%")
		% unsigned(limitesSol.idSorteo) % conexionActiva.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_BA);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			limitesExisten = true;
			limites.lim_tri = atoi(row[0]);
			limites.lim_ter = atoi(row[1]);
			limites.porc_tri = atoi(row[2]);
			limites.porc_ter = atoi(row[3]);
		}
		mysql_free_result(result);
	}
	if (not limitesExisten) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: no existen los limites para el sorteo <%u>.",
		         limitesSol.idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
		return ;
	}

	boost::uint8_t * buffer = limites_t2b(limites);
	send2cliente(conexionActiva, LIMITES, buffer, LIMITES_LON);
	delete [] buffer;

	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "MENSAJE: solicitud de limite <%u> resuelta con exito",
		         limitesSol.idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
	}
}

/**************************************************************************/

void
atenderPeticionSorteo_V_0(ConexionActiva& conexionActiva, sorteo_sol_t sorteoSol)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Sorteo");

	bool sorteoExiste = false;
	sorteo_V_0_t sorteo;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Id, Nombre, Dia, HoraCierre+0, TipoJugada FROM Sorteos "
		                "WHERE Id=%1% AND Estado=1")
		% unsigned(sorteoSol.idSorteo);
		ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			sorteoExiste = true;
			sorteo.idSorteo = atoi(row[0]);
			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(sorteo.nombre_loteria, ' ', NOMBRE_LOTERIA_LON);
			strcpy(sorteo.nombre_loteria, row[1]);
			sorteo.nombre_loteria[strlen(row[1])] = ' ';
			sorteo.dias_de_juego = atoi(row[2]);
			sorteo.hora_cierre = atoi(row[3]) / 100;
			sorteo.tipojugada = atoi(row[4]);
		}
		mysql_free_result(result);
	}

	if (not sorteoExiste) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: no existe el sorteo <%u>.", sorteoSol.idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
		return ;
	}
	bool limitesExisten = false;
	boost::uint32_t idMontoTriple = 0, idMontoTerminal = 0;
	sorteo.forma_pago = 0;
	sorteo.comodin = 0;
	sorteo.monto_adic = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT IdMontoTriple, IdMontoTerminal, IdFormaPago, Comodin, MontoAdd "
		                "FROM LimitesPorc WHERE IdSorteo=%1% AND IdAgente=%2%")
		% unsigned(sorteoSol.idSorteo) % conexionActiva.idAgente ;
		ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			limitesExisten = true;
			idMontoTriple = atoi(row[0]);
			idMontoTerminal = atoi(row[1]);
			sorteo.forma_pago = atoi(row[2]);
			sorteo.comodin = atoi(row[3]);
			sorteo.monto_adic = atoi(row[4]);
		}
		mysql_free_result(result);
	}
	if (not limitesExisten) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: no existen los limites para el sorteo <%u>.",
		         sorteoSol.idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
		return ;
	}
	bool montoTripleExiste = false;
	sorteo.monto_triple = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Monto FROM TiposMontoTriple WHERE ID =%1%") % idMontoTriple;
		ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			montoTripleExiste = true;
			sorteo.monto_triple = atoi(row[0]);
		}
		mysql_free_result(result);
	}
	if (not montoTripleExiste) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		         "ADVERTENCIA: no existe el tipo de monto <%u> para el triple del sorteo <%u>.",
		         idMontoTriple, sorteoSol.idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_TIPOMONTOTRINOEXIS, 0, 0);
		return ;
	}

	bool montosTerminalExisten = false;
	sorteo.monto_term1 = 0;
	sorteo.monto_term2 = 0;
	sorteo.monto_term3 = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Monto1ro, Monto2do, Monto3ro FROM TiposMontoTerminal WHERE ID =%1%")
		% idMontoTerminal;
		ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			montosTerminalExisten = true;
			sorteo.monto_term1 = atoi(row[0]);
			sorteo.monto_term2 = atoi(row[1]);
			sorteo.monto_term3 = atoi(row[2]);
		}
		mysql_free_result(result);
	}
	if (not montosTerminalExisten) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		         "ADVERTENCIA: no existe el tipo de monto <%u> para el terminal del sorteo <%u>.",
		         idMontoTerminal, sorteoSol.idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_TIPOMONTOTERNOEXIS, 0, 0);
		return ;
	}

	boost::uint8_t * buffer = sorteo_V_0_t2b(sorteo);
	send2cliente(conexionActiva, SORTEO_V_0, buffer, SORTEO_V_0_LON);
	delete [] buffer;

	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "MENSAJE: solicitud de sorteo <%u> resuelta con exito",
		         sorteoSol.idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
	}
}

/**************************************************************************/
void
atenderPeticionTodosSorteos_V_0(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Todos los Sorteos");    
    
    boost::format sqlQuery;
    sqlQuery.parse("SELECT Sorteos.Dia, Sorteos.Id, Sorteos.Nombre, Sorteos.HoraCierre+0, " 
                   "TiposMontoTriple.Monto, TiposMontoTerminal.Monto1ro, TiposMontoTerminal.Monto2do, "
                   "TiposMontoTerminal.Monto3ro, LimitesPorc.IdFormaPago, LimitesPorc.Comodin, "
                   "LimitesPorc.MontoAdd, Sorteos.TipoJugada, Sorteos.IdProducto "
                   "FROM Sorteos, LimitesPorc, TiposMontoTriple, TiposMontoTerminal "
                   "WHERE Sorteos.Estado=1 AND Sorteos.Id=LimitesPorc.IdSorteo AND "
                   "LimitesPorc.IdMontoTriple=TiposMontoTriple.Id AND "
                   "LimitesPorc.IdMontoTerminal=TiposMontoTerminal.Id AND IdAgente=%1%")
    % conexionActiva.idAgente;
    
    ejecutar_sql(mysql, sqlQuery, DEBUG_TODOS_SORTEOS);
    MYSQL_RES * result = mysql_store_result(mysql);
    todos_sorteos_V_0_t respuesta;
    for (MYSQL_ROW row = mysql_fetch_row(result); row != NULL; row = mysql_fetch_row(result)) {
        sorteo_V_1_t sorteo;        
        sorteo.dias_de_juego = atoi(row[0]);
        sorteo.idSorteo      = atoi(row[1]);        
        memset(sorteo.nombre_loteria, ' ', NOMBRE_LOTERIA_LON);
        strcpy(sorteo.nombre_loteria, row[2]);
        sorteo.nombre_loteria[strlen(row[2])] = ' ';        
        sorteo.hora_cierre   = atoi(row[3]) / 100;
        sorteo.monto_triple  = atoi(row[4]);
        sorteo.monto_term1   = atoi(row[5]);
        sorteo.monto_term2   = atoi(row[6]);
        sorteo.monto_term3   = atoi(row[7]);
        sorteo.forma_pago    = atoi(row[8]);
        sorteo.comodin       = atoi(row[9]);
        sorteo.monto_adic    = atoi(row[10]);
        sorteo.tipojugada    = atoi(row[11]);
        sorteo.producto      = atoi(row[12]);
        respuesta.sorteos.push_back(sorteo);        
    }
    mysql_free_result(result);
    
    respuesta.numeroSorteos = respuesta.sorteos.size();
    
    boost::uint8_t * buffer = todos_sorteos_V_0_t2b(respuesta);
    std::size_t bufferSize = TODOS_SORTEOS_V_0_LON + (SORTEO_V_1_LON * respuesta.numeroSorteos);
    send2cliente(conexionActiva, TODOS_SORTEOS_V_0, buffer, bufferSize);
    delete [] buffer;
}

void
atenderPeticionSorteo_V_1(ConexionActiva& conexionActiva, sorteo_sol_t sorteoSol)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Sorteo");

    bool sorteoExiste = false;
    sorteo_V_1_t sorteo;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Id, Nombre, Dia, HoraCierre+0, TipoJugada, IdProducto FROM Sorteos "
                        "WHERE Id=%1% AND Estado=1")
        % unsigned(sorteoSol.idSorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            sorteoExiste = true;
            sorteo.idSorteo = atoi(row[0]);
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(sorteo.nombre_loteria, ' ', NOMBRE_LOTERIA_LON);
            strcpy(sorteo.nombre_loteria, row[1]);
            sorteo.nombre_loteria[strlen(row[1])] = ' ';
            sorteo.dias_de_juego = atoi(row[2]);
            sorteo.hora_cierre = atoi(row[3]) / 100;
            sorteo.tipojugada = atoi(row[4]);
            sorteo.producto = atoi(row[5]);
        }
        mysql_free_result(result);
    }

    if (not sorteoExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: no existe el sorteo <%u>.", sorteoSol.idSorteo);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
        return ;
    }
    bool limitesExisten = false;
    boost::uint32_t idMontoTriple = 0, idMontoTerminal = 0;
    sorteo.forma_pago = 0;
    sorteo.comodin = 0;
    sorteo.monto_adic = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdMontoTriple, IdMontoTerminal, IdFormaPago, Comodin, MontoAdd "
                        "FROM LimitesPorc WHERE IdSorteo=%1% AND IdAgente=%2%")
        % unsigned(sorteoSol.idSorteo) % conexionActiva.idAgente ;
        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            limitesExisten = true;
            idMontoTriple = atoi(row[0]);
            idMontoTerminal = atoi(row[1]);
            sorteo.forma_pago = atoi(row[2]);
            sorteo.comodin = atoi(row[3]);
            sorteo.monto_adic = atoi(row[4]);
        }
        mysql_free_result(result);
    }
    if (not limitesExisten) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: no existen los limites para el sorteo <%u>.",
                 sorteoSol.idSorteo);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
        return ;
    }
    bool montoTripleExiste = false;
    sorteo.monto_triple = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Monto FROM TiposMontoTriple WHERE ID =%1%") % idMontoTriple;
        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            montoTripleExiste = true;
            sorteo.monto_triple = atoi(row[0]);
        }
        mysql_free_result(result);
    }
    if (not montoTripleExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: no existe el tipo de monto <%u> para el triple del sorteo <%u>.",
                 idMontoTriple, sorteoSol.idSorteo);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TIPOMONTOTRINOEXIS, 0, 0);
        return ;
    }

    bool montosTerminalExisten = false;
    sorteo.monto_term1 = 0;
    sorteo.monto_term2 = 0;
    sorteo.monto_term3 = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Monto1ro, Monto2do, Monto3ro FROM TiposMontoTerminal WHERE ID =%1%")
        % idMontoTerminal;
        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            montosTerminalExisten = true;
            sorteo.monto_term1 = atoi(row[0]);
            sorteo.monto_term2 = atoi(row[1]);
            sorteo.monto_term3 = atoi(row[2]);
        }
        mysql_free_result(result);
    }
    if (not montosTerminalExisten) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: no existe el tipo de monto <%u> para el terminal del sorteo <%u>.",
                 idMontoTerminal, sorteoSol.idSorteo);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TIPOMONTOTERNOEXIS, 0, 0);
        return ;
    }

    boost::uint8_t * buffer = sorteo_V_1_t2b(sorteo);
    send2cliente(conexionActiva, SORTEO_V_1, buffer, SORTEO_V_1_LON);
    delete [] buffer;

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: solicitud de sorteo v1 <%u> resuelta con exito",
                 sorteoSol.idSorteo);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    }
}
/*-----------------------------------------------------------------------------*/

void
atenderPeticionTodosSorteos_V_1(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Todos los Sorteos");    
    
    boost::format sqlQuery;
    sqlQuery.parse("SELECT Sorteos.Id, Sorteos.Nombre, Sorteos.Dia, Sorteos.HoraCierre+0, "
                   "Sorteos.TipoJugada, Productos.Id, LimitesPorc.IdFormaPago, LimitesPorc.Comodin, "
                   "LimitesPorc.MontoAdd, TiposMontoTriple.Monto, TiposMontoTerminal.Monto1ro, "
                   "TiposMontoTerminal.Monto2do , TiposMontoTerminal.Monto3ro, Beneficiencias.Nombre, "
                   "Beneficiencias.RIF, Loterias.Nombre, Loterias.Rif "
                   "FROM ((((((Sorteos inner join LimitesPorc on Sorteos.Id = LimitesPorc.IdSorteo) "
                   "inner join TiposMontoTriple on LimitesPorc.IdMontoTriple = TiposMontoTriple.Id) "
                   "inner join TiposMontoTerminal on LimitesPorc.IdMontoTerminal = TiposMontoTerminal.Id) "
                   "inner join Productos on Sorteos.IdProducto = Productos.Id) "
                   "inner join Juegos on Productos.IdJuego = Juegos.Id) "
                   "inner join Beneficiencias on Juegos.IdBeneficiencia = Beneficiencias.Id) "
                   "inner join Loterias on Juegos.IdLoteria = Loterias.Id "                       
                   "WHERE Sorteos.Estado=1 AND LimitesPorc.IdAgente=%1%")
    % conexionActiva.idAgente;
    
    ejecutar_sql(mysql, sqlQuery, DEBUG_TODOS_SORTEOS);
    MYSQL_RES * result = mysql_store_result(mysql);
    TodosLosSorteosV1 respuesta;
    for (MYSQL_ROW row = mysql_fetch_row(result); row != NULL; row = mysql_fetch_row(result)) {
        SorteoV2 sorteo;            
        sorteo.idSorteo           = atoi(row[0]);            
        sorteo.nombreSorteo       = String(row[1]);            
        sorteo.diasDeJuego        = atoi(row[2]);            
        sorteo.horaDeCierre       = atoi(row[3]) / 100;            
        sorteo.tipoDeJugada       = atoi(row[4]);            
        sorteo.producto           = atoi(row[5]);            
        sorteo.formaDePago        = atoi(row[6]);
        sorteo.comodin            = atoi(row[7]);            
        sorteo.montoAdicional     = atoi(row[8]);
        sorteo.montoTriple        = atoi(row[9]);
        sorteo.montoTerminal1     = atoi(row[10]);
        sorteo.montoTerminal2     = atoi(row[11]);
        sorteo.montoTerminal3     = atoi(row[12]);
        sorteo.nombreBeneficencia = String(row[13]);
        sorteo.rifBeneficencia    = Rif(row[14]);
        sorteo.nombreOperadora    = String(row[15]);
        sorteo.rifOperadora       = Rif(row[16]);
        respuesta.addSorteo(sorteo);        
    }
    mysql_free_result(result);
    
    std::vector<boost::uint8_t> buffer = respuesta.toRawBuffer();
    send2cliente(conexionActiva, TODOS_SORTEOS_V_1, &buffer[0], buffer.size());
}


void
atenderPeticionSorteo_V_2(ConexionActiva& conexionActiva, sorteo_sol_t sorteoSol)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Sorteo");

    bool sorteoExiste = false;
    SorteoV2 sorteo;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Sorteos.Id, Sorteos.Nombre, Sorteos.Dia, Sorteos.HoraCierre+0, "
                       "Sorteos.TipoJugada, Productos.Id, LimitesPorc.IdFormaPago, LimitesPorc.Comodin, "
                       "LimitesPorc.MontoAdd, TiposMontoTriple.Monto, TiposMontoTerminal.Monto1ro, "
                       "TiposMontoTerminal.Monto2do , TiposMontoTerminal.Monto3ro, Beneficiencias.Nombre, "
                       "Beneficiencias.RIF, Loterias.Nombre, Loterias.Rif "
                       "FROM ((((((Sorteos inner join LimitesPorc on Sorteos.Id = LimitesPorc.IdSorteo) "
                       "inner join TiposMontoTriple on LimitesPorc.IdMontoTriple = TiposMontoTriple.Id) "
                       "inner join TiposMontoTerminal on LimitesPorc.IdMontoTerminal = TiposMontoTerminal.Id) "
                       "inner join Productos on Sorteos.IdProducto = Productos.Id) "
                       "inner join Juegos on Productos.IdJuego = Juegos.Id) "
                       "inner join Beneficiencias on Juegos.IdBeneficiencia = Beneficiencias.Id) "
                       "inner join Loterias on Juegos.IdLoteria = Loterias.Id "                       
                       "WHERE Sorteos.Estado=1 AND Sorteos.Id=%1% AND LimitesPorc.IdAgente=%2%")
        % unsigned(sorteoSol.idSorteo) % conexionActiva.idAgente;   
        
        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEOS_SOL_AL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            sorteoExiste = true;            
            sorteo.idSorteo           = atoi(row[0]);            
            sorteo.nombreSorteo       = String(row[1]);            
            sorteo.diasDeJuego        = atoi(row[2]);            
            sorteo.horaDeCierre       = atoi(row[3]) / 100;            
            sorteo.tipoDeJugada       = atoi(row[4]);            
            sorteo.producto           = atoi(row[5]);            
            sorteo.formaDePago        = atoi(row[6]);
            sorteo.comodin            = atoi(row[7]);            
            sorteo.montoAdicional     = atoi(row[8]);
            sorteo.montoTriple        = atoi(row[9]);
            sorteo.montoTerminal1     = atoi(row[10]);
            sorteo.montoTerminal2     = atoi(row[11]);
            sorteo.montoTerminal3     = atoi(row[12]);
            sorteo.nombreBeneficencia = String(row[13]);
            sorteo.rifBeneficencia    = Rif(row[14]);
            sorteo.nombreOperadora    = String(row[15]);
            sorteo.rifOperadora       = Rif(row[16]);
        }
        mysql_free_result(result);
    }

    if (not sorteoExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: no existe el sorteo <%u>.", sorteoSol.idSorteo);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_SORTEONOEXIS, 0, 0);
        return ;
    }

    std::vector<boost::uint8_t> buffer = sorteo.toRawBuffer();        
    send2cliente(conexionActiva, SORTEO_V_2, buffer);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: solicitud de sorteo v2 <%u> resuelta con exito",
                 sorteoSol.idSorteo);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    }
}
/*-----------------------------------------------------------------------------*/

void
atenderPeticionDatosBasicos_V_0(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Datos Basicos");    

	bool agenteExiste = false;
	datos_basicos_V_0_t datosBasicos;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Nombre, CI, Telefono, Celular, Direccion, IdRecogedor, Representante "
		                "FROM Agentes WHERE Id=%1%") % conexionActiva.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			agenteExiste = true;
			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(datosBasicos.nom_agente, ' ', NOMBRE_AGENTE_V_0_LON);
			strncpy(datosBasicos.nom_agente, row[0], NOMBRE_AGENTE_V_0_LON - 1);
            for (size_t i = 0; i < NOMBRE_AGENTE_V_0_LON; ++i)
                if (datosBasicos.nom_agente[i] == 13) datosBasicos.nom_agente[i] = ' ';
			datosBasicos.nom_agente[strlen(row[0])] = ' ';

			datosBasicos.cedula = atoi(row[1]);

			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(datosBasicos.telefono, ' ', TELEFONO_LON);
			strncpy(datosBasicos.telefono, row[2], TELEFONO_LON - 1);
			datosBasicos.telefono[strlen(row[2])] = ' ';

			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(datosBasicos.celular, ' ', TELEFONO_LON);
			if (row[3] == 0) {
				strncpy(datosBasicos.celular, "NO TIENE", TELEFONO_LON - 1);
			} else {
				strncpy(datosBasicos.celular, row[3], TELEFONO_LON - 1);
			}
			datosBasicos.celular[strlen(datosBasicos.celular)] = ' ';

			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(datosBasicos.direccion, ' ', DIRECCION_LON);
			strncpy(datosBasicos.direccion, row[4], DIRECCION_LON - 1);
			datosBasicos.direccion[strlen(datosBasicos.direccion)] = ' ';

			if (row[5] == 0) {
				datosBasicos.codigoRecogedor = 0;
			} else {
				datosBasicos.codigoRecogedor = atoi(row[5]);
			}

			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(datosBasicos.nom_representante, ' ', NOMBRE_REPRESENTANTE_LON);
			strncpy(datosBasicos.nom_representante, row[6], NOMBRE_REPRESENTANTE_LON - 1);
			datosBasicos.nom_representante[strlen(datosBasicos.nom_representante)] = ' ';
		}
		mysql_free_result(result);
	}
	if (not agenteExiste) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ERROR: solicitud de datos basicos de la agencia inactiva <%u>.",
		         conexionActiva.idAgente);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
		return ;
	}

	datosBasicos.cod_agente = conexionActiva.idAgente;

	bool agenteBancaExiste = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Nombre FROM Agentes WHERE IdTipoAgente=3");
		ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			agenteBancaExiste = true;
			// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
			memset(datosBasicos.banca, ' ', NOMBRE_AGENTE_V_0_LON);
			strncpy(datosBasicos.banca, row[0], NOMBRE_AGENTE_V_0_LON - 1);
            for (size_t i = 0; i < NOMBRE_AGENTE_V_0_LON; ++i)
                if (datosBasicos.banca[i] == 13) datosBasicos.banca[i] = ' ';            
			datosBasicos.banca[strlen(datosBasicos.banca)] = ' ';
		}
		mysql_free_result(result);
	}
	if (not agenteBancaExiste) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ERROR: no existe ningun agente de tipo banca en la base de datos <%u>.",
		         conexionActiva.idAgente);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
		return ;
	}

	// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
	memset(datosBasicos.nom_reco, ' ', NOMBRE_AGENTE_V_0_LON);
	if (datosBasicos.codigoRecogedor != 0) {
		bool agenteRecogedorExiste = false;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT Nombre FROM Agentes WHERE Id=%1%") % datosBasicos.codigoRecogedor;
			ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
			MYSQL_RES * result = mysql_store_result(mysql);
			MYSQL_ROW row = mysql_fetch_row(result);
			if (NULL != row) {
				agenteRecogedorExiste = true;
				// FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
				strncpy(datosBasicos.nom_reco, row[0], NOMBRE_AGENTE_V_0_LON - 1);
                for (size_t i = 0; i < NOMBRE_AGENTE_V_0_LON; ++i)
                    if (datosBasicos.nom_reco[i] == 13) datosBasicos.nom_reco[i] = ' ';
				datosBasicos.nom_reco[strlen(datosBasicos.nom_reco)] = ' ';
			}
			mysql_free_result(result);
		}
		if (not agenteRecogedorExiste) {
			char mensaje[BUFFER_SIZE];
			sprintf(mensaje, "ERROR: no existe el agente de id=%u en la base de datos <%u>.",
			         datosBasicos.codigoRecogedor, conexionActiva.idAgente);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
			return ;
		}
	}
	boost::uint8_t * buff = datos_basicos_V_0_t2b(datosBasicos);
	send2cliente(conexionActiva, DATOS_BASICOS_V_0, buff, DATOS_BASICOS_V_0_LON);
	delete [] buff;
	log_clientes(NivelLog::Bajo, &conexionActiva,
	              "MENSAJE: solicitud de datos basicos resuelta con exito");
}

/*-----------------------------------------------------------------------------*/


boost::uint32_t 
getIdAgenteBanca(ConexionActiva& conexionActiva, MYSQL * mysql)
{
    boost::uint32_t idAgente = conexionActiva.idAgente;
    do {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdRecogedor FROM Agentes WHERE Id=%1%") % idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL == row) {
            mysql_free_result(result);
            return 0;
        }
        boost::uint32_t const idRecogedor = strtoul(row[0], NULL, 10);
        if (idRecogedor == 1) {
            mysql_free_result(result);
            return idAgente;
        }
        idAgente = idRecogedor;
        mysql_free_result(result);
    } while (true);
    return 0;
}

void
atenderPeticionDatosBasicos_V_1(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Datos Basicos");

    bool agenteExiste = false;
    datos_basicos_V_1_t datosBasicos;
    
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Nombre, CI, Telefono, Celular, Direccion, IdRecogedor, Representante, Rif "
                        "FROM Agentes WHERE Id=%1%") % conexionActiva.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            agenteExiste = true;
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.nom_agente, ' ', NOMBRE_AGENTE_V_1_LON);
            strncpy(datosBasicos.nom_agente, row[0], NOMBRE_AGENTE_V_1_LON - 1);
            datosBasicos.nom_agente[strlen(row[0])] = ' ';

            datosBasicos.cedula = atoi(row[1]);

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.telefono, ' ', TELEFONO_LON);
            strncpy(datosBasicos.telefono, row[2], TELEFONO_LON - 1);
            datosBasicos.telefono[strlen(row[2])] = ' ';

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.celular, ' ', TELEFONO_LON);
            if (row[3] == 0) {
                strncpy(datosBasicos.celular, "NO TIENE", TELEFONO_LON - 1);
            } else {
                strncpy(datosBasicos.celular, row[3], TELEFONO_LON - 1);
            }
            datosBasicos.celular[strlen(datosBasicos.celular)] = ' ';

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.direccion, ' ', DIRECCION_LON);
            strncpy(datosBasicos.direccion, row[4], DIRECCION_LON - 1);
            datosBasicos.direccion[strlen(datosBasicos.direccion)] = ' ';

            if (row[5] == 0) {
                datosBasicos.codigoRecogedor = 0;
            } else {
                datosBasicos.codigoRecogedor = atoi(row[5]);
            }

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.nom_representante, ' ', NOMBRE_REPRESENTANTE_LON);
            strncpy(datosBasicos.nom_representante, row[6], NOMBRE_REPRESENTANTE_LON - 1);
            datosBasicos.nom_representante[strlen(datosBasicos.nom_representante)] = ' ';
            
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.rif_agente, ' ', RIF_LON);
            strncpy(datosBasicos.rif_agente, row[7], RIF_LON - 1);
            datosBasicos.rif_agente[strlen(row[7])] = ' ';
        }
        mysql_free_result(result);
    }
    if (not agenteExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: solicitud de datos basicos de la agencia inactiva <%u>.",
                 conexionActiva.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
        return ;
    }

    datosBasicos.cod_agente = conexionActiva.idAgente;

    boost::uint32_t idAgenteBanca = getIdAgenteBanca(conexionActiva, mysql);
    bool agenteBancaExiste = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Nombre, Rif FROM Agentes WHERE Id=%1%") % idAgenteBanca;
        ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            agenteBancaExiste = true;
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.banca, ' ', NOMBRE_AGENTE_V_1_LON);
            strncpy(datosBasicos.banca, row[0], NOMBRE_AGENTE_V_1_LON - 1);
            datosBasicos.banca[strlen(datosBasicos.banca)] = ' ';
            
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.rif_banca, ' ', RIF_LON);
            strncpy(datosBasicos.rif_banca, row[1], RIF_LON - 1);
            datosBasicos.rif_banca[strlen(row[1])] = ' ';
        }
        mysql_free_result(result);
    }
    if (not agenteBancaExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: no existe ningun agente de tipo banca en la base de datos <%u>.",
                 conexionActiva.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
        return ;
    }

    // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
    memset(datosBasicos.nom_reco, ' ', NOMBRE_AGENTE_V_1_LON);
    if (datosBasicos.codigoRecogedor != 0) {
        bool agenteRecogedorExiste = false;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Nombre FROM Agentes WHERE Id=%1%") % datosBasicos.codigoRecogedor;
            ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
            MYSQL_RES * result = mysql_store_result(mysql);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (NULL != row) {
                agenteRecogedorExiste = true;
                // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
                strncpy(datosBasicos.nom_reco, row[0], NOMBRE_AGENTE_V_1_LON - 1);
                datosBasicos.nom_reco[strlen(datosBasicos.nom_reco)] = ' ';
            }
            mysql_free_result(result);
        }
        if (not agenteRecogedorExiste) {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: no existe el agente de id=%u en la base de datos <%u>.",
                     datosBasicos.codigoRecogedor, conexionActiva.idAgente);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
            return ;
        }
    }
    boost::uint8_t * buff = datos_basicos_V_1_t2b(datosBasicos);
    send2cliente(conexionActiva, DATOS_BASICOS_V_1, buff, DATOS_BASICOS_V_1_LON);
    delete [] buff;
    log_clientes(NivelLog::Bajo, &conexionActiva,
                  "MENSAJE: solicitud de datos basicos resuelta con exito");
}

/**************************************************************************/

void
atenderPeticionDatosBasicos_V_2(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Detallado, &conexionActiva, "Solicitud de Datos Basicos");

    bool agenteExiste = false;
    datos_basicos_V_2_t datosBasicos;
    
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Agencia.Nombre, Agencia.CI, Agencia.Telefono, Agencia.Celular, "
                       "Agencia.Direccion, Agencia.Representante, Agencia.Rif, "
                       "Banca.Nombre, Banca.Rif, Banca.Direccion, "
                       "Recogedor.Id, Recogedor.Nombre "
                       "FROM (Agentes as Agencia left join  Agentes as Banca on Agencia.IdBanca = Banca.Id) "
                       "left join Agentes as Recogedor on Agencia.IdRecogedor = Recogedor.Id "
                       "WHERE Agencia.Id=%1%") 
        % conexionActiva.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_DATOS_BASICOS_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            agenteExiste = true;
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.nombreAgencia, ' ', NOMBRE_AGENTE_V_1_LON);
            strncpy(datosBasicos.nombreAgencia, row[0], NOMBRE_AGENTE_V_1_LON - 1);
            datosBasicos.nombreAgencia[strlen(datosBasicos.nombreAgencia)] = ' ';

            datosBasicos.cedula = atoi(row[1]);

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.telefono, ' ', TELEFONO_LON);
            strncpy(datosBasicos.telefono, row[2], TELEFONO_LON - 1);
            datosBasicos.telefono[strlen(datosBasicos.telefono)] = ' ';

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.celular, ' ', TELEFONO_LON);
            if (row[3] == 0) {
                strncpy(datosBasicos.celular, "NO TIENE", TELEFONO_LON - 1);
            } else {
                strncpy(datosBasicos.celular, row[3], TELEFONO_LON - 1);
            }
            datosBasicos.celular[strlen(datosBasicos.celular)] = ' ';

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.direccionAgencia, ' ', DIRECCION_LON);
            strncpy(datosBasicos.direccionAgencia, row[4], DIRECCION_LON - 1);
            datosBasicos.direccionAgencia[strlen(datosBasicos.direccionAgencia)] = ' ';            

            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.nombreRepresentante, ' ', NOMBRE_REPRESENTANTE_LON);
            strncpy(datosBasicos.nombreRepresentante, row[5], NOMBRE_REPRESENTANTE_LON - 1);
            datosBasicos.nombreRepresentante[strlen(datosBasicos.nombreRepresentante)] = ' ';
            
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.rifAgencia, ' ', RIF_LON);
            strncpy(datosBasicos.rifAgencia, row[6], RIF_LON - 1);
            datosBasicos.rifAgencia[strlen(datosBasicos.rifAgencia)] = ' ';            
            
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.nombreBanca, ' ', NOMBRE_AGENTE_V_1_LON);
            strncpy(datosBasicos.nombreBanca, row[7], NOMBRE_AGENTE_V_1_LON - 1);
            datosBasicos.nombreBanca[strlen(datosBasicos.nombreBanca)] = ' ';
            
            // FIXME: esta manipulacion de c_str es bastante peligrosa, hay que cambiarla !
            memset(datosBasicos.rifBanca, ' ', RIF_LON);
            strncpy(datosBasicos.rifBanca, row[8], RIF_LON - 1);
            datosBasicos.rifBanca[strlen(datosBasicos.rifBanca)] = ' ';
            
            memset(datosBasicos.direccionBanca, ' ', DIRECCION_LON);
            strncpy(datosBasicos.direccionBanca, row[9], DIRECCION_LON - 1);
            datosBasicos.direccionBanca[strlen(datosBasicos.direccionBanca)] = ' ';            
            
            datosBasicos.codigoRecogedor = atoi(row[10]);
            
            memset(datosBasicos.nombreRecogedor, ' ', NOMBRE_AGENTE_V_1_LON);
            strncpy(datosBasicos.nombreRecogedor, row[11], NOMBRE_AGENTE_V_1_LON - 1);
            datosBasicos.nombreRecogedor[strlen(datosBasicos.nombreRecogedor)] = ' ';
            
            datosBasicos.codigoAgencia = conexionActiva.idAgente;
            
        }
        mysql_free_result(result);
    }
    if (not agenteExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: solicitud de datos basicos de la agencia inactiva <%u>.",
                 conexionActiva.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
        return ;
    }

    boost::uint8_t * buff = datos_basicos_V_2_t2b(datosBasicos);
    send2cliente(conexionActiva, DATOS_BASICOS_V_2, buff, DATOS_BASICOS_V_2_LON);
    delete [] buff;
    log_clientes(NivelLog::Bajo, &conexionActiva,
                  "MENSAJE: solicitud de datos basicos resuelta con exito");
}

/**************************************************************************/
void
cargar_tipomonto_x_sorteo(boost::uint32_t idAgente, MYSQL * mysql)
{
	TipoDeMontoPorAgencia * tiposDeMontoPorAgencia = new TipoDeMontoPorAgencia;

	TipoDeMontoPorAgencia::add(idAgente, tiposDeMontoPorAgencia);

	std::vector<std::pair<boost::uint8_t, TipoDeMonto*> > tiposDeMontoPorSorteo;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT IdSorteo, IdMontoTriple, IdMontoTerminal, LimDiscTrip, LimDiscTerm, "
		                "IdFormaPago, PorcTrip, PorcTerm, Comodin, MontoAdd FROM LimitesPorc "
		                "WHERE IdAgente=%1%")
		% idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_CARGAR_TIPOMONTO_X_SORTEO);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); row != 0; row = mysql_fetch_row(result)) {
			boost::uint8_t idSorteo = atoi(row[0]);
			TipoDeMonto* tipoDeMontoPorSorteo = new TipoDeMonto;
			tipoDeMontoPorSorteo->tipomonto_triple = atoi(row[1]);
			tipoDeMontoPorSorteo->tipomonto_terminal = atoi(row[2]);
			tipoDeMontoPorSorteo->limdisctrip = double(atoi(row[3])) / 10000.0;
			tipoDeMontoPorSorteo->limdiscterm = double(atoi(row[4])) / 10000.0;
			tipoDeMontoPorSorteo->forma_pago = atoi(row[5]);
			tipoDeMontoPorSorteo->porctriple = double(atoi(row[6])) / 10000.0;
			tipoDeMontoPorSorteo->porcterminal = double(atoi(row[7])) / 10000.0;
			tipoDeMontoPorSorteo->comodin = atoi(row[8]);
			tipoDeMontoPorSorteo->montoAdicional = (row[9] == NULL) ? 0 : atoi(row[9]);
			tiposDeMontoPorSorteo.push_back(std::make_pair(idSorteo, tipoDeMontoPorSorteo));
		}
		mysql_free_result(result);
	}

	for (std::size_t i = 0, size = tiposDeMontoPorSorteo.size(); i < size ; ++i) {
		boost::uint8_t idSorteo = tiposDeMontoPorSorteo[i].first;
		TipoDeMonto* tipoDeMontoPorSorteo = tiposDeMontoPorSorteo[i].second;
		tiposDeMontoPorAgencia->updateTipoDeMonto(idSorteo, tipoDeMontoPorSorteo);
	}
}

/**************************************************************************/

void
atenderPeticionMensajeria(ConexionActiva& conexionActiva, boost::uint8_t * buffer)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	enviar_mensaje(mensajeria_b2t(buffer), mysql, conexionActiva);
}

/**************************************************************************/

void
atenderPeticionCambiarPassword(ConexionActiva& conexionActiva, boost::uint8_t * buffer)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	cambiar_password_t c = cambiar_password_b2t(buffer);

	char password_act[33];
	genera_password(c.password_act, password_act);

	char password_new[33];
	genera_password(c.password_new, password_new);

	bool usuarioExiste = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Id FROM Usuarios WHERE Id=%1% AND Password='%2%'")
		% conexionActiva.idUsuario % password_act;
		ejecutar_sql(mysql, sqlQuery, DEBUG_CAMBIAR_PASSWORD);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			usuarioExiste = true;
		}
		mysql_free_result(result);
	}
	if (usuarioExiste) {
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Usuarios SET Password='%1%' WHERE Id=%2%")
		% password_new % conexionActiva.idUsuario ;
		ejecutar_sql(mysql, "BEGIN", DEBUG_CAMBIAR_PASSWORD);
		ejecutar_sql(mysql, sqlQuery, DEBUG_CAMBIAR_PASSWORD);
		ejecutar_sql(mysql, "COMMIT", DEBUG_CAMBIAR_PASSWORD);
		send2cliente(conexionActiva, CAMBIAR_PASSWORD, 0, 0);
	} else {
		send2cliente(conexionActiva, ERR_LOGINPASSWORD, 0, 0);
	}
}

void
atenderPeticionConsultarMensajes(ConexionActiva& conexionActiva)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
	std::vector<mensajeria_t> mensajes;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Login, Tipo, Mensaje, UNIX_TIMESTAMP(FechaHora) "
		                "FROM Mensajeria, Usuarios "
		                "WHERE IdUsuarioD=%1% AND Usuarios.Id=Mensajeria.IdusuarioR AND Leido=0 "
		                "ORDER BY Mensajeria.Id DESC") % conexionActiva.idUsuario;
		ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_MENSAJES);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result);
		        NULL != row;
		        row = mysql_fetch_row(result)) {
			mensajeria_t mensajeria;
			memset(mensajeria.login, ' ', LOGIN_LENGTH);
			strncpy(mensajeria.login, row[0], LOGIN_LENGTH - 1);
			mensajeria.tipo = (atoi(row[1]) >= MSJURGENTE)
			                  ? MSJURGENTE
			                  : atoi(row[1]);
			strncpy(mensajeria.mensaje, row[2], MENSAJE_MAX_LON - 1);
			mensajeria.fechahora = atoi(row[3]);
			mensajeria.longitud = strlen(mensajeria.mensaje) + 1;
			mensajes.push_back(mensajeria);
		}
		mysql_free_result(result);
	}

	for (std::size_t i = 0, size = mensajes.size(); i < size ;++i) {
		boost::uint8_t * buffer = mensajeria_t2b(mensajes[i]);
		send2cliente(conexionActiva, MENSAJERIA, buffer, MENSAJERIA_LON + mensajes[i].longitud);
		delete [] buffer;
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// FIXME: esto no hace lo que se espera, ya que si llegan mensajes nuevos  !
	//       entre el tiempo en el que se obtuvo la lista de mensajes no      !
	//       leidos de la base de datos hasta el momento en que se van a      !
	//       colocar como leidos, dichos mensajes nunca seran enviados al     !
	//       cliente                                                          !
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Mensajeria SET Leido=1 WHERE IdUsuarioD=%1% AND Leido=0")
		% conexionActiva.idUsuario;
		ejecutar_sql(mysql, "BEGIN", DEBUG_CONSULTAR_MENSAJES);
		ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_MENSAJES);
		ejecutar_sql(mysql, "COMMIT", DEBUG_CONSULTAR_MENSAJES);
	}
}

boost::uint16_t const parteMaxLon = 60000;

void
atenderPeticionActualizarCliente(ConexionActiva& conexionActiva, boost::uint8_t * buffer)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();

	bool puedeActualizar = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT PuedeActualizar FROM Medios, Agentes "
		                "WHERE Medios.Id = Agentes.IdMedio AND Agentes.Id=%1%")
		% conexionActiva.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_CAMBIAR_PASSWORD);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			puedeActualizar = (atoi(row[0]) != 0);
		}
		mysql_free_result(result);
	}

	if (not puedeActualizar) {
		boost::uint8_t c = 0;
		send2cliente(conexionActiva, ACTUALIZAR_CLIENTE, &c, 1);
		log_clientes(NivelLog::Bajo, &conexionActiva, "Medio no autorizado para actualizar");
		return ;
	}

	// leer el archivo del cliente
	std::ifstream file(configuracion.rutaTaquilla, std::ios::binary);

	// if no puede abrir el archivo ...
	if (not file) {
		boost::uint8_t c = 0;
		send2cliente(conexionActiva, ACTUALIZAR_CLIENTE, &c, 1);

		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		         "No se pudo abrir el archivo %s para actualizar cliente",
		         configuracion.rutaTaquilla);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}

	std::vector<char> bufferArchivo;
	char c;
	while (file.get(c)) {
		bufferArchivo.push_back(c);
	}

	if (bufferArchivo.size() == 0) {
		boost::uint8_t c = 0;
		send2cliente(conexionActiva, ACTUALIZAR_CLIENTE, &c, 1);
		log_clientes(NivelLog::Bajo, &conexionActiva, "No hay actualizacion para el cliente");
		return ;
	}

	// calcular el MD5 del archivo del cliente
	file.clear();
	file.seekg(std::ios::beg);
	MD5 context(file);

	if (memcmp(buffer, context.raw_digest(), 16) == 0) {
		boost::uint8_t c = 0;
		send2cliente(conexionActiva, ACTUALIZAR_CLIENTE, &c, 1);
		log_clientes(NivelLog::Bajo, &conexionActiva, "Cliente ya actualizado");
		return ;
	}

	// crear los paquetes de respuesta
	boost::uint8_t numeroPartes = (bufferArchivo.size() / parteMaxLon)
	                         + ((bufferArchivo.size() % parteMaxLon) ? 1 : 0);

	size_t partePos = 0;
	for (boost::uint8_t numeroParte = 1; numeroParte <= numeroPartes ; ++numeroParte) {

		boost::uint16_t parteLon = (bufferArchivo.size() % parteMaxLon and numeroParte == numeroPartes)
		                      ? (bufferArchivo.size() % parteMaxLon)
		                      : parteMaxLon;

		size_t bufferRespuestaLon = 18 * sizeof(boost::uint8_t) + sizeof(boost::uint16_t) + parteLon;
		boost::uint8_t* bufferRespuesta = new boost::uint8_t[bufferRespuestaLon];

		*reinterpret_cast<boost::uint8_t*>(bufferRespuesta) = numeroParte;
		*reinterpret_cast<boost::uint8_t*>(bufferRespuesta + sizeof(boost::uint8_t)) = numeroPartes;
		*reinterpret_cast<boost::uint16_t*>(bufferRespuesta + 2 * sizeof(boost::uint8_t)) = parteLon;

		memcpy(bufferRespuesta + 2 * sizeof(boost::uint8_t) + sizeof(boost::uint16_t),
		        context.raw_digest(),
		        16);

		memcpy(bufferRespuesta + 18 * sizeof(boost::uint8_t) + sizeof(boost::uint16_t),
		        &bufferArchivo[partePos],
		        parteLon);

		// enviar el paquete de respuesta
		send2cliente(conexionActiva,
		              ACTUALIZAR_CLIENTE,
		              bufferRespuesta,
		              bufferRespuestaLon);

		delete [] bufferRespuesta;

		partePos += parteLon;
	}
}

/*--------------------------------------------------------------------------------------------*/

void atenderLoginV2(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login);

void
atender_peticion(SocketThread * socketsThread, boost::uint8_t * buffer)
{
	ConexionActiva& conexionActiva = socketsThread->getConexionTaquilla();
	Cabecera cabecera(buffer);
    
    std::ostringstream oss;
    oss << "Atendiendo peticion numero " << unsigned(cabecera.getPeticion());
    
    log_clientes(NivelLog::Debug, &conexionActiva, oss.str().c_str());

	conexionActiva.idTaquilla = cabecera.getIdTaquilla();
        
    switch (cabecera.getPeticion()) {
                
		case LOGIN_V_0: {
        
            SolicitudLoginV0 login(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);			
			RespuestaLoginV0 respuestaLogin;
			boost::uint8_t idZona = 0;
            bool usarProductos = false;
			if (atenderLoginV0(conexionActiva,
			                   login,
			                   &respuestaLogin,
			                   &idZona,
                               &usarProductos) == 1) {
                                            
				conexionActiva.idAgente = login.idAgente;
				conexionActiva.idUsuario = respuestaLogin.idUsuario;
				conexionActiva.idZona = idZona;
                conexionActiva.usarProductos = usarProductos;
				strcpy(conexionActiva.login, login.login);
				
				boost::uint32_t idAgente = conexionActiva.idAgente;                    
                {
                    TipoDeMontoPorAgencia::MutexGuard g;
					if (TipoDeMontoPorAgencia::get(idAgente) == NULL) {
                        DataBaseConnetion dbConnetion;
                        MYSQL* mysql = dbConnetion.get();                        
						cargar_tipomonto_x_sorteo(conexionActiva.idAgente, mysql);
					}
                }
				
				boost::uint64_t agenteTaquilla = idAgente;
				agenteTaquilla = agenteTaquilla << 32;
				agenteTaquilla += conexionActiva.idTaquilla;
				n_t_preventas_x_taquilla_t * preventasPorTaquilla =
				    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla,
				                                                    &agenteTaquilla);
				if (preventasPorTaquilla == 0) {
					preventasPorTaquilla = new n_t_preventas_x_taquilla_t;
					boost::uint64_t * p_agente_taquilla = new boost::uint64_t;
					*p_agente_taquilla = agenteTaquilla;
					preventasPorTaquilla->idZona = idZona;
					sem_init(&preventasPorTaquilla->mutex_preventas_x_taquilla, 0, 1);
					preventasPorTaquilla->hora_ultimo_renglon = time(NULL);
					preventasPorTaquilla->hora_ultimo_peticion = time(NULL);
					preventasPorTaquilla->preventas =
					    g_tree_new_full(uint16_t_gcomp_func,
					                     0,
					                     uint16_t_destroy_key,
					                     t_n_t_preventas_t_destroy);
					g_tree_insert(t_preventas_x_taquilla, p_agente_taquilla,
					               preventasPorTaquilla);
				} else {
					preventasPorTaquilla->hora_ultimo_renglon = time(NULL);
					preventasPorTaquilla->hora_ultimo_peticion = time(NULL);
				}				
				
				log_clientes(NivelLog::Bajo, &conexionActiva,
				              "MENSAJE: login - inicio de sesion exitoso.");
			}
			break;
		}        
        case LOGIN_V_1: {
                   
            SolicitudLoginV0 login(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);            
            RespuestaLoginV1 respuestaLogin;
            boost::uint8_t idZona = 0;        
            bool usarProductos = false;
            if (atenderLoginV1(conexionActiva,
                               login,
                               &respuestaLogin,
                               &idZona,
                               &usarProductos) == 1) {
                conexionActiva.idAgente = login.idAgente;
                conexionActiva.idUsuario = respuestaLogin.idUsuario;
                conexionActiva.idZona = idZona;
                conexionActiva.usarProductos = usarProductos;
                strcpy(conexionActiva.login, login.login);
                {
                    boost::uint32_t idAgente = conexionActiva.idAgente;
                    {
                        TipoDeMontoPorAgencia::MutexGuard g;
                        if (TipoDeMontoPorAgencia::get(idAgente) == NULL) {
                            DataBaseConnetion dbConnetion;
                            MYSQL* mysql = dbConnetion.get();                        
                            cargar_tipomonto_x_sorteo(conexionActiva.idAgente, mysql);
                        }
                    }
                    {
                        boost::uint64_t agenteTaquilla = idAgente;
                        agenteTaquilla = agenteTaquilla << 32;
                        agenteTaquilla += conexionActiva.idTaquilla;
                        n_t_preventas_x_taquilla_t * preventasPorTaquilla =
                            (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla,
                                                                            &agenteTaquilla);
                        if (preventasPorTaquilla == 0) {
                            preventasPorTaquilla = new n_t_preventas_x_taquilla_t;
                            boost::uint64_t * p_agente_taquilla = new boost::uint64_t;
                            *p_agente_taquilla = agenteTaquilla;
                            preventasPorTaquilla->idZona = idZona;
                            sem_init(&preventasPorTaquilla->mutex_preventas_x_taquilla, 0, 1);
                            preventasPorTaquilla->hora_ultimo_renglon = time(NULL);
                            preventasPorTaquilla->hora_ultimo_peticion = time(NULL);
                            preventasPorTaquilla->preventas =
                                g_tree_new_full(uint16_t_gcomp_func,
                                                 0,
                                                 uint16_t_destroy_key,
                                                 t_n_t_preventas_t_destroy);
                            g_tree_insert(t_preventas_x_taquilla, p_agente_taquilla,
                                           preventasPorTaquilla);
                        } else {
                            preventasPorTaquilla->hora_ultimo_renglon = time(NULL);
                            preventasPorTaquilla->hora_ultimo_peticion = time(NULL);
                        }
                    }
                }
                log_clientes(NivelLog::Bajo, &conexionActiva,
                              "MENSAJE: login - inicio de sesion exitoso.");
            }
            break;
        }                
        case LOGIN_V_2: {                
            SolicitudLoginV1 login(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);  
            atenderLoginV2(conexionActiva, login);
            break;
        }  
                      
		case SORTEO_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, SORTEO_V_0);
			} else {
				sorteo_sol_t sorteos = sorteo_sol_b2t(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
				atenderPeticionSorteo_V_0(conexionActiva, sorteos);
			}
			break;
		}
        case SORTEO_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, SORTEO_V_1);
            } else {
                sorteo_sol_t sorteos = sorteo_sol_b2t(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
                atenderPeticionSorteo_V_1(conexionActiva, sorteos);
            }
            break;
        }
        case SORTEO_V_2: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, SORTEO_V_2);
            } else {
                sorteo_sol_t sorteos = sorteo_sol_b2t(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
                atenderPeticionSorteo_V_2(conexionActiva, sorteos);
            }
            break;
        }
        case TODOS_SORTEOS_V_0: {            
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, TODOS_SORTEOS_V_0);
            } else {
                atenderPeticionTodosSorteos_V_0(conexionActiva);
            }
            break;
        }
        case TODOS_SORTEOS_V_1: {            
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, TODOS_SORTEOS_V_1);
            } else {
                atenderPeticionTodosSorteos_V_1(conexionActiva);
            }
            break;
        }
		case LIMITES: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, LIMITES);
			} else {
                limites_sol_t limitesSol = limites_sol_b2t(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
				atenderPeticionLimites(conexionActiva, limitesSol);
			}
			break;
		}
        case TODOS_LIMITES: {            
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, TODOS_LIMITES);
            } else {
                atenderPeticionTodosLimites(conexionActiva);
            }
            break;
        }
		case DATOS_BASICOS_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, DATOS_BASICOS_V_0);
			} else {
				atenderPeticionDatosBasicos_V_0(conexionActiva);
			}
			break;
		}
        case DATOS_BASICOS_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, DATOS_BASICOS_V_1);
            } else {
                atenderPeticionDatosBasicos_V_1(conexionActiva);
            }
            break;
        }
        case DATOS_BASICOS_V_2: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, DATOS_BASICOS_V_2);
            } else {
                atenderPeticionDatosBasicos_V_2(conexionActiva);
            }
            break;
        }               
		case LOGOUT: {
			{
				char mensaje[BUFFER_SIZE];
				sprintf(mensaje, "Punto de Venta Desconectado");
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			}
			break;
		}
		case VENTA: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, VENTA);
			} else {
				atenderPeticionVenta(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
		case CANCELAR_PREVENTA: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, CANCELAR_PREVENTA);
			} else {
				atenderPeticionCancelarPreventa(conexionActiva);
			}
			break;
		}
		case ELIMINAR_RENGLON: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, ELIMINAR_RENGLON);
			} else {
				renglon_t renglon = renglon_eli_b2t(buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
				atenderPeticionEliminarRenglon(conexionActiva, renglon);
			}
			break;
		}
		case TERMINALES: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, TERMINALES);
			} else {
				atenderPeticionTerminales(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
		case REPETIR_JUGADA: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, REPETIR_JUGADA);
			} else {
				atenderPeticionRepetirJugada(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		} 
        case GUARDAR_TICKET_V_0: {
              if (conexionActiva.idAgente == 0) {
                  enviarPeticionNoLogeado(&conexionActiva, GUARDAR_TICKET_V_0);
              } else {
                  DataBaseConnetion dbConnetion;
                  MYSQL* mysql = dbConnetion.get();
                  atenderPeticionGuardarTicket_V_0(conexionActiva, 
                                                   buffer + Cabecera::CONST_RAW_BUFFER_SIZE,
                                                   mysql, 
                                                   true);
              }
              break;
        }                        
        case GUARDAR_TICKET_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, GUARDAR_TICKET_V_1);
                
            } else {
                DataBaseConnetion dbConnetion;
                MYSQL* mysql = dbConnetion.get();
                atenderPeticionGuardarTicket_V_1(conexionActiva,
                                                 buffer + Cabecera::CONST_RAW_BUFFER_SIZE,
                                                 mysql,
                                                 true);
            }
            break;
        }
        case GUARDAR_TICKET_V_2: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, GUARDAR_TICKET_V_2);
                
            } else {
                DataBaseConnetion dbConnetion;
                MYSQL* mysql = dbConnetion.get();
                atenderPeticionGuardarTicket_V_2(conexionActiva,
                                                 buffer + Cabecera::CONST_RAW_BUFFER_SIZE,
                                                 mysql,
                                                 true);
            }
            break;
        }        
        case GUARDAR_TICKET_V_3: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, GUARDAR_TICKET_V_3);
                
            } else {
                DataBaseConnetion dbConnetion;
                MYSQL* mysql = dbConnetion.get();
                atenderPeticionGuardarTicket_V_3(conexionActiva,
                                                 buffer + Cabecera::CONST_RAW_BUFFER_SIZE,
                                                 mysql,
                                                 true);
            }
            break;
        }          
		case MODIFICAR_TICKET_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, MODIFICAR_TICKET_V_0);
			} else {
				atenderPeticionModificarTicket_V_0(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
        case MODIFICAR_TICKET_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, MODIFICAR_TICKET_V_1);
            } else {
                atenderPeticionModificarTicket_V_1(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
        case MODIFICAR_TICKET_V_2: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, MODIFICAR_TICKET_V_2);
            } else {
                atenderPeticionModificarTicket_V_2(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
        case MODIFICAR_TICKET_V_3: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, MODIFICAR_TICKET_V_3);
            } else {
                atenderPeticionModificarTicket_V_3(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
		case ANULAR_TICKET_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, ANULAR_TICKET_V_0);
			} else {
				atenderPeticionCancelarPreventa(conexionActiva);
                DataBaseConnetion dbConnetion;
                MYSQL* mysql = dbConnetion.get();
				atenderPeticionAnularTicket_V_0(conexionActiva,
				                                buffer + Cabecera::CONST_RAW_BUFFER_SIZE,
				                                mysql,
				                                true);
			}
			break;
		}
        case ANULAR_TICKET_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, ANULAR_TICKET_V_1);
            } else {
                atenderPeticionCancelarPreventa(conexionActiva);
                DataBaseConnetion dbConnetion;
                MYSQL* mysql = dbConnetion.get();
                atenderPeticionAnularTicket_V_1(conexionActiva,
                                                buffer + Cabecera::CONST_RAW_BUFFER_SIZE,
                                                mysql,
                                                true);
            }
            break;
        }
		case REPETIR: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, REPETIR);
			} else {
				atenderPeticionRepetirTicket(conexionActiva,
				                              buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
		case PAGAR_TICKET_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, PAGAR_TICKET_V_0);
			} else {
				atenderPeticionPagarTicket_V_0(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
        case PAGAR_TICKET_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, PAGAR_TICKET_V_1);
            } else {
                atenderPeticionPagarTicket_V_1(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
        case PAGAR_TICKET_V_2: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, PAGAR_TICKET_V_2);
            } else {
                atenderPeticionPagarTicket_V_2(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }        
		case DESCARGAR_TICKET_PREM_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, DESCARGAR_TICKET_PREM_V_0);
			} else {			
                atenderPeticionDescargarTicketPremiado_V_0(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
        case DESCARGAR_TICKET_PREM_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, DESCARGAR_TICKET_PREM_V_1);
            } else {          
                atenderPeticionDescargarTicketPremiado_V_1(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
		case SALDO_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, SALDO_V_0);
			} else {
				atenderPeticionSaldo_V_0(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
        case SALDO_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, SALDO_V_1);
            } else {
                atenderPeticionSaldo_V_1(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }        
		case PREMIOS_V_0: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, PREMIOS_V_0);
			} else {
				atenderPeticionPremios_V_0(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
        case PREMIOS_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, PREMIOS_V_1);
            } else {
                atenderPeticionPremios_V_1(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
        case TODOS_PREMIOS_V_0: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, TODOS_PREMIOS_V_0);
            } else {
                atenderPeticionTodosPremios_V_0(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
        case TODOS_PREMIOS_V_1: {
            if (conexionActiva.idAgente == 0) {
                enviarPeticionNoLogeado(&conexionActiva, TODOS_PREMIOS_V_1);
            } else {
                atenderPeticionTodosPremios_V_1(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
            }
            break;
        }
		case CONSULTAR_PREMIOS: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, CONSULTAR_PREMIOS);
			} else {
				atenderPeticionConsultarPremios(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
		case MENSAJERIA: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, MENSAJERIA);
			} else {
				atenderPeticionMensajeria(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
		case CAMBIAR_PASSWORD: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, CAMBIAR_PASSWORD);
			} else {
				atenderPeticionCambiarPassword(conexionActiva, buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
		case CONSULTAR_MENSAJES: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, CONSULTAR_MENSAJES);
			} else {
				atenderPeticionConsultarMensajes(conexionActiva);
			}
			break;
		}
		case ACTUALIZAR_CLIENTE: {
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, ACTUALIZAR_CLIENTE);
			} else {
				atenderPeticionActualizarCliente(conexionActiva,
				                                  buffer + Cabecera::CONST_RAW_BUFFER_SIZE);
			}
			break;
		}
        case PING:
        case FLUSH: {
            break;
        }
		default: {
			{
				char mensaje[BUFFER_SIZE];
				sprintf(mensaje, "Peticion desconocida %u", cabecera.getPeticion());
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			}
			if (conexionActiva.idAgente == 0) {
				enviarPeticionNoLogeado(&conexionActiva, cabecera.getPeticion());
			}
		}
   }
}
