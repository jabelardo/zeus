#include <database/DataBaseConnetionPool.h>
#include <ConexionActiva.h>
#include <global.h>
#include <utils.h>
#include <sockets.h>
#include <atender_peticion.h>
#include <SocketThread.h>
#include <TipoDeMontoDeAgencia.h>
#include <ventas.h>

#include <ProtocoloZeus/Protocolo.h>
#include <ProtocoloZeus/Mensajes.h>

#include <zlib.h>

using namespace ProtocoloZeus;

namespace { bool DEBUG_LOGIN_SOL = false; }

bool
verificarAutentificacion(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login)
{
    boost::uint32_t llave = crc32(crc32(0L, Z_NULL, 0), (boost::uint8_t *) autentificacion,
                              strlen(autentificacion));
                              
    if (llave == login.autentificacion) {
        return true;
    }
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

bool
verificarAgencia(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, MYSQL* mysql, 
                 boost::uint8_t& idZona, bool& usarProductos)
{   
    bool agenteActivo = false;
    bool agenteExiste = false;
    idZona = 0;
    usarProductos = true;        
    
    boost::format sqlQuery;
    sqlQuery.parse("SELECT Agencia.Estado, Agencia.IdZona, Agencia.UsarProductos "
                   "FROM Agentes as Agencia WHERE Id=%1%") 
    % login.idAgente;
    
    ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    
    if (NULL != row) {
        agenteExiste = true;
        agenteActivo = (atoi(row[0]) == 1);
        idZona = atoi(row[1]);
        usarProductos = (atoi(row[2]) != 0);
    }
        
    mysql_free_result(result);    

    if (!agenteExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: login - el agente <%u> no existe.", login.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
        return false;
    }
                
    if (!agenteActivo) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: login - agente <%u> desactivado por razones administrativas.",
                 login.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_AGENCIANOACT, 0, 0);
        return false;
    }
    
    return true;
}

bool
verificarUsuario(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, MYSQL* mysql,
                 boost::uint32_t& idUsuario, boost::uint8_t& prioridadUsuario)
{            
    boost::uint8_t estadoUsuario = 0;    
    boost::uint32_t idAgenteUsuario = 0;
    prioridadUsuario = 0;
    idUsuario = 0;
    
    char password[33];
    genera_password(&login.password[0], password);
    boost::format sqlQuery;
    sqlQuery.parse("SELECT Estado, Id, Prioridad, IdAgente FROM Usuarios "
                   "WHERE Login='%1%' AND Password='%2%'")
    % login.login % password;
    ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    MYSQL_RES * result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (NULL != row) {
        estadoUsuario = atoi(row[0]);
        idUsuario = strtoul(row[1], NULL, 10);
        prioridadUsuario = atoi(row[2]);
        idAgenteUsuario = strtoul(row[3], NULL, 10);
    }
    mysql_free_result(result);    
    
    if (!estadoUsuario) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: login - el usuario con login <%s>@<%u> no esta activo.",
                 login.login.getValue().c_str(), login.idAgente);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_USUARIONOACT, 0, 0);
        return false;
    }

    boost::uint32_t idAgente = login.idAgente;
        
    while (idAgenteUsuario != idAgente) { 
         
        boost::uint32_t idRecogedor = 0;      
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdRecogedor "
                       "FROM Agentes WHERE Id=%1% AND Agentes.IdRecogedor IS NOT NULL")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            idRecogedor = strtoul(row[0], NULL, 10);
        }
        mysql_free_result(result);
        
        if (!idRecogedor) {                                         
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje,
                     "ADVERTENCIA: login - login <%s>@<%u>  ne se encontró el agente del usuario.",
                     login.login.getValue().c_str(), idAgente);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ERR_LOGINPASSWORD, 0, 0);
            return false;
        }
                            
        idAgente = idRecogedor;
    }
    return true;
}

bool
verificarTickets(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, MYSQL* mysql)
{
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
    return true;
}

bool
verificarTaquilla(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, MYSQL* mysql)
{    
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
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Taquillas SET Serial='1' WHERE Numero=%1% AND IdAgente=%2%")
        % unsigned(conexionActiva.idTaquilla) % login.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    }
    return true;
}

pthread_t
registrarLogin(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, MYSQL* mysql,
               boost::uint32_t idUsuario)
{
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
        sqlQuery.parse("UPDATE Taquillas SET Hilo=%1%, Hora_Ult_Conexion=FROM_UNIXTIME(%2%), "
                       "Version = '%3%' "
                       "WHERE IdAgente=%4% AND Numero=%5%")
        % (boost::uint32_t) pthread_self() % time(NULL) % login.versionTaquilla.getValue() 
        % login.idAgente % unsigned(conexionActiva.idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Usuarios SET Hilo=%1% WHERE Id=%2%")
        % unsigned(pthread_self())
        % idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_SOL);
    }
    return hilo;
}

void
cerrarConexionAnterior(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, pthread_t thread)
{
    if (!thread) {
        return;
    }
    SocketThread::MutexGuard g;
    SocketThread * socketsThread = SocketThread::get(thread);
    if (!socketsThread) {
        return;
    }
    ConexionActiva& conexion_activa2 = socketsThread->getConexionTaquilla();
    if (conexion_activa2.idAgente == login.idAgente
        and conexion_activa2.idTaquilla == conexionActiva.idTaquilla
        and conexion_activa2.sd != conexionActiva.sd) {
        shutdown(conexion_activa2.sd, SHUT_RDWR);
    }
}
 
void 
iniciarSesion(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login, 
              RespuestaLoginV2 const& respuestaLogin, boost::uint8_t idZona,
              bool usarProductos)
{                                            
    conexionActiva.idAgente = login.idAgente;
    conexionActiva.idUsuario = respuestaLogin.idUsuario;
    conexionActiva.idZona = idZona;
    conexionActiva.usarProductos = usarProductos;
    strcpy(conexionActiva.login, login.login.getValue().c_str());
    
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
    
void
atenderLoginV2(ConexionActiva& conexionActiva, SolicitudLoginV1 const& login)
{        
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    log_clientes(NivelLog::Bajo, &conexionActiva, "Solicitud de Login");

    if (!verificarAutentificacion(conexionActiva, login)) return;
        
    boost::uint8_t idZona;
    bool usarProductos;    
    if (!verificarAgencia(conexionActiva, login, mysql, idZona, usarProductos)) return;    
    
    boost::uint32_t idUsuario = 0;
    boost::uint8_t prioridadUsuario = 0;
    if (!verificarUsuario(conexionActiva, login, mysql, idUsuario, prioridadUsuario)) return;

    if (!verificarTickets(conexionActiva, login, mysql)) return;

    if (!verificarTaquilla(conexionActiva, login, mysql)) return;
    
    pthread_t thread = registrarLogin(conexionActiva, login, mysql, idUsuario);
    
    cerrarConexionAnterior(conexionActiva, login, thread);

    RespuestaLoginV2 respuestaLogin;  
    respuestaLogin.prioridad = prioridadUsuario;
    respuestaLogin.idUsuario = idUsuario;
    respuestaLogin.fechaHora = time(NULL) - 1800;
    respuestaLogin.impuesto_ganancia_fortuita = configuracion.impuesto_ganancia_fortuita;
    respuestaLogin.usar_productos = usarProductos;    
    std::vector<boost::uint8_t> buff = respuestaLogin.toRawBuffer();    
    send2cliente(conexionActiva, LOGIN_V_2, &buff[0], buff.size());
    
    iniciarSesion(conexionActiva, login, respuestaLogin, idZona, usarProductos);
}                                            
