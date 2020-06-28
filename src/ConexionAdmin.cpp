#include <ConexionAdmin.h>
#include <global.h>
#include <utils.h>
#include <sockets.h>
#include <sorteo.h>
#include <atender_peticion.h>
#include <database/DataBaseConnetionPool.h>
#include <SocketThread.h>
#include <TipoDeMontoDeAgencia.h>

#include <ace/Guard_T.h>
#include <ace/Task.h>

#include <unistd.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <sstream>

#include <ProtocoloZeus/Mensajes.h>
#include <ProtocoloZeus/Protocolo.h>
#include <ProtocoloZeus/Implementacion/ProtocoloUtils.h>

#include <ProtocoloExterno/Mensajes/MensajeFactory.h>
#include <ProtocoloExterno/Exceptions/ServiceNoFoundException.h>
#include <ProtocoloExterno/Exceptions/ReponseException.h>

#include <CierreSorteoZeus/controlador/ControlResumen.h>

#include <iostream>

using namespace ProtocoloZeus;

#define DEBUG_LOGIN_ADMIN_SOL 0
#define DEBUG_SORTEO_MOD_ST 0
#define DEBUG_SORTEO_MOD_H_SOL 0
#define DEBUG_TAQUILLA_CONECT_BUSCAR 0
#define DEBUG_AGENCIA_MOD_ST 0
#define DEBUG_NUMERO_REST 0
#define DEBUG_TICKET_ELIM 0
#define DEBUG_LIMITE 0
#define DEBUG_TAQUILLA_NUEVA 0
#define DEBUG_TAQUILLA_INSTALAR 0
#define DEBUG_TAREA_PERMITIDA 0
#define DEBUG_ATENDER_PETICION_ADMIN 0
#define DEBUG_CARGAR_TIPOS_MONTO 0
#define DEBUG_CALCULAR_RESUMENES 0
#define DEBUG_VERIFICAR_SORTEOS 0
#define DEBUG_CALCULAR_SALDOS 0

namespace ZeusServicioAdmin
{
    
ConexionAdmin::ConexionAdmin()
{
}

ConexionAdmin::~ConexionAdmin()
{
}

int 
ConexionAdmin::getSd() const
{
    return sd;
}

void
ConexionAdmin::close()
{
    ::shutdown(sd, SHUT_RDWR);
    ::close(sd);
}


bool 
ConexionAdmin::isConnected() const
{
    return sd != 0;
}

bool 
ConexionAdmin::isSet(fd_set* set) const
{
    return FD_ISSET(sd, set);
}

std::vector<boost::uint8_t> 
ConexionAdmin::getMessage()
{
    log_mensaje(NivelLog::Debug, "debug", "servidor/admin", "ConexionAdmin::getMessage()");
    return readSocket(*this);
}

std::string 
ConexionAdmin::getIpAddress() const
{
    return ipAddress;
}

void 
ConexionAdmin::sendClose()
{
    send2cliente(*this, ProtocoloZeus::DESCONECCION, NULL, 0);
}

std::auto_ptr<ConexionAdmin> 
ConexionAdmin::accept(int socket)
{
    sockaddr_in sockaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    memset(&sockaddr, 0, socklen);

    ConexionAdmin* c = new ConexionAdmin;
    std::auto_ptr<ConexionAdmin> conexion(c);
    conexion->sd = ::accept(socket, reinterpret_cast<struct sockaddr*>(&sockaddr), &socklen);

    if (conexion->sd <= 0) {
        log_mensaje(NivelLog::Bajo, "advertencia", "servidor/admin",
                     "No se pudo aceptar conexion: accept()");
        return std::auto_ptr<ConexionAdmin>();                     
    }
    
    int socketOption = 1;
    setsockopt(conexion->sd, SOL_SOCKET, SO_REUSEADDR, &socketOption, sizeof(socketOption));

    conexion->ipAddress =  inet_ntoa(sockaddr.sin_addr);

    std::ostringstream oss;
    oss << "Conexion abierta - socket admin : " << conexion->sd;
    log_mensaje(NivelLog::Detallado, "mensaje",  conexion->ipAddress.c_str(), oss.str().c_str());   
    return conexion;    
}    

class AdminTask : public ACE_Task_Base
{
    ConexionAdmin& conexion;
    std::vector<boost::uint8_t> buffer;

public:      
    virtual ~AdminTask() {}
      
    AdminTask(ConexionAdmin& conexion_, std::vector<boost::uint8_t> const& buffer_)
        : conexion(conexion_), buffer(buffer_) 
    {}
                
    virtual int svc() 
    {
        log_mensaje(NivelLog::Debug, "debug",  
                    conexion.ipAddress.c_str(), 
                    "AdminTask.svc()");
        conexion.dipatchTask(buffer); 
        return 0;
    }  
    
    virtual int close(u_long flags) 
    {
        int result = ACE_Task_Base::close(flags);
        if (thr_count() == 0) {
            delete this;
        }
        return result;
    }
};

void 
ConexionAdmin::dipatch(std::vector<boost::uint8_t> const& vectorBuffer)
{
    AdminTask* task = new AdminTask(*this, vectorBuffer);
    task->open();
    task->activate(THR_NEW_LWP|THR_INHERIT_SCHED|THR_DETACHED);
}

login_admin_t
dipatch_LOGIN_ADMIN(ConexionAdmin * conexion, login_admin_sol_t login)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
                
    bool agenteExiste = false;
    bool agenteEstaActivo = false;
    {
        /*
        * hay que validar que el agente sea banca
        */
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Estado FROM Agentes WHERE Id=%1% AND IdRecogedor IS NULL")
        % login.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_ADMIN_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            agenteExiste = true;
            agenteEstaActivo = (strcmp(row[0], "1") == 0) ? true : false;
        }
        mysql_free_result(result);
    }

    if (not agenteExiste) {
        char gmensaje[BUFFER_SIZE];
        sprintf(gmensaje, "ADVERTENCIA: login - el agente <%u> no es banca.", login.idAgente);
        log_admin(NivelLog::Bajo, conexion, gmensaje);
        send2cliente(*conexion, ERR_AGENTENOBANCA, NULL, 0);
        login_admin_t loginresp;
        loginresp.idUsuario = 0;
        return loginresp;
    }

    if (not agenteEstaActivo) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje,
                 "ADVERTENCIA: login - agente <%u> desactivado por razones administrativas.",
                 login.idAgente);
        log_admin(NivelLog::Bajo, conexion, mensaje);
        send2cliente(*conexion, ERR_AGENCIANOACT, NULL, 0);
        login_admin_t loginresp;
        loginresp.idUsuario = 0;
        return loginresp;
    }

    /*
     * hay que buscar en la BD si el usuario existe y verificar su
     * password y estado
     */
    bool usuarioExiste = false;
    bool usuarioEstaActivo = false;
    boost::uint32_t idUsuario = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Estado, Id FROM Usuarios WHERE Login='%1%'AND Password='%2%' AND IdAgente=%3%")
        % login.login % login.password % login.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_LOGIN_ADMIN_SOL);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            usuarioExiste = true;
            usuarioEstaActivo = (strcmp(row[0], "1") == 0) ? true : false;
            idUsuario = atoi(row[1]);
        }
        mysql_free_result(result);
    }

    if (not usuarioExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: login - login <%s>@<%u>  no existe o clave incorrecta.",
                 login.login, login.idAgente);
        log_admin(NivelLog::Bajo, conexion, mensaje);
        send2cliente(*conexion, ERR_LOGINPASSWORD, NULL, 0);
        login_admin_t loginresp;
        loginresp.idUsuario = 0;
        return loginresp;
    }

    if (not usuarioEstaActivo) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA: login - el usuario con login <%s>@<%u> no esta activo.",
                 login.login, login.idAgente);
        log_admin(NivelLog::Bajo, conexion, mensaje);
        send2cliente(*conexion, ERR_USUARIONOACT, NULL, 0);
        login_admin_t loginresp;
        loginresp.idUsuario = 0;
        return loginresp;
    }

    strcpy(conexion->login, login.login);

    login_admin_t loginresp;
    loginresp.idUsuario = idUsuario;

    return loginresp;
}

sorteo_mod_res_t
sorteo_mod_st(ConexionAdmin * conexion, sorteo_mod_st_t sorteo_mod)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
                
    sorteo_mod_res_t sorteo_mod_res;
    sorteo_mod_res.idSorteo = sorteo_mod.idSorteo;

    Sorteo * sorteo = (Sorteo*)g_tree_lookup(G_sorteos, &sorteo_mod.idSorteo);

    if (sorteo == NULL) {
        sorteo_mod_res.estado = ERR_SORTEONOEXIS;
        return sorteo_mod_res;
    }
    
    struct tm tm_t = fecha_i2t(sorteo_mod.fecha_final);
     
    /*
     * Activar sorteo
     */
    if (not sorteo->isEstadoActivo()) {
        if (sorteo->getHoraCierre() >= hora_actual()) {
            
            sorteo->setEstadoActivo();
            
            if (not sorteo->isEstadoActivo()) {
                sorteo_mod_res.estado = ERR_SORTEONOEXIS;
                return sorteo_mod_res;
            }
        }
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Sorteos SET FechaSuspHasta=NULL WHERE Id=%1%")
        % unsigned(sorteo_mod.idSorteo);

        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEO_MOD_ST);
    
    /*
     * Desactivar sorteo
     */
    } else {
        sorteo->setEstadoActivo(false);
        if (sorteo_mod.fecha_final == 0) {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Sorteos SET FechaSuspHasta=NULL WHERE Id=%u")
            % unsigned(sorteo_mod.idSorteo);
            ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEO_MOD_ST);
        } else {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Sorteos SET FechaSuspHasta='%1%-%2%-%3%'  WHERE Id=%4%")
            % tm_t.tm_year % tm_t.tm_mon % tm_t.tm_mday % unsigned(sorteo_mod.idSorteo);
            ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEO_MOD_ST);
        }
    }

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: estado del sorteo <%u> mofificado con exito a <%u>",
                 sorteo_mod.idSorteo, sorteo->isEstadoActivo());
        if (not sorteo->isEstadoActivo() and sorteo_mod.fecha_final != 0) {
            sprintf(mensaje, " hasta el <%u/%u/%u>", tm_t.tm_mday, tm_t.tm_mon, tm_t.tm_year);
        }
        strcat(mensaje, ".");
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    sorteo_mod_res.estado = OK;
    return sorteo_mod_res;
}

/*--------------------------------------------------------------------------------*/

sorteo_mod_res_t
sorteo_mod_h_sol(ConexionAdmin * conexion, sorteo_mod_h_t sorteo_mod)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    sorteo_mod_res_t sorteo_mod_res;
    sorteo_mod_res.idSorteo = sorteo_mod.idSorteo;

    Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &sorteo_mod.idSorteo);
    if (sorteo == NULL) {
        sorteo_mod_res.estado = ERR_SORTEONOEXIS;
        return sorteo_mod_res;
    }

    sorteo->setHoraCierre(sorteo_mod.nueva_hora);

    if (sorteo->getHoraCierre() < hora_actual()) {
        sorteo->setEstadoActivo(false);
    }

    struct tm tm_t = hora_i2t(sorteo_mod.nueva_hora);

    if (sorteo_mod.fecha_final != 0) {
        tm_t = fecha_i2t(sorteo_mod.fecha_final);

        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Sorteos "
                        "SET ModHora='%d:%d:0', FechaModHoraHasta='%u-%u-%u' "
                        "WHERE Id=%u")
        % tm_t.tm_hour % tm_t.tm_min % tm_t.tm_year % tm_t.tm_mon % tm_t.tm_mday
        % unsigned(sorteo_mod.idSorteo);

        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEO_MOD_H_SOL);

    } else {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Sorteos "
                        "SET ModHora='%d:%d:0', FechaModHoraHasta=NULL "
                        "WHERE Id=%u")
        % tm_t.tm_hour % tm_t.tm_min % unsigned(sorteo_mod.idSorteo);

        ejecutar_sql(mysql, sqlQuery, DEBUG_SORTEO_MOD_H_SOL);
    }

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: hora de cierre del sorteo <%u> mofificada con exito a <%d:%d>",
                 sorteo_mod.idSorteo, tm_t.tm_hour, tm_t.tm_min);
        if (sorteo_mod.fecha_final != 0) {
            sprintf(mensaje, " hasta el <%u/%u/%u>", tm_t.tm_mday, tm_t.tm_mon, tm_t.tm_year);
        }
        strcat(mensaje, ".");
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    sorteo_mod_res.estado = OK;
    return sorteo_mod_res;
}

/*--------------------------------------------------------------------------------*/

void
taquilla_conect_buscar(taquilla_conect_sol_t taquilla_conect_sol, std::vector<taquilla_conect_t> & taquillas)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    std::vector<std::pair<boost::uint8_t, boost::uint32_t> > taquillasHilos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Numero, Hilo FROM Taquillas WHERE Hilo IS NOT NULL AND IdAgente=%1%")
        % taquilla_conect_sol.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_TAQUILLA_CONECT_BUSCAR);
        MYSQL_RES *result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            taquillasHilos.push_back(std::make_pair(atoi(row[0]),
                                                      (boost::uint32_t) strtoul(row[1], NULL, 10)));
        }
        mysql_free_result(result);
    }

    if (taquillasHilos.size() == 0) {
        return ;
    }

    for (std::size_t i = 0, size = taquillasHilos.size(); i < size ; ++i) {
        taquilla_conect_t taquilla_conect;
        taquilla_conect.numero = taquillasHilos[i].first;
        taquilla_conect.idAgente = taquilla_conect_sol.idAgente;
        boost::uint32_t hilo = taquillasHilos[i].second;

        bool encontrado = false;
        {
            SocketThread::MutexGuard g;
            SocketThread * socketsThread = SocketThread::get(hilo);
            if (socketsThread != NULL) {
                ConexionActiva& conexionActiva = socketsThread->getConexionTaquilla();
                if (conexionActiva.idAgente == taquilla_conect.idAgente 
                    && conexionActiva.idTaquilla == taquilla_conect.numero) {
                    taquillas.push_back(taquilla_conect);
                    encontrado = true;
                }
            } else {
                char mensaje[BUFFER_SIZE];
                sprintf(mensaje,
                         "taquilla_conect_buscar: hilo %u no encontrado en socketsThreads",
                         hilo);
                log_admin(NivelLog::Detallado, NULL, mensaje);
            }
        }

        if (not encontrado) {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Taquillas SET Hilo=NULL WHERE IdAgente=%1% AND Numero=%2%")
            % taquilla_conect.idAgente % unsigned(taquilla_conect.numero);
            ejecutar_sql(mysql, sqlQuery, DEBUG_TAQUILLA_CONECT_BUSCAR);
        }
    }
}

