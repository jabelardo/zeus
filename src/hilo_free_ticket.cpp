#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <mysql/mysql.h>
#include <zlib.h>
#include <unistd.h>

#include <global.h>
#include <sockets.h>
#include <utils.h>
#include <sorteo.h>
#include <database/DataBaseConnetionPool.h>
#include <SocketThread.h>
#include <TipoDeMontoDeAgencia.h>

#include <ProtocoloZeus/Mensajes.h>
#include <ProtocoloZeus/Protocolo.h>

using namespace ProtocoloZeus;

#define DEBUG_CADA_TAQUILLA 0

gboolean
para_cada_taquilla(void* key, void* value, void*)
{
	boost::uint64_t agente_taquilla = *(boost::uint64_t *) key;
	agente_taquilla = agente_taquilla >> 32;
	boost::uint32_t idAgente = agente_taquilla;
	agente_taquilla = *(boost::uint64_t *) key;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla = agente_taquilla >> 32;
	boost::uint8_t idTaquilla = agente_taquilla;

	n_t_preventas_x_taquilla_t * preventas_taquilla = (n_t_preventas_x_taquilla_t *) value;
	boost::uint8_t idZona = preventas_taquilla->idZona;
	n_t_zona_numero_restringido_t * zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);

	time_t hora1 = preventas_taquilla->hora_ultimo_renglon;
	time_t hora2 = time(NULL);
	time_t dif = (hora2 - hora1) / 60;
	if (dif >= configuracion.maximoTiempoVenta) {
		boost::uint32_t nnodes = g_tree_nnodes(preventas_taquilla->preventas);
		TipoDeMontoPorAgencia * tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
		if (nnodes > 0) {
			pthread_t hilo = 0;
			bool hiloExiste = false;
			{
                DataBaseConnetion dbConnetion;
                MYSQL* mysql = dbConnetion.get();
				boost::format sqlQuery;
				sqlQuery.parse("SELECT Hilo FROM Taquillas WHERE IdAgente=%1% AND Numero=%2% AND Hilo IS NOT NULL")
				% idAgente % unsigned(idTaquilla);
				ejecutar_sql(mysql, sqlQuery, DEBUG_CADA_TAQUILLA);
				MYSQL_RES * result = mysql_store_result(mysql);
				MYSQL_ROW row = mysql_fetch_row(result);
				if (NULL != row) {
					hilo = pthread_t(strtoul(row[0], NULL, 10));
					hiloExiste = true;
				}
				mysql_free_result(result);
			}

			if (hiloExiste) {
				SocketThread::MutexGuard g;
				SocketThread * socketsThread = SocketThread::get(hilo);
				if (socketsThread != NULL) {
					ConexionActiva& conexionActiva = socketsThread->getConexionTaquilla();
					if (conexionActiva.idAgente == idAgente && conexionActiva.idTaquilla == idTaquilla) {
						char mensaje[BUFFER_SIZE];
						sprintf(mensaje, "Ticket Vencido");
						log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
						send2cliente(conexionActiva, TICKET_VENCIDO, NULL, 0);
					}					
				}
			}
			preventas_taquilla->hora_ultimo_renglon = time(NULL);
		}
		boost::uint16_t renglon = 1;
		while (nnodes > 0) {
			Preventa * preventa =
			    (Preventa*) g_tree_lookup(preventas_taquilla->preventas, &renglon);
			if (preventa != NULL) {
				boost::uint8_t idSorteo = preventa->idSorteo;
				boost::uint16_t numero = preventa->numero;
				Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
				n_t_sorteo_numero_restringido_t * sorteo_num_restringido =
				    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
				                                                         t_sorteo_numero_restringido,
				                                                         &idSorteo);
				TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
				if (tipomonto_sorteo == NULL) {
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "La agencia [%u] no tiene Tipos de Monto para el sorteo [%u]",
					         idAgente, idSorteo);
					log_mensaje(NivelLog::Bajo, "ADVERTENCIA", "Cancelar Preventa", mensaje);
					break;
				}
				if (preventa->tipo == TERMINAL || preventa->tipo == TERMINALAZO) {
					boost::uint8_t tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
					n_t_tipomonto_t * tipoDeMonto =
					    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
					                                         &tipomonto_terminal);

					sem_wait(&tipoDeMonto->sem_t_tipomonto);
					n_t_venta_numero_t * venta_numero =
					    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
					venta_numero->preventa -= preventa->monto_preventa;
					sem_post(&tipoDeMonto->sem_t_tipomonto);
					sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
					n_t_numero_restringido_t * num_restringido =
					    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
					                                                  t_numero_restringido_terminal,
					                                                  &numero);
					if (num_restringido != NULL) {
						num_restringido->preventa -= preventa->monto_preventa;
					}
					sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
				} else if (preventa->tipo == TRIPLE || preventa->tipo == TRIPLETAZO) {
					boost::uint8_t tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
					n_t_tipomonto_t * tipoDeMonto =
					    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(),
					                                         &tipomonto_triple);
					sem_wait(&tipoDeMonto->sem_t_tipomonto);
					n_t_venta_numero_t * venta_numero =
					    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
					venta_numero->preventa -= preventa->monto_preventa;
					sem_post(&tipoDeMonto->sem_t_tipomonto);
					sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
					n_t_numero_restringido_t * num_restringido =
					    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
					                                                  t_numero_restringido_triple,
					                                                  &numero);
					if (num_restringido != NULL) {
						num_restringido->preventa -= preventa->monto_preventa;
					}
					sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
				}
				g_tree_remove(preventas_taquilla->preventas, &renglon);
			}
			renglon++;
			nnodes = g_tree_nnodes(preventas_taquilla->preventas);
		}
	}
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	return FALSE;
}

void *
hilo_free_tickets(void*)
{
	struct timespec t;
	t.tv_sec = tiempoChequeoPreventa;
	t.tv_nsec = 0;

	while (!finalizar) {
		g_tree_foreach(t_preventas_x_taquilla, para_cada_taquilla, NULL);
		nanosleep(&t, NULL);
	}
	return NULL;
}
