#ifndef UTILS_H
#define UTILS_H

#include <global.h>
#include <mysql/mysql.h>
#include <ProtocoloZeus/Protocolo.h>
#include <boost/format.hpp>
#include <string>
#include <ConexionActiva.h>

std::string getString(char const * source, std::size_t bufferSize);

boost::int16_t hora_actual();

struct tm hora_i2t(boost::uint16_t hora);

int diadehoy();

struct tm fecha_i2t(boost::uint32_t fecha);

void genera_password(boost::uint8_t const* fuente, char destino[33]);

bool ejecutar_sql(MYSQL * mysql_con, char const* tsql, bool debug_sql);

bool ejecutar_sql(MYSQL * mysql_con, boost::format const& sqlQuery, bool debug);

void limite_tipomonto(n_t_tipomonto_t * n_t_tipomonto, boost::uint8_t tipo);

void log_mensaje(boost::uint8_t nivel, const char * tipo, const char * origen,
                  const char * mensaje);

#define log_clientes(nivel,conexionActiva,mensaje) log_acceso(nivel,nivelLogAccesoTaquillas,conexionActiva,mensaje,&mutex_t_fd_log_clientes,fd_log_clientes)

#define log_admin(nivel,conexionActiva,mensaje) log_acceso(nivel,nivelLogAccesoAdmin,conexionActiva,mensaje,&mutex_t_fd_log_admin,fd_log_admin)

void log_acceso(boost::uint8_t nivel, boost::uint8_t NIVEL_DE_LOG, ConexionActiva * conexionActiva,
                 const char * mensaje, sem_t * mutex_t_fd_log, int fd_log);

void cerrar_archivos_de_registro();

void enviarPeticionNoLogeado(ConexionActiva * conexionActiva, boost::int8_t peticion);

void tarea_no_permitida(ConexionActiva * conexionActiva, boost::int8_t peticion);

void enviar_mensaje(ProtocoloZeus::mensajeria_t m, MYSQL * mysql_con,
                     ConexionActiva& conexionActiva);

struct datos_premios_t
{
	boost::uint16_t numeros[4];
	boost::uint8_t comodin;
	boost::uint32_t montoAdicional;
};

#endif // UTILS_H