/*--------------------------------------------------------------------------------*/

agencia_mod_res_t
agencia_mod_st(ConexionAdmin * conexion, agencia_mod_st_t agencia_mod)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    agencia_mod_res_t agencia_mod_res;
    agencia_mod_res.idAgente = agencia_mod.idAgente;
    boost::uint32_t idAgente = agencia_mod.idAgente;

    bool agenteExiste = false;
    bool agenteEstaActivo = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Estado From Agentes WHERE Id=%1%")
        % agencia_mod.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_AGENCIA_MOD_ST);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            agenteExiste = true;
            agenteEstaActivo = (atoi(row[0]) == 1);
        }
        mysql_free_result(result);
    }

    //if (not agenteExiste) {} <<< FIXME: hay que agregar manejo de esta condici�.

    if (not agenteEstaActivo) {
        {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Agentes SET Estado=1 WHERE Id=%1%")
            % agencia_mod.idAgente;
            ejecutar_sql(mysql, sqlQuery, DEBUG_AGENCIA_MOD_ST);
        }

        agenteEstaActivo = true;

    } else {
        {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Agentes SET Estado=2 WHERE Id=%1%")
            % agencia_mod.idAgente;
            ejecutar_sql(mysql, sqlQuery, DEBUG_AGENCIA_MOD_ST);
        }

        agenteEstaActivo = false;

        std::vector<std::pair<boost::uint8_t, pthread_t> > taquillasHilos;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Numero, Hilo FROM Taquillas WHERE Hilo IS NOT NULL AND IdAgente=%1%")
            % agencia_mod.idAgente;
            ejecutar_sql(mysql, sqlQuery, DEBUG_TAQUILLA_CONECT_BUSCAR);
            MYSQL_RES *result = mysql_store_result(mysql);
            for (MYSQL_ROW row = mysql_fetch_row(result);
                    NULL != row;
                    row = mysql_fetch_row(result)) {
                taquillasHilos.push_back(std::make_pair(atoi(row[0]),
                                                          strtoul(row[1], NULL, 10)));
            }
            mysql_free_result(result);
        }

        for (std::size_t i = 0, size = taquillasHilos.size(); i < size ; ++i) {

            boost::uint8_t ntaquilla = taquillasHilos[i].first;
            pthread_t hilo = taquillasHilos[i].second;

            idAgente = agencia_mod.idAgente;

            boost::uint64_t agente_taquilla = idAgente;
            agente_taquilla = agente_taquilla << 32;
            agente_taquilla += ntaquilla;

            n_t_preventas_x_taquilla_t * preventas_taquilla =
                (n_t_preventas_x_taquilla_t *)
                g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
            if (preventas_taquilla != NULL) {
                sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
                preventas_taquilla->hora_ultimo_renglon = 0;
                sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
            }
            bool encontrado = false;
            {
                SocketThread::MutexGuard g;
                SocketThread * socketsThread = SocketThread::get(hilo);
                
                if (socketsThread != NULL) {
                    ConexionActiva& conexion_activa_cliente = socketsThread->getConexionTaquilla();
                    if ((conexion_activa_cliente.idAgente == idAgente)
                            and (conexion_activa_cliente.idTaquilla == ntaquilla)) {
                        send2cliente(conexion_activa_cliente, DESCONECCION, NULL, 0);
                        shutdown(conexion_activa_cliente.sd, SHUT_RDWR);
                        encontrado = true;
                    }                
                }
            }
            if (not encontrado) {
                boost::format sqlQuery;
                sqlQuery.parse("UPDATE Taquillas SET Hilo=NULL WHERE IdAgente=%1% AND Numero=%2%")
                % idAgente % unsigned(ntaquilla);
                ejecutar_sql(mysql, sqlQuery, DEBUG_AGENCIA_MOD_ST);
            }
        }
    }
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: el estado de la agencia <%u> mofificado con exito a <%u>.",
                 idAgente, agenteEstaActivo);
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    agencia_mod_res.estado = OK;
    return agencia_mod_res;
}

/*--------------------------------------------------------------------------------*/

numero_rest_res_t
numero_rest(ConexionAdmin * conexion, numero_rest_t num_rest)
{    
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    numero_rest_res_t numero_rest_res;
    numero_rest_res.idSorteo = num_rest.idSorteo;
    numero_rest_res.idZona = num_rest.idZona;
    numero_rest_res.tipo = num_rest.tipo;
    numero_rest_res.numero = num_rest.numero;

    bool restringidoExiste = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT * FROM Restringidos "
                        "WHERE IdSorteo=%1% AND IdZona=%2% AND Numero=%3% AND Tipo=%4%")
        % unsigned(num_rest.idSorteo) % unsigned(num_rest.idZona)
        % unsigned(num_rest.numero) % unsigned(num_rest.tipo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_NUMERO_REST);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            restringidoExiste = true;
        }
        mysql_free_result(result);
    }

    struct tm tm_t = fecha_i2t(num_rest.fecha_hasta);

    n_t_zona_numero_restringido_t * zona =
        (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido,
                                                           (boost::uint8_t *) & num_rest.idZona);
    if (zona == NULL) {
        zona = new n_t_zona_numero_restringido_t;
        sem_init(&zona->sem_t_sorteo_numero_restringido, 0, 1);
        zona->t_sorteo_numero_restringido = g_tree_new(uint8_t_comp_func);
        g_tree_insert(t_zona_numero_restringido, (boost::uint8_t *) & num_rest.idZona, zona);
    }

    sem_wait(&zona->sem_t_sorteo_numero_restringido);

    n_t_sorteo_numero_restringido_t * sorteo =
        (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona->t_sorteo_numero_restringido,
                                                             &num_rest.idSorteo);
    if (sorteo == NULL) {
        sorteo = new n_t_sorteo_numero_restringido_t;

        sorteo->t_numero_restringido_triple = g_tree_new(uint16_t_comp_func);
        sorteo->t_numero_restringido_terminal = g_tree_new(uint16_t_comp_func);
        g_tree_insert(zona->t_sorteo_numero_restringido, (boost::uint32_t *) & num_rest.idSorteo,
                       sorteo);
    }

    if (num_rest.tipo == TRIPLE or num_rest.tipo == TRIPLETAZO) {
        n_t_numero_restringido_t * numerorest =
            (n_t_numero_restringido_t *) g_tree_lookup(sorteo->t_numero_restringido_triple,
                                                          (boost::uint16_t *) & num_rest.numero);
        if (numerorest == NULL) {
            boost::uint16_t * num = new boost::uint16_t;
            *num = num_rest.numero;
            numerorest = new n_t_numero_restringido_t;
            numerorest->venta = 0;
            numerorest->preventa = 0;
            g_tree_insert(sorteo->t_numero_restringido_triple, (boost::uint16_t *) num, numerorest);
        }
        numerorest->proporcion = num_rest.porcventa;
        numerorest->proporcion /= 10000;
        numerorest->tope = 0;

    } else if (num_rest.tipo == TERMINAL or num_rest.tipo == TERMINALAZO) {
        n_t_numero_restringido_t * numerorest =
            (n_t_numero_restringido_t *) g_tree_lookup(sorteo->t_numero_restringido_terminal,
                                                          (boost::uint16_t *) & num_rest.numero);
        if (numerorest == NULL) {
            boost::uint16_t * num = new boost::uint16_t;
            *num = num_rest.numero;
            numerorest = new n_t_numero_restringido_t;
            numerorest->venta = 0;
            numerorest->preventa = 0;
            g_tree_insert(sorteo->t_numero_restringido_terminal, (boost::uint16_t *) num, numerorest);
        }
        numerorest->proporcion = num_rest.porcventa;
        numerorest->proporcion /= 10000;
        numerorest->tope = 0;
    }

    sem_post(&zona->sem_t_sorteo_numero_restringido);

    if (not restringidoExiste) {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Restringidos "
                        "SET PorcVenta=%1%, FechaHasta='%2%-%3%-%4%', "
                        "IdSorteo=%5%, IdZona=%6%, Numero=%7%, Tipo=%8%")
        % num_rest.porcventa % tm_t.tm_year % tm_t.tm_mon % tm_t.tm_mday
        % unsigned(num_rest.idSorteo) % unsigned(num_rest.idZona)
        % unsigned(num_rest.numero) % unsigned(num_rest.tipo);

        ejecutar_sql(mysql, sqlQuery, DEBUG_NUMERO_REST);

    } else {
        if (num_rest.porcventa < 10000) {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Restringidos "
                            "SET PorcVenta=%1%, FechaHasta='%2%-%3%-%4%' "
                            "WHERE IdSorteo=%5% AND IdZona=%6% AND Numero=%7% AND Tipo=%8%")
            % num_rest.porcventa % tm_t.tm_year % tm_t.tm_mon % tm_t.tm_mday
            % unsigned(num_rest.idSorteo) % unsigned(num_rest.idZona)
            % unsigned(num_rest.numero) % unsigned(num_rest.tipo);
    
            ejecutar_sql(mysql, sqlQuery, DEBUG_NUMERO_REST);
        } else {
            boost::format query;
            query.parse("delete from Restringidos where "
                         "Numero=%1% and Tipo=%2% and IdSorteo=%3% and IdZona=%4% ")
            % unsigned(num_rest.numero) 
            % unsigned(num_rest.tipo)
            % unsigned(num_rest.idSorteo) 
            % unsigned(num_rest.idZona);    
            
            ejecutar_sql(mysql, query, DEBUG_NUMERO_REST);
        }
    }

    {
        char mensaje[BUFFER_SIZE];
        if (restringidoExiste and num_rest.porcventa >= 10000) {
            strcpy(mensaje, "MENSAJE: se elimino la restriccion del ");
        } else {
            strcpy(mensaje, "MENSAJE: se restringio el ");
        }

        if (num_rest.tipo == TERMINAL) {
            strcat(mensaje, "terminal: ");

        } else if (num_rest.tipo == TRIPLE) {
            strcat(mensaje, "triple: ");
        }

        sprintf(mensaje, "%s%u, sorteo: %u, zona: %u, hasta el %u/%u/%u. proporcion: %u", 
                mensaje,
                num_rest.numero,
                num_rest.idSorteo, num_rest.idZona, tm_t.tm_mday, tm_t.tm_mon, tm_t.tm_year,
                num_rest.porcventa);
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    numero_rest_res.estado = OK;
    return numero_rest_res;
}


numero_rest_res_t
numero_rest_tope(ConexionAdmin * conexion, numero_rest_t num_rest)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    numero_rest_res_t numero_rest_res;
    numero_rest_res.idSorteo = num_rest.idSorteo;
    numero_rest_res.idZona = num_rest.idZona;
    numero_rest_res.tipo = num_rest.tipo;
    numero_rest_res.numero = num_rest.numero;

    bool restringidoExiste = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT * FROM Restringidos "
                        "WHERE IdSorteo=%1% AND IdZona=%2% AND Numero=%3% AND Tipo=%4%")
        % unsigned(num_rest.idSorteo) % unsigned(num_rest.idZona)
        % unsigned(num_rest.numero) % unsigned(num_rest.tipo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_NUMERO_REST);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            restringidoExiste = true;
        }
        mysql_free_result(result);
    }

    struct tm tm_t = fecha_i2t(num_rest.fecha_hasta);

    n_t_zona_numero_restringido_t * zona =
        (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido,
                                                           (boost::uint8_t *) & num_rest.idZona);
    if (zona == NULL) {
        zona = new n_t_zona_numero_restringido_t;
        sem_init(&zona->sem_t_sorteo_numero_restringido, 0, 1);
        zona->t_sorteo_numero_restringido = g_tree_new(uint8_t_comp_func);
        g_tree_insert(t_zona_numero_restringido, (boost::uint8_t *) & num_rest.idZona, zona);
    }

    sem_wait(&zona->sem_t_sorteo_numero_restringido);

    n_t_sorteo_numero_restringido_t * sorteo =
        (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona->t_sorteo_numero_restringido,
                                                             &num_rest.idSorteo);
    if (sorteo == NULL) {
        sorteo = new n_t_sorteo_numero_restringido_t;

        sorteo->t_numero_restringido_triple = g_tree_new(uint16_t_comp_func);
        sorteo->t_numero_restringido_terminal = g_tree_new(uint16_t_comp_func);
        g_tree_insert(zona->t_sorteo_numero_restringido, (boost::uint32_t *) & num_rest.idSorteo,
                       sorteo);
    }

    if (num_rest.tipo == TRIPLE or num_rest.tipo == TRIPLETAZO) {
        n_t_numero_restringido_t * numerorest =
            (n_t_numero_restringido_t *) g_tree_lookup(sorteo->t_numero_restringido_triple,
                                                          (boost::uint16_t *) & num_rest.numero);
        if (numerorest == NULL) {
            boost::uint16_t * num = new boost::uint16_t;
            *num = num_rest.numero;
            numerorest = new n_t_numero_restringido_t;
            numerorest->venta = 0;
            numerorest->preventa = 0;
            g_tree_insert(sorteo->t_numero_restringido_triple, (boost::uint16_t *) num, numerorest);
        }
        numerorest->proporcion = 1;
        numerorest->tope = num_rest.porcventa;

    } else if (num_rest.tipo == TERMINAL or num_rest.tipo == TERMINALAZO) {
        n_t_numero_restringido_t * numerorest =
            (n_t_numero_restringido_t *) g_tree_lookup(sorteo->t_numero_restringido_terminal,
                                                          (boost::uint16_t *) & num_rest.numero);
        if (numerorest == NULL) {
            boost::uint16_t * num = new boost::uint16_t;
            *num = num_rest.numero;
            numerorest = new n_t_numero_restringido_t;
            numerorest->venta = 0;
            numerorest->preventa = 0;
            g_tree_insert(sorteo->t_numero_restringido_terminal, (boost::uint16_t *) num, numerorest);
        }
        numerorest->proporcion = 1;
        numerorest->tope = num_rest.porcventa;
    }

    sem_post(&zona->sem_t_sorteo_numero_restringido);

    if (not restringidoExiste) {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Restringidos "
                        "SET Tope=%1%, FechaHasta='%2%-%3%-%4%', "
                        "IdSorteo=%5%, IdZona=%6%, Numero=%7%, Tipo=%8%")
        % num_rest.porcventa % tm_t.tm_year % tm_t.tm_mon % tm_t.tm_mday
        % unsigned(num_rest.idSorteo) % unsigned(num_rest.idZona)
        % unsigned(num_rest.numero) % unsigned(num_rest.tipo);

        ejecutar_sql(mysql, sqlQuery, DEBUG_NUMERO_REST);

    } else {
        if (num_rest.porcventa < 10000) {
            boost::format sqlQuery;
            sqlQuery.parse("UPDATE Restringidos "
                            "SET Tope=%1%, FechaHasta='%2%-%3%-%4%' "
                            "WHERE IdSorteo=%5% AND IdZona=%6% AND Numero=%7% AND Tipo=%8%")
            % num_rest.porcventa % tm_t.tm_year % tm_t.tm_mon % tm_t.tm_mday
            % unsigned(num_rest.idSorteo) % unsigned(num_rest.idZona)
            % unsigned(num_rest.numero) % unsigned(num_rest.tipo);
    
            ejecutar_sql(mysql, sqlQuery, DEBUG_NUMERO_REST);
        } else {
            boost::format query;
            query.parse("delete from Restringidos where "
                         "Numero=%1% and Tipo=%2% and IdSorteo=%3% and IdZona=%4% ")
            % unsigned(num_rest.numero) 
            % unsigned(num_rest.tipo)
            % unsigned(num_rest.idSorteo) 
            % unsigned(num_rest.idZona);    
            
            ejecutar_sql(mysql, query, DEBUG_NUMERO_REST);
        }
    }

    {
        char mensaje[BUFFER_SIZE];
        if (restringidoExiste and num_rest.porcventa >= 10000) {
            strcpy(mensaje, "MENSAJE: se elimino la restriccion del ");
        } else {
            strcpy(mensaje, "MENSAJE: se restringio el ");
        }

        if (num_rest.tipo == TERMINAL) {
            strcat(mensaje, "terminal: ");

        } else if (num_rest.tipo == TRIPLE) {
            strcat(mensaje, "triple: ");
        }

        sprintf(mensaje, "%s%u, sorteo: %u, zona: %u, hasta el %u/%u/%u. tope: %u", 
                mensaje,
                num_rest.numero,
                num_rest.idSorteo, num_rest.idZona, tm_t.tm_mday, tm_t.tm_mon, tm_t.tm_year,
                num_rest.porcventa);
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    numero_rest_res.estado = OK;
    return numero_rest_res;
}

/*--------------------------------------------------------------------------------*/

struct ticket_elim_data
{
    boost::uint8_t idSorteo;
    boost::uint32_t idAgente;
    boost::uint16_t numero;
    boost::uint8_t tipo;
    boost::int32_t monto;
    boost::uint8_t idTipoMontoTriple;
    boost::uint8_t idTipoMontoTerminal;
    boost::uint8_t estadoTicket;
};

ticket_elim_res_t
ticket_elim(ConexionAdmin * conexion, ticket_elim_t ticket_a_elim)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    ticket_elim_res_t ticket_elim_res;
    ticket_elim_res.idTicket = ticket_a_elim.idTicket;

    std::vector<ticket_elim_data> datosParaEliminar;
    {
        time_t tim = time(NULL);
        struct tm t = *localtime(&tim);
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Renglones.IdSorteo, Tickets.IdAgente, Renglones.Numero, Renglones.Tipo, "
                        "Renglones.Monto, LimitesPorc.IdMontoTriple, LimitesPorc.IdMontoTerminal, Tickets.Estado "
                        "FROM Tickets, Renglones, LimitesPorc, Sorteos "
                        "WHERE Tickets.Id=%u AND Tickets.Id=Renglones.IdTicket "
                        "AND LimitesPorc.IdAgente=Tickets.IdAgente AND LimitesPorc.IdSorteo=Sorteos.Id "
                        "AND DATE_FORMAT(Tickets.Fecha,'%cY-%cm-%cd')='%d-%02d-%02d' "
                        "AND Tickets.Estado = %d AND  Sorteos.Id=Renglones.IdSorteo "
                        "ORDER BY Renglones.Id")
        % ticket_a_elim.idTicket % '%' % '%' % '%'
        % (t.tm_year + 1900) % (t.tm_mon + 1) % t.tm_mday % unsigned(NORMAL);
        ejecutar_sql(mysql, sqlQuery, DEBUG_TICKET_ELIM);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {

            ticket_elim_data datoParaEliminar;

            datoParaEliminar.idSorteo = atoi(row[0]);
            datoParaEliminar.idAgente = atoi(row[1]);
            datoParaEliminar.numero = atoi(row[2]);
            datoParaEliminar.tipo = atoi(row[3]);
            datoParaEliminar.monto = atoi(row[4]);
            datoParaEliminar.idTipoMontoTriple = atoi(row[5]);
            datoParaEliminar.idTipoMontoTerminal = atoi(row[6]);
            datoParaEliminar.estadoTicket = atoi(row[7]);

            datosParaEliminar.push_back(datoParaEliminar);
        }
        mysql_free_result(result);
    }

    if (datosParaEliminar.size() == 0) {
        // No existe el ticket o esta vacio
        ticket_elim_res.estado = ERR_TICKORENGNOEXIS;
        return ticket_elim_res;
    }    
    
    std::auto_ptr<ProtocoloExterno::AnularTicket> 
    anularExterno = ProtocoloExterno::MensajeFactory::getInstance()
                  . crearAnularTicket(mysql, ticket_a_elim.idTicket, ProtocoloExterno::AnulacionManual);
            
    try {
        (*anularExterno)(mysql);
        
    } catch (ProtocoloExterno::ServiceNoFoundException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, imposible conectar con el servidor local de enlace: %s", 
                e.what());
        log_admin(NivelLog::Bajo, conexion, mensaje);
        ticket_elim_res.estado = ERR_TICKETNOELIM;
        return ticket_elim_res;
    } catch (ProtocoloExterno::ReponseException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error retornado por el servidor externo: %s", e.what());
        log_admin(NivelLog::Bajo, conexion, mensaje);
        ticket_elim_res.estado = ERR_TICKETNOELIM;
        return ticket_elim_res;
    } catch (...) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error desconocido en protocolo externo");
        log_admin(NivelLog::Bajo, conexion, mensaje);
        ticket_elim_res.estado = ERR_TICKETNOELIM;
        return ticket_elim_res;
    }    

    GArray * a_renglon = g_array_new(false, true, sizeof(n_a_renglon_t));
    g_array_set_size(a_renglon, datosParaEliminar.size());

    boost::uint32_t idAgente = 0;
    boost::uint8_t estadoTicket = 0;

    for (std::size_t i = 0, size = datosParaEliminar.size(); i < size ; ++i) {
        n_a_renglon_t n_a_renglon;
        n_a_renglon.idSorteo = datosParaEliminar[i].idSorteo;

        Sorteo * sorteo =
            (Sorteo *) g_tree_lookup(G_sorteos, (boost::uint8_t *) & n_a_renglon.idSorteo);

        if (hora_actual() > sorteo->getHoraCierre()) {
            sorteo->setEstadoActivo(false);
        }
        if (sorteo->isEstadoActivo() == false) {
            // El ticket contiene renglones con sorteos cerrados
            ticket_elim_res.estado = ERR_TICKETNOELIM;
            return ticket_elim_res;
        }
        if (idAgente == 0) {
            idAgente = datosParaEliminar[i].idAgente;
        }
        if (estadoTicket == 0) {
            estadoTicket = datosParaEliminar[i].estadoTicket;
        }
        n_a_renglon.numero = datosParaEliminar[i].numero;
        n_a_renglon.tipo = datosParaEliminar[i].tipo;
        n_a_renglon.monto = datosParaEliminar[i].monto;

        if (n_a_renglon.tipo == TRIPLE or n_a_renglon.tipo == BONO
                or n_a_renglon.tipo == TRIPLETAZO or n_a_renglon.tipo == BONOTRIPLETAZO) {
            n_a_renglon.idTipoMonto = datosParaEliminar[i].idTipoMontoTriple;

        } else if (n_a_renglon.tipo == TERMINAL or n_a_renglon.tipo == TERMINALAZO) {
            n_a_renglon.idTipoMonto = datosParaEliminar[i].idTipoMontoTerminal;
        }
        g_array_index(a_renglon, n_a_renglon_t, i) = n_a_renglon;
    }

    for (std::size_t i = 0, size = datosParaEliminar.size(); i < size ; ++i) {
        n_a_renglon_t n_a_renglon = g_array_index(a_renglon, n_a_renglon_t, i);
        Sorteo * sorteo =
            (Sorteo *) g_tree_lookup(G_sorteos, (boost::uint8_t *) & n_a_renglon.idSorteo);

        n_t_tipomonto_t * tipoMonto = NULL;
        if (n_a_renglon.tipo == TRIPLE or n_a_renglon.tipo == BONO
                or n_a_renglon.tipo == TRIPLETAZO or n_a_renglon.tipo == BONOTRIPLETAZO) {
            tipoMonto =
                (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(),
                                                     (boost::uint8_t *) & n_a_renglon.idTipoMonto);
        } else if (n_a_renglon.tipo == TERMINAL or n_a_renglon.tipo == TERMINALAZO) {
            tipoMonto =
                (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
                                                     (boost::uint8_t *) & n_a_renglon.idTipoMonto);
        } else {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Tipo de renglon %d no existe", n_a_renglon.tipo);
            log_admin(NivelLog::Bajo, conexion, mensaje);
            ticket_elim_res.estado = ERR_TICKETNOELIM;
            return ticket_elim_res;
        }

        /*-------------------------------------------------------------------------------------------*/
        sem_wait(&tipoMonto->sem_t_tipomonto);
        tipoMonto->venta_global -= n_a_renglon.monto;
        limite_tipomonto(tipoMonto, n_a_renglon.tipo);
        n_t_venta_numero_t * n_t_venta_numero = (n_t_venta_numero_t *)
                                                g_tree_lookup(tipoMonto->t_venta_numero,
                                                               &n_a_renglon.numero);
        n_t_venta_numero->venta -= n_a_renglon.monto;
        sem_post(&tipoMonto->sem_t_tipomonto);

        /*-------------------------------------------------------------------------------------------*/
    }

    ejecutar_sql(mysql, "BEGIN", DEBUG_TICKET_ELIM);

    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
        % unsigned(ELIMINADO) % time(NULL) % ticket_a_elim.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_TICKET_ELIM);
    }

    {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Cambio_Ticket VALUES(%1%, %1%, %2%, %3%)")
        % ticket_a_elim.idTicket % unsigned(estadoTicket) % conexion->idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_TICKET_ELIM);
    }

    ejecutar_sql(mysql, "COMMIT", DEBUG_TICKET_ELIM);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: eliminado ticket <%u>.", ticket_a_elim.idTicket);
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }
    ticket_elim_res.estado = OK;
    return ticket_elim_res;
}

/*--------------------------------------------------------------------------------*/

limite_t
limite(ConexionAdmin * conexion, limite_sol_t limite_sol)
{ 
    limite_t limit;
    limit.idSorteo = limite_sol.idSorteo;
    limit.idTipoMonto = limite_sol.idTipoMonto;
    limit.tipo = limite_sol.tipo;
    limit.venta = 0;
    Sorteo *sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &limit.idSorteo);

    if (sorteo == NULL) {
        /*
         * el sorteo no existe
         */
        limit.limite = ERR_SORTEONOEXIS;
        return limit;
    }
    n_t_tipomonto_t *tipoMonto = 0;
    if (limit.tipo == TRIPLE or limit.tipo == TRIPLETAZO) {
        tipoMonto =
            (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(),
                                                 &limit.idTipoMonto);
        if (tipoMonto == NULL) {
            /*
             * el tipoDeMonto no existe
             */
            limit.limite = ERR_TIPOMONTOTRINOEXIS;
            return limit;
        }
    } else if (limit.tipo == TERMINAL or limit.tipo == TERMINALAZO) {
        tipoMonto =
            (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
                                                 &limit.idTipoMonto);
        if (tipoMonto == NULL) {
            /*
             * el tipoDeMonto no existe
             */
            limit.limite = ERR_TIPOMONTOTRINOEXIS;
            return limit;
        }
    } else {
        char mensaje[BUFFER_SIZE];

        sprintf(mensaje, "Tipo de Numero no existe <%d>", limit.tipo);
        log_admin(NivelLog::Bajo, conexion, mensaje);
        limit.limite = ERR_TIPOMONTOTRINOEXIS;
        return limit;
    }
    limit.limite = tipoMonto->limite_actual;
    limit.venta = tipoMonto->venta_global;
    {
        char mensaje[BUFFER_SIZE];

        sprintf(mensaje, "Limite %d Venta %d Actual Sorteo %u Tipo Monto %d Tipo %d", limit.limite,
                 limit.venta, limit.idSorteo, limit.idTipoMonto, limit.tipo);
        log_admin(NivelLog::Detallado, conexion, mensaje);
    }
    return limit;
}

/*--------------------------------------------------------------------------------*/

taquilla_nueva_t
taquilla_nueva(ConexionAdmin * conexion, taquilla_nueva_sol_t taquilla_nueva_sol)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    taquilla_nueva_t taquilla_n;
    taquilla_n.idAgente = taquilla_nueva_sol.idAgente;
    taquilla_n.ntaquilla = taquilla_nueva_sol.ntaquilla;

    bool taquillaExiste = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT * FROM Taquillas WHERE IdAgente=%1% AND Numero=%2%")
        % taquilla_n.idAgente % unsigned(taquilla_n.ntaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_TAQUILLA_NUEVA);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            taquillaExiste = true;
        }
        mysql_free_result(result);
    }

    if (not taquillaExiste) {
        taquilla_n.estado = ERR_TAQUILLANOEXIS;
        return taquilla_n;
    }

    boost::uint64_t llave = taquilla_n.idAgente;
    llave = (llave << 32) + taquilla_n.ntaquilla;

    if (g_tree_lookup(t_preventas_x_taquilla, &llave) != NULL) {
        taquilla_n.estado = ERR_TAQUILLAEXIS;
        return taquilla_n;
    }

    n_t_preventas_x_taquilla_t * n_t_preventas_x_taquilla = new n_t_preventas_x_taquilla_t;
    n_t_preventas_x_taquilla->hora_ultimo_renglon = 0;
    n_t_preventas_x_taquilla->preventas = NULL;

    g_tree_insert(t_preventas_x_taquilla, &llave, n_t_preventas_x_taquilla);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "MENSAJE: taquilla <%u> de la agencia <%u> agregada con exito.",
                 taquilla_n.ntaquilla, taquilla_n.idAgente);
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    taquilla_n.estado = OK;
    return taquilla_n;
}

/*--------------------------------------------------------------------------------*/

bool
tarea_permitida(ConexionAdmin * conexion, char * tarea)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    bool tareaEsPermitida = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT * FROM Usuarios_Grupos_R,Tareas_Nodos_Grupos_R,Tareas "
                        "WHERE Usuarios_Grupos_R.IdGrupo=Tareas_Nodos_Grupos_R.IdGrupo "
                        "AND Tareas_Nodos_Grupos_R.IdTarea=Tareas.ID "
                        "AND Tareas.Nombre='%1%' AND Usuarios_Grupos_R.IdUsuario=%2%")
        % tarea % conexion->idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_TAREA_PERMITIDA);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            tareaEsPermitida = true;
        }
        mysql_free_result(result);
    }

    return tareaEsPermitida;
}

/*--------------------------------------------------------------------------------*/
struct lista_terminales_t
{
    boost::uint8_t a[3];
    boost::uint8_t b[3];
    boost::uint8_t c[3];
    boost::uint16_t d[1];
    boost::uint8_t x[3];
};

lista_terminales_t *
calcular_lista_terminales(boost::uint16_t triple)
{
    char cnumero[6], terminal[5];
    lista_terminales_t *lista_terminales;

    lista_terminales = new lista_terminales_t;

    sprintf(cnumero, "%03d", triple);
    /*
     * para tipo de pago a
     */
    sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
    lista_terminales->a[0] = atoi(terminal);
    sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
    lista_terminales->a[1] = atoi(terminal);
    sprintf(terminal, "%c%c", cnumero[0], cnumero[2]);
    lista_terminales->a[2] = atoi(terminal);
    /*
     * para tipo de pago b
     */
    sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
    lista_terminales->b[0] = atoi(terminal);
    sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
    lista_terminales->b[1] = atoi(terminal);
    sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
    lista_terminales->b[2] = atoi(terminal);
    /*
     * Para tipo de pago c
     */
    sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
    lista_terminales->c[0] = atoi(terminal);
    lista_terminales->c[1] = (lista_terminales->c[0] + 1 > 99 ? 00 : lista_terminales->c[0] + 1);
    lista_terminales->c[2] = (lista_terminales->c[0] - 1 < 0 ? 99 : lista_terminales->c[0] - 1);
    /*
     * Para tipo de pago d
     */
    sprintf(cnumero, "%05d", triple);
    sprintf(terminal, "%c%c%c%c", cnumero[0], cnumero[1], cnumero[3], cnumero[4]);
    lista_terminales->d[0] = atoi(terminal);

    return lista_terminales;
}

/*--------------------------------------------------------------------------------*/

void
cargar_online_tipos_montoxagencia(boost::uint8_t const* buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    boost::uint32_t idAgente;
    memcpy(&idAgente, buff, sizeof(idAgente));

    TipoDeMontoPorAgencia::MutexGuard g;
    TipoDeMontoPorAgencia * tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);

    if (tipomonto_agencia == NULL) {
        cargar_tipomonto_x_sorteo(idAgente, mysql);

    } else {

        std::vector<std::pair<boost::uint8_t, TipoDeMonto*> > limitesPorcentajes;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT IdSorteo, IdMontoTriple, IdMontoTerminal, LimDiscTrip, LimDiscTerm, "
                            "IdFormaPago, PorcTrip, PorcTerm, Comodin, MontoAdd "
                            "FROM LimitesPorc WHERE IdAgente=%1%")
            % idAgente;
            ejecutar_sql(mysql, sqlQuery, DEBUG_CARGAR_TIPOS_MONTO);
            MYSQL_RES * result = mysql_store_result(mysql);
            for (MYSQL_ROW row = mysql_fetch_row(result);
                    NULL != row;
                    row = mysql_fetch_row(result)) {

                boost::uint8_t idSorteo = atoi(row[0]);

                TipoDeMonto* tipoMontoSorteo = new TipoDeMonto;
                tipoMontoSorteo->tipomonto_triple = atoi(row[1]);
                tipoMontoSorteo->tipomonto_terminal = atoi(row[2]);
                tipoMontoSorteo->limdisctrip = double(atoi(row[3])) / 10000.0;
                tipoMontoSorteo->limdiscterm = double(atoi(row[4])) / 10000.0;
                tipoMontoSorteo->forma_pago = atoi(row[5]);
                tipoMontoSorteo->porctriple = double(atoi(row[6])) / 10000.0;
                tipoMontoSorteo->porcterminal = double(atoi(row[7])) / 10000.0;
                tipoMontoSorteo->comodin = atoi(row[8]);
                tipoMontoSorteo->montoAdicional = (NULL != row[9]) ? atoi(row[9]) : 0;

                limitesPorcentajes.push_back(std::make_pair(idSorteo, tipoMontoSorteo));
            }
            mysql_free_result(result);
        }

        for (std::size_t i = 0, size = limitesPorcentajes.size(); i < size ; ++i) {

            boost::uint8_t idSorteo = limitesPorcentajes[i].first;

            TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
                                        
            tipomonto_agencia->updateTipoDeMonto(idSorteo, tipomonto_sorteo);
        }
    }
}

/*--------------------------------------------------------------------------------*/

double
round(double x) throw()
{
    x = x * 10.0 + 5.0;
    x = x / 10.0;
    return (boost::int64_t)(x);
}

/*--------------------------------------------------------------------------------*/

struct limite_porcentaje_asignados_t
{
    boost::uint32_t tipo;
    boost::uint32_t porcentaje;
    boost::uint32_t tipoDeMonto;
};

struct resumen_venta_t
{
    boost::uint32_t venta;
    boost::uint32_t premios;
    boost::uint32_t porcentajeAgencias;
    boost::uint32_t tipo;
    boost::uint32_t tipoDeMonto;
    boost::uint32_t retencionEstimada;
    boost::uint32_t porcRetencion;
};

struct TipoDeAgente
{
    boost::uint32_t idAgente;
    boost::uint32_t idTipoAgente;
    boost::uint32_t idBanca;
};

void
calcular_saldos(MYSQL * mysql, boost::uint32_t idAgente, boost::uint32_t idBanca, boost::uint8_t idSorteo, boost::uint32_t fechas[3])
{
    std::vector<TipoDeAgente> tiposDeLosAgentes;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Id, IdTipoAgente, IdBanca FROM Agentes WHERE IdRecogedor=%1% ORDER BY Nombre")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            TipoDeAgente tipoDeAgente;
            tipoDeAgente.idAgente = strtoul(row[0], NULL, 10);
            tipoDeAgente.idTipoAgente = strtoul(row[1], NULL, 10); 
            tipoDeAgente.idBanca = strtoul(row[2], NULL, 10); 
            tiposDeLosAgentes.push_back(tipoDeAgente);
        }
        mysql_free_result(result);
    }

    std::ostringstream agenciasOSS;
    bool esPrimero = true;
    for (std::size_t i = 0, size = tiposDeLosAgentes.size(); i < size ; ++i) {
        //boost::uint32_t id = tiposDeLosAgentes[i].first;
        //boost::uint32_t idtipoagente = tiposDeLosAgentes[i].second;

        if (esPrimero) {
            agenciasOSS << tiposDeLosAgentes[i].idAgente;
            esPrimero = false;

        } else {
            agenciasOSS << ", " << tiposDeLosAgentes[i].idAgente;
        }

        if (tiposDeLosAgentes[i].idTipoAgente == 2) {
            calcular_saldos(mysql, 
                            tiposDeLosAgentes[i].idAgente,
                            tiposDeLosAgentes[i].idBanca, 
                            idSorteo, 
                            fechas);
        }
    }

    std::string agencias(agenciasOSS.str());

    if (agencias.length() > 0) {

        std::vector<limite_porcentaje_asignados_t> limitePorcentajeAsignados;
        {
            // la primera columna del select union representa el tipo de la siguiente manera:
            // TERMINAL       2
            // TRIPLE         3
            // TERMINALAZO    4
            // TRIPLETAZO     5
            boost::format sqlQuery;
            sqlQuery.parse("SELECT 3, PorcTrip, IdMontoTriple FROM LimitesPorc "
                            "WHERE IdAgente=%1% AND IdSorteo=%2% "
                            "UNION SELECT 2, PorcTerm, IdMontoTerminal FROM LimitesPorc "
                            "WHERE IdAgente=%1% AND IdSorteo=%2% "
                            "UNION SELECT 5, PorcTrip, IdMontoTriple FROM LimitesPorc "
                            "WHERE IdAgente=%1% AND IdSorteo=%2% "
                            "UNION SELECT 4, PorcTerm, IdMontoTerminal FROM LimitesPorc "
                            "WHERE IdAgente=%1% AND IdSorteo=%2%")
            % idAgente % unsigned(idSorteo);
            ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
            MYSQL_RES * result = mysql_store_result(mysql);
            for (MYSQL_ROW row = mysql_fetch_row(result);
                    NULL != row;
                    row = mysql_fetch_row(result)) {
                limite_porcentaje_asignados_t limitePorcentajeAsignado;
                limitePorcentajeAsignado.tipo = atoi(row[0]);
                limitePorcentajeAsignado.porcentaje = atoi(row[1]);
                limitePorcentajeAsignado.tipoDeMonto = atoi(row[2]);
                limitePorcentajeAsignados.push_back(limitePorcentajeAsignado);
            }
            mysql_free_result(result);
        }

        double porcentajes[10][50];
        {
            for (boost::uint32_t i = 0; i < 10; i++) {
                for (boost::uint32_t j = 0; j < 50; j++)
                    porcentajes[i][j] = 0;
            }

            for (std::size_t i = 0, size = limitePorcentajeAsignados.size(); i < size ; ++i) {
                boost::uint32_t tipo = limitePorcentajeAsignados[i].tipo;
                boost::uint32_t porcentaje = limitePorcentajeAsignados[i].porcentaje;
                boost::uint32_t tipoDeMonto = limitePorcentajeAsignados[i].tipoDeMonto;
                porcentajes[tipo][tipoDeMonto] = porcentaje;
                porcentajes[tipo][tipoDeMonto] = porcentajes[tipo][tipoDeMonto] / 100 / 100;
            }
        }

        std::vector<resumen_venta_t> resumenDeVentas;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT SUM(Venta), SUM(Premios), SUM(Porcentaje), Tipo, TipoMonto, "
                           "SUM(RetencionEstimada), PorcRetencion " 
                            "FROM Resumen_Ventas "
                            "WHERE Fecha='%d-%02d-%02d' AND Resumen_Ventas.IdSorteo=%u "
                            "AND Resumen_Ventas.IdAgente in (%s) "
                            "AND Tipo IN(2, 3, 4, 5, 10, 11, 12) GROUP BY Tipo, TipoMonto")
            % fechas[2] % fechas[1] % fechas[0] % unsigned(idSorteo) % agencias;

            ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
            MYSQL_RES * result = mysql_store_result(mysql);

            for (MYSQL_ROW row = mysql_fetch_row(result);
                    NULL != row;
                    row = mysql_fetch_row(result)) {
                resumen_venta_t resumenDeVenta;
                resumenDeVenta.venta = atoi(row[0]);
                resumenDeVenta.premios = atoi(row[1]);
                resumenDeVenta.porcentajeAgencias = atoi(row[2]);
                resumenDeVenta.tipo = atoi(row[3]);
                resumenDeVenta.tipoDeMonto = atoi(row[4]);
                resumenDeVenta.retencionEstimada = atoi(row[5]);                
                resumenDeVenta.porcRetencion = atoi(row[6]);
                
                resumenDeVentas.push_back(resumenDeVenta);
            }
            mysql_free_result(result);
        }

        for (std::size_t i = 0, size = resumenDeVentas.size(); i < size ; ++i) {
            boost::uint32_t venta = resumenDeVentas[i].venta;
            boost::uint32_t premios = resumenDeVentas[i].premios;
            boost::uint32_t porcentajeAgencias = resumenDeVentas[i].porcentajeAgencias;
            boost::uint32_t tipo = resumenDeVentas[i].tipo;
            boost::uint32_t tipoDeMonto = resumenDeVentas[i].tipoDeMonto;
            double retencionEstimada = resumenDeVentas[i].retencionEstimada;
            boost::uint32_t porcRetencion = resumenDeVentas[i].porcRetencion;
            double porcentaje = 0;
            if (tipo == TERMINAL or tipo == TRIPLE or tipo == TERMINALAZO or tipo == TRIPLETAZO) {
                porcentaje = venta;
                porcentaje = porcentaje * porcentajes[tipo][tipoDeMonto];
                porcentaje = round(porcentaje);
                if (tipoDeMonto == 0) {
                    char mensaje[BUFFER_SIZE];
                    sprintf(mensaje, "TipoMonto 0 para Tipo de numero %i agencias %s sorteo %i",
                             tipo, agencias.c_str(), idSorteo);
                    log_mensaje(NivelLog::Bajo, "ERROR", "Resumen Ventas", mensaje);
                }
            } else {
                porcentaje = 0;
            }

            double ganancia = 0;
            boost::int32_t saldo;

            if (idAgente == 1) {
                
            } else if (idAgente == idBanca) {
                ganancia = porcentajeAgencias;
                //saldo = venta - (premios + (boost::uint32_t) ganancia);
                saldo = venta - (premios + boost::int32_t(ganancia)) + boost::int32_t(retencionEstimada/100);
            } else {
                ganancia = porcentaje - porcentajeAgencias;
                //saldo = venta - (premios + (boost::uint32_t) porcentaje);
                saldo = venta - (premios + boost::int32_t(porcentaje)) + boost::int32_t(retencionEstimada/100);
            }
            {
                boost::format sqlQuery;
                sqlQuery.parse("INSERT INTO Resumen_Ventas "
                                "VALUES(%u, %u, '%d-%02d-%02d', %d, %u, %u, %u, %u, %i, %i, %i, %i, %i)")
                % idAgente 
                % tipoDeMonto
                % fechas[2] % fechas[1] % fechas[0]
                % tipo 
                % unsigned(idSorteo) 
                % venta 
                % boost::uint32_t(porcentaje) 
                % premios 
                % saldo
                % tipoDeMonto 
                % boost::int32_t(ganancia)
                % retencionEstimada % porcRetencion;
                ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
            }
        }
    }
}

/*--------------------------------------------------------------------------------*/
struct saldo_agente_t
{
    boost::uint32_t idAgente;
    boost::int64_t saldo;
    boost::uint32_t tipoAgente;
};

void
calcular_saldos_listas(MYSQL * mysql, boost::uint32_t fechas[3])
{
    mysql_select_db(mysql, configuracion.dbSisacName);
    {
        boost::format sqlQuery;
        sqlQuery.parse("DELETE FROM Asientos WHERE IdCuenta = 1 "
                        "AND Fecha = '%d-%02d-%02d' AND Tipo = 1")
        % fechas[2] % fechas[1] % fechas[0];
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("DELETE FROM AsientosxRecogedor WHERE IdCuenta = 1 "
                        "AND Fecha = '%d-%02d-%02d' AND Tipo = 1")
        % fechas[2] % fechas[1] % fechas[0];
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
    }

    mysql_select_db(mysql, configuracion.dbZeusName);
    std::vector<saldo_agente_t> saldosDeAgentes;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdAgente, SUM(Venta - (Porcentaje + Premios) + (RetencionEstimada/100)) AS Saldo, "
                       "IdTipoAgente "
                       "FROM Resumen_Ventas, Agentes WHERE Fecha = '%d-%02d-%02d' AND Tipo IN (2, 3, 4, 5) "
                       "AND Agentes.Id = Resumen_Ventas.IdAgente "
                       "GROUP BY idAgente")
        % fechas[2] % fechas[1] % fechas[0];
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result);
                row != NULL;
                row = mysql_fetch_row(result)) {
            saldo_agente_t saldoDeAgente;
            saldoDeAgente.idAgente = strtoul(row[0], NULL, 10);
            saldoDeAgente.saldo = static_cast<boost::int64_t>(atof(row[1]));
            saldoDeAgente.tipoAgente = atoi(row[2]);

            saldosDeAgentes.push_back(saldoDeAgente);
        }
        mysql_free_result(result);
    }

    mysql_select_db(mysql, configuracion.dbSisacName);
    for (std::size_t i = 0, size = saldosDeAgentes.size(); i < size ; ++i) {
        boost::uint32_t idAgente = saldosDeAgentes[i].idAgente;
        boost::int32_t saldo = saldosDeAgentes[i].saldo;
        boost::uint32_t tipoAgente = saldosDeAgentes[i].tipoAgente;

        {
            boost::format sqlQuery;
            sqlQuery.parse("INSERT INTO Asientos "
                            "VALUES(NULL, 1, %i, '%d-%02d-%02d', %i, 'Lista diaria', 1)")
            % idAgente % fechas[2] % fechas[1] % fechas[0] % saldo;
            ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
        }

        if (tipoAgente == BANCA or tipoAgente == RECOGEDOR) {
            {
                boost::format sqlQuery;
                sqlQuery.parse("INSERT INTO AsientosxRecogedor "
                                "VALUES(NULL, 1, %i, '%d-%02d-%02d', %i, 'Lista diaria', 1)")
                % idAgente % fechas[2] % fechas[1] % fechas[0] % saldo;
                ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_SALDOS);
            }
        }
    }

    mysql_select_db(mysql, configuracion.dbZeusName);
}

/*--------------------------------------------------------------------------------*/
#if 0
struct datos_renglon_t
{
    boost::uint32_t idAgente;
    boost::uint32_t idTicket;
    boost::uint16_t numero;
    boost::uint32_t monto;
    boost::uint8_t  tipo;
    boost::uint8_t  idTaquilla;
};

long double 
impuestoGananciaFortuita(long double montoBruto)
{
    return   (long double)(montoBruto) 
           * (long double)(configuracion.impuesto_ganancia_fortuita)
           / (long double)(100);       
}

enum ResultadoCalcularResumen
{
      ResumenOk = 1
    , SorteoNoCargado
    , NoHayRenglones
};

ResultadoCalcularResumen
ejecutarCalcularResumen(ConexionAdmin * conexion, MYSQL* mysql, CalcularResumenV0 const& calcular_resumen)
{

#define GUARDAR_TICKET_PREMIADO() \
    if (montoPremiado > 0) { \
        { \
            boost::format sqlQuery; \
            sqlQuery.parse("INSERT INTO Tickets_Premiados " \
                            "VALUES(%u, %u, %u, '%d-%02d-%02d', %u, %u)") \
            % idAgente % unsigned(idTaquilla) % unsigned(idSorteo) \
            % fechas[2] % fechas[1] % fechas[0] % idTicket % montoPremiado; \
            ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES); \
        } \
    } \
    montoPremiado = 0;

#define GUARDAR_RESUMEN() \
    { \
        porcentaje = (double) monto_terminal; \
        porcentaje *= tipomonto_sorteo->porcterminal; \
        porcentaje = round(porcentaje); \
        monto_terminal = (boost::uint32_t) round((double) monto_terminal); \
        premios_terminales = round(premios_terminales); \
        saldo = monto_terminal - (boost::uint32_t) (porcentaje + premios_terminales); \
        { \
            boost::format sqlQuery; \
            sqlQuery.parse("INSERT INTO Resumen_Ventas " \
                           "VALUES(%u, %u, '%d-%02d-%02d', %d, %u, %u, %u, %u, %i, %i, %i, %i, %i)") \
            % idAgente % unsigned(idTaquilla) % fechas[2] % fechas[1] % fechas[0] \
            % unsigned(tipo1) % unsigned(idSorteo) % monto_terminal % boost::uint32_t(porcentaje) \
            % boost::uint32_t(premios_terminales) % boost::int32_t(saldo) \
            % unsigned(tipomonto_sorteo->tipomonto_terminal) % boost::uint32_t(porcentaje) \
            % boost::uint32_t(impuestoGananciaFortuita(premios_terminales)) \
            % configuracion.impuesto_ganancia_fortuita; \
            ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES); \
        } \
        porcentaje = monto_triple; \
        porcentaje *= tipomonto_sorteo->porctriple; \
        porcentaje = round(porcentaje); \
        monto_triple = (boost::uint32_t) round((double) monto_triple); \
        premios_triples = round(premios_triples); \
        saldo = monto_triple - (boost::uint32_t) (porcentaje + premios_triples); \
        { \
            boost::format sqlQuery; \
            sqlQuery.parse("INSERT INTO Resumen_Ventas " \
                            "VALUES(%u, %u, '%d-%02d-%02d', %d, %u, %u, %u, %u, %i, %i, %i, %i, %i)") \
            % idAgente % unsigned(idTaquilla) % fechas[2] % fechas[1] % fechas[0] \
            % unsigned(tipo2) % unsigned(idSorteo) % monto_triple % boost::uint32_t(porcentaje) \
            % boost::uint32_t(premios_triples) % boost::int32_t(saldo) \
            % unsigned(tipomonto_sorteo->tipomonto_triple) % boost::uint32_t(porcentaje) \
            % boost::uint32_t(impuestoGananciaFortuita(premios_triples)) \
            % configuracion.impuesto_ganancia_fortuita; \
            ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES); \
        } \
        int w = (tipomonto_sorteo->forma_pago != 4) ? 3 : 0; \
        for (int i = 0 ; i < w ; i++) { \
            { \
                boost::format sqlQuery; \
                sqlQuery.parse("INSERT INTO Resumen_Ventas " \
                                "VALUES(%u, %u, '%d-%02d-%02d', %d, %u, %u, %u, %u, %i, %i, %i, %i, %i)") \
                % idAgente % unsigned(idTaquilla) % fechas[2] % fechas[1] % fechas[0] \
                % (10 + i) % unsigned(idSorteo) % 0 % 0 \
                % boost::uint32_t(montos_premios[i]) % 0 \
                % unsigned(tipomonto_sorteo->tipomonto_terminal) % 0 \
                % boost::uint32_t(impuestoGananciaFortuita(montos_premios[i])) \
                % configuracion.impuesto_ganancia_fortuita; \
                ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES); \
            } \
        } \
        idTicket = tmpidticket; \
        idTaquilla = tmptaquilla; \
        porcentaje = 0; \
        premio = 0; \
        monto_triple = 0; \
        monto_terminal = 0; \
        saldo = 0; \
        premios_terminales = 0; \
        premios_triples = 0; \
        montoPremiado = 0; \
        for (int i = 0; i < 3 ; i++) { \
            montos_premios[i] = 0; \
        } \
    }

#define CALCULA_PREMIOS() \
    switch (tipomonto_sorteo->forma_pago) { \
        case 0: { \
                if (tipo == TERMINAL) { \
                    for (int i = 0; i < 3; i++) { \
                        if (numero == lista_terminales->x[i]) { \
                            premio = monto * tipomonto_terminal->montos[i] / 10; \
                            montoPremiado += (boost::uint32_t) premio; \
                            premios_terminales += premio; \
                            montos_premios[i] += premio; \
                        } \
                    } \
                } else { \
                    if (numero == numeros[0]) { \
                        if (tipomonto_sorteo->comodin == 1 and comodin >= 1) { \
                            boost::uint32_t montos = 0; \
                            montos = tipomonto_triple->montos[0] + tipomonto_sorteo->montoAdicional; \
                            for (int i = 1; i <= comodin - 1; i++) { \
                                montos += montoAdicional; \
                            } \
                            premio = monto * montos / 10; \
                        } else { \
                            premio = monto * tipomonto_triple->montos[0] / 10; \
                        } \
                        montoPremiado += (boost::uint32_t) premio; \
                        premios_triples += premio; \
                    } \
                } \
                break; \
            } \
        case 1: { \
                if (tipo == TERMINAL) { \
                    for (int i = 0; i < 3; i++) { \
                        if (numero == lista_terminales->a[i]) { \
                            premio = monto * tipomonto_terminal->montos[i] / 10; \
                            montoPremiado += (boost::uint32_t) premio; \
                            premios_terminales += premio; \
                            montos_premios[i] += premio; \
                        } \
                    } \
                } else { \
                    if (numero == numeros[0]) { \
                        if (tipomonto_sorteo->comodin == 1 and comodin >= 1) { \
                            boost::uint32_t montos = 0; \
                            montos = tipomonto_triple->montos[0] + tipomonto_sorteo->montoAdicional; \
                            for (int i = 1; i <= comodin - 1; i++) { \
                                montos += montoAdicional; \
                            } \
                            premio = monto * montos / 10; \
                        } else { \
                            premio = monto * tipomonto_triple->montos[0] / 10; \
                        } \
                        montoPremiado += (boost::uint32_t) premio; \
                        premios_triples += premio; \
                    } \
                } \
                break; \
            } \
        case 2: { \
                if (tipo == TERMINAL) { \
                    for (int i = 0; i < 3; i++) { \
                        if (numero == lista_terminales->b[i]) { \
                            premio = monto * tipomonto_terminal->montos[i] / 10; \
                            montoPremiado += (boost::uint32_t) premio; \
                            premios_terminales += premio; \
                            montos_premios[i] += premio; \
                        } \
                    } \
                } else { \
                    if (numero == numeros[0]) { \
                        if (tipomonto_sorteo->comodin == 1 and comodin >= 1) { \
                            boost::uint32_t montos = 0; \
                            montos = tipomonto_triple->montos[0] + tipomonto_sorteo->montoAdicional; \
                            for (int i = 1; i <= comodin - 1; i++) { \
                                montos += montoAdicional; \
                            } \
                            premio = monto * montos / 10; \
                        } else { \
                            premio = monto * tipomonto_triple->montos[0] / 10; \
                        } \
                        montoPremiado += (boost::uint32_t) premio; \
                        premios_triples += premio; \
                    } \
                } \
                break; \
            } \
        case 3: { \
                if (tipo == TERMINAL) { \
                    for (int i = 0; i < 3; i++) { \
                        if (numero == lista_terminales->c[i]) { \
                            premio = monto * tipomonto_terminal->montos[i] / 10; \
                            montoPremiado += (boost::uint32_t) premio; \
                            premios_terminales += premio; \
                            montos_premios[i] += premio; \
                        } \
                    } \
                } else { \
                    if (numero == numeros[0]) { \
                        if (tipomonto_sorteo->comodin == 1 and comodin >= 1) { \
                            boost::uint32_t montos = 0; \
                            montos = tipomonto_triple->montos[0] + tipomonto_sorteo->montoAdicional; \
                            for (int i = 1; i <= comodin - 1; i++) { \
                                montos += montoAdicional; \
                            } \
                            premio = monto * montos / 10; \
                        } else { \
                            premio = monto * tipomonto_triple->montos[0] / 10; \
                        } \
                        montoPremiado += (boost::uint32_t) premio; \
                        premios_triples += premio; \
                    } \
                } \
                break; \
            } \
        case 4: { \
                if (tipo == TERMINALAZO) { \
                    for (int i = 0; i < 1; i++) { \
                        if (numero == lista_terminales->d[i]) { \
                            premio = monto * tipomonto_terminal->montos[i] / 10; \
                            montoPremiado += (boost::uint32_t) premio; \
                            premios_terminales += premio; \
                            montos_premios[i] += premio; \
                        } \
                    } \
                } else { \
                    if (numero == numeros[0]) { \
                        if (tipomonto_sorteo->comodin == 1 and comodin >= 1) { \
                            boost::uint32_t montos = 0; \
                            montos = tipomonto_triple->montos[0] + tipomonto_sorteo->montoAdicional; \
                            for (int i = 1; i <= comodin - 1; i++) { \
                                montos += montoAdicional; \
                            } \
                            premio = monto * montos / 10; \
                        } else { \
                            premio = monto * tipomonto_triple->montos[0] / 10; \
                        } \
                        montoPremiado += (boost::uint32_t) premio; \
                        premios_triples += premio; \
                    } \
                } \
                break; \
            } \
    }

#define INICIALIZAR_AGENTE() \
    { \
        idAgente = tmpidagente; \
        idTaquilla = tmptaquilla; \
        do { \
            tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente); \
            if (tipomonto_agencia == NULL) { \
                cargar_tipomonto_x_sorteo(idAgente, mysql); \
            } \
        } while (tipomonto_agencia == NULL); \
        tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo); \
        tipodemonto = tipomonto_sorteo->tipomonto_triple; \
        tipomonto_triple = \
            (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipodemonto); \
        tipodemonto = tipomonto_sorteo->tipomonto_terminal; \
        tipomonto_terminal = \
            (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipodemonto); \
        idTicket = tmpidticket; \
        idTaquilla = tmptaquilla; \
        monto_triple = 0; \
        monto_terminal = 0; \
        porcentaje = 0; \
        premio = 0; \
        saldo = 0; \
        montoPremiado = 0; \
        premios_terminales = 0; \
        premios_triples = 0; \
        for (int i = 0; i < 3 ; i++) { \
            montos_premios[i] = 0; \
        } \
    }
            
    boost::uint8_t idSorteo = calcular_resumen.idSorteo;
    boost::uint32_t fecha = calcular_resumen.fecha;
    Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
    if (sorteo == NULL) {
        
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Error calculado resumen del sorteo <%d>, "
                         "sorteo == NULL", idSorteo);
        log_admin(NivelLog::Bajo, conexion, mensaje);
        
        return SorteoNoCargado;
    }
    boost::uint8_t tipo1 = TERMINAL;
    boost::uint8_t tipo2 = TRIPLE;
    if (sorteo->isConSigno()) {
        tipo1 = TERMINALAZO;
        tipo2 = TRIPLETAZO;
    }

    boost::uint32_t fechas[3];
    for (int i = 0; i < 2; i++) {
        fechas[i] = fecha % 100;
        fecha = fecha / 100;
    }
    fechas[2] = fecha;
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Calcular el resumen para el sorteo <%d> y la fecha <%02d-%02d-%d>",
                 idSorteo, fechas[0], fechas[1], fechas[2]);
        log_admin(NivelLog::Bajo, conexion, mensaje);
    }

    datos_premios_t datoPremio;
    bool hayPremios = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Triple, Term1ro, Term2do, Term3ro, Comodin, MontoAdd "
                        "FROM Premios WHERE Fecha='%d-%02d-%02d' AND IdSorteo=%u")
        % fechas[2] % fechas[1] % fechas[0] % unsigned(idSorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            hayPremios = true;
            for (int i = 0; i < 4; i++) {
                datoPremio.numeros[i] = (row[i] == NULL) ? -1 : atoi(row[i]);
            }
            datoPremio.comodin = (row[4] == NULL) ? 0 : atoi(row[4]);
            datoPremio.montoAdicional = (row[5] == NULL) ? 0 : atoi(row[5]);

        }
        mysql_free_result(result);
    }

    boost::uint8_t comodin = 0;
    boost::uint16_t numeros[4];
    boost::uint32_t montoAdicional = 0;
    lista_terminales_t * lista_terminales = NULL;

    if (hayPremios) {

        for (int i = 0; i < 4; i++) {
            numeros[i] = datoPremio.numeros[i];
        }

        comodin = datoPremio.comodin;
        montoAdicional = datoPremio.montoAdicional;

        lista_terminales = calcular_lista_terminales(numeros[0]);
        lista_terminales->x[0] = numeros[1];   /*Para tipo */
        lista_terminales->x[1] = numeros[2];   /*de pago x */
        lista_terminales->x[2] = numeros[3];
    }

    std::vector<datos_renglon_t> datosRenglones;
    {                        
        boost::format sqlQuery;
        sqlQuery.parse("SELECT T.IdAgente, R.IdTicket, R.Numero, R.Monto,Tipo, T.NumTaquilla "
                        "FROM Tickets AS T, Renglones AS R "
                        "WHERE T.Fecha >= '%d-%02d-%02d 00:00:00' "
                        "AND   T.Fecha <= '%d-%02d-%02d 23:59:59' " 
                        "AND T.Estado IN (%i,%i,%i) "
                        "AND R.IdTicket = T.Id AND R.IdSorteo=%u "
                        "ORDER BY T.IdAgente, T.NumTaquilla, R.IdTicket")
        % fechas[2] % fechas[1] % fechas[0]
        % fechas[2] % fechas[1] % fechas[0]
        % unsigned(NORMAL) % unsigned(PAGADO) % unsigned(4 /*GENERADO*/)
        % unsigned(idSorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            datos_renglon_t datoRenglon;
            datoRenglon.idAgente = atoi(row[0]);
            datoRenglon.idTicket = atoi(row[1]);
            datoRenglon.numero = atoi(row[2]);
            datoRenglon.monto = atoi(row[3]);
            datoRenglon.tipo = atoi(row[4]);
            datoRenglon.idTaquilla = atoi(row[5]);

            datosRenglones.push_back(datoRenglon);
        }
        mysql_free_result(result);
    }

    if (datosRenglones.empty()) {        
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Error calculado resumen del sorteo <%d> al <%02d-%02d-%d>, "
                         "datosRenglones.empty()",
                 idSorteo, fechas[0], fechas[1], fechas[2]);
        log_admin(NivelLog::Bajo, conexion, mensaje);        

        if (hayPremios) {
            delete lista_terminales;
        }
        return NoHayRenglones;
    }
    boost::uint32_t montoPremiado = true;
    boost::uint32_t idAgente = 0;
    boost::uint32_t idTicket = 0;

    ejecutar_sql(mysql, "BEGIN", DEBUG_CALCULAR_RESUMENES);
    {
        boost::format sqlQuery;
        sqlQuery.parse("DELETE FROM Tickets_Premiados WHERE IdSorteo=%u AND Fecha='%d-%02d-%02d'")
        % unsigned(idSorteo) % fechas[2] % fechas[1] % fechas[0];
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("DELETE FROM Resumen_Ventas WHERE Fecha='%d-%02d-%02d' AND IdSorteo=%u")
        % fechas[2] % fechas[1] % fechas[0] % unsigned(idSorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CALCULAR_RESUMENES);
    }

    boost::uint8_t idTaquilla = 0;
    boost::uint8_t tipodemonto = 0;
    boost::int32_t saldo = 0;
    boost::uint32_t monto_terminal = 0;
    boost::uint32_t monto_triple = 0;
    double porcentaje = 0.0;
    double premios_terminales = 0.0;
    double premios_triples = 0.0;
    double premio = 0.0;
    double montos_premios[3] = { 0.0, 0.0, 0.0 };

    TipoDeMontoPorAgencia * tipomonto_agencia = NULL;
    TipoDeMonto * tipomonto_sorteo = NULL;
    n_t_tipomonto_t * tipomonto_terminal = NULL;
    n_t_tipomonto_t * tipomonto_triple = NULL;

    for (std::size_t i = 0, size = datosRenglones.size(); i < size ; ++i) {
        boost::uint32_t tmpidagente = datosRenglones[i].idAgente;
        boost::uint32_t tmpidticket = datosRenglones[i].idTicket;
        boost::uint16_t numero = datosRenglones[i].numero;
        boost::uint32_t monto = datosRenglones[i].monto;
        boost::uint8_t tipo = datosRenglones[i].tipo;
        boost::uint8_t tmptaquilla = datosRenglones[i].idTaquilla;

        if (tmpidagente != idAgente) {
            if (idAgente != 0) {
                GUARDAR_TICKET_PREMIADO();
                GUARDAR_RESUMEN();
            }
            INICIALIZAR_AGENTE();
        }
        if (tmptaquilla != idTaquilla) {
            GUARDAR_TICKET_PREMIADO();
            GUARDAR_RESUMEN();
        }
        if (tmpidticket != idTicket) {
            GUARDAR_TICKET_PREMIADO();
            idTicket = tmpidticket;
        }
        if (hayPremios) {
            CALCULA_PREMIOS();
        }
        if (tipo == TRIPLE or tipo == BONO or tipo == TRIPLETAZO or tipo == BONOTRIPLETAZO) {
            monto_triple += monto;
        } else if (tipo == TERMINAL or tipo == TERMINALAZO) {
            monto_terminal += monto;
        }

        if (i + 1 == size) {
            GUARDAR_TICKET_PREMIADO();
            GUARDAR_RESUMEN();
        }
    }

    calcular_saldos(mysql, 1, 0, idSorteo, fechas);
    calcular_saldos_listas(mysql, fechas);

    ejecutar_sql(mysql, "COMMIT", DEBUG_CALCULAR_RESUMENES);

    if (hayPremios) {
        delete lista_terminales;
    }

    {        
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Calculado resumen del sorteo <%d> al <%02d-%02d-%d>",
                 idSorteo, fechas[0], fechas[1], fechas[2]);
        log_admin(NivelLog::Bajo, conexion, mensaje);      
    }
    return ResumenOk;
}
#endif



bool
ejecutarCalcularResumen(ConexionAdmin * conexion, MYSQL* mysql, CalcularResumenV0 const& calcularResumen)
{
    std::ostringstream ossFecha;
    
    try {
        boost::uint32_t year = calcularResumen.fecha;
        boost::uint32_t day = year % 100;                             
        year /= 100;
        boost::uint32_t month = year % 100;
        year /= 100;
        ossFecha << day << "-" << month << "-" << year;
        std::ostringstream ossFechaSQL;
        ossFechaSQL << year << "-" << month << "-" << day;
        
        {
            std::ostringstream ossLog;
            ossLog << "Calcular el resumen para el sorteo <" 
                   << unsigned(calcularResumen.idSorteo) 
                   << "> al <" << ossFecha.str() << ">";
            log_admin(NivelLog::Bajo, conexion, ossLog.str().c_str());
        }
        
        CierreSorteoZeus::ControlResumen control(ossFechaSQL.str(), calcularResumen.idSorteo);
        control.generarCierre(mysql); 
        control.almacenar(mysql);
        
        {
            std::ostringstream ossLog;
            ossLog << "Calculado el resumen para el sorteo <" 
                   << unsigned(calcularResumen.idSorteo) 
                   << "> al <" << ossFecha.str() << ">";
            log_admin(NivelLog::Bajo, conexion, ossLog.str().c_str());
        }   
        
        return true;
    } catch (std::exception const& e) {     
        std::ostringstream ossLog;
        ossLog << "Error calculando el resumen para el sorteo <" 
               << unsigned(calcularResumen.idSorteo) 
               << "> al <" << ossFecha.str() << ">: "
               << e.what();
        log_admin(NivelLog::Bajo, conexion, ossLog.str().c_str());
        return false;
    }
}

void
dispatch_CALCULAR_RESUMEN_V_0(ConexionAdmin * conexion, boost::uint8_t const* buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    CalcularResumenV0 calcularResumen(buff);            
     boost::uint8_t resultado = ejecutarCalcularResumen(conexion, mysql, calcularResumen);    
    send2cliente(*conexion, CALCULAR_RESUMEN_V_0, &resultado, sizeof(resultado));
}

void
dispatch_CALCULAR_RESUMEN_V_1(ConexionAdmin * conexion, boost::uint8_t const* buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    CalcularResumenV1 calcularResumen(buff);    
    
    if (nivelLogMensajes >= NivelLog::Debug) {
        std::ostringstream oss;
        oss << "Calcular Resumen para la fecha " << calcularResumen.fecha << " y el o los sorteos ";
        for (std::vector<boost::uint8_t>::const_iterator iter = calcularResumen.idSorteos.begin();
             iter != calcularResumen.idSorteos.end(); ++iter)
             oss << unsigned(*iter) << " ";
        log_admin(NivelLog::Debug, conexion, oss.str().c_str());
    }
    /*
     * SorteoNoCargado = 2
     * NoHayRenglones  = 3
     */
    std::vector<std::pair<boost::uint8_t, boost::uint8_t> > resultados;
    for (std::vector<boost::uint8_t>::const_iterator iter = calcularResumen.idSorteos.begin();
         iter != calcularResumen.idSorteos.end(); ++iter) {       
        boost::uint8_t resultado =             
            ejecutarCalcularResumen(conexion, mysql, CalcularResumenV0(*iter, calcularResumen.fecha));
        resultados.push_back(std::make_pair(*iter, resultado));
    }
    
    ResultadoCalcularResumenV1 resultado(resultados);
    std::vector<boost::uint8_t> resultBuffer =  resultado.toRawBuffer();   
    
    send2cliente(*conexion, CALCULAR_RESUMEN_V_1, &resultBuffer[0], resultBuffer.size());    
}


/*--------------------------------------------------------------------------------*/

bool
para_cada_tipo_monto_triple(void* key, void* value, void* data)
{
    boost::uint16_t numero;
    boost::uint8_t tipo;
    n_t_tipomonto_t *tipoDeMonto;
    n_t_venta_numero_t *venta_numero;
    ConexionAdmin *conexion;
    char mensaje[BUFFER_SIZE];

    conexion = (ConexionAdmin *) data;
    tipo = *(boost::uint8_t *) key;
    tipoDeMonto = (n_t_tipomonto_t *) value;
    for (numero = 0; numero < 1000; numero++) {
        venta_numero = (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
        if (venta_numero != NULL) {
            sprintf(mensaje, "TM=[%i] Monto=[%u] Numero=%03u Venta=%u", tipo, tipoDeMonto->montos[0],
                     numero, venta_numero->venta);
            log_admin(NivelLog::Detallado, conexion, mensaje);
        }
    }
    return false;
}

/*--------------------------------------------------------------------------------*/

struct buffer_t
{
    boost::uint16_t cuantos;
    boost::uint8_t *buffer;
};

struct info_renglones_t
{
    boost::uint16_t nrorenglones;
    boost::uint32_t monto;
};

/*--------------------------------------------------------------------------------*/

gboolean
para_cada_preventa1(void*, void* value, void* data)
{
    info_renglones_t * info_renglones = (info_renglones_t *) data;
    Preventa * preventa = (Preventa*) value;

    info_renglones->nrorenglones++;
    info_renglones->monto += preventa->monto_preventa;
    return FALSE;
}

/*--------------------------------------------------------------------------------*/

gboolean
para_todas_taquillas(void* key, void*, void* data)
{
    boost::uint16_t lon, lon1, lon2, lon3, lon4, lon5;
    buffer_t *buffer = (buffer_t *) data;
    boost::uint8_t *buff;
    boost::uint64_t agente_taquilla;
    boost::uint32_t idAgente;
    boost::uint8_t idTaquilla;
    n_t_preventas_x_taquilla_t *preventas_taquilla;
    info_renglones_t *info_renglones;

    agente_taquilla = *(boost::uint64_t *) key;
    preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    info_renglones = new info_renglones_t;
    info_renglones->nrorenglones = 0;
    info_renglones->monto = 0;
    g_tree_foreach(preventas_taquilla->preventas, para_cada_preventa1, info_renglones);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    if (info_renglones->nrorenglones == 0) {
        return FALSE;
    }
    agente_taquilla = agente_taquilla >> 32;
    idAgente = agente_taquilla;
    agente_taquilla = *(boost::uint64_t *) key;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla = agente_taquilla >> 32;
    idTaquilla = agente_taquilla;
    lon1 = sizeof(idAgente);
    lon2 = sizeof(idTaquilla);
    lon3 = sizeof(info_renglones->nrorenglones);
    lon4 = sizeof(info_renglones->monto);
    lon = lon1 + lon2 + lon3 + lon4;
    if (buffer->cuantos == 0) {
        buff = new boost::uint8_t[lon];
        memcpy(&buff[0], &idAgente, lon1);
        memcpy(&buff[lon1], &idTaquilla, lon2);
        memcpy(&buff[lon1 + lon2], &info_renglones->nrorenglones, lon3);
        memcpy(&buff[lon1 + lon2 + lon3], &info_renglones->monto, lon4);
        buffer->cuantos++;
        buffer->buffer = buff;
    } else {
        lon1 = buffer->cuantos * lon;
        lon2 = sizeof(idAgente);
        lon3 = sizeof(idTaquilla);
        lon4 = sizeof(info_renglones->nrorenglones);
        lon5 = sizeof(info_renglones->monto);
        lon = lon1 + lon2 + lon3 + lon4 + lon5;
        buff = new boost::uint8_t[lon];
        memcpy(&buff[0], buffer->buffer, lon1);
        memcpy(&buff[lon1], &idAgente, lon2);
        memcpy(&buff[lon1 + lon2], &idTaquilla, lon3);
        memcpy(&buff[lon1 + lon2 + lon3], &info_renglones->nrorenglones, lon4);
        memcpy(&buff[lon1 + lon2 + lon3 + lon4], &info_renglones->monto, lon5);
        delete [] buffer->buffer;
        buffer->cuantos++;
        buffer->buffer = buff;
    }
    return FALSE;
}

/*--------------------------------------------------------------------------------*/

void
consultar_preventas(ConexionAdmin * conexion)
{
    boost::uint16_t lon, lon1, lon2;
    boost::uint8_t *buff;
    buffer_t buffer;

    buffer.cuantos = 0;
    g_tree_foreach(t_preventas_x_taquilla, para_todas_taquillas, &buffer);
    lon1 = 2;
    lon2 = buffer.cuantos * 11;
    lon = lon1 + lon2;
    buff = new boost::uint8_t[lon];
    memcpy(&buff[0], &buffer.cuantos, lon1);
    if (lon2 != 0) {
        memcpy(&buff[lon1], buffer.buffer, lon2);
        delete [] buffer.buffer;
    }
    send2cliente(*conexion, CALCULAR_PREVENTA_TAQ, buff, lon);
    delete [] buff;
}

/*--------------------------------------------------------------------------------*/

gboolean
para_cada_preventa2(void* key, void* value, void* data)
{
    boost::uint16_t lon, lon0, lon1, lon2, lon3, lon4, lon5, lon6;
    boost::uint16_t renglon;
    Preventa * preventa = (Preventa*) value;
    buffer_t *buffer = (buffer_t *) data;
    boost::uint8_t *buff;

    renglon = *(boost::uint16_t *) key;

    lon1 = sizeof(renglon);
    lon2 = sizeof(preventa->numero);
    lon3 = sizeof(preventa->idSorteo);
    lon4 = sizeof(preventa->monto_preventa);
    lon5 = sizeof(preventa->tipo);
    lon6 = lon1 + lon2 + lon3 + lon4 + lon5;
    lon0 = buffer->cuantos * lon6;
    lon = lon0 + lon6;
    buff = new boost::uint8_t[lon];
    if (buffer->cuantos > 0) {
        memcpy(&buff[0], buffer->buffer, lon0);
    }
    memcpy(&buff[lon0], &renglon, lon1);
    memcpy(&buff[lon0 + lon1], &preventa->numero, lon2);
    memcpy(&buff[lon0 + lon1 + lon2], &preventa->idSorteo, lon3);
    memcpy(&buff[lon0 + lon1 + lon2 + lon3], &preventa->monto_preventa, lon4);
    memcpy(&buff[lon0 + lon1 + lon2 + lon3 + lon4], &preventa->tipo, lon5);
    if (buffer->cuantos > 0) {
        delete [] buffer->buffer;
    }
    buffer->cuantos++;
    buffer->buffer = buff;
    if (buffer->cuantos >= 6000) {
        return TRUE;
    }
    return FALSE;
}

/*--------------------------------------------------------------------------------*/

void
consultar_preventas_taquilla(ConexionAdmin * conexion, boost::uint8_t const* data)
{
    boost::uint16_t lon, lon1, lon2;
    boost::uint8_t idTaquilla;
    boost::uint32_t nnodes, agente;
    boost::uint64_t agente_taquilla;
    n_t_preventas_x_taquilla_t *preventas_taquilla;
    buffer_t buffer;
    boost::uint8_t *buff;

    lon1 = sizeof(agente);
    lon2 = sizeof(idTaquilla);
    memcpy(&agente, &data[0], lon1);
    memcpy(&idTaquilla, &data[lon1], lon2);
    agente_taquilla = agente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += idTaquilla;
    preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    if (preventas_taquilla == NULL) {
        buffer.cuantos = 0;
        send2cliente(*conexion, MOSTRAR_PREVENTA_TAQ, (boost::uint8_t const*) &buffer.cuantos, 2);
        return ;
    }
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    nnodes = g_tree_nnodes(preventas_taquilla->preventas);
    buffer.cuantos = 0;
    if (nnodes > 0) {
        g_tree_foreach(preventas_taquilla->preventas, para_cada_preventa2, &buffer);
    }
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    lon1 = 2;
    lon2 = 10 * buffer.cuantos;
    lon = lon1 + lon2;
    buff = new boost::uint8_t[lon];
    memcpy(&buff[0], &buffer.cuantos, lon1);
    if (lon2 > 0) {
        memcpy(&buff[lon1], buffer.buffer, lon2);
        delete [] buffer.buffer;
    }
    send2cliente(*conexion, MOSTRAR_PREVENTA_TAQ, buff, lon);
    delete [] buff;
}

/*--------------------------------------------------------------------------------*/

void
atender_mensaje(ConexionAdmin * conexion, boost::uint8_t const* buffer)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    mensajeria_t m = mensajeria_b2t(buffer);
    enviar_mensaje(m, mysql, *conexion);
}

/*--------------------------------------------------------------------------------*/

void
verificar_estatus_sorteos(ConexionAdmin * conexion)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
    
    std::vector<boost::uint8_t> idSorteos;
    {
        char const* sqlSelectSorteos = "SELECT Id FROM Sorteos ORDER BY HoraCierre ASC";
        ejecutar_sql(mysql, sqlSelectSorteos, DEBUG_VERIFICAR_SORTEOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            idSorteos.push_back(atoi(row[0]));
        }
        mysql_free_result(result);
    }

    boost::uint16_t nrosorteos = 0;
    boost::uint8_t lon = sizeof(nrosorteos);
    boost::uint8_t * buffer = new boost::uint8_t[lon];

    boost::uint8_t idSorteo = 0;
    boost::uint8_t lon1 = sizeof(idSorteo);

    boost::uint8_t estadoActivo;
    boost::uint8_t lon2 = sizeof(estadoActivo);

    for (std::size_t i = 0, size = idSorteos.size(); i < size ; ++i) {
        idSorteo = idSorteos[i];
        Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
        if (sorteo != NULL) {
            if (sorteo->isEstadoActivo() == true) {
                if (hora_actual() > sorteo->getHoraCierre() or sorteo->getDiaActual() != diadehoy()) {
                    sorteo->setEstadoActivo(false);
                }
            }
            nrosorteos++;
            estadoActivo = sorteo->isEstadoActivo() ? 1 : 0;
            boost::uint8_t * aux = new boost::uint8_t[lon + lon1 + lon2];
            memcpy(aux, buffer, lon);
            memcpy(aux + lon, &idSorteo, lon1);
            memcpy(aux + lon + lon1, &estadoActivo, lon2);
            delete [] buffer;
            buffer = aux;
            lon = lon + lon1 + lon2;
        }
    }

    memcpy(buffer, &nrosorteos, sizeof(nrosorteos));
    send2cliente(*conexion, ESTATUS_SORTEOS, buffer, lon);
    delete [] buffer;
}

/*--------------------------------------------------------------------------------*/

void
desactivar_sorteo(boost::uint8_t const* buffer)
{
    boost::uint8_t idSorteo;
    memcpy(&idSorteo, buffer, sizeof(boost::uint8_t));
    Sorteo *sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
    if (sorteo != NULL) {
        bool nuevoEstadoActivo = not sorteo->isEstadoActivo();
        
        if (hora_actual() > sorteo->getHoraCierre()) {
            nuevoEstadoActivo = false;
        }
        if (not sorteo->isEstadoInicialActivo()) {
            nuevoEstadoActivo = false;
        }
        if (nuevoEstadoActivo) {
            sorteo->setEstadoActivo();
        } else {
            sorteo->setEstadoActivo(false);
        }
    }
}

void 
ConexionAdmin::dipatchTask(std::vector<boost::uint8_t> const& buffer)
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> mon(mutex);    
    Cabecera cabeza = Cabecera(&buffer[0]);
    int const longitud = buffer.size() - Cabecera::CONST_RAW_BUFFER_SIZE;
    
    std::ostringstream oss;    
    oss <<  "MENSAJE: peticion = " << unsigned(cabeza.getPeticion()) 
        << " size = " <<  buffer.size() 
        << " hexDump = " << hexDump(buffer);
    log_mensaje(NivelLog::Debug, "debug", "servidor/admin", oss.str().c_str());
       
    switch (cabeza.getPeticion()) {
        case LOGIN_ADMIN: {                
                login_admin_sol_t login = login_admin_sol_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);

                login_admin_t loginresp = dipatch_LOGIN_ADMIN(this, login);

                if (loginresp.idUsuario != 0) {
                    this->idAgente = login.idAgente;
                    this->idUsuario = loginresp.idUsuario;
                    boost::uint8_t *buffer_salida = login_admin_t2b(loginresp);

                    log_admin(NivelLog::Bajo, this,
                               "MENSAJE: login - inicio de sesion exitoso.");
                    send2cliente(*this, LOGIN_ADMIN, buffer_salida, LOGIN_ADMIN_LON);
                    delete [] buffer_salida;
                }
                break;
            }
        case SORTEO_MOD_ST: {
                sorteo_mod_st_t sorteo_mod;
                sorteo_mod_res_t sorteo_mod_res;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, SORTEO_MOD_ST);
                    break;
                }

                if (!tarea_permitida(this, "SORTEO_MOD_ST")) {
                    tarea_no_permitida(this, SORTEO_MOD_ST);
                    break;
                }
                buffer_salida_lon = SORTEO_MOD_RES_LON * (longitud / SORTEO_MOD_ST_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += SORTEO_MOD_ST_LON) {
                    sorteo_mod = sorteo_mod_st_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);
                    sorteo_mod_res = sorteo_mod_st(this, sorteo_mod);

                    memcpy(&buffer_salida[j],
                            sorteo_mod_res_t2b(sorteo_mod_res),
                            SORTEO_MOD_RES_LON);

                    j += SORTEO_MOD_RES_LON;
                }

                send2cliente(*this, SORTEO_MOD_ST, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
        case SORTEO_MOD_H: {
                sorteo_mod_h_t sorteo_mod;
                sorteo_mod_res_t sorteo_mod_res;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, SORTEO_MOD_H);
                    break;
                }

                if (!tarea_permitida(this, "SORTEO_MOD_H")) {
                    tarea_no_permitida(this, SORTEO_MOD_H);
                    break;
                }
                buffer_salida_lon = SORTEO_MOD_RES_LON * (longitud / SORTEO_MOD_H_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += SORTEO_MOD_H_LON) {
                    sorteo_mod = sorteo_mod_h_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);
                    sorteo_mod_res = sorteo_mod_h_sol(this, sorteo_mod);
                    memcpy(&buffer_salida[j],
                            sorteo_mod_res_t2b(sorteo_mod_res),
                            SORTEO_MOD_RES_LON);

                    j += SORTEO_MOD_RES_LON;
                }

                send2cliente(*this, SORTEO_MOD_H, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
        case TAQUILLA_CONECT: {
                log_admin(NivelLog::Bajo, this,
                           "MENSAJE: solicitando las taquillas conectadas");

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, TAQUILLA_CONECT);
                    break;
                }

                if (!tarea_permitida(this, "TAQUILLA_CONECT")) {
                    tarea_no_permitida(this, TAQUILLA_CONECT);
                    break;
                }

                std::vector<taquilla_conect_t> taquillas;

                if (longitud > 0) {
                    for (int i = 0; i < longitud; i += TAQUILLA_CONECT_SOL_LON) {
                        taquilla_conect_sol_t taquilla_conect_sol =
                            taquilla_conect_sol_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);
                        taquilla_conect_buscar(taquilla_conect_sol, taquillas);
                    }
                } else {
                    std::vector<boost::uint32_t> idAgentes;
                    {
                        DataBaseConnetion dbConnetion;
                        MYSQL* mysql = dbConnetion.get();
                        
                        char const* sqlSelectAgentes = "SELECT Agentes.Id FROM Agentes, TiposAgente "
                                                       "WHERE Agentes.IdTipoAgente=TiposAgente.Id "
                                                       "AND TiposAgente.Descripcion='Agencia'";
                        ejecutar_sql(mysql, sqlSelectAgentes,
                                      DEBUG_ATENDER_PETICION_ADMIN + DEBUG_TAQUILLA_CONECT_BUSCAR);
                        MYSQL_RES * result = mysql_store_result(mysql);
                        for (MYSQL_ROW row = mysql_fetch_row(result);
                                NULL != row;
                                row = mysql_fetch_row(result)) {
                            idAgentes.push_back(atoi(row[0]));
                        }
                        mysql_free_result(result);
                    }

                    if (idAgentes.size() < 1) {
                        send2cliente(*this, TAQUILLA_CONECT, NULL, 0);
                        break;
                    }

                    for (std::size_t i = 0, size = idAgentes.size(); i < size ; ++i) {
                        taquilla_conect_sol_t taquilla_conect_sol;
                        taquilla_conect_sol.idAgente = idAgentes[i];
                        taquilla_conect_buscar(taquilla_conect_sol, taquillas);
                    }
                }

                if (taquillas.empty()) {
                    /*
                     * No hay taquillas conectadas a la agencia
                     */
                    send2cliente(*this, TAQUILLA_CONECT, NULL, 0);
                    break;
                }

                int buffer_salida_lon = taquillas.size() * TAQUILLA_CONECT_LON;
                boost::uint8_t *buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0, j = 0; i < buffer_salida_lon; i += TAQUILLA_CONECT_LON) {
                    boost::uint8_t * taquilla_conect =
                        taquilla_conect_t2b(taquillas.at(j));
                    memcpy(&buffer_salida[i], taquilla_conect, TAQUILLA_CONECT_LON);
                    delete [] taquilla_conect;
                    j++;
                }

                send2cliente(*this, TAQUILLA_CONECT, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }

        case AGENCIA_MOD_ST: {
                agencia_mod_st_t agencia_mod;
                agencia_mod_res_t agencia_mod_res;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, AGENCIA_MOD_ST);
                    break;
                }

                if (!tarea_permitida(this, "AGENCIA_MOD_ST")) {
                    tarea_no_permitida(this, AGENCIA_MOD_ST);
                    break;
                }
                buffer_salida_lon = AGENCIA_MOD_RES_LON * (longitud / AGENCIA_MOD_ST_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += AGENCIA_MOD_ST_LON) {
                    agencia_mod = agencia_mod_st_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);

                    agencia_mod_res = agencia_mod_st(this, agencia_mod);
                    memcpy(&buffer_salida[j], agencia_mod_res_t2b(agencia_mod_res),
                            AGENCIA_MOD_RES_LON);

                    j += AGENCIA_MOD_RES_LON;
                }
                send2cliente(*this, AGENCIA_MOD_ST, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
        case NUMERO_REST: {
                numero_rest_t numero_a_restringir;
                numero_rest_res_t numero_rest_res;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, NUMERO_REST);
                    break;
                }

                if (!tarea_permitida(this, "NUMERO_REST")) {
                    tarea_no_permitida(this, NUMERO_REST);
                    break;
                }

                buffer_salida_lon = NUMERO_REST_RES_LON * (longitud / NUMERO_REST_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += NUMERO_REST_LON) {
                    numero_a_restringir = numero_rest_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);
                    numero_rest_res = numero_rest(this, numero_a_restringir);
                    memcpy(&buffer_salida[j], numero_rest_res_t2b(numero_rest_res),
                            NUMERO_REST_RES_LON);

                    j += NUMERO_REST_RES_LON;
                }
                send2cliente(*this, NUMERO_REST, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
            
            case NUMERO_REST_TOPE: {
                numero_rest_t numero_a_restringir;
                numero_rest_res_t numero_rest_res;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, NUMERO_REST);
                    break;
                }

                if (!tarea_permitida(this, "NUMERO_REST")) {
                    tarea_no_permitida(this, NUMERO_REST_TOPE);
                    break;
                }

                buffer_salida_lon = NUMERO_REST_RES_LON * (longitud / NUMERO_REST_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += NUMERO_REST_LON) {
                    numero_a_restringir = numero_rest_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);
                    numero_rest_res = numero_rest_tope(this, numero_a_restringir);
                    memcpy(&buffer_salida[j], numero_rest_res_t2b(numero_rest_res),
                            NUMERO_REST_RES_LON);

                    j += NUMERO_REST_RES_LON;
                }
                send2cliente(*this, NUMERO_REST_TOPE, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
            /*
             * Elimina tickets cuyo tiempo de vencimiento ha expirado y los sorteos esten activos
             */
        case TICKET_ELIM: {
                ticket_elim_t ticket_a_elim;
                ticket_elim_res_t ticket_elim_res;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, TICKET_ELIM);
                    break;
                }
                if (!tarea_permitida(this, "TICKET_ELIM")) {
                    tarea_no_permitida(this, TICKET_ELIM);
                    break;
                }

                buffer_salida_lon = TICKET_ELIM_RES_LON * (longitud / TICKET_ELIM_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += TICKET_ELIM_LON) {
                    ticket_a_elim = ticket_elim_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);
                    ticket_elim_res = ticket_elim(this, ticket_a_elim);

                    memcpy(&buffer_salida[j], ticket_elim_res_t2b(ticket_elim_res),
                            TICKET_ELIM_RES_LON);

                    j += TICKET_ELIM_RES_LON;
                }
                send2cliente(*this, TICKET_ELIM, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
        case LIMITE: {
                limite_sol_t limite_sol;
                limite_t limit;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, LIMITE);
                    break;
                }

                /*
                 * insert into Tareas set Nombre='NUMERO_REST';
                 */
                if (!tarea_permitida(this, "LIMITE")) {
                    tarea_no_permitida(this, LIMITE);
                    break;
                }

                buffer_salida_lon = LIMITE_LON * (longitud / LIMITE_SOL_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += LIMITE_SOL_LON) {
                    limite_sol = limite_sol_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);

                    limit = limite(this, limite_sol);

                    memcpy(&buffer_salida[j], limite_t2b(limit), LIMITE_LON);

                    j += LIMITE_LON;
                }

                send2cliente(*this, LIMITE, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
        case TAQUILLA_NUEVA: {
                taquilla_nueva_sol_t taquilla_nueva_sol;
                taquilla_nueva_t taquilla_n;
                boost::uint8_t *buffer_salida;
                int buffer_salida_lon;
                int j = 0;

                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, TAQUILLA_NUEVA);
                    break;
                }

                if (!tarea_permitida(this, "TAQUILLA_NUEVA")) {
                    tarea_no_permitida(this, TAQUILLA_NUEVA);
                    break;
                }

                buffer_salida_lon = TAQUILLA_NUEVA_LON * (longitud / TAQUILLA_NUEVA_SOL_LON);
                buffer_salida = new boost::uint8_t[buffer_salida_lon];

                for (int i = 0; i < longitud; i += TAQUILLA_NUEVA_SOL_LON) {
                    taquilla_nueva_sol = taquilla_nueva_sol_b2t(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE + i);

                    taquilla_n = taquilla_nueva(this, taquilla_nueva_sol);

                    memcpy(&buffer_salida[j], taquilla_nueva_t2b(taquilla_n), TAQUILLA_NUEVA_LON);

                    j += TAQUILLA_NUEVA_LON;
                }

                send2cliente(*this, TAQUILLA_NUEVA, buffer_salida, buffer_salida_lon);
                delete [] buffer_salida;
                break;
            }
        case ACTUALIZAR_TIPOSMONTO: {
                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, ACTUALIZAR_TIPOSMONTO);
                    break;
                }
                if (!tarea_permitida
                        (this, "ACTUALIZAR_TIPOSMONTO")) {
                    tarea_no_permitida(this, ACTUALIZAR_TIPOSMONTO);
                    break;
                }
                cargar_online_tipos_montoxagencia(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);
                break;
            }
        case CALCULAR_RESUMEN_V_0: {
            if (this->idUsuario == 0) {
                enviarPeticionNoLogeado(this, CALCULAR_RESUMEN_V_0);
                break;
            }
            if (!tarea_permitida(this, "CALCULAR_RESUMEN")) {
                tarea_no_permitida(this, CALCULAR_RESUMEN_V_0);
                break;
            }
            dispatch_CALCULAR_RESUMEN_V_0(this, &buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);
            break;
        }
        case CALCULAR_RESUMEN_V_1: {
            if (this->idUsuario == 0) {
                enviarPeticionNoLogeado(this, CALCULAR_RESUMEN_V_1);
                break;
            }
            if (!tarea_permitida(this, "CALCULAR_RESUMEN")) {
                tarea_no_permitida(this, CALCULAR_RESUMEN_V_1);
                break;
            }
            dispatch_CALCULAR_RESUMEN_V_1(this, &buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);
            break;
        }
        case CALCULAR_PREVENTA_TAQ: {
                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, CALCULAR_PREVENTA_TAQ);
                    break;
                }
                if (!tarea_permitida
                        (this, "CALCULAR_PREVENTA_TAQ ")) {
                    tarea_no_permitida(this, CALCULAR_PREVENTA_TAQ);
                    break;
                }
                consultar_preventas(this);
                break;
            }
        case MOSTRAR_PREVENTA_TAQ: {
                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, MOSTRAR_PREVENTA_TAQ);
                    break;
                }
                if (!tarea_permitida(this, "MOSTRAR_PREVENTA_TAQ")) {
                    tarea_no_permitida(this, MOSTRAR_PREVENTA_TAQ);
                    break;
                }
                consultar_preventas_taquilla(this, &buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);
                break;
            }
        case MENSAJERIA: {
                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, MENSAJERIA);
                    break;
                }
                if (!tarea_permitida(this, "MENSAJERIA")) {
                    tarea_no_permitida(this, MENSAJERIA);
                    break;
                }
                atender_mensaje(this, &buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);
                break;
            }           
        case ESTATUS_SORTEOS: {
                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, MENSAJERIA);
                    break;
                }
                verificar_estatus_sorteos(this);
                break;
            }
        case DESACTIVAR_SORTEO: {
                if (this->idUsuario == 0) {
                    enviarPeticionNoLogeado(this, MENSAJERIA);
                    break;
                }
                desactivar_sorteo(&buffer[0] + Cabecera::CONST_RAW_BUFFER_SIZE);
                break;
            }
    }
}

}
