#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <set>
#include <map>
#include <limits>
#include <iostream>

#include <zlib.h>
#include <mysql/mysql.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Connector.h>

#include <global.h>
#include <sockets.h>
#include <ventas.h>
#include <utils.h>
#include <atender_peticion.h>
#include <sorteo.h>
#include <database/DataBaseConnetionPool.h>
#include <TipoDeMontoDeAgencia.h>

#include <ProtocoloExterno/Mensajes/MensajeFactory.h>
#include <ProtocoloExterno/Exceptions/ServiceNoFoundException.h>
#include <ProtocoloExterno/Exceptions/ReponseException.h>
#include <ProtocoloExterno/Exceptions/RecordNotFoundException.h>
#include <ProtocoloExterno/Exceptions/InvalidArgumentException.h>

#include <ProtocoloZeus/Mensajes.h>
#include <ProtocoloZeus/Protocolo.h>

using namespace ProtocoloZeus;

/**************************************************************************/

#define DEBUG_CONFIRMACION_GUARDADOS 0
#define DEBUG_VERIFICAR_ULTIMO_TICKET 0
#define DEBUG_PROCESAR_TICKET 0
#define DEBUG_LIBERAR_VENTA 0
#define DEBUG_ELIMINAR_TICKET 0
#define DEBUG_MODIFICAR_TICKET 0
#define DEBUG_REPETIR_TICKET 0
#define DEBUG_PAGAR_TICKET 0
#define DEBUG_CONSULTAR_TICKET 1
#define DEBUG_CONSULTAR_SALDO 0
#define DEBUG_CONSULTAR_PREMIOS 0
#define DEBUG_RESUMEN_TICKETS_PREMIADOS 0
#define DEBUG_TACHAR_TICKET 0
#define DEBUG_realizarVentaExterna 0
#define DEBUG_TODOS_PREMIOS 0

#define BASE 10

/**************************************************************************/

boost::int16_t
busquedaBinaria(boost::uint16_t arreglo[], boost::uint16_t num, boost::int16_t posini, boost::int16_t posfin)
{
	if (posini > posfin) {
		return -1;
	}
	boost::int16_t centro = posini + ((posfin - posini) / 2);
	boost::int16_t n = arreglo[centro];
	if (n == num) {
		return centro;
	}
	if (num < arreglo[centro]) {
		posfin = centro - 1;
	} else {
		posini = centro + 1;
	}
	return busquedaBinaria(arreglo, num, posini, posfin);
}

/**************************************************************************/
bool
encontrar(boost::uint16_t arreglo[], boost::uint16_t numero, boost::uint16_t sizeVal)
{
	bool encontrado = false;
	for (boost::uint16_t i = 0; i < sizeVal and not encontrado; i++) {
		if (numero == arreglo[i])
			encontrado = true;
	}
	return encontrado;
}

void
permutarTerminal(boost::uint32_t conjunto, boost::uint16_t permutas[], boost::uint16_t * cuantas)
{
	char n[12];
	sprintf(n, "%u", conjunto);

	boost::int16_t len = strlen(n);
	boost::uint8_t permuta[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	{
		boost::int16_t i = 0;
		do {
			permuta[i++] = conjunto % 10;
			conjunto /= 10;
		} while (conjunto > 0);
	}

	if (len == 1) {
		boost::int16_t w = 0;
		for (boost::int16_t i = 0; i <= 9; i++) {
			long numero = 10 * i + permuta[0];
			long numero2 = permuta[0] * 10 + i;
			if (not encontrar(permutas, numero, w)) {
				permutas[w] = numero;
				++w;
			}
			if (not encontrar(permutas, numero2, w)) {
				permutas[w] = numero2;
				++w;
			}
		}
		*cuantas = w;
		return ;
	}

	boost::uint16_t permutas2[1000];
	boost::int16_t w = 0;
	for (boost::int16_t k = 0; k < len; k++) {
		for (boost::int16_t l = 0; l < len; l++) {
			if (l != k) {
				long numero = permuta[k] * 10 + permuta[l];
				if (busquedaBinaria(permutas2, numero, 0, w - 1) == -1) {
					permutas2[w] = numero;
					w++;
				}
			}
		}
	}

	boost::int16_t k = 0;
	for (boost::int16_t i = w - 1; i >= 0; i--) {
		permutas[k] = permutas2[i];
		k++;
	}

	*cuantas = w;
}

/**************************************************************************/

void
separar(char n[], int pos)
{
	switch (pos) {
		case 0:
			n[0] = '*';
			break;
		case 1:
			n[0] = n[1];
			n[1] = '*';
			break;
		case 2:
			n[0] = n[1];
			n[1] = n[2];
			n[2] = '*';
			break;
	}
}

/**************************************************************************/

void
separar2(char n[], int pos)
{
	switch (pos) {
		case 0:
			n[0] = '*';
			break;
		case 1:
			n[0] = n[1];
			n[1] = '*';
			break;
	}
}

/**************************************************************************/
template<typename Serie_T>
void
seriesTriple(Serie_T const& serieTriple, boost::uint16_t series[], boost::uint16_t * cuantas)
{
	char num[4];
	sprintf(num, "%03d", serieTriple.numero);
	num[3] = '\0';

	for (int i = 2; i >= 0; i--) {
		int j = 1;
		j = j << i;
		if (j & serieTriple.posicion)
			separar(num, i);
	}

	int w = 0;
	for (int i = 0; i <= 9; i++) {
		char number[4];
		strcpy(number, num);
		for (int j = 0; j < 3; j++) {
			if (num[j] == '*') {
				number[j] = i + 48;
			}
		}
		series[w] = atoi(number);
		w++;
	}
	*cuantas = w;
}

/**************************************************************************/

template<typename Serie_T>
void
seriesTerminal(Serie_T const& serieTerminal, boost::uint16_t series[], boost::uint16_t * cuantas)
{
	char num[3];
	sprintf(num, "%02d", serieTerminal.numero);
	num[2] = '\0';

	for (int i = 1; i >= 0; i--) {
		int j = 1;
		j = j << i;
		if (j & serieTerminal.posicion) {
			separar2(num, i);
		}
	}

	int w = 0;
	for (int i = 0; i <= 9; i++) {
		char number[3];
		strcpy(number, num);
		for (int j = 0; j < 2; j++) {
			if (num[j] == '*') {
				number[j] = i + 48;
			}
		}
		series[w] = atoi(number);
		w++;
	}
	*cuantas = w;
}

/**************************************************************************/

void
corrida(boost::uint16_t begin, boost::uint16_t final, boost::uint16_t corridas[], boost::uint16_t * cuantas)
{
	int w = 0;
	for (int i = begin; i <= final; i++) {
		corridas[w] = i;
		w++;
	}
	*cuantas = w;
}

/**************************************************************************/

void
permutarTriple(boost::uint32_t conjunto, boost::uint16_t permutas[], boost::uint16_t * cuantas)
{
	char n[12];
	int len = 0;
	short int permuta[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	if (conjunto > 999999999) {
		conjunto /= 1000000000;
		switch (conjunto) {
			case 1: {
					len = 1;
					permuta[0] = 0;
					strcpy(n, "0");
					break;
				}
			case 2: {
					len = 2;
					permuta[0] = permuta[1] = 0;
					strcpy(n, "00");
					break;
				}
			case 3: {
					len = 3;
					permuta[0] = permuta[1] = permuta[2] = 0;
					strcpy(n, "000");
					break;
				}
		}
	} else {
		sprintf(n, "%u", conjunto);
		len = strlen(n);

		int i = 0;
		do {
			permuta[i++] = conjunto % 10;
			conjunto /= 10;
		} while (conjunto > 0);
	}

	int w = 0;
	if (len == 1) {
		char ctriple[4];
		for (int i = 0; i <= 999; i++) {
			sprintf(ctriple, "%03d", i);
			if (strstr(ctriple, n) != NULL) {
				permutas[w] = atoi(ctriple);
				w++;
			}
		}
	} else if (len == 2) {
		char cone[2];
		cone[0] = n[0];
		cone[1] = '\0';

		char ctwo[2];
		ctwo[0] = n[1];
		ctwo[1] = '\0';

		for (int i = 0; i <= 999; i++) {
			char ctriple[4], ctripleaux[4];
			sprintf(ctriple, "%03d", i);
			strcpy(ctripleaux, ctriple);

			for (int pos = 0; pos < 3; pos++) {
				if (ctriple[pos] == cone[0]) {
					ctripleaux[pos] = ' ';
					break;
				}
			}
			if (strstr(ctriple, cone) != NULL and strstr(ctripleaux, ctwo) != NULL) {
				permutas[w] = atoi(ctriple);
				w++;
			}
		}
	} else {
		boost::uint16_t permutas2[1000];
		for (int i = 0; i < len; i++) {
			for (int k = 0; k < len; k++) {
				for (int l = 0; l < len; l++) {
					if (i != k and i != l and l != k) {
						long numero = permuta[i] * 100 + permuta[k] * 10 + permuta[l];
						if (busquedaBinaria(permutas2, numero, 0, w - 1) == -1) {
							permutas2[w] = numero;
							w++;
						}
					}
				}
			}
		}
		int k = 0;
		for (int i = w - 1; i >= 0; i--) {
			permutas[k] = permutas2[i];
			k++;
		}
	}
	*cuantas = w;
}

/**************************************************************************/

void
morochosTerminal(boost::uint32_t conjunto, boost::uint16_t numeros[], boost::uint16_t * cuantas)
{
	char sconjunto[10];
	sprintf(sconjunto, "%u", conjunto);

	boost::uint16_t n = strlen(sconjunto);
	short int permuta[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	{
		boost::uint16_t i = n - 1;
		do {
			permuta[i] = conjunto % 10;
			conjunto /= 10;
			i = i - 1;
		} while (conjunto > 0);
	}

	for (boost::uint16_t i = 0; i < n; i++) {
		numeros[i] = 100;
	}

	boost::uint16_t w = 0;
	for (boost::uint16_t i = 0; i < n; i++) {
		boost::uint16_t terminal = permuta[i] * 10 + permuta[i];
		if (not encontrar(numeros, terminal, w)) {
			numeros[w] = terminal;
			w++;
		}
	}
	*cuantas = w;
}

/*-------------------------------------------------------------------------*/

void
morochos(boost::uint32_t conjunto, boost::uint16_t numeros[], boost::uint16_t * cuantas)
{
	char sconjunto[10];
	sprintf(sconjunto, "%u", conjunto);

	int n = strlen(sconjunto);
	if (n == 1) {
		char ctriple[4];
		int w = 0;
		for (int i = 0; i <= 999; i++) {
			sprintf(ctriple, "%03d", i);
			if ((ctriple[0] == sconjunto[0] and ctriple[1] == sconjunto[0])
			        or (ctriple[0] == sconjunto[0] and ctriple[2] == sconjunto[0])
			        or (ctriple[1] == sconjunto[0] and ctriple[2] == sconjunto[0])) {
				numeros[w] = atoi(ctriple);
				w++;
			}
		}
		*cuantas = w;
		return ;
	}

	int w = 0;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			for (int k = 0; k < n; k++) {
				if (sconjunto[i] == sconjunto[j]
				        or sconjunto[i] == sconjunto[k]
				        or sconjunto[j] == sconjunto[k]) {
					char snumero[4];
					sprintf(snumero, "%c%c%c", sconjunto[i], sconjunto[j], sconjunto[k]);
					int numero = atoi(snumero);

					bool encontrado = false;

					for (int z = 0; z < w and not encontrado; z++) {
						if (numeros[z] == numero)
							encontrado = true;
					}

					if (not encontrado) {
						numeros[w] = numero;
						w++;
					}
				}
			}
		}
	}
	*cuantas = w;
}

/**************************************************************************/
void
modificarRenglon(ConexionActiva& conexionActiva, boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                 boost::uint16_t renglon, bool acero)
{
	boost::uint64_t agente_taquilla;
	n_t_preventas_x_taquilla_t *preventas_taquilla;
	Preventa *preventa;
	boost::uint8_t idSorteo, idZona;
	boost::uint16_t numero;
	Sorteo *sorteo;
	TipoDeMontoPorAgencia *tipomonto_agencia;
	TipoDeMonto *tipomonto_sorteo;
	n_t_venta_numero_t *venta_numero;
	n_t_tipomonto_t *tipoDeMonto;
	n_t_zona_numero_restringido_t *zona_num_restringido;
	n_t_sorteo_numero_restringido_t *sorteo_num_restringido;
	n_t_numero_restringido_t *num_restringido;

	agente_taquilla = idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += idTaquilla;
	preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	idZona = preventas_taquilla->idZona;
	zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	if (zona_num_restringido == NULL) {
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "ADVERTENCIA: la zona <%u> esta en memoria", idZona);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL) {
		idSorteo = preventa->idSorteo;
		numero = preventa->numero;
		sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
		sorteo_num_restringido =
		    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
		                                                         t_sorteo_numero_restringido,
		                                                         &idSorteo);
		tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
		if (preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO) {
			boost::uint8_t tipomonto_terminal;

			tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
			tipoDeMonto =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
			                                         &tipomonto_terminal);
			if (tipoDeMonto == NULL) {
				char mensaje[BUFFER_SIZE];

				sprintf(mensaje,
				         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria "
                         "'debe reiniciar el servidor'.",
				         tipomonto_terminal);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				return ;
			}
			sem_wait(&tipoDeMonto->sem_t_tipomonto);
			venta_numero =
			    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
			venta_numero->preventa -= preventa->monto_preventa;
			{
				char mensaje[BUFFER_SIZE];

				if (preventa->tipo == TERMINAL)
					sprintf(mensaje, "Modificando el renglon %d para terminales", renglon);
				if (preventa->tipo == TERMINALAZO)
					sprintf(mensaje, "Modificando el renglon %d para terminalazo", renglon);
				log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
			}
			{
				char mensaje[BUFFER_SIZE];
				if (preventa->tipo == TERMINAL)
					sprintf(mensaje, "Terminal %02d preventa %d", numero, venta_numero->preventa);
				if (preventa->tipo == TERMINALAZO)
					sprintf(mensaje, "Terminalazo %04d preventa %d", numero, venta_numero->preventa);
				log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
			}
			sem_post(&tipoDeMonto->sem_t_tipomonto);
			sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			num_restringido =
			    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
			                                                  t_numero_restringido_terminal,
			                                                  &numero);
			if (num_restringido != NULL) {
				num_restringido->preventa -= preventa->monto_preventa;
			}
			sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			if (acero) {
				preventa->monto_preventa = 0;
				preventa->monto_pedido = 0;
			} else
				g_tree_remove(preventas_taquilla->preventas, &renglon);
		} else if (preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO) {
			boost::uint8_t tipomonto_triple;

			tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
			tipoDeMonto =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipomonto_triple);
			if (tipoDeMonto == NULL) {
				char mensaje[BUFFER_SIZE];

				sprintf(mensaje,
				         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
				         tipomonto_triple);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			}
			sem_wait(&tipoDeMonto->sem_t_tipomonto);
			venta_numero =
			    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
			venta_numero->preventa -= preventa->monto_preventa;
			{
				char mensaje[BUFFER_SIZE];
				if (preventa->tipo == TRIPLE)
					sprintf(mensaje, "Modificando el renglon %d para triples", renglon);
				if (preventa->tipo == TRIPLETAZO)
					sprintf(mensaje, "Modificando el renglon %d para tripletazo", renglon);
				log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
			}
			{
				char mensaje[BUFFER_SIZE];
				if (preventa->tipo == TRIPLE)
					sprintf(mensaje, "Triple %03d preventa %d", numero, venta_numero->preventa);
				if (preventa->tipo == TRIPLETAZO)
					sprintf(mensaje, "Tripletazo %05d preventa %d", numero, venta_numero->preventa);
				log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
			}
			sem_post(&tipoDeMonto->sem_t_tipomonto);
			sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			num_restringido =
			    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
			                                                  t_numero_restringido_triple, &numero);
			if (num_restringido != NULL) {
				num_restringido->preventa -= preventa->monto_preventa;
			}
			sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			if (acero) {
				preventa->monto_preventa = 0;
				preventa->monto_pedido = 0;
			} else
				g_tree_remove(preventas_taquilla->preventas, &renglon);
		}
	}
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
}

/*-------------------------------------------------------------------------*/
Preventa *
preventa_terminal(ConexionActiva& conexionActiva, boost::int32_t monto, boost::uint16_t terminal,
                   n_t_tipomonto_t * tipoDeMonto,
                   n_t_zona_numero_restringido_t * zona_num_restringido,
                   n_t_sorteo_numero_restringido_t * sorteo_num_restringido, boost::uint16_t renglon,
                   boost::uint8_t idSorteo, n_t_preventas_x_taquilla_t * preventas_taquilla,
                   boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                   TipoDeMonto * tipomontoterminal)
{
	boost::int8_t completo;
	boost::int32_t monto_disponible, limite, limite_restringido;
	double pro_restringido;
	n_t_venta_numero_t *venta_numero;
	n_t_numero_restringido_t *num_restringido;
	Preventa *preventa;
	boost::uint16_t *renglonPtr;

	preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	sem_wait(&tipoDeMonto->sem_t_tipomonto);
	sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	{
		venta_numero =
		    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero,
		                                            (boost::uint16_t *) & terminal);
		if (venta_numero == NULL) {
			char mensaje[BUFFER_SIZE];

			sprintf(mensaje, "ERROR Llegando Numero %i que no es un Terminal", terminal);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, TICKET_VENCIDO, NULL, 0);
			completo = true;
			goto end;
		}
		num_restringido =
		    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
		                                                  t_numero_restringido_terminal,
		                                                  (boost::uint16_t *) & terminal);
		pro_restringido = num_restringido != NULL ? num_restringido->proporcion : 1;
		limite = tipoDeMonto->limite_actual - (venta_numero->venta + venta_numero->preventa);
		limite_restringido = (boost::int32_t) (tipoDeMonto->limite_actual * pro_restringido);
		
		if (num_restringido != NULL
		    and num_restringido->tope > 0 
	        and boost::uint16_t(limite_restringido) > num_restringido->tope) 
	    {
			limite_restringido = num_restringido->tope;
		}
		
		limite_restringido = (limite_restringido / BASE) * BASE;
		monto_disponible = (boost::int32_t) (limite * tipomontoterminal->limdiscterm);
		monto_disponible = (boost::int32_t) (monto_disponible * pro_restringido);
		monto_disponible = (monto_disponible / BASE) * BASE;
		if (num_restringido != NULL) {
			boost::int32_t sumatoria;

			sumatoria = monto_disponible + num_restringido->venta + num_restringido->preventa;
			if (sumatoria > limite_restringido) {
				monto_disponible =
				    limite_restringido - (num_restringido->venta + num_restringido->preventa);
				monto_disponible = monto_disponible > 0 ? monto_disponible : 0;
			}
		}
		monto_disponible = (monto_disponible / BASE) * BASE;
		preventa = new Preventa;
		preventa->tipo = TERMINAL;
		preventa->numero = terminal;
		preventa->monto_pedido = monto;
		if (monto_disponible >= monto) {
			preventa->monto_preventa = monto;
			completo = true;
			if (num_restringido != NULL)
				num_restringido->preventa += monto;
		} else {
			preventa->monto_preventa = monto_disponible;
			completo = false;
			if (num_restringido != NULL)
				num_restringido->preventa += monto_disponible;
		}
		preventa->monto_preventa = ((preventa->monto_preventa < 0)
		                             or (preventa->monto_preventa % BASE !=
		                                  0)) ? 0 : preventa->monto_preventa;
		venta_numero->preventa += preventa->monto_preventa;

		preventa->idSorteo = idSorteo;
		renglonPtr = new boost::uint16_t;
		*renglonPtr = renglon;
		g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
		preventas_taquilla->hora_ultimo_renglon = time(NULL);
		preventas_taquilla->hora_ultimo_peticion = time(NULL);
	}
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Terminal %02d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
end:
	sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	sem_post(&tipoDeMonto->sem_t_tipomonto);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	if (completo)
		return NULL;
	else
		return preventa;
}

/**************************************************************************/

Preventa *
preventa_terminalazo(ConexionActiva& conexionActiva, boost::int32_t monto, boost::uint16_t terminalazo,
                      n_t_tipomonto_t * tipoDeMonto,
                      n_t_zona_numero_restringido_t * zona_num_restringido,
                      n_t_sorteo_numero_restringido_t * sorteo_num_restringido, boost::uint16_t renglon,
                      boost::uint8_t idSorteo, n_t_preventas_x_taquilla_t * preventas_taquilla,
                      boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                      TipoDeMonto * tipomontoterminal)
{
	boost::int8_t completo;
	boost::int32_t monto_disponible, limite, limite_restringido;
	double pro_restringido;
	n_t_venta_numero_t *venta_numero;
	n_t_numero_restringido_t *num_restringido;
	Preventa *preventa;
	boost::uint16_t *renglonPtr;

	preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	sem_wait(&tipoDeMonto->sem_t_tipomonto);
	sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	{
		venta_numero =
		    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero,
		                                            (boost::uint16_t *) & terminalazo);
		if (venta_numero == NULL) {
			char mensaje[BUFFER_SIZE];

			sprintf(mensaje, "ERROR Llegando Numero %i que no es un Terminalazo", terminalazo);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, TICKET_VENCIDO, NULL, 0);
			completo = true;
			goto end;
		}
		num_restringido =
		    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
		                                                  t_numero_restringido_terminal,
		                                                  (boost::uint16_t *) & terminalazo);
		pro_restringido = num_restringido != NULL ? num_restringido->proporcion : 1;
		limite = tipoDeMonto->limite_actual - (venta_numero->venta + venta_numero->preventa);
		limite_restringido = (boost::int32_t) (tipoDeMonto->limite_actual * pro_restringido);
		
		if (num_restringido != NULL
		    and num_restringido->tope > 0 
	        and boost::uint16_t(limite_restringido) > num_restringido->tope) 
	    {
			limite_restringido = num_restringido->tope;
		}
		
		limite_restringido = (limite_restringido / BASE) * BASE;
		monto_disponible = (boost::int32_t) (limite * tipomontoterminal->limdiscterm);
		monto_disponible = (boost::int32_t) (monto_disponible * pro_restringido);
		monto_disponible = (monto_disponible / BASE) * BASE;
		if (num_restringido != NULL) {
			boost::int32_t sumatoria;

			sumatoria = monto_disponible + num_restringido->venta + num_restringido->preventa;
			if (sumatoria > limite_restringido) {
				monto_disponible =
				    limite_restringido - (num_restringido->venta + num_restringido->preventa);
				monto_disponible = monto_disponible > 0 ? monto_disponible : 0;
			}
		}
		monto_disponible = (monto_disponible / BASE) * BASE;
		preventa = new Preventa;
		preventa->tipo = TERMINALAZO;
		preventa->numero = terminalazo;
		preventa->monto_pedido = monto;
		if (monto_disponible >= monto) {
			preventa->monto_preventa = monto;
			completo = true;
			if (num_restringido != NULL)
				num_restringido->preventa += monto;
		} else {
			preventa->monto_preventa = monto_disponible;
			completo = false;
			if (num_restringido != NULL)
				num_restringido->preventa += monto_disponible;
		}
		preventa->monto_preventa = ((preventa->monto_preventa < 0)
		                             or (preventa->monto_preventa % BASE !=
		                                  0)) ? 0 : preventa->monto_preventa;
		venta_numero->preventa += preventa->monto_preventa;

		preventa->idSorteo = idSorteo;
		renglonPtr = new boost::uint16_t;
		*renglonPtr = renglon;
		g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
		preventas_taquilla->hora_ultimo_renglon = time(NULL);
		preventas_taquilla->hora_ultimo_peticion = time(NULL);
	}
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Terminalazo %04d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
end:
	sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	sem_post(&tipoDeMonto->sem_t_tipomonto);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	if (completo)
		return NULL;
	else
		return preventa;
}

/**************************************************************************/

void
agregar_renglon_vacio_terminal(ConexionActiva& conexionActiva, boost::uint16_t renglon,
                                boost::uint16_t terminal, boost::uint8_t idSorteo,
                                n_t_preventas_x_taquilla_t * preventas_taquilla, boost::uint32_t idAgente,
                                boost::uint8_t idTaquilla)
{
	Preventa * preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	preventa = new Preventa;
	preventa->tipo = TERMINAL;
	preventa->numero = terminal;
	preventa->idSorteo = idSorteo;
	preventa->monto_preventa = 0;
	preventa->monto_pedido = 0;
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Terminal %02d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
	boost::uint16_t *renglonPtr = new boost::uint16_t;
	*renglonPtr = renglon;
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
}

/*-------------------------------------------------------------------------*/

void
agregar_renglon_vacio_terminalazo(ConexionActiva& conexionActiva, boost::uint16_t renglon,
                                   boost::uint16_t terminalazo, boost::uint8_t idSorteo,
                                   n_t_preventas_x_taquilla_t * preventas_taquilla, boost::uint32_t idAgente,
                                   boost::uint8_t idTaquilla)
{
	Preventa * preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	preventa = new Preventa;
	preventa->tipo = TERMINALAZO;
	preventa->numero = terminalazo;
	preventa->idSorteo = idSorteo;
	preventa->monto_preventa = 0;
	preventa->monto_pedido = 0;
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Terminalazo %04d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
	boost::uint16_t *renglonPtr = new boost::uint16_t;
	*renglonPtr = renglon;
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
}

/*-------------------------------------------------------------------------*/

Preventa *
preventa_triple(ConexionActiva& conexionActiva, boost::int32_t monto, boost::uint16_t triple,
                 n_t_tipomonto_t * tipoDeMonto,
                 n_t_zona_numero_restringido_t * zona_num_restringido,
                 n_t_sorteo_numero_restringido_t * sorteo_num_restringido, boost::uint16_t renglon,
                 boost::uint8_t idSorteo, n_t_preventas_x_taquilla_t * preventas_taquilla,
                 boost::uint32_t idAgente, boost::uint8_t idTaquilla, TipoDeMonto * tipomontotriple)
{
	boost::int8_t completo;
	boost::int32_t monto_disponible, limite, limite_restringido;
	double pro_restringido;
	n_t_venta_numero_t *venta_numero;
	n_t_numero_restringido_t *num_restringido;
	boost::uint16_t *renglonPtr;

	Preventa *preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	sem_wait(&tipoDeMonto->sem_t_tipomonto);
	sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	{
		venta_numero =
		    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero,
		                                            (boost::uint16_t *) & triple);

		if (venta_numero == NULL) {
			char mensaje[BUFFER_SIZE];

			sprintf(mensaje, "ERROR Llegando Numero %i que no es un Triple", triple);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, TICKET_VENCIDO, NULL, 0);
			completo = true;
			goto end;
		}

		num_restringido =
		    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
		                                                  t_numero_restringido_triple,
		                                                  (boost::uint16_t *) & triple);
		pro_restringido = num_restringido != NULL ? num_restringido->proporcion : 1;
		limite = tipoDeMonto->limite_actual - (venta_numero->venta + venta_numero->preventa);
		limite_restringido = (boost::int32_t) (tipoDeMonto->limite_actual * pro_restringido);
		
		if (num_restringido != NULL
		    and num_restringido->tope > 0 
	        and boost::uint16_t(limite_restringido) > num_restringido->tope) 
	    {
			limite_restringido = num_restringido->tope;
		}
		
		limite_restringido = (limite_restringido / BASE) * BASE;
		monto_disponible = (boost::int32_t) (limite * tipomontotriple->limdisctrip);
		monto_disponible = (boost::int32_t) (monto_disponible * pro_restringido);
		monto_disponible = (monto_disponible / BASE) * BASE;
		if (num_restringido != NULL) {
			boost::int32_t sumatoria;

			sumatoria = monto_disponible + num_restringido->venta + num_restringido->preventa;
			if (sumatoria > limite_restringido) {
				monto_disponible =
				    limite_restringido - (num_restringido->venta + num_restringido->preventa);
				monto_disponible = monto_disponible > 0 ? monto_disponible : 0;
			}
		}
		monto_disponible = (monto_disponible / BASE) * BASE;
		preventa = new Preventa;
		preventa->tipo = TRIPLE;
		preventa->numero = triple;
		preventa->monto_pedido = monto;
		if (monto_disponible >= monto) {
			preventa->monto_preventa = monto;
			completo = 1;
			if (num_restringido != NULL)
				num_restringido->preventa += monto;
		} else {
			preventa->monto_preventa = monto_disponible;
			completo = 0;
			if (num_restringido != NULL)
				num_restringido->preventa += monto_disponible;
		}
		preventa->monto_preventa = ((preventa->monto_preventa < 0)
		                             or (preventa->monto_preventa % BASE !=
		                                  0)) ? 0 : preventa->monto_preventa;
		venta_numero->preventa += preventa->monto_preventa;
		preventa->idSorteo = idSorteo;
		renglonPtr = new boost::uint16_t;
		*renglonPtr = renglon;
		g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
		preventas_taquilla->hora_ultimo_renglon = time(NULL);
		preventas_taquilla->hora_ultimo_peticion = time(NULL);
	}
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Triple %03d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
end:
	sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	sem_post(&tipoDeMonto->sem_t_tipomonto);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	if (completo)
		return NULL;
	else
		return preventa;
}

/*-------------------------------------------------------------------------*/

Preventa *
preventa_tripletazo(ConexionActiva& conexionActiva,
                     boost::int32_t monto,
                     boost::uint16_t tripletazo,
                     n_t_tipomonto_t * tipoDeMonto,
                     n_t_zona_numero_restringido_t * zona_num_restringido,
                     n_t_sorteo_numero_restringido_t * sorteo_num_restringido,
                     boost::uint16_t renglon,
                     boost::uint8_t idSorteo,
                     n_t_preventas_x_taquilla_t * preventas_taquilla,
                     boost::uint32_t idAgente,
                     boost::uint8_t idTaquilla,
                     TipoDeMonto * tipomontotriple)
{
	boost::int8_t completo;
	boost::int32_t monto_disponible, limite, limite_restringido;
	double pro_restringido;
	n_t_venta_numero_t *venta_numero;
	n_t_numero_restringido_t *num_restringido;
	Preventa *preventa;
	boost::uint16_t *renglonPtr;

	preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	sem_wait(&tipoDeMonto->sem_t_tipomonto);
	sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	{
		venta_numero =
		    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero,
		                                            (boost::uint16_t *) & tripletazo);

		if (venta_numero == NULL) {
			char mensaje[BUFFER_SIZE];

			sprintf(mensaje, "ERROR Llegando Numero %i que no es un Tripletazo", tripletazo);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, TICKET_VENCIDO, NULL, 0);
			completo = true;
			goto end;
		}

		num_restringido =
		    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
		                                                  t_numero_restringido_triple,
		                                                  (boost::uint16_t *) & tripletazo);
		pro_restringido = num_restringido != NULL ? num_restringido->proporcion : 1;
		limite = tipoDeMonto->limite_actual - (venta_numero->venta + venta_numero->preventa);
		limite_restringido = (boost::int32_t) (tipoDeMonto->limite_actual * pro_restringido);
		
		if (num_restringido != NULL
		    and num_restringido->tope > 0 
	        and boost::uint16_t(limite_restringido) > num_restringido->tope) 
	    {
			limite_restringido = num_restringido->tope;
		}
		
		limite_restringido = (limite_restringido / BASE) * BASE;
		monto_disponible = (boost::int32_t) (limite * tipomontotriple->limdisctrip);
		monto_disponible = (boost::int32_t) (monto_disponible * pro_restringido);
		monto_disponible = (monto_disponible / BASE) * BASE;
		if (num_restringido != NULL) {
			boost::int32_t sumatoria;

			sumatoria = monto_disponible + num_restringido->venta + num_restringido->preventa;
			if (sumatoria > limite_restringido) {
				monto_disponible =
				    limite_restringido - (num_restringido->venta + num_restringido->preventa);
				monto_disponible = monto_disponible > 0 ? monto_disponible : 0;
			}
		}
		monto_disponible = (monto_disponible / BASE) * BASE;
		preventa = new Preventa;
		preventa->tipo = TRIPLETAZO;
		preventa->numero = tripletazo;
		preventa->monto_pedido = monto;
		if (monto_disponible >= monto) {
			preventa->monto_preventa = monto;
			completo = 1;
			if (num_restringido != NULL)
				num_restringido->preventa += monto;
		} else {
			preventa->monto_preventa = monto_disponible;
			completo = 0;
			if (num_restringido != NULL)
				num_restringido->preventa += monto_disponible;
		}
		preventa->monto_preventa = ((preventa->monto_preventa < 0)
		                             or (preventa->monto_preventa % BASE !=
		                                  0)) ? 0 : preventa->monto_preventa;
		venta_numero->preventa += preventa->monto_preventa;
		preventa->idSorteo = idSorteo;
		renglonPtr = new boost::uint16_t;
		*renglonPtr = renglon;
		g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
		preventas_taquilla->hora_ultimo_renglon = time(NULL);
		preventas_taquilla->hora_ultimo_peticion = time(NULL);
	}
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Tripletazo %05d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
end:
	sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
	sem_post(&tipoDeMonto->sem_t_tipomonto);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	if (completo)
		return NULL;
	else
		return preventa;
}

/*-------------------------------------------------------------------------*/

void
agregar_renglon_vacio_triple(ConexionActiva& conexionActiva, boost::uint16_t renglon,
                              boost::uint16_t triple, boost::uint8_t idSorteo,
                              n_t_preventas_x_taquilla_t * preventas_taquilla, boost::uint32_t idAgente,
                              boost::uint8_t idTaquilla, boost::int32_t monto_pedido)
{
	Preventa * preventa;
	boost::uint16_t *renglonPtr;

	preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	preventa = new Preventa;
	preventa->tipo = TRIPLE;
	preventa->numero = triple;
	preventa->idSorteo = idSorteo;
	preventa->monto_preventa = 0;
	preventa->monto_pedido = monto_pedido;
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Triple %03d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
	renglonPtr = new boost::uint16_t;
	*renglonPtr = renglon;
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
}

/*-------------------------------------------------------------------------*/

void
agregar_renglon_vacio_tripletazo(ConexionActiva& conexionActiva, boost::uint16_t renglon,
                                  boost::uint16_t triple, boost::uint8_t idSorteo,
                                  n_t_preventas_x_taquilla_t * preventas_taquilla, boost::uint32_t idAgente,
                                  boost::uint8_t idTaquilla, boost::int32_t monto_pedido)
{
	Preventa * preventa;
	boost::uint16_t *renglonPtr;

	preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
	if (preventa != NULL)
		modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
	preventa = new Preventa;
	preventa->tipo = TRIPLE;
	preventa->numero = triple;
	preventa->idSorteo = idSorteo;
	preventa->monto_preventa = 0;
	preventa->monto_pedido = monto_pedido;
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "[%d] Tripletazo %05d dado Por %d Sorteo %d", renglon, preventa->numero,
		         preventa->monto_preventa, preventa->idSorteo);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
	renglonPtr = new boost::uint16_t;
	*renglonPtr = renglon;
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	g_tree_insert(preventas_taquilla->preventas, renglonPtr, preventa);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
}

/*-------------------------------------------------------------------------*/

void
verificar_sorteo(ConexionActiva * conexionActiva, Sorteo * sorteo, boost::uint8_t idSorteo,
                  boost::uint8_t mensaje)
{
	if (sorteo->isEstadoActivo()) {
		if (hora_actual() > sorteo->getHoraCierre() or sorteo->getDiaActual() != diadehoy()) {
			sorteo->setEstadoActivo(false);
		}
	}
	if ((not sorteo->isEstadoActivo()) and mensaje == true) {
		loteria_cerrada_t loteria_cerrada;
		boost::uint8_t *buff;
		char gmensaje[BUFFER_SIZE];

		loteria_cerrada.idSorteo = idSorteo;
		buff = loteria_cerrada_t2b(loteria_cerrada);
		send2cliente(*conexionActiva, LOTERIA_CERRADA, buff, LOTERIA_CERRADA_LON);
		delete [] buff;
		sprintf(gmensaje, "Sorteo %u cerrado", idSorteo);
		log_clientes(NivelLog::Bajo, conexionActiva, gmensaje);
	}
}

/*-------------------------------------------------------------------------*/

#define SALTO4() \
if (not sorteo->isEstadoActivo()) { \
 for (int i = 0; i < nums; i++) { \
 	numeros[i] = numeros[i] + 100 * comodin; \
 	agregar_renglon_vacio_terminalazo(conexionActiva, \
 	                                   solicitudVenta.renglon + i, \
 	                                   numeros[i], \
 	                                   idSorteo, \
 	                                   preventas_taquilla, \
 	                                   idAgente, \
 	                                   idTaquilla); \
 } \
 RespuestaVentaRango respuestaVentaRango; \
 respuestaVentaRango.begin = solicitudVenta.renglon; \
 respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1; \
 respuestaVentaRango.size = -1; \
 send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango); \
} else { \
 RespuestaVentaRango respuestaVentaRango; \
 respuestaVentaRango.begin = solicitudVenta.renglon; \
 respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1; \
 respuestaVentaRango.size = 0; \
 boost::uint8_t *bufferz = new boost::uint8_t[RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE * 1000]; \
 for (int i = 0; i < nums; i++) { \
 	numeros[i] = numeros[i] + 100 * comodin; \
 	preventa = preventa_terminalazo(conexionActiva, \
 	                                 solicitudVenta.monto, \
 	                                 numeros[i], \
 	                                 tipoDeMonto, \
 	                                 zona_num_restringido, \
 	                                 sorteo_num_restringido, \
 	                                 solicitudVenta.renglon + i, \
 	                                 idSorteo, \
 	                                 preventas_taquilla, \
 	                                 idAgente, \
 	                                 idTaquilla, \
 	                                 tipomonto_sorteo); \
 	if (preventa != NULL) { \
 		RespuestaVentaRenglonModificado \
            respuestaVentaRenglonModificado(solicitudVenta.renglon + i, \
                                            preventa->monto_preventa); \
 		std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer(); \
 		memcpy(bufferz + (respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), \
 		       &buff2[0], \
 		       RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE); \
 		respuestaVentaRango.size++; \
 	} \
 } \
 if (respuestaVentaRango.size == 0) { \
    send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango); \
 } else { \
 	boost::uint16_t sizeVal = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE; \
 	sizeVal += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE; \
 	boost::uint8_t * buffer3 = new boost::uint8_t[sizeVal]; \
 	std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer(); \
 	memcpy(buffer3, &buff[0], RespuestaVentaRango::CONST_RAW_BUFFER_SIZE); \
 	memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, sizeVal - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE); \
 	send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, sizeVal); \
 	delete [] buffer3; \
 } \
 delete [] bufferz; \
}

#define SALTO2() \
if (not sorteo->isEstadoActivo()) { \
	for (int i = 0; i < nums; i++) { \
		numeros[i] = numeros[i] + 1000 * comodin; \
		agregar_renglon_vacio_tripletazo(conexionActiva, \
		                                  solicitudVenta.renglon + i, \
		                                  numeros[i], \
		                                  idSorteo, \
		                                  preventas_taquilla, \
		                                  idAgente, \
		                                  idTaquilla, \
		                                  solicitudVenta.monto); \
	} \
	RespuestaVentaRango respuestaVentaRango; \
	respuestaVentaRango.begin = solicitudVenta.renglon; \
	respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1; \
	respuestaVentaRango.size = -1; \
	send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango); \
} else { \
	RespuestaVentaRango respuestaVentaRango; \
	respuestaVentaRango.begin = solicitudVenta.renglon; \
	respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1; \
	respuestaVentaRango.size = 0; \
	boost::uint8_t *bufferz = new boost::uint8_t[6000]; \
	for (int i = 0; i < nums; i++) { \
		numeros[i] = numeros[i] + 1000 * comodin; \
		preventa = preventa_tripletazo(conexionActiva, \
		                                solicitudVenta.monto, \
		                                numeros[i], \
		                                tipoDeMonto, \
		                                zona_num_restringido, \
		                                sorteo_num_restringido, \
		                                solicitudVenta.renglon + i, \
		                                idSorteo, \
		                                preventas_taquilla, \
		                                idAgente, \
		                                idTaquilla, \
		                                tipomonto_sorteo); \
		if (preventa != NULL) { \
			RespuestaVentaRenglonModificado \
                respuestaVentaRenglonModificado(solicitudVenta.renglon + i, \
                                                preventa->monto_preventa); \
			std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer(); \
			memcpy(bufferz + (respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), \
                   &buff2[0], RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE); \
			respuestaVentaRango.size++; \
		} \
	} \
	if (respuestaVentaRango.size == 0) { \
		send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango); \
	} else { \
		boost::uint16_t sizeVal = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE; \
		sizeVal += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE; \
		boost::uint8_t *buffer3 = new boost::uint8_t[sizeVal]; \
		std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer(); \
		memcpy(buffer3, &buff[0], RespuestaVentaRango::CONST_RAW_BUFFER_SIZE); \
		memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, sizeVal - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE); \
		send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, sizeVal); \
		delete [] buffer3; \
	} \
	delete [] bufferz; \
}

void
atenderPeticionVenta(ConexionActiva& conexionActiva, boost::uint8_t * buffer)
{
	boost::uint8_t idZona;
	boost::uint16_t numeros[1000];
	boost::uint16_t nums;
	boost::uint32_t idAgente;
	boost::uint8_t idTaquilla;
	Sorteo *sorteo;
	boost::uint8_t idSorteo;
	Triple triple;
	Terminal terminal;
	Triple terminalazo;
	TipoDeMonto *tipomonto_sorteo;
	TipoDeMontoPorAgencia *tipomonto_agencia;
	n_t_zona_numero_restringido_t *zona_num_restringido;
	n_t_sorteo_numero_restringido_t *sorteo_num_restringido;
	boost::uint64_t agente_taquilla;
	n_t_preventas_x_taquilla_t *preventas_taquilla;
	Preventa *preventa;

	idAgente = conexionActiva.idAgente;
	agente_taquilla = conexionActiva.idAgente;
	agente_taquilla = agente_taquilla << 32;
	idTaquilla = conexionActiva.idTaquilla;
	agente_taquilla += idTaquilla;
	preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	/*
	 * Busco los tipos de montos por agente
	 */
    {
        TipoDeMontoPorAgencia::MutexGuard g;
        tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
	    
    	if (tipomonto_agencia == NULL) {
    		char mensaje[BUFFER_SIZE];
    		send2cliente(conexionActiva, ERR_AGENCIANOACT, NULL, 0);
    		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de montos.");
    		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    		return ;
    	}
    }
	/*
	 * Busco lo numeros restringidos por la zona del agente
	 */
	idZona = conexionActiva.idZona;
	zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);

	if (zona_num_restringido == NULL) {
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "ADVERTENCIA: La zona <%u> no esta en memoria.", idZona);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}

	SolicitudVenta solicitudVenta(buffer);
	idSorteo = solicitudVenta.idSorteo;

	/*
	 * Busco los tipos de monto de un agente de un sorteo especial
	 */

	tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
	if (tipomonto_sorteo == NULL)
	{
		char mensaje[BUFFER_SIZE];

		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: No hay tipo de monto para el sorteo <%u>.", idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	/*
	 * Buscar el sorteo y dependiendo del tipo de numero busco el tipo de monto
	 */
	sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
	if (sorteo == NULL)
	{
		char mensaje[BUFFER_SIZE];

		send2cliente(conexionActiva, ERR_SORTEONOEXIS, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: No existe el sorteo <%u>.", idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	verificar_sorteo(&conexionActiva, sorteo, idSorteo, true);
	/**/
	/*
	 * Obtengo todos los numeros restringidos de un sorteo de una zona
	 */
	sorteo_num_restringido =
	    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
	                                                         t_sorteo_numero_restringido,
	                                                         &idSorteo);

	if (sorteo_num_restringido == NULL)
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje,
		         "El sorteo %d esta mal configurado para la agencia %d o alguno de sus recogedores",
		         idSorteo, idAgente);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);

		//FIXME: deberia enviar un mensaje al punto de venta notificando el error.
		return ;
	}

	buffer += SolicitudVenta::CONST_RAW_BUFFER_SIZE;
    
    boost::uint8_t tipoSolicitud = solicitudVenta.tipo >> 4;
	SolicitudVenta::Tipo tipoJugada = SolicitudVenta::Tipo(tipoSolicitud);
    
    tipoSolicitud = solicitudVenta.tipo << 4;
    tipoSolicitud = tipoSolicitud >> 4;
	SolicitudVenta::Tipo tipoCombinacion = SolicitudVenta::Tipo(tipoSolicitud);
        
    std::ostringstream oss;
    oss << "atenderPeticionVenta: tipoJugada = " << tipoJugada 
        << ", tipoCombinacion = " << tipoCombinacion;
    
    log_clientes(NivelLog::Debug, &conexionActiva, oss.str().c_str());

	if (tipoJugada == SolicitudVenta::terminal)
	{
		n_t_tipomonto_t * tipoDeMonto;
		boost::uint8_t tipomonto_terminal;

		tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
		tipoDeMonto =
		    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
		                                         &tipomonto_terminal);
		if (tipoDeMonto == NULL)
		{
			char mensaje[BUFFER_SIZE];

			send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
			              sizeof(idSorteo));
			sprintf(mensaje,
			         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
			         tipomonto_terminal);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			return ;
		}
		switch (tipoCombinacion)
		{
			case SolicitudVenta::normal:
			{

				terminal = Terminal(buffer);
				buffer += Terminal::CONST_RAW_BUFFER_SIZE;
			terminal1:
				if (not sorteo->isEstadoActivo())
				{
					agregar_renglon_vacio_terminal(conexionActiva, solicitudVenta.renglon,
					                                terminal.numero, idSorteo, preventas_taquilla,
					                                idAgente, idTaquilla);
					
                    
					send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, 0));

				} else
				{
					preventa =
					    preventa_terminal(conexionActiva,
					                       solicitudVenta.monto,
					                       terminal.numero,
					                       tipoDeMonto,
					                       zona_num_restringido,
					                       sorteo_num_restringido,
					                       solicitudVenta.renglon,
					                       idSorteo,
					                       preventas_taquilla,
					                       idAgente,
					                       idTaquilla,
					                       tipomonto_sorteo);
					if (preventa == NULL) {                                                
                        send2cliente(conexionActiva, RESPVENTAXNUMOK,
                                 RespuestaVentaRenglon(solicitudVenta.renglon));                        
					} else {
                        send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, preventa->monto_preventa));
					}
				}
				break;
			}
			case SolicitudVenta::permuta:
			{
                {
					Permuta permuta = Permuta(buffer);
					permutarTerminal(permuta.numero, numeros, &nums);
					{
						char mensaje[BUFFER_SIZE];

						sprintf(mensaje, "Permuta del %u para terminales Resultado %d numeros",
						         permuta.numero, nums);
						log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
					}
                }
				buffer += Permuta::CONST_RAW_BUFFER_SIZE;
			salto1:
				if (not sorteo->isEstadoActivo())
				{
					int i;
					RespuestaVentaRango respuestaVentaRango;
					for (i = 0; i < nums; i++) {
						agregar_renglon_vacio_terminal(conexionActiva, solicitudVenta.renglon + i,
						                                numeros[i], idSorteo, preventas_taquilla,
						                                idAgente, idTaquilla);
					}
					respuestaVentaRango.begin = solicitudVenta.renglon;
					respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1;
					respuestaVentaRango.size = -1;
					send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
				} else
				{
					boost::uint8_t *bufferz;
					int i;
					RespuestaVentaRango respuestaVentaRango;

					respuestaVentaRango.begin = solicitudVenta.renglon;
					respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1;
					respuestaVentaRango.size = 0;
					bufferz = new boost::uint8_t[RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE * 1000];
					for (i = 0; i < nums; i++) {
						preventa =
						    preventa_terminal(conexionActiva, solicitudVenta.monto, numeros[i],
						                       tipoDeMonto, zona_num_restringido,
						                       sorteo_num_restringido, solicitudVenta.renglon + i,
						                       idSorteo, preventas_taquilla, idAgente, idTaquilla,
						                       tipomonto_sorteo);
						if (preventa != NULL) {
                            
							RespuestaVentaRenglonModificado 
                                respuestaVentaRenglonModificado(solicitudVenta.renglon + i,
                                                                preventa->monto_preventa);
                            
                            std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
                            
							memcpy(bufferz + 
                                    (respuestaVentaRango.size * 
                                     RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), 
                                   &buff2[0], buff2.size());
                                   							
                            respuestaVentaRango.size++;
						}
					}
					if (respuestaVentaRango.size == 0) {
						send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
					} else {
						boost::uint16_t sizeVal = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE;
						sizeVal += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
						boost::uint8_t *buffer3 = new boost::uint8_t[sizeVal];
                        std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer();
						memcpy(buffer3, &buff[0], buff.size());
						memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, sizeVal - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
						send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, sizeVal);
						delete [] buffer3;
					}
					delete [] bufferz;
				}
				break;
			}
			case SolicitudVenta::serie:
			{
				Serie serieTerminal(buffer);
				seriesTerminal(serieTerminal, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje,
					         "Serie del %d posicion %d para terminales Resultado %d numeros",
					         serieTerminal.numero, serieTerminal.posicion, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Serie::CONST_RAW_BUFFER_SIZE;
				goto salto1;
				break;
			}
			case SolicitudVenta::corrida:
			{
				CorridaTerminal corridaTerminal(buffer);
				corridaTerminal.end = corridaTerminal.end > 99 ? 99 : corridaTerminal.end;
				corrida(corridaTerminal.begin, corridaTerminal.end, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje, "Corrida del %d al %d para terminales Resultado %d numeros",
					         corridaTerminal.begin, corridaTerminal.end, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += CorridaTerminal::CONST_RAW_BUFFER_SIZE;
				goto salto1;
				break;
			}
			case SolicitudVenta::morocho:
			{
				Morocho morocho(buffer);
				morochosTerminal(morocho.numero, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje, "Morocho del %u para terminales Resultado %d numeros",
					         morocho.numero, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Morocho::CONST_RAW_BUFFER_SIZE;
				goto salto1;
				break;
			}
			case SolicitudVenta::modificar:
			{
				terminal = Terminal(buffer);
				modificarRenglon(conexionActiva, idAgente, conexionActiva.idTaquilla,
				                  solicitudVenta.renglon, 0);
				buffer += Terminal::CONST_RAW_BUFFER_SIZE;
				goto terminal1;
				break;
			}
            default: break;           
		}
	}
	else if (tipoJugada == SolicitudVenta::triple)
	{
		n_t_tipomonto_t * tipoDeMonto;
		boost::uint8_t tipomonto_triple;

		tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
		tipoDeMonto =
		    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipomonto_triple);
		if (tipoDeMonto == NULL) {
			char mensaje[BUFFER_SIZE];

			send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
			              sizeof(idSorteo));
			sprintf(mensaje,
			         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
			         tipomonto_triple);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			return ;
		}
		switch (tipoCombinacion) {
			case SolicitudVenta::normal:
			{
				triple = Triple(buffer);
				buffer += Triple::CONST_RAW_BUFFER_SIZE;
			triple1:
				if (not sorteo->isEstadoActivo())
				{
					agregar_renglon_vacio_triple(conexionActiva,
					                              solicitudVenta.renglon,
					                              triple.numero,
					                              idSorteo,
					                              preventas_taquilla,
					                              idAgente,
					                              idTaquilla,
					                              solicitudVenta.monto);
                    send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, 0));
					
				} else
				{
					preventa = preventa_triple(conexionActiva,
					                            solicitudVenta.monto,
					                            triple.numero,
					                            tipoDeMonto,
					                            zona_num_restringido,
					                            sorteo_num_restringido,
					                            solicitudVenta.renglon,
					                            idSorteo,
					                            preventas_taquilla,
					                            idAgente,
					                            idTaquilla,
					                            tipomonto_sorteo);
					if (preventa == NULL) {
						send2cliente(conexionActiva, RESPVENTAXNUMOK,
                                 RespuestaVentaRenglon(solicitudVenta.renglon));
					} else {
						send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, preventa->monto_preventa));
					}
				}
				break;
			}
			case SolicitudVenta::permuta:
			{                        
                {                            
                    Permuta permuta = Permuta(buffer);
					permutarTriple(permuta.numero, numeros, &nums);
					{
						char mensaje[BUFFER_SIZE];

						sprintf(mensaje, "Permuta del %u para triples Resultado %d numeros",
						         permuta.numero, nums);
						log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
					}
                }
				buffer += Permuta::CONST_RAW_BUFFER_SIZE;
			salto:
				if (not sorteo->isEstadoActivo())
				{
					int i;
					RespuestaVentaRango respuestaVentaRango;

					for (i = 0; i < nums; i++) {
						agregar_renglon_vacio_triple(conexionActiva, solicitudVenta.renglon + i,
						                              numeros[i], idSorteo,
						                              preventas_taquilla,
						                              idAgente, idTaquilla, solicitudVenta.monto);
					}
					respuestaVentaRango.begin = solicitudVenta.renglon;
					respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1;
					respuestaVentaRango.size = -1;
					send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
				} else
				{
					RespuestaVentaRango respuestaVentaRango;
					respuestaVentaRango.begin = solicitudVenta.renglon;
					respuestaVentaRango.end = respuestaVentaRango.begin + nums - 1;
					respuestaVentaRango.size = 0;
					boost::uint8_t *bufferz = new boost::uint8_t[6000];
					for (int i = 0; i < nums; i++) {
						preventa = preventa_triple(conexionActiva,
						                            solicitudVenta.monto,
						                            numeros[i],
						                            tipoDeMonto,
						                            zona_num_restringido,
						                            sorteo_num_restringido,
						                            solicitudVenta.renglon + i,
						                            idSorteo,
						                            preventas_taquilla,
						                            idAgente,
						                            idTaquilla,
						                            tipomonto_sorteo);
						if (preventa != NULL) {                            
                            RespuestaVentaRenglonModificado 
                                respuestaVentaRenglonModificado(solicitudVenta.renglon + i,
                                                                preventa->monto_preventa);
                            
                            std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
                            
                            memcpy(bufferz + (respuestaVentaRango.size * 
                                              RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), 
                                   &buff2[0], buff2.size());                            
                            
							respuestaVentaRango.size++;
						}
					}
					if (respuestaVentaRango.size == 0) {
						send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
					} else {
						boost::uint8_t *buffer3;
						boost::uint16_t sizeVal;

						sizeVal = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE;
						sizeVal += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
						buffer3 = new boost::uint8_t[sizeVal];
                        std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer();
                        memcpy(buffer3, &buff[0], buff.size());
						memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, sizeVal - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
						send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, sizeVal);
						delete [] buffer3;
					}
					delete [] bufferz;
				}
				break;
			}
			case SolicitudVenta::serie:
			{
				Serie serieTriple(buffer);
				seriesTriple(serieTriple, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje, "Serie del %d posicion %d para triples Resultado %d numeros",
					         serieTriple.numero, serieTriple.posicion, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Serie::CONST_RAW_BUFFER_SIZE;
				goto salto;
				break;
			}
			case SolicitudVenta::corrida:
			{
				CorridaTriple corridaTriple(buffer);
				corridaTriple.end = corridaTriple.end > 999 ? 999 : corridaTriple.end;
				corrida(corridaTriple.begin, corridaTriple.end, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje, "Corrida del %d al %d para triples Resultado %d numeros",
					         corridaTriple.begin, corridaTriple.end, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += CorridaTriple::CONST_RAW_BUFFER_SIZE;
				goto salto;
				break;
			}
			case SolicitudVenta::morocho:
			{
				Morocho morocho(buffer);
				morochos(morocho.numero, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje, "Morocho del %u para triples Resultado %d numeros",
					         morocho.numero, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Morocho::CONST_RAW_BUFFER_SIZE;
				goto salto;
				break;
			}
			case SolicitudVenta::modificar:
			{
				triple = Triple(buffer);
				buffer += Triple::CONST_RAW_BUFFER_SIZE;
				modificarRenglon(conexionActiva, idAgente, conexionActiva.idTaquilla,
				                  solicitudVenta.renglon, 0);
				goto triple1;
				break;
			}
            default: break;
		}
	} else if (tipoJugada == SolicitudVenta::terminalSigno)
	{
		n_t_tipomonto_t * tipoDeMonto;
		boost::uint8_t tipomonto_terminal;
		int comodin;

		tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
		tipoDeMonto =
		    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
		                                         &tipomonto_terminal);
		if (tipoDeMonto == NULL)
		{
			send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
			              sizeof(idSorteo));
			char mensaje[BUFFER_SIZE];
			sprintf(mensaje,
			         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
			         tipomonto_terminal);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			return ;
		}
		switch (tipoCombinacion)
		{
			case SolicitudVenta::normal:
			{
				terminalazo = Triple(buffer);
				if (not sorteo->isEstadoActivo())
				{
					agregar_renglon_vacio_terminalazo(conexionActiva,
					                                   solicitudVenta.renglon,
					                                   terminalazo.numero,
					                                   idSorteo,
					                                   preventas_taquilla,
					                                   idAgente,
					                                   idTaquilla);
					send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, 0));
				} else
				{
					preventa = preventa_terminalazo(conexionActiva,
					                                 solicitudVenta.monto,
					                                 terminalazo.numero,
					                                 tipoDeMonto,
					                                 zona_num_restringido,
					                                 sorteo_num_restringido,
					                                 solicitudVenta.renglon,
					                                 idSorteo,
					                                 preventas_taquilla,
					                                 idAgente,
					                                 idTaquilla,
					                                 tipomonto_sorteo);
					if (preventa == NULL) {
						send2cliente(conexionActiva, RESPVENTAXNUMOK,
                                 RespuestaVentaRenglon(solicitudVenta.renglon));
					} else {
						send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, preventa->monto_preventa));
					}
				}
				break;
			}
			case SolicitudVenta::permuta:
			{
				Permuta permuta(buffer);
				comodin = permuta.signo;
				permutarTerminal(permuta.numero, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Permuta del %u para terminalazo Resultado %d numeros",
					         permuta.numero, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Permuta::CONST_RAW_BUFFER_SIZE;
				SALTO4();
				break;
			}
			case SolicitudVenta::serie:
			{
				SerieSigno serieTerminalSigno(buffer);
				comodin = serieTerminalSigno.signo;
				seriesTerminal(serieTerminalSigno, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje,
					         "Serie del %d posicion %d para terminalazo Resultado %d numeros",
					         serieTerminalSigno.numero, serieTerminalSigno.posicion, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				SALTO4();
				break;
			}
			case SolicitudVenta::corrida:
			{
				CorridaTerminalSigno corridaTerminalSigno(buffer);
				comodin = corridaTerminalSigno.signo;
				corridaTerminalSigno.end = corridaTerminalSigno.end > 99 ? 99 : corridaTerminalSigno.end;
				corrida(corridaTerminalSigno.begin, corridaTerminalSigno.end, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Corrida del %d al %d para terminalazo Resultado %d numeros",
					         corridaTerminalSigno.begin, corridaTerminalSigno.end, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				SALTO4();
				break;
			}
			case SolicitudVenta::morocho:
			{
				Morocho morocho(buffer);
				comodin = morocho.signo;
				morochosTerminal(morocho.numero, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Morocho del %u para terminalazo Resultado %d numeros",
					         morocho.numero, nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				SALTO4();
				break;
			}
            default: break;
		}
	} else if (tipoJugada == SolicitudVenta::tripleSigno)
	{
		boost::uint8_t tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
		n_t_tipomonto_t * tipoDeMonto =
		    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipomonto_triple);
		if (tipoDeMonto == NULL) {
			char mensaje[BUFFER_SIZE];

			send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
			              sizeof(idSorteo));
			sprintf(mensaje,
			         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
			         tipomonto_triple);
			log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			return ;
		}
		int comodin;
		switch (tipoCombinacion) {
			case SolicitudVenta::normal:
			{
				triple = Triple(buffer);
				if (not sorteo->isEstadoActivo())
				{
					agregar_renglon_vacio_tripletazo(conexionActiva,
					                                  solicitudVenta.renglon,
					                                  triple.numero,
					                                  idSorteo,
					                                  preventas_taquilla,
					                                  idAgente,
					                                  idTaquilla,
					                                  solicitudVenta.monto);
					send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, 0));
				} else
				{
					preventa = preventa_tripletazo(conexionActiva,
					                                solicitudVenta.monto,
					                                triple.numero,
					                                tipoDeMonto,
					                                zona_num_restringido,
					                                sorteo_num_restringido,
					                                solicitudVenta.renglon,
					                                idSorteo,
					                                preventas_taquilla,
					                                idAgente,
					                                idTaquilla,
					                                tipomonto_sorteo);
					if (preventa == NULL) {
						send2cliente(conexionActiva, RESPVENTAXNUMOK,
                                 RespuestaVentaRenglon(solicitudVenta.renglon));
					} else {
						send2cliente(conexionActiva, RESPVENTAXNUMNOOK,
                                 RespuestaVentaRenglonModificado(solicitudVenta.renglon, preventa->monto_preventa));
					}
				}
				break;
			}
			case SolicitudVenta::permuta:
			{
				Permuta permuta(buffer);
				comodin = permuta.signo;
				permutarTriple(permuta.numero, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Permuta del %u para tripletazo %u Resultado %d numeros",
					         permuta.numero, unsigned(comodin), nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Permuta::CONST_RAW_BUFFER_SIZE;
				SALTO2();
				break;
			}
			case SolicitudVenta::serie:
			{
				SerieSigno serieTripleSigno(buffer);
				comodin = serieTripleSigno.signo;
				seriesTriple(serieTripleSigno, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Serie del %d posicion %d para tripletazo %u Resultado %d numeros",
					         serieTripleSigno.numero, serieTripleSigno.posicion, unsigned(comodin), nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				SALTO2();
				break;
			}
			case SolicitudVenta::corrida:
			{
				CorridaTripleSigno corridaTripleSigno(buffer);
				comodin = corridaTripleSigno.signo;
				corridaTripleSigno.end = corridaTripleSigno.end > 999 ? 999 : corridaTripleSigno.end;
				corrida(corridaTripleSigno.begin, corridaTripleSigno.end, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Corrida del %d al %d para tripletazo %u Resultado %d numeros comodin %d",
					         corridaTripleSigno.begin, corridaTripleSigno.end, unsigned(comodin), nums, comodin);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				SALTO2();
				break;
			}
			case SolicitudVenta::morocho:
			{
				Morocho morocho(buffer);
				comodin = morocho.signo;
				morochos(morocho.numero, numeros, &nums);
				{
					char mensaje[BUFFER_SIZE];
					sprintf(mensaje, "Morocho del %u para tripletazo %u Resultado %d numeros",
					         morocho.numero, unsigned(comodin), nums);
					log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
				}
				buffer += Morocho::CONST_RAW_BUFFER_SIZE;
				SALTO2();
				break;
			}
            default: break;
		}
	}
}

/*-------------------------------------------------------------------------*/

void
atenderPeticionCancelarPreventa(ConexionActiva& conexionActiva)
{
	boost::uint32_t idAgente;
	boost::uint64_t agente_taquilla;
	n_t_preventas_x_taquilla_t *preventas_taquilla;
	boost::uint16_t renglon;
	Preventa *preventa;
	boost::uint32_t nnodes;
	boost::uint8_t idSorteo, idZona;
	boost::uint16_t numero;
	Sorteo *sorteo;
	TipoDeMontoPorAgencia *tipomonto_agencia;
	TipoDeMonto *tipomonto_sorteo;
	n_t_venta_numero_t *venta_numero;
	n_t_tipomonto_t *tipoDeMonto;
	n_t_zona_numero_restringido_t *zona_num_restringido;
	n_t_sorteo_numero_restringido_t *sorteo_num_restringido;
	n_t_numero_restringido_t *num_restringido;

	idZona = conexionActiva.idZona;
	zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	if (zona_num_restringido == NULL) {
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "ADVERTENCIA: La zona <%u> no esta en memoria.", idZona);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	idAgente = conexionActiva.idAgente;
	agente_taquilla = conexionActiva.idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += conexionActiva.idTaquilla;
	preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
	renglon = 1;
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	nnodes = g_tree_nnodes(preventas_taquilla->preventas);
	while (nnodes > 0) {
		preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
		if (preventa != NULL) {
			idSorteo = preventa->idSorteo;
			numero = preventa->numero;
			sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
			sorteo_num_restringido =
			    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
			                                                         t_sorteo_numero_restringido,
			                                                         &idSorteo);
			tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
			if (tipomonto_sorteo == NULL) {
				char mensaje[BUFFER_SIZE];

				send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
				              sizeof(idSorteo));
				sprintf(mensaje,
				         "ADVERTENCIA: La Agencia no tiene tipo de monto para el sorteo <%u>.",
				         idSorteo);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				return ;
			}
			if (preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO) {
				boost::uint8_t tipomonto_terminal;

				tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
				tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
				                                         &tipomonto_terminal);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipomonto_terminal);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}
				sem_wait(&tipoDeMonto->sem_t_tipomonto);
				venta_numero =
				    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
				venta_numero->preventa -= preventa->monto_preventa;
				sem_post(&tipoDeMonto->sem_t_tipomonto);
				sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
				num_restringido =
				    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
				                                                  t_numero_restringido_terminal,
				                                                  &numero);
				if (num_restringido != NULL) {
					num_restringido->preventa -= preventa->monto_preventa;
				}
				sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			} else
				if (preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO) {
					boost::uint8_t tipomonto_triple;

					tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
					tipoDeMonto =
					    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(),
					                                         &tipomonto_triple);
					if (tipoDeMonto == NULL) {
						char mensaje[BUFFER_SIZE];

						sprintf(mensaje,
						         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
						         tipomonto_triple);
						log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
						return ;
					}
					sem_wait(&tipoDeMonto->sem_t_tipomonto);
					venta_numero =
					    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
					venta_numero->preventa -= preventa->monto_preventa;
					sem_post(&tipoDeMonto->sem_t_tipomonto);
					sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
					num_restringido =
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
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	{
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "Cancelando la Preventa");
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
}

/*-------------------------------------------------------------------------*/

void
atenderPeticionEliminarRenglon(ConexionActiva& conexionActiva, renglon_t renglon)
{
	modificarRenglon(conexionActiva, conexionActiva.idAgente, conexionActiva.idTaquilla,
	                  renglon.renglon, 1);

    send2cliente(conexionActiva, ELIMINAR_RENGLON,
                                 RespuestaVentaRenglon(renglon.renglon));

	char mensaje[BUFFER_SIZE];
	sprintf(mensaje, "Eliminado el renglon %d", renglon.renglon);
	log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
}

/*-------------------------------------------------------------------------*/

void
atenderPeticionTerminales(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
	GenerarTerminales generarTerminales(buff);
	boost::uint32_t idAgente = conexionActiva.idAgente;
	boost::uint64_t agente_taquilla = conexionActiva.idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += conexionActiva.idTaquilla;
	boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
	n_t_preventas_x_taquilla_t* preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	boost::uint16_t renglon = generarTerminales.renglon;
    
    TipoDeMontoPorAgencia::MutexGuard g;
    TipoDeMontoPorAgencia*	tipomonto_agencia =  TipoDeMontoPorAgencia::get(idAgente);
    
	if (tipomonto_agencia == NULL) {
		char mensaje[BUFFER_SIZE];    
		send2cliente(conexionActiva, ERR_AGENCIANOACT, NULL, 0);    
		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de montos.");
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}

	boost::uint8_t idZona = conexionActiva.idZona;
	n_t_zona_numero_restringido_t* zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	if (zona_num_restringido == NULL) {
		char mensaje[BUFFER_SIZE];

		sprintf(mensaje, "ADVERTENCIA: La zona <%u> no esta en memoria.", idZona);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	boost::uint8_t idSorteo = generarTerminales.idSorteo;

	TipoDeMonto* tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
	if (tipomonto_sorteo == NULL) {
		char mensaje[BUFFER_SIZE];

		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de monto para el sorteo <%u>.",
		         idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}

	Sorteo* sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
	if (sorteo == NULL) {
		char mensaje[BUFFER_SIZE];

		send2cliente(conexionActiva, ERR_SORTEONOEXIS, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: No existe el sorteo <%u>.", idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	n_t_sorteo_numero_restringido_t* sorteo_num_restringido =
	    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
	                                                         t_sorteo_numero_restringido,
	                                                         &idSorteo);

	boost::uint8_t tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
	n_t_tipomonto_t* tipoDeMonto =
	    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipomonto_terminal);
	if (tipoDeMonto == NULL) {
		char mensaje[BUFFER_SIZE];

		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje,
		         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
		         tipomonto_terminal);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return;
	}
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Generando Terminales %u", unsigned(generarTerminales.signo));
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
    boost::int32_t numeros[1000];
	for (int i = 0; i < 1000; i++)
		numeros[i] = -1;
    
    boost::uint8_t idloteria = 0;
    boost::uint16_t cantnumeros = 0;
	while (true) {
		Preventa* preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
		if (preventa != NULL) {
			if (preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO)
				break;
			if (idloteria == 0)
				idloteria = preventa->idSorteo;
			if (idloteria != preventa->idSorteo)
				break;
			cantnumeros++;
		}
		renglon--;
		if (renglon <= 0)
			break;
	}
	verificar_sorteo(&conexionActiva, sorteo, idSorteo, true);
	if (not sorteo->isEstadoActivo()) {
        RespuestaVentaRango respuestaVentaRango;
		respuestaVentaRango.begin = 0;
		respuestaVentaRango.end = 0;
		respuestaVentaRango.size = -1;
		int j = 0;
		for (int i = renglon + 1; i <= generarTerminales.renglon; i++) {
			Preventa* preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &i);
			if (preventa != NULL) {
				boost::uint16_t w, terminal, encontrado = false;

				if (!generarTerminales.signo)
					terminal = preventa->numero % 100;
                    
				else {
					char cnumero[6], terminalazo[5];
					terminal = preventa->numero % 100;
					sprintf(cnumero, "%02d%02d", generarTerminales.signo, terminal);
					sprintf(terminalazo,
					         "%c%c%c%c",
					         cnumero[0],
					         cnumero[1],
					         cnumero[2],
					         cnumero[3]);
					terminal = atoi(terminalazo);
			    }
				for (w = 0; w < j and !encontrado; w++) {
					if (terminal == numeros[w]) {
						encontrado = true;
					}
				}
				if (!encontrado) {
					numeros[j] = terminal;
					j++;
					if (respuestaVentaRango.begin == 0)
						respuestaVentaRango.begin = generarTerminales.renglon + j;
					respuestaVentaRango.end = generarTerminales.renglon + j;
					if (!generarTerminales.signo) {
						agregar_renglon_vacio_terminal(conexionActiva,
						                               generarTerminales.renglon + j,
						                               terminal,
						                               idSorteo,
						                               preventas_taquilla,
						                               idAgente,
						                               idTaquilla);
					} else {
						agregar_renglon_vacio_terminalazo(conexionActiva,
						                                  generarTerminales.renglon + j,
						                                  terminal,
						                                  idSorteo,
						                                  preventas_taquilla,
						                                  idAgente,
						                                  idTaquilla);
					}
				}
			}
		}
		send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
		return;
	}
	if (cantnumeros > 0) {
		int j = 0;
        RespuestaVentaRango respuestaVentaRango;
		respuestaVentaRango.begin = 0;
		respuestaVentaRango.end = 0;
		respuestaVentaRango.size = 0;
		boost::uint16_t sizeVal = 1024;
		boost::uint8_t* bufferz = new boost::uint8_t[sizeVal];
		boost::uint16_t van = 0;
		for (int i = renglon + 1; i <= generarTerminales.renglon; i++) {
			Preventa * preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &i);
			if (preventa != NULL) {
				boost::uint16_t w, terminal, encontrado = false;

				if (!generarTerminales.signo)
					terminal = preventa->numero % 100;
                    
				else {
					char cnumero[6], terminalazo[5];
					if (preventa->tipo == TRIPLE) {
						terminal = preventa->numero % 100;
						sprintf(cnumero, "%02d%02d", generarTerminales.signo, terminal);
						sprintf(terminalazo,
						         "%c%c%c%c",
						         cnumero[0],
						         cnumero[1],
						         cnumero[2],
						         cnumero[3]);
					} else {
						sprintf(cnumero, "%05d", preventa->numero);
						sprintf(terminalazo,
						         "%c%c%c%c",
						         cnumero[0],
						         cnumero[1],
						         cnumero[3],
						         cnumero[4]);
					}
					terminal = atoi(terminalazo);
				}
				for (w = 0; w < j and !encontrado; w++) {
					if (terminal == numeros[w]) {
						encontrado = true;
					}
				}
				if (!encontrado) {
					numeros[j] = terminal;
					j++;
					if (respuestaVentaRango.begin == 0)
						respuestaVentaRango.begin = generarTerminales.renglon + j;
					respuestaVentaRango.end = generarTerminales.renglon + j;
                                                              
                    boost::int32_t monto = (generarTerminales.monto == 0)
                                         ? preventa->monto_pedido
                                         : (preventa->monto_pedido == 0) ? 0 : generarTerminales.monto; 
                                            
                    Preventa* preventa2 = NULL;
					if (!generarTerminales.signo) {
						preventa2 = preventa_terminal(conexionActiva, 
                                                      monto, 
                                                      terminal, 
                                                      tipoDeMonto,
						                              zona_num_restringido, 
                                                      sorteo_num_restringido,
						                              generarTerminales.renglon + j, 
                                                      idSorteo, 
                                                      preventas_taquilla,
                                                      idAgente, 
                                                      idTaquilla, 
                                                      tipomonto_sorteo);
                                               
					} else {
						preventa2 = preventa_terminalazo(conexionActiva,
						                                 monto,
						                                 terminal,
						                                 tipoDeMonto,
						                                 zona_num_restringido,
						                                 sorteo_num_restringido,
						                                 generarTerminales.renglon + j,
						                                 idSorteo,
						                                 preventas_taquilla,
						                                 idAgente,
						                                 idTaquilla,
						                                 tipomonto_sorteo);
					}
                    
					if (preventa2 != NULL) {
						RespuestaVentaRenglonModificado 
                            respuestaVentaRenglonModificado(generarTerminales.renglon + j,
                                                            preventa2->monto_preventa);
						std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
						van += RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
						if (van > sizeVal) {
							boost::uint8_t * aux = new boost::uint8_t[sizeVal + 1024];
							memcpy(aux, bufferz, sizeVal);
							delete [] bufferz;
							bufferz = aux;
							sizeVal += 1024;
						}
						memcpy(bufferz + (respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE),
						       &buff2[0],
						       RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE);
						respuestaVentaRango.size++;
					}
				}
			}
		}
		if (respuestaVentaRango.size == 0) {
			send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
		} else {
			boost::uint16_t size = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE;
			size += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
			boost::uint8_t *buffer3 = new boost::uint8_t[size];
            std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer();
			memcpy(buffer3, &buff[0], RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
			memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, size - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
			send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, size);
			delete [] buffer3;
		}
		delete [] bufferz;
	}
}

/*-------------------------------------------------------------------------*/

void
atenderPeticionRepetirJugada(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
	boost::uint8_t idZona;
	boost::uint16_t cantnumeros = 0;
	boost::uint16_t renglon;
	boost::uint8_t idloteria = 0;
	boost::uint16_t i, j, pos;
	Preventa *preventa, *preventa2;
	boost::uint32_t idAgente;
	boost::uint8_t idTaquilla;
	boost::uint8_t idSorteo;
	boost::uint64_t agente_taquilla;
	n_t_preventas_x_taquilla_t *preventas_taquilla;
	Sorteo *sorteo;
	TipoDeMonto *tipomonto_sorteo;
	TipoDeMontoPorAgencia *tipomonto_agencia;
	n_t_zona_numero_restringido_t *zona_num_restringido;
	n_t_sorteo_numero_restringido_t *sorteo_num_restringido;
	n_t_tipomonto_t *tipomontoterminal;
	boost::uint8_t tipomonto_terminal;
	n_t_tipomonto_t *tipomontotriple;
	boost::uint8_t tipomonto_triple;
	RespuestaVentaRango respuestaVentaRango;
	boost::uint8_t *bufferz;
	boost::uint16_t sizeVal, van;
	boost::int32_t monto;
	boost::uint8_t comodines[40];

	repetir_jugada_t solicitudVenta2 = repetir_jugada_b2t(buff);
	for (i = 0; i < solicitudVenta2.cant_comodines; i++) {
		comodines[i] = buff[REPETIR_JUGADA_LON + i];
	}
	idAgente = conexionActiva.idAgente;
	agente_taquilla = conexionActiva.idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += conexionActiva.idTaquilla;
	idTaquilla = conexionActiva.idTaquilla;
	preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	renglon = solicitudVenta2.renglon;
    {
        TipoDeMontoPorAgencia::MutexGuard g;
    	tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
    
    	{
    		char mensaje[BUFFER_SIZE];
    		sprintf(mensaje, "Generando Ambas");
    		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    	}
    
    	if (tipomonto_agencia == NULL) {
    		char mensaje[BUFFER_SIZE];
    		send2cliente(conexionActiva, ERR_AGENCIANOACT, NULL, 0);
    		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de montos.");
    		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    		return ;
    	}
    }
	idZona = conexionActiva.idZona;
	zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	if (zona_num_restringido == NULL) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: La zona <%u> no esta en memoria.", idZona);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	idSorteo = solicitudVenta2.idSorteo;
	tipomonto_sorteo =tipomonto_agencia->getTipoDeMonto(idSorteo);
	if (tipomonto_sorteo == NULL) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de monto para el sorteo <%u>.",
		         idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
	if (sorteo == NULL) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_SORTEONOEXIS, &idSorteo, sizeof(&idSorteo));
		sprintf(mensaje, "ADVERTENCIA: No existe el sorteo <%u>.", idSorteo);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	if (not sorteo->isConSigno())
		solicitudVenta2.cant_comodines = 0;
	sorteo_num_restringido =
	    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
	                                                         t_sorteo_numero_restringido,
	                                                         &idSorteo);
	if (sorteo_num_restringido == NULL)
		return ;
	tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
	tipomontoterminal =
	    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipomonto_terminal);
	if (tipomontoterminal == NULL) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje,
		         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
		         tipomonto_terminal);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
	tipomontotriple =
	    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipomonto_triple);
	if (tipomontotriple == NULL) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje,
		         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
		         tipomonto_triple);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}
	while (1) {
		preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
		if (preventa != NULL) {
			if (idloteria == 0)
				idloteria = preventa->idSorteo;
			if (idloteria != preventa->idSorteo)
				break;
			cantnumeros++;
		}
		renglon--;
		if (renglon <= 0)
			break;
	}
	sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
	verificar_sorteo(&conexionActiva, sorteo, idSorteo, true);
	if (not sorteo->isEstadoActivo()) {
		j = 0;
		respuestaVentaRango.begin = 0;
		respuestaVentaRango.end = 0;
		respuestaVentaRango.size = -1;
		if (solicitudVenta2.cant_comodines == 0) {
			for (i = renglon + 1; i <= solicitudVenta2.renglon; i++) {
				preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &i);
				if (preventa != NULL) {
					boost::uint16_t numero;

					numero = preventa->numero;
					j++;
					if (respuestaVentaRango.begin == 0)
						respuestaVentaRango.begin = solicitudVenta2.renglon + j;
					respuestaVentaRango.end = solicitudVenta2.renglon + j;
					if ((preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO)
					        and (not sorteo->isConSigno())) {
						numero = numero % 100;
						agregar_renglon_vacio_terminal(conexionActiva,
						                                solicitudVenta2.renglon + j,
						                                numero,
						                                idSorteo,
						                                preventas_taquilla,
						                                idAgente,
						                                idTaquilla);

					} else if ((preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO)
					            and (not sorteo->isConSigno())) {
						numero = numero % 1000;
						agregar_renglon_vacio_triple(conexionActiva,
						                              solicitudVenta2.renglon + j,
						                              numero,
						                              idSorteo,
						                              preventas_taquilla,
						                              idAgente,
						                              idTaquilla,
						                              preventa->monto_pedido);
					} else if (preventa->tipo == TERMINALAZO) {
						agregar_renglon_vacio_terminalazo(conexionActiva,
						                                   solicitudVenta2.renglon + j,
						                                   numero,
						                                   idSorteo,
						                                   preventas_taquilla,
						                                   idAgente,
						                                   idTaquilla);
					} else if (preventa->tipo == TRIPLETAZO) {
						agregar_renglon_vacio_tripletazo(conexionActiva,
						                                  solicitudVenta2.renglon + j,
						                                  numero,
						                                  idSorteo,
						                                  preventas_taquilla,
						                                  idAgente,
						                                  idTaquilla,
						                                  preventa->monto_pedido);
					}
				}
			}
		} else {
			for (int w = 0; w < solicitudVenta2.cant_comodines; w++) {
				for (i = renglon + 1; i <= solicitudVenta2.renglon; i++) {
					preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &i);
					if (preventa != NULL) {
						boost::uint16_t numero = preventa->numero;
						j++;
						if (respuestaVentaRango.begin == 0)
							respuestaVentaRango.begin = solicitudVenta2.renglon + j;
						respuestaVentaRango.end = solicitudVenta2.renglon + j;
						if (preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO) {
							int terminalazo = numero % 100;
							terminalazo = comodines[w] * 100 + terminalazo;
							agregar_renglon_vacio_terminalazo(conexionActiva,
							                                   solicitudVenta2.renglon + j,
							                                   terminalazo,
							                                   idSorteo,
							                                   preventas_taquilla,
							                                   idAgente,
							                                   idTaquilla);
						} else if (preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO) {
							int tripletazo = numero % 1000;
							tripletazo = comodines[w] * 1000 + tripletazo;
							agregar_renglon_vacio_tripletazo(conexionActiva,
							                                  solicitudVenta2.renglon + j,
							                                  tripletazo,
							                                  idSorteo,
							                                  preventas_taquilla,
							                                  idAgente,
							                                  idTaquilla,
							                                  preventa->monto_pedido);
						}
					}
				}
			}
		}
		send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
		return ;
	}
	if (cantnumeros > 0) {
		pos = 0;
		j = 0;
		respuestaVentaRango.begin = 0;
		respuestaVentaRango.end = 0;
		respuestaVentaRango.size = 0;
		sizeVal = 1024;
		bufferz = new boost::uint8_t[sizeVal];
		van = 0;
		if (solicitudVenta2.cant_comodines == 0) {
			for (i = renglon + 1; i <= solicitudVenta2.renglon; i++) {
				preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &i);
				if (preventa != NULL) {
					boost::uint16_t numero;

					numero = preventa->numero;
					j++;
					if (respuestaVentaRango.begin == 0)
						respuestaVentaRango.begin = solicitudVenta2.renglon + j;
					respuestaVentaRango.end = solicitudVenta2.renglon + j;
					if ((preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO)
					        and (not sorteo->isConSigno())) {
						numero = numero % 100;
						if (solicitudVenta2.montoTerminales == 0)
							monto = preventa->monto_pedido;
						else
							monto = preventa->monto_pedido == 0 ? 0 : solicitudVenta2.montoTerminales;

						preventa2 = preventa_terminal(conexionActiva,
						                               monto,
						                               numero,
						                               tipomontoterminal,
						                               zona_num_restringido,
						                               sorteo_num_restringido,
						                               solicitudVenta2.renglon + j,
						                               idSorteo,
						                               preventas_taquilla,
						                               idAgente,
						                               idTaquilla,
						                               tipomonto_sorteo);

					} else if ((preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO)
					            and (not sorteo->isConSigno())) {
						numero = numero % 1000;
						if (solicitudVenta2.montoTriple == 0)
							monto = preventa->monto_pedido;
						else
							monto = preventa->monto_pedido == 0 ? 0 : solicitudVenta2.montoTriple;

						preventa2 = preventa_triple(conexionActiva,
						                             monto,
						                             numero,
						                             tipomontotriple,
						                             zona_num_restringido,
						                             sorteo_num_restringido,
						                             solicitudVenta2.renglon + j,
						                             idSorteo,
						                             preventas_taquilla,
						                             idAgente,
						                             idTaquilla,
						                             tipomonto_sorteo);
					} else if (preventa->tipo == TERMINALAZO or preventa->tipo == TERMINAL) {
						if (solicitudVenta2.montoTerminales == 0)
							monto = preventa->monto_pedido;
						else
							monto = preventa->monto_pedido == 0 ? 0 : solicitudVenta2.montoTerminales;

						preventa2 = preventa_terminalazo(conexionActiva,
						                                  monto,
						                                  numero,
						                                  tipomontoterminal,
						                                  zona_num_restringido,
						                                  sorteo_num_restringido,
						                                  solicitudVenta2.renglon + j,
						                                  idSorteo,
						                                  preventas_taquilla,
						                                  idAgente, idTaquilla, tipomonto_sorteo);
					} else if (preventa->tipo == TRIPLETAZO or preventa->tipo == TRIPLE) {
						if (solicitudVenta2.montoTriple == 0)
							monto = preventa->monto_pedido;
						else
							monto = preventa->monto_pedido == 0 ? 0 : solicitudVenta2.montoTriple;
						preventa2 = preventa_tripletazo(conexionActiva,
						                                 monto,
						                                 numero,
						                                 tipomontotriple,
						                                 zona_num_restringido,
						                                 sorteo_num_restringido,
						                                 solicitudVenta2.renglon + j,
						                                 idSorteo,
						                                 preventas_taquilla,
						                                 idAgente,
						                                 idTaquilla,
						                                 tipomonto_sorteo);
					} else {
						preventa2 = NULL;
					}

					if (preventa2 != NULL) {
						RespuestaVentaRenglonModificado 
                            respuestaVentaRenglonModificado(solicitudVenta2.renglon + j,
                                                            preventa2->monto_preventa);
						std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
						van += RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
						if (van > sizeVal) {
							boost::uint8_t * aux = new boost::uint8_t[sizeVal + 1024];
							memcpy(aux, bufferz, sizeVal);
							delete [] bufferz;
							bufferz = aux;
							sizeVal += 1024;
						}
						memcpy(bufferz + (respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE),
						       &buff2[0],
						       RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE);
						respuestaVentaRango.size++;
					}
				}
			}
			if (respuestaVentaRango.size == 0) {
				send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
			} else {
				boost::uint16_t size = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE;
				size += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
				boost::uint8_t *buffer3 = new boost::uint8_t[size];
                std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer();
				memcpy(buffer3, &buff[0], RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
				memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, size - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);

				send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, size);
				delete [] buffer3;
			}
			delete [] bufferz;
		} else {
			for (int w = 0; w < solicitudVenta2.cant_comodines; w++) {
				for (i = renglon + 1; i <= solicitudVenta2.renglon; i++) {
					preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &i);
					if (preventa != NULL) {
						boost::uint16_t numero = preventa->numero;
						j++;
						if (respuestaVentaRango.begin == 0)
							respuestaVentaRango.begin = solicitudVenta2.renglon + j;
						respuestaVentaRango.end = solicitudVenta2.renglon + j;
						if (preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO) {
							int terminalazo = numero % 100;
							terminalazo = comodines[w] * 100 + terminalazo;
							if (solicitudVenta2.montoTerminales == 0)
								monto = preventa->monto_pedido;
							else
								monto = preventa->monto_pedido == 0 ? 0 : solicitudVenta2.montoTerminales;
							preventa2 = preventa_terminalazo(conexionActiva,
							                                  monto,
							                                  terminalazo,
							                                  tipomontoterminal,
							                                  zona_num_restringido,
							                                  sorteo_num_restringido,
							                                  solicitudVenta2.renglon + j,
							                                  idSorteo,
							                                  preventas_taquilla,
							                                  idAgente,
							                                  idTaquilla,
							                                  tipomonto_sorteo);
						} else if (preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO) {
							int tripletazo = numero % 1000;
							tripletazo = comodines[w] * 1000 + tripletazo;
							if (solicitudVenta2.montoTriple == 0)
								monto = preventa->monto_pedido;
							else
								monto = preventa->monto_pedido == 0 ? 0 : solicitudVenta2.montoTriple;
							preventa2 = preventa_tripletazo(conexionActiva,
							                                 monto,
							                                 tripletazo,
							                                 tipomontotriple,
							                                 zona_num_restringido,
							                                 sorteo_num_restringido,
							                                 solicitudVenta2.renglon + j,
							                                 idSorteo,
							                                 preventas_taquilla,
							                                 idAgente,
							                                 idTaquilla,
							                                 tipomonto_sorteo);
						} else {
							preventa2 = NULL;
						}
						if (preventa2 != NULL) {
							RespuestaVentaRenglonModificado 
                                respuestaVentaRenglonModificado(solicitudVenta2.renglon + j,
                                                                preventa2->monto_preventa);							
                            std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
							van += RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
							if (van > sizeVal) {
								boost::uint8_t * aux = new boost::uint8_t[sizeVal + 1024];
								memcpy(aux, bufferz, sizeVal);
								delete [] bufferz;
								bufferz = aux;
								sizeVal += 1024;
							}
							memcpy(bufferz + (respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), 
                                   &buff2[0],
							       RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE);
							
							respuestaVentaRango.size++;
						}
					}
				}
			}
			if (respuestaVentaRango.size == 0) {
				send2cliente(conexionActiva, RESPVENTAXRANGO, respuestaVentaRango);
			} else {
				boost::uint16_t size = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE;
				size += respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
				boost::uint8_t *buffer3 = new boost::uint8_t[size];
				std::vector<boost::uint8_t> buff = respuestaVentaRango.toRawBuffer();
				memcpy(buffer3, &buff[0], RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
				memcpy(buffer3 + RespuestaVentaRango::CONST_RAW_BUFFER_SIZE, bufferz, size - RespuestaVentaRango::CONST_RAW_BUFFER_SIZE);
				
				send2cliente(conexionActiva, RESPVENTAXRANGO, buffer3, size);
				delete [] buffer3;
			}
			delete [] bufferz;
		}
	}
}

/*-------------------------------------------------------------------------*/

void
liberar_preventa(ConexionActiva& conexionActiva)
{
	boost::uint32_t idAgente;
	boost::uint64_t agente_taquilla;
	n_t_preventas_x_taquilla_t *preventas_taquilla;
	boost::uint16_t renglon;
	Preventa *preventa;
	boost::uint32_t nnodes;
	boost::uint8_t idSorteo, idZona;
	boost::uint16_t numero;
	Sorteo *sorteo;
	TipoDeMontoPorAgencia *tipomonto_agencia;
	TipoDeMonto *tipomonto_sorteo;
	n_t_venta_numero_t *venta_numero;
	n_t_tipomonto_t *tipoDeMonto;
	n_t_zona_numero_restringido_t *zona_num_restringido;
	n_t_sorteo_numero_restringido_t *sorteo_num_restringido;
	n_t_numero_restringido_t *num_restringido;

	idZona = conexionActiva.idZona;
	zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	idAgente = conexionActiva.idAgente;
	agente_taquilla = conexionActiva.idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += conexionActiva.idTaquilla;
	preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
	renglon = 1;
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	nnodes = g_tree_nnodes(preventas_taquilla->preventas);
	while (nnodes > 0) {
		preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
		if (preventa != NULL) {
			idSorteo = preventa->idSorteo;
			numero = preventa->numero;
			sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
			sorteo_num_restringido =
			    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
			                                                         t_sorteo_numero_restringido,
			                                                         &idSorteo);
			tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
			if (preventa->tipo == TERMINAL or preventa->tipo == TERMINALAZO) {
				boost::uint8_t tipomonto_terminal;

				tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
				tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
				                                         &tipomonto_terminal);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipomonto_terminal);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}

				/*---------------------------------------------------------------------------------*/
				sem_wait(&tipoDeMonto->sem_t_tipomonto);
				tipoDeMonto->venta_global += preventa->monto_preventa;
				limite_tipomonto(tipoDeMonto, preventa->tipo);
				venta_numero =
				    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
				venta_numero->preventa -= preventa->monto_preventa;
				venta_numero->venta += preventa->monto_preventa;
				sem_post(&tipoDeMonto->sem_t_tipomonto);

				/*-------------------------------------------------------------------------*/

				sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
				num_restringido =
				    (n_t_numero_restringido_t *)
				    g_tree_lookup(sorteo_num_restringido->t_numero_restringido_terminal,
				                   &numero);
				if (num_restringido != NULL) {
					num_restringido->preventa -= preventa->monto_preventa;
					num_restringido->venta += preventa->monto_preventa;
				}
				sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			} else if (preventa->tipo == TRIPLE or preventa->tipo == TRIPLETAZO) {
				boost::uint8_t tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
				tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(),
				                                         &tipomonto_triple);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];

					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipomonto_triple);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}

				/*-------------------------------------------------------------------------*/
				sem_wait(&tipoDeMonto->sem_t_tipomonto);
				tipoDeMonto->venta_global += preventa->monto_preventa;
				limite_tipomonto(tipoDeMonto, preventa->tipo);
				venta_numero = (n_t_venta_numero_t *)
				               g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
				venta_numero->preventa -= preventa->monto_preventa;
				venta_numero->venta += preventa->monto_preventa;
				sem_post(&tipoDeMonto->sem_t_tipomonto);

				/*--------------------------------------------------------------------------------------*/

				sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
				num_restringido =
				    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
				                                                  t_numero_restringido_triple,
				                                                  &numero);
				if (num_restringido != NULL) {
					num_restringido->preventa -= preventa->monto_preventa;
					num_restringido->venta += preventa->monto_preventa;
				}
				sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			}
			g_tree_remove(preventas_taquilla->preventas, &renglon);
		}
		renglon++;
		nnodes = g_tree_nnodes(preventas_taquilla->preventas);
	}
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Liberando la Preventa");
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
}

/*-------------------------------------------------------------------------*/

boost::uint32_t
genera_serial(boost::uint32_t nticket, boost::int32_t monto_total, boost::uint32_t idAgente, boost::uint8_t idTaquilla,
               boost::uint32_t usuario, char *fechahora)
{
	char cad[200];
	sprintf(cad, "%u%u%u%u%u%s", nticket, monto_total, idAgente, idTaquilla, usuario, fechahora);
	boost::uint32_t crc = crc32(0, (Bytef *) cad, strlen(cad));
	return crc;
}

void
genera_serial(char serial[SERIAL_LENGTH], boost::uint32_t nticket, boost::int32_t monto_total, boost::uint32_t idAgente, 
              boost::uint8_t idTaquilla, boost::uint32_t usuario, char *fechahora)
{
    char cad[200];
    sprintf(cad, "%u%u%u%u%u%s", nticket, monto_total, idAgente, idTaquilla, usuario, fechahora);
    boost::uint32_t crc = crc32(0, (Bytef *) cad, strlen(cad));
    sprintf(cad, "%026u", crc);
    memcpy(serial, cad, SERIAL_LENGTH);
}

/*-------------------------------------------------------------------------*/

void
enviar_ticket_guardado_V_0(ConexionActiva& conexionActiva, 
                           boost::int32_t fechaPosix, 
                           boost::uint32_t idTicket,
                           boost::uint32_t serial, boost::int32_t monto,
                           std::vector<boost::uint16_t> const& bonos,
                           std::vector<boost::uint8_t> const& sorteos, 
                           boost::uint8_t idSorteo)
{
	TicketGuardadoV0 ticketGuardado;
	ticketGuardado.fechaHora = fechaPosix - 1800;
	ticketGuardado.nticket = idTicket;
	ticketGuardado.serialtic = serial;
	ticketGuardado.monto = monto;
	ticketGuardado.nsorteos = sorteos.size();
	std::vector<boost::uint8_t> bufferConfirmarTicket = ticketGuardado.toRawBuffer();
    
    boost::uint16_t sorteosLon = (ticketGuardado.nsorteos > 0) ? ticketGuardado.nsorteos * sizeof(boost::uint8_t) : 0;
    boost::uint16_t bonosLon = (not bonos.empty()) ? bonos.size() * sizeof(boost::uint16_t) + 1 : 0;
	boost::uint16_t lonTotal = TicketGuardadoV0::CONST_RAW_BUFFER_SIZE + sorteosLon + bonosLon;
	boost::uint8_t * bufferTotal = new boost::uint8_t[lonTotal];
	memcpy(bufferTotal, &bufferConfirmarTicket[0], bufferConfirmarTicket.size());
    
	if (sorteosLon > 0)
		memcpy(bufferTotal + TicketGuardadoV0::CONST_RAW_BUFFER_SIZE, &sorteos[0], sorteosLon);
	if (bonosLon > 0) {
		memcpy(bufferTotal + TicketGuardadoV0::CONST_RAW_BUFFER_SIZE + sorteosLon, &idSorteo, 1);
		memcpy(bufferTotal + TicketGuardadoV0::CONST_RAW_BUFFER_SIZE + sorteosLon + 1, &bonos[0], bonosLon - 1);
	}
	send2cliente(conexionActiva, GUARDAR_TICKET_V_0, bufferTotal, lonTotal);
	delete [] bufferTotal;
}

void
enviar_ticket_guardado_V_1(ConexionActiva& conexionActiva, 
                           boost::int32_t fechaPosix, 
                           boost::uint32_t idTicket,
                           char const serial[SERIAL_LENGTH], 
                           boost::int32_t monto,
                           std::vector<boost::uint16_t> const& bonos,
                           std::vector<boost::uint8_t> const& sorteos, 
                           boost::uint8_t idSorteo,
                           boost::uint32_t correlativo)
{
    TicketGuardadoV1 ticketGuardado;
    ticketGuardado.fechaHora = fechaPosix - 1800;
    ticketGuardado.nticket = idTicket;
    memcpy(ticketGuardado.serial, serial, SERIAL_LENGTH);
    ticketGuardado.monto = monto;
    ticketGuardado.nsorteos = sorteos.size();
    ticketGuardado.impuesto = 0;
    ticketGuardado.correlativo = correlativo;
    std::vector<boost::uint8_t> bufferConfirmarTicket = ticketGuardado.toRawBuffer();
    
    boost::uint16_t sorteosLon = (ticketGuardado.nsorteos > 0) ? ticketGuardado.nsorteos * sizeof(boost::uint8_t) : 0;
    boost::uint16_t bonosLon = (not bonos.empty()) ? bonos.size() * sizeof(boost::uint16_t) + 1 : 0;
    boost::uint16_t lonTotal = TicketGuardadoV1::CONST_RAW_BUFFER_SIZE + sorteosLon + bonosLon;
    boost::uint8_t * bufferTotal = new boost::uint8_t[lonTotal];
    memcpy(bufferTotal, &bufferConfirmarTicket[0], TicketGuardadoV1::CONST_RAW_BUFFER_SIZE);
    
    if (sorteosLon > 0)
        memcpy(bufferTotal + TicketGuardadoV1::CONST_RAW_BUFFER_SIZE, &sorteos[0], sorteosLon);
    if (bonosLon > 0) {
        memcpy(bufferTotal + TicketGuardadoV1::CONST_RAW_BUFFER_SIZE + sorteosLon, &idSorteo, 1);
        memcpy(bufferTotal + TicketGuardadoV1::CONST_RAW_BUFFER_SIZE + sorteosLon + 1, &bonos[0], bonosLon - 1);
    }
    send2cliente(conexionActiva, GUARDAR_TICKET_V_1, bufferTotal, lonTotal);
    delete [] bufferTotal;
}

void
enviar_ticket_guardado_V_2(ConexionActiva& conexionActiva, 
                           boost::int32_t fechaPosix, 
                           boost::uint32_t idTicket,
                           char const serial[SERIAL_LENGTH], 
                           boost::int32_t monto,
                           std::vector<boost::uint16_t> const& bonos,
                           std::vector<boost::uint8_t> const& sorteos, 
                           boost::uint8_t idSorteo,
                           boost::uint32_t correlativo)
{
    TicketGuardadoV1 ticketGuardado;
    ticketGuardado.fechaHora = fechaPosix - 1800;
    ticketGuardado.nticket = idTicket;
    memcpy(ticketGuardado.serial, serial, SERIAL_LENGTH);
    ticketGuardado.monto = monto;
    ticketGuardado.nsorteos = sorteos.size();
    ticketGuardado.impuesto = 0;
    ticketGuardado.correlativo = correlativo;
    std::vector<boost::uint8_t> bufferConfirmarTicket = ticketGuardado.toRawBuffer();
    
    boost::uint16_t sorteosLon = (ticketGuardado.nsorteos > 0) ? ticketGuardado.nsorteos * sizeof(boost::uint8_t) : 0;
    boost::uint16_t bonosLon = (not bonos.empty()) ? bonos.size() * sizeof(boost::uint16_t) + 1 : 0;
    boost::uint16_t lonTotal = TicketGuardadoV1::CONST_RAW_BUFFER_SIZE + sorteosLon + bonosLon;
    boost::uint8_t * bufferTotal = new boost::uint8_t[lonTotal];
    memcpy(bufferTotal, &bufferConfirmarTicket[0], TicketGuardadoV1::CONST_RAW_BUFFER_SIZE);
    
    if (sorteosLon > 0)
        memcpy(bufferTotal + TicketGuardadoV1::CONST_RAW_BUFFER_SIZE, &sorteos[0], sorteosLon);
    if (bonosLon > 0) {
        memcpy(bufferTotal + TicketGuardadoV1::CONST_RAW_BUFFER_SIZE + sorteosLon, &idSorteo, 1);
        memcpy(bufferTotal + TicketGuardadoV1::CONST_RAW_BUFFER_SIZE + sorteosLon + 1, &bonos[0], bonosLon - 1);
    }
    send2cliente(conexionActiva, GUARDAR_TICKET_V_2, bufferTotal, lonTotal);
    delete [] bufferTotal;
}

void
enviar_ticket_guardado_V_3(ConexionActiva& conexionActiva,
                           boost::int32_t fechaPosix, 
                           boost::uint32_t idTicket,
                           boost::int32_t monto,
                           boost::uint32_t correlativo,
                           std::string const& serial,
                           boost::uint8_t idSorteoBono,                       
                           std::vector<boost::uint16_t> const& bonos,
                           std::vector<RenglonModificado> const& renglonesModificados)
{
    TicketGuardadoV2 ticketGuardado;
    ticketGuardado.fechaHora = fechaPosix - 1800;
    ticketGuardado.idTicket = idTicket;
    ticketGuardado.monto = monto;
    ticketGuardado.correlativo = correlativo;
    ticketGuardado.idSorteoBono = idSorteoBono;
    ticketGuardado.serial = String(serial);
    ticketGuardado.bonos = bonos; 
    ticketGuardado.renglonesModificados = renglonesModificados;
    std::vector<boost::uint8_t> buffer = ticketGuardado.toRawBuffer();
    send2cliente(conexionActiva, GUARDAR_TICKET_V_3, buffer);
}
/*-------------------------------------------------------------------------*/

void
confirmacion_guardados_V_0(ConexionActiva& conexionActiva, boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                        MYSQL * mysql)
{
	boost::uint32_t idTicket = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT MAX(Id) FROM Tickets WHERE IdAgente=%1% AND NumTaquilla=%2%")
		% idAgente % unsigned(idTaquilla);
		ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			idTicket = atoi(row[0]);
		}
		mysql_free_result(result);
	}

	// if (idTicket == 0) { ... } <= FIXME: hay que implementar verificacion de errores para este caso

	boost::int32_t fechaPosix = 0;
	boost::uint32_t monto = 0, serial = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), Monto, Serial FROM Tickets WHERE Id=%1%")
		% idTicket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			fechaPosix = atoi(row[0]);
			monto = atoi(row[1]);
			serial = strtoul(row[2], NULL, 10);
		}
		mysql_free_result(result);
	}

	std::vector<std::pair<boost::uint8_t, boost::uint16_t> > bonosDeSorteos;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT IdSorteo, Numero FROM Renglones "
		                "WHERE IdTicket=%1% AND Tipo IN (%2%, %3%) ORDER BY Id")
		% idTicket % unsigned(BONO) % unsigned(BONOTRIPLETAZO);
		ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			bonosDeSorteos.push_back(std::make_pair(atoi(row[0]), atoi(row[1])));
		}
		mysql_free_result(result);
	}

	std::vector<boost::uint16_t> bonos;
	boost::uint8_t idSorteo = 0;
	for (std::vector<std::pair<boost::uint8_t, boost::uint16_t> >::const_iterator iter = bonosDeSorteos.begin();
         iter != bonosDeSorteos.end(); ++iter) {
		idSorteo = iter->first;
		bonos.push_back(iter->second);
	}

	std::vector<boost::uint8_t> idSorteos;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT IdSorteo FROM Info_Ticket WHERE IdTicket=%1%") % idTicket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			idSorteos.push_back(atoi(row[0]));
		}
		mysql_free_result(result);
	}

    enviar_ticket_guardado_V_0(conexionActiva, fechaPosix, idTicket, serial, monto, bonos, idSorteos, idSorteo);
}

void
confirmacion_guardados_V_1(ConexionActiva& conexionActiva, boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                           MYSQL * mysql)
{
    boost::uint32_t idTicket = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT MAX(Id) FROM Tickets WHERE IdAgente=%1% AND NumTaquilla=%2%")
        % idAgente % unsigned(idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            idTicket = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    // if (idTicket == 0) { ... } <= FIXME: hay que implementar verificacion de errores para este caso

    boost::int32_t fechaPosix = 0;
    boost::uint32_t monto = 0;
    boost::uint32_t correlativo = 0;
    char serial[SERIAL_LENGTH];
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), Monto, Serial, Correlativo FROM Tickets WHERE Id=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            fechaPosix = strtoul(row[0], NULL, 10);
            monto = atoi(row[1]);
            memcpy(serial, row[2], SERIAL_LENGTH);
            correlativo = strtoul(row[3], NULL, 10);
        }
        mysql_free_result(result);
    }

    std::vector<std::pair<boost::uint8_t, boost::uint16_t> > bonosDeSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo, Numero FROM Renglones "
                        "WHERE IdTicket=%1% AND Tipo IN (%2%, %3%) ORDER BY Id")
        % idTicket % unsigned(BONO) % unsigned(BONOTRIPLETAZO);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            bonosDeSorteos.push_back(std::make_pair(atoi(row[0]), atoi(row[1])));
        }
        mysql_free_result(result);
    }
    
    std::vector<boost::uint16_t> bonos;
    boost::uint8_t idSorteo = 0;
    for (std::vector<std::pair<boost::uint8_t, boost::uint16_t> >::const_iterator iter = bonosDeSorteos.begin();
         iter != bonosDeSorteos.end(); ++iter) {
        idSorteo = iter->first;
        bonos.push_back(iter->second);
    }

    std::vector<boost::uint8_t> idSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Info_Ticket WHERE IdTicket=%1%") % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            idSorteos.push_back(atoi(row[0]));
        }
        mysql_free_result(result);
    }

    enviar_ticket_guardado_V_1(conexionActiva, 
                            fechaPosix, 
                            idTicket, 
                            serial, 
                            monto, 
                            bonos, 
                            idSorteos, 
                            idSorteo, 
                            correlativo);
}

void
confirmacion_guardados_V_2(ConexionActiva& conexionActiva, boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                           MYSQL * mysql)
{
    boost::uint32_t idTicket = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT MAX(Id) FROM Tickets WHERE IdAgente=%1% AND NumTaquilla=%2%")
        % idAgente % unsigned(idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            idTicket = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    // if (idTicket == 0) { ... } <= FIXME: hay que implementar verificacion de errores para este caso

    boost::int32_t fechaPosix = 0;
    boost::uint32_t monto = 0;
    boost::uint32_t correlativo = 0;
    char serial[SERIAL_LENGTH];
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), Monto, Serial, Correlativo FROM Tickets WHERE Id=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            fechaPosix = strtoul(row[0], NULL, 10);
            monto = atoi(row[1]);
            memcpy(serial, row[2], SERIAL_LENGTH);
            correlativo = strtoul(row[3], NULL, 10);
        }
        mysql_free_result(result);
    }

    std::vector<std::pair<boost::uint8_t, boost::uint16_t> > bonosDeSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo, Numero FROM Renglones "
                        "WHERE IdTicket=%1% AND Tipo IN (%2%, %3%) ORDER BY Id")
        % idTicket % unsigned(BONO) % unsigned(BONOTRIPLETAZO);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            bonosDeSorteos.push_back(std::make_pair(atoi(row[0]), atoi(row[1])));
        }
        mysql_free_result(result);
    }
    
    std::vector<boost::uint16_t> bonos;
    boost::uint8_t idSorteo = 0;
    for (std::vector<std::pair<boost::uint8_t, boost::uint16_t> >::const_iterator iter = bonosDeSorteos.begin();
         iter != bonosDeSorteos.end(); ++iter) {
        idSorteo = iter->first;
        bonos.push_back(iter->second);
    }

    std::vector<boost::uint8_t> idSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Info_Ticket WHERE IdTicket=%1%") % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            idSorteos.push_back(atoi(row[0]));
        }
        mysql_free_result(result);
    }

    enviar_ticket_guardado_V_2(conexionActiva, 
                               fechaPosix, 
                               idTicket, 
                               serial, 
                               monto, 
                               bonos, 
                               idSorteos, 
                               idSorteo, 
                               correlativo);
}

/*-------------------------------------------------------------------------*/

void
confirmacion_guardados_V_3(ConexionActiva& conexionActiva, boost::uint32_t idAgente, boost::uint8_t idTaquilla,
                           MYSQL * mysql)
{
    boost::uint32_t idTicket = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT MAX(Id) FROM Tickets WHERE IdAgente=%1% AND NumTaquilla=%2%")
        % idAgente % unsigned(idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            idTicket = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    // if (idTicket == 0) { ... } <= FIXME: hay que implementar verificacion de errores para este caso

    boost::int32_t fechaPosix = 0;
    boost::uint32_t monto = 0;
    boost::uint32_t correlativo = 0;
    char serial[SERIAL_LENGTH];
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), Monto, Serial, Correlativo FROM Tickets WHERE Id=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            fechaPosix = strtoul(row[0], NULL, 10);
            monto = atoi(row[1]);
            memcpy(serial, row[2], SERIAL_LENGTH);
            correlativo = strtoul(row[3], NULL, 10);
        }
        mysql_free_result(result);
    }

    std::vector<std::pair<boost::uint8_t, boost::uint16_t> > bonosDeSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo, Numero FROM Renglones "
                        "WHERE IdTicket=%1% AND Tipo IN (%2%, %3%) ORDER BY Id")
        % idTicket % unsigned(BONO) % unsigned(BONOTRIPLETAZO);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            bonosDeSorteos.push_back(std::make_pair(atoi(row[0]), atoi(row[1])));
        }
        mysql_free_result(result);
    }
    
    std::vector<boost::uint16_t> bonos;
    boost::uint8_t idSorteoBono = 0;
    for (std::vector<std::pair<boost::uint8_t, boost::uint16_t> >::const_iterator iter = bonosDeSorteos.begin();
         iter != bonosDeSorteos.end(); ++iter) {
        idSorteoBono = iter->first;
        bonos.push_back(iter->second);
    }

    std::vector<boost::uint8_t> idSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Info_Ticket WHERE IdTicket=%1%") % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONFIRMACION_GUARDADOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            idSorteos.push_back(atoi(row[0]));
        }
        mysql_free_result(result);
    }
                          
    enviar_ticket_guardado_V_3(conexionActiva, 
                               fechaPosix, 
                               idTicket,
                               monto,
                               correlativo, 
                               serial,
                               idSorteoBono,                           
                               bonos, 
                               std::vector<RenglonModificado>());
}

std::vector<boost::uint16_t>
generar_bonos(ConexionActiva& conexionActiva, boost::uint8_t idSorteo, boost::uint8_t cuantos,
              boost::uint8_t idZona, boost::uint32_t idAgente, boost::int32_t monto, boost::uint16_t renglon,
              boost::uint8_t idTaquilla, n_t_preventas_x_taquilla_t * preventas_taquilla,
              boost::uint8_t * tipoJugada)
{
    std::vector<boost::uint16_t> result;
    
	TipoDeMontoPorAgencia * tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);

	if (tipomonto_agencia == NULL) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de montos.");
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return result;
	}

	TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
	if (tipomonto_sorteo == NULL) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
		sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de montos.");
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return result;
	}

	Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);

	n_t_zona_numero_restringido_t * zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);

	n_t_sorteo_numero_restringido_t * sorteo_num_restringido =
	    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
	                                                         t_sorteo_numero_restringido,
	                                                         &idSorteo);

	boost::uint8_t tipomonto_triple = tipomonto_sorteo->tipomonto_triple;

	n_t_tipomonto_t * tipomontotriple =
	    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipomonto_triple);

	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Generando Los Bonos");
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
	*tipoJugada = ((not sorteo->isConSigno())) ? BONO : BONOTRIPLETAZO;
	for (boost::uint16_t i = 0; i < cuantos;) {        
        SolicitudVenta solicitudVenta;
		solicitudVenta.monto = monto;
		Preventa *preventa = NULL;
		boost::uint16_t bono;

		if ((not sorteo->isConSigno())) {
			bono = g_rand_int_range(aleatorio, 0, MAXTRIPLE);
			preventa =
			    preventa_triple(conexionActiva, solicitudVenta.monto, bono, tipomontotriple,
			                     zona_num_restringido, sorteo_num_restringido, renglon, idSorteo,
			                     preventas_taquilla, idAgente, idTaquilla, tipomonto_sorteo);
		} else {
			bono = g_rand_int_range(aleatorio, MAXTRIPLE + 1, MAXTRIPLETAZO);
			preventa =
			    preventa_tripletazo(conexionActiva, solicitudVenta.monto, bono, tipomontotriple,
			                         zona_num_restringido, sorteo_num_restringido, renglon, idSorteo,
			                         preventas_taquilla, idAgente, idTaquilla, tipomonto_sorteo);
		}

		if (preventa == NULL) {
			result.push_back(bono);
			i++;
			renglon++;
		} else {
			modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
		}
	}
	return result;
}

/*-------------------------------------------------------------------------*/

boost::uint32_t
generCrcDelTicket(boost::uint16_t end, n_t_preventas_x_taquilla_t * preventas_taquilla,
                  std::set<boost::uint8_t>& idSorteos, boost::int32_t * montoPtr)
{
begin:
	boost::uint32_t van = 0;
	boost::int32_t monto = 0;
	boost::uint32_t sizeVal = 1024;
	char * buffer = new char[sizeVal];
	memset(buffer, 0, sizeVal);
	for (boost::uint16_t renglon = 1; renglon <= end; renglon++) {
		Preventa * preventa = (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
		if (preventa != NULL) {
			boost::uint8_t idSorteo = preventa->idSorteo;
			Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
			verificar_sorteo(NULL, sorteo, idSorteo, false);
			if (not sorteo->isEstadoActivo()) {
                std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(idSorteo);
				if (idSorteoPos == idSorteos.end()) {
                    idSorteos.insert(idSorteo);
					delete [] buffer;
					goto begin;
				}
			}
			if (preventa->monto_preventa > 0) {
				boost::uint32_t crc = preventa->idSorteo + preventa->numero + preventa->monto_preventa;
				if (sorteo->isEstadoActivo()) {
					monto += preventa->monto_preventa;
				}
                char ccrc[15];
				sprintf(ccrc, "%u%u", renglon, crc);
				log_mensaje (NivelLog::Detallado, "crc", "", ccrc);
				van += strlen(ccrc);
				if (van + 10 >= sizeVal) {
					char * aux = new char[sizeVal + 1024];
					memcpy(aux, buffer, sizeVal);
					delete [] buffer;
					sizeVal += 1024;
					buffer = aux;
				}
				strcat(buffer, ccrc);
			}
		}
	}
	boost::uint32_t crc = crc32(0, (Bytef *) buffer, van);
	delete [] buffer;
	*montoPtr = monto;
	return crc;
}

/*-------------------------------------------------------------------------*/

std::vector<boost::uint8_t>
liberarIdSorteosSet(std::set<boost::uint8_t>& idSorteos)
{
	boost::uint8_t idSorteo = 1;
    std::vector<boost::uint8_t> result(idSorteos.size(), 0);
	boost::uint8_t pos = 0;
	while (not idSorteos.empty()) {
        std::set<boost::uint8_t>::iterator idSorteoPos = idSorteos.find(idSorteo);
		if (idSorteoPos != idSorteos.end()) {
			result[pos] = idSorteo;
            idSorteos.erase(idSorteoPos);
            ++pos;
		}
		++idSorteo;
	}
	return result;
}

/*--------------------------------------------------------------------------------*/

void
tachar_ticket(ConexionActiva& conexionActiva, MYSQL * dbConnection, boost::uint32_t idTicket,
              ProtocoloExterno::AnulacionType anulacion)
{
    
    boost::uint8_t idSorteo = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Renglones WHERE IdTicket=%1% limit 1")
        % idTicket;
        ejecutar_sql(dbConnection, sqlQuery, DEBUG_ELIMINAR_TICKET);
        MYSQL_RES * result = mysql_store_result(dbConnection);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            idSorteo = atoi(row[0]);
        }
        mysql_free_result(result);
    }
    
    if (idSorteo == 0) {
            send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
            return;
    }    
    
    std::auto_ptr<ProtocoloExterno::AnularTicket> 
    anularExterno = ProtocoloExterno::MensajeFactory::getInstance()
                  . crearAnularTicket(dbConnection, idTicket, anulacion);
    bool puedeEliminar = false;            
    try {
        (*anularExterno)(dbConnection);
        puedeEliminar = true;
        
    } catch (ProtocoloExterno::ServiceNoFoundException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, imposible conectar con el servidor local de enlace: %s", 
                e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    } catch (ProtocoloExterno::ReponseException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error retornado por el servidor remoto: %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    } catch (ProtocoloExterno::InvalidArgumentException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error retornado por el servidor remoto: %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    } catch (...) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error desconocido en protocolo externo");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
    }
    
	// obtener el estado del ticket
	boost::uint8_t estadoTicket = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Estado FROM Tickets WHERE Id=%1%") % idTicket;
		ejecutar_sql(dbConnection, sqlQuery, DEBUG_TACHAR_TICKET);
		MYSQL_RES * result = mysql_store_result(dbConnection);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			estadoTicket = atoi(row[0]);
		}
		mysql_free_result(result);
	}   
                       
   boost::uint8_t nuevoEstadoTicket = (puedeEliminar) ? TACHADO : BANQUEADO;
    
	// actualizar el estado y la fecha del ticket
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Tickets SET Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
		% unsigned(nuevoEstadoTicket) % time(NULL) % idTicket;
		ejecutar_sql(dbConnection, sqlQuery, DEBUG_TACHAR_TICKET);
	}
    
	// verifica que el ticket tenga fecha de hoy
	bool esTicketDeHoy = false;
	{
		time_t now = time(NULL);
		struct tm timeNow = *localtime(&now);
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Fecha FROM Tickets WHERE "
                       "Fecha >= '%d-%02d-%02d 00:00:00' AND "
                       "Fecha <= '%d-%02d-%02d 23:59:59' AND "
                       "Id = %u")
		% (timeNow.tm_year + 1900) % (timeNow.tm_mon + 1) % timeNow.tm_mday 
        % (timeNow.tm_year + 1900) % (timeNow.tm_mon + 1) % timeNow.tm_mday
        % idTicket;
		ejecutar_sql(dbConnection, sqlQuery, DEBUG_TACHAR_TICKET);
		MYSQL_RES * result = mysql_store_result(dbConnection);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			esTicketDeHoy = true;
		}
		mysql_free_result(result);
	}
	if (esTicketDeHoy) {
		liberar_venta(conexionActiva, dbConnection, idTicket);
	}
}

//-------------------------------------------------------------------------------------------

boost::uint8_t
verificar_ultimo_ticket(ConexionActiva& conexionActiva, MYSQL * mysql, boost::uint32_t lastidticket)
{
	if (lastidticket == 0) {

		boost::uint32_t numeroDeTicketsDiferentes = 0;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT COUNT(Id) FROM Tickets WHERE IdAgente=%1% "
			                "AND NumTaquilla=%2% AND Estado <> %3%")
			% conexionActiva.idAgente % unsigned(conexionActiva.idTaquilla) % unsigned(TACHADO);
			ejecutar_sql(mysql, sqlQuery, DEBUG_VERIFICAR_ULTIMO_TICKET);
			MYSQL_RES * result = mysql_store_result(mysql);
			MYSQL_ROW row = mysql_fetch_row(result);
			if (NULL != row) {
				numeroDeTicketsDiferentes = atoi(row[0]);
			}
			mysql_free_result(result);
		}

		switch (numeroDeTicketsDiferentes) {
			case 0:
				break;

			case 1: {
					boost::uint32_t idTicket = 0;
					{
						boost::format sqlQuery;
						sqlQuery.parse("SELECT Id FROM Tickets "
						                "WHERE IdAgente=%1% AND NumTaquilla=%2% AND Estado <> %3%")
						% conexionActiva.idAgente % unsigned(conexionActiva.idTaquilla)
						% unsigned(TACHADO);
						ejecutar_sql(mysql, sqlQuery, DEBUG_VERIFICAR_ULTIMO_TICKET);
						MYSQL_RES * result = mysql_store_result(mysql);
						MYSQL_ROW row = mysql_fetch_row(result);
						if (NULL != row) {
							idTicket = atoi(row[0]);
						}
						mysql_free_result(result);
					}
					tachar_ticket(conexionActiva, mysql, idTicket, ProtocoloExterno::AnulacionErrorComunicacion);
				}
				break;

			default: {
					char mensaje[BUFFER_SIZE];
					mensajes(conexionActiva, ERR_TICKETINCONSS);
					sprintf(mensaje,
					         "INCONSISTENCIA DE TICKETS: "
					         "El servidor tiene %d tickets mas que la Taquilla.",
					         numeroDeTicketsDiferentes);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return false;
				}
		}
	} else {

		std::vector<boost::uint32_t> idTickets;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT Id FROM Tickets WHERE IdAgente=%1% "
			                "AND NumTaquilla=%2% AND Estado <> %3% AND Id >=%4% ORDER BY Id DESC LIMIT 2")
			% conexionActiva.idAgente % unsigned(conexionActiva.idTaquilla) % unsigned(TACHADO)
			% lastidticket;
			ejecutar_sql(mysql, sqlQuery, DEBUG_VERIFICAR_ULTIMO_TICKET);
			MYSQL_RES * result = mysql_store_result(mysql);
			for (MYSQL_ROW row = mysql_fetch_row(result);
			        NULL != row;
			        row = mysql_fetch_row(result)) {
				idTickets.push_back(atoi(row[0]));
			}
			mysql_free_result(result);
		}

		if (idTickets.size() < 1) {
			mensajes(conexionActiva, ERR_TICKETINCONSA);
			return false;
		}

		std::vector<boost::uint32_t>::const_iterator idTicketsIter = idTickets.begin();

		boost::uint32_t numticket, idTicket;
		numticket = idTicket = *idTicketsIter;

		if (lastidticket != numticket) {
			++idTicketsIter;
			if (idTicketsIter == idTickets.end()) {
				char mensaje[BUFFER_SIZE];
				mensajes(conexionActiva, ERR_TICKETINCONSS);
				sprintf(mensaje,
				         "INCONSISTENCIA DE TICKETS: La Agencia no tiene tipo de montos.");
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				return false;
			}

			numticket = *idTicketsIter;
			if (lastidticket != numticket) {
				mensajes(conexionActiva, ERR_TICKETINCONSS);
				return false;
			}

			tachar_ticket(conexionActiva, mysql, idTicket, ProtocoloExterno::AnulacionErrorComunicacion);
		}
	}
	return true;
}

struct ResumenRenglones 
{
	boost::uint8_t idSorteo;
    boost::uint8_t tipo;
    boost::uint32_t monto;
};


class RealizarVentaExterna
{
    std::string serial;
    boost::uint8_t idProtocolo;
    ConexionActiva& conexionActiva;
    MYSQL * mysql;
    ProtocoloExterno::Ticket& ticket;
    
    int errorCode;
    std::string errorMessage;
    std::vector<RenglonModificado> renglonesModificados;
    
public:
    int getErrorCode() const { return errorCode; }
    std::string getErrorMessage() const { return errorMessage; }
    
    std::string getSerial() { return serial; }
    boost::uint8_t getIdProtocolo() const { return idProtocolo; }
    
    RealizarVentaExterna(ConexionActiva& conexionActiva_, 
                         MYSQL * mysql_, 
                         ProtocoloExterno::Ticket& ticket_)
        : serial(), idProtocolo(0), conexionActiva(conexionActiva_),  mysql(mysql_), ticket(ticket_)
        , errorCode(0), errorMessage(), renglonesModificados()
    {}
    
    
    std::vector<RenglonModificado> getRenglonesModificados() const { return renglonesModificados; }
    
    bool operator()() {
        try {        
            boost::format sqlQuery;
            sqlQuery.parse("select Tipo, IdSorteo, Numero, Monto, Id from Renglones "
                           "where IdTicket = %1% order by Id") % ticket.getIdTicket();
                           
            ejecutar_sql(mysql, sqlQuery, DEBUG_realizarVentaExterna);
            MYSQL_RES * result = mysql_store_result(mysql);
            for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
                ticket.add(ProtocoloExterno::Renglon(atoi(row[4]),
                                                     atoi(row[0]), 
                                                     atoi(row[1]), 
                                                     atoi(row[2]), 
                                                     strtoul(row[3], NULL, 10)));
            }
            mysql_free_result(result);
             
            std::auto_ptr<ProtocoloExterno::VenderTicket> 
            venderTicket = ProtocoloExterno::MensajeFactory::getInstance()
                         . crearVenderTicket(mysql, ticket, conexionActiva.usarProductos);

            if (!venderTicket->validar()) {
                char mensaje[BUFFER_SIZE];
                sprintf(mensaje, "Error en realizarVentaExterna: ticket no valido");
                log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
                return false;
            }
     
            serial = (*venderTicket)(mysql);
            idProtocolo = venderTicket->getIdProtocolo();
            
            typedef ProtocoloExterno::VenderTicket::VectorRenglones Renglones;
            Renglones const renglones = venderTicket->getRenglonesModificados();
            
            for (Renglones::const_iterator renglon = renglones.begin();
                 renglon != renglones.end(); ++renglon) {
             
                renglonesModificados.push_back(RenglonModificado(renglon->getId(), renglon->getMonto()));
            }
            
            return true;
            
        } catch (ProtocoloExterno::ReponseException const& e) {
            errorCode = e.getCode();
            errorMessage = e.what();
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error en realizarVentaExterna (%i): %s", e.getCode(), e.what());
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            return false;
        } catch (std::runtime_error const& e) {
            errorCode = -1;
            errorMessage = e.what();
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error en realizarVentaExterna: %s", e.what());
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            return false;
        } catch (ProtocoloExterno::InvalidArgumentException const& e) {
            errorCode = -1;
            errorMessage = e.what();
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error en realizarVentaExterna: %s", e.what());
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);   
            return false;         
        } catch (...) {
            errorCode = -1;
            errorMessage = "Error desconocido";
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error en realizarVentaExterna: error desconocido");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            return false;
        }
    }
};

std::string
formatNumeroJugado(boost::uint16_t jugada, boost::uint8_t digits)
{
    std::ostringstream jugadaOss;
    jugadaOss << jugada;
    std::string jugadaStr = jugadaOss.str();
    if (jugadaStr.length() > digits) {
        std::size_t const delta = jugadaStr.length() - digits;
        jugadaStr.assign(jugadaStr.c_str() + delta); 
    }
    
    std::ostringstream resultOss;
    resultOss.width(digits);
    resultOss.fill('0');
    resultOss << jugadaStr;
    return resultOss.str();
}

std::string
numeroJugado(boost::uint16_t jugada, boost::uint8_t tipoRenglon)
{
    switch (tipoRenglon) {
        case TERMINAL:       return formatNumeroJugado(jugada, 2);
        
        case BONO:
        case TRIPLE:         return formatNumeroJugado(jugada, 3);
        
        case TERMINALAZO:    return formatNumeroJugado(jugada, 2);
        
        case BONOTRIPLETAZO:
        case TRIPLETAZO:     return formatNumeroJugado(jugada, 3);
        
        default: throw std::runtime_error("numeroJugado(): tipoRenglon no valido");
    }    
} 

boost::uint32_t
getIdSimbolo(MYSQL* mysql, boost::uint16_t jugada, boost::uint8_t idSorteo, boost::uint8_t digits)
{    
    std::ostringstream oss;
    oss.width(digits);
    oss.fill('0');
    oss << jugada;
    std::string jugadaStr = oss.str();
    std::string simbolo(jugadaStr.begin(), jugadaStr.begin() + 2); 
    
    boost::format sqlQuery;
    sqlQuery.parse("select Simbolos.id from Simbolos inner join Sorteos on "
                   "Simbolos.tipo = Sorteos.tipoJugada "
                   "where Simbolos.valor = %1% and Sorteos.id = %2%") 
    % simbolo % unsigned(idSorteo);
                   
    ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    MYSQL_RES * result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (NULL == row) {
        throw std::runtime_error("getIdSimbolo(): simbolo no registrado");
    }
    boost::uint32_t id = strtoul(row[0], NULL, 10);    
    mysql_free_result(result);
    return id;    
}

boost::uint32_t
idSimbolo(MYSQL* mysql, boost::uint16_t jugada, boost::uint8_t tipoRenglon, boost::uint8_t idSorteo)
{
    switch (tipoRenglon) {
        case TERMINAL:        
        case BONO:
        case TRIPLE:         return 1;
        
        case TERMINALAZO:    return getIdSimbolo(mysql, jugada, idSorteo, 4);
        
        case BONOTRIPLETAZO:
        case TRIPLETAZO:     return getIdSimbolo(mysql, jugada, idSorteo, 5);
        
        default: throw std::runtime_error("idSimbolo(): tipoRenglon no valido");
    }    
}

bool
guardarRenglonesRegulares(MYSQL * mysql,
                          boost::uint32_t idTicket,
                          GArray const* renglonesDeVenta,
                          boost::uint32_t numeroDeRenglones,
                          std::set<boost::uint8_t> const& idSorteos,
                          boost::uint8_t& idSorteoBono, 
                          GArray *& renglonesDeResumen,
                          boost::uint16_t& guardados)
{
    try {
        std::ostringstream sqlRenglones;
        sqlRenglones << "insert into Renglones values ";
        std::ostringstream sqlRenglonesJugadas;
        sqlRenglonesJugadas << "insert into RenglonesJugadas values ";
        boost::uint16_t cuantos = 0;
        guardados = 0;
        idSorteoBono = 0;
        renglonesDeResumen = g_array_new(false, false, sizeof(ResumenRenglones));
        typedef std::map<boost::uint16_t, boost::uint16_t> IdRenglon_Renglon_Type;
        IdRenglon_Renglon_Type idRenglonPorRenglon;
        for (boost::uint32_t renglon = 0; renglon < renglonesDeVenta->len; renglon++) {
            Preventa preventa = g_array_index(renglonesDeVenta, Preventa, renglon);
            boost::uint8_t idSorteo = preventa.idSorteo;
            std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(preventa.idSorteo);
            if (preventa.monto_preventa > 0 and idSorteoPos == idSorteos.end()) {
                ResumenRenglones resumen_renglones;
                Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
                if (sorteo->isConSigno()) {
                    idSorteoBono = idSorteo;
                }
                if (idSorteoBono == 0 and renglon == renglonesDeVenta->len -1) {
                    idSorteoBono = idSorteo;
                }
                bool encontrado = false;
                for (boost::uint32_t r = 0; r < renglonesDeResumen->len and not encontrado; r++) {
                    resumen_renglones = g_array_index(renglonesDeResumen, ResumenRenglones, r);
                    if (resumen_renglones.idSorteo == preventa.idSorteo
                            and resumen_renglones.tipo == preventa.tipo) {
                        encontrado = true;
                        resumen_renglones.monto += preventa.monto_preventa;
                        g_array_index(renglonesDeResumen, ResumenRenglones, r) =
                            resumen_renglones;
                    }
                }
                if (not encontrado) {
                    resumen_renglones.idSorteo = preventa.idSorteo;
                    resumen_renglones.tipo = preventa.tipo;
                    resumen_renglones.monto = preventa.monto_preventa;
                    g_array_append_val(renglonesDeResumen, resumen_renglones);
                }
                guardados++;
                idRenglonPorRenglon.insert(std::make_pair(guardados, preventa.renglon));
                sqlRenglones << "(" << guardados << "," << preventa.numero << "," 
                             << preventa.monto_preventa << "," << unsigned(preventa.tipo) << "," 
                             << idTicket << "," << unsigned(preventa.idSorteo) << "," << "0)";
                             
                sqlRenglonesJugadas << "(" << guardados << "," << idTicket << "," 
                                    << numeroJugado(preventa.numero, preventa.tipo) << "," 
                                    << idSimbolo(mysql, preventa.numero, preventa.tipo, preventa.idSorteo) 
                                    << ")";
                
                cuantos++;
                if (cuantos == 100) {
                    ejecutar_sql(mysql, sqlRenglones.str().c_str(), DEBUG_PROCESAR_TICKET);
                    ejecutar_sql(mysql, sqlRenglonesJugadas.str().c_str(), DEBUG_PROCESAR_TICKET);
                    sqlRenglones.str("");
                    sqlRenglonesJugadas.str("");
                    sqlRenglones << "insert into Renglones values ";
                    sqlRenglonesJugadas << "insert into RenglonesJugadas values ";
                    cuantos = 0;
                } else if (renglon < numeroDeRenglones && cuantos < 100 && cuantos > 0) {
                    sqlRenglones << ",";
                    sqlRenglonesJugadas << ",";
                }
            }
        }
        if (cuantos > 0) {
            std::string sqlRenglonesStr = sqlRenglones.str();
            if (sqlRenglonesStr[sqlRenglonesStr.length() - 1] == ',') {
                sqlRenglonesStr.resize(sqlRenglonesStr.length() - 1);
            }
            ejecutar_sql(mysql, sqlRenglonesStr.c_str(), DEBUG_PROCESAR_TICKET);
                    
            std::string sqlRenglonesJugadasStr = sqlRenglonesJugadas.str();
            if (sqlRenglonesJugadasStr[sqlRenglonesJugadasStr.length() - 1] == ',') {
                sqlRenglonesJugadasStr.resize(sqlRenglonesJugadasStr.length() - 1);
            }
            ejecutar_sql(mysql, sqlRenglonesJugadasStr.c_str(), DEBUG_PROCESAR_TICKET);
        }
        return true;
    } catch (std::runtime_error const&) {
        return false;
    }
}

template<class GuardarTicketT>
bool
guardarRenglonesBonos(MYSQL * mysql,
                      boost::uint32_t idTicket,
                      GuardarTicketT const& guardarTicket,                          
                      boost::uint8_t idSorteo,
                      Sorteo const* sorteo,
                      std::vector<boost::uint16_t> const& bonos,
                      GArray* renglonesDeResumen,
                      boost::uint16_t& guardados)
{
    try {
        for (int i = 0; i < guardarTicket.numeroDeBonos; i++) {
            ResumenRenglones resumen_renglones;
    
            bool encontrado = false;
            for (boost::uint32_t r = 0; r < renglonesDeResumen->len and not encontrado; r++) {
                resumen_renglones = g_array_index(renglonesDeResumen, ResumenRenglones, r);
                if (resumen_renglones.idSorteo == idSorteo
                        and (resumen_renglones.tipo == BONO
                              or resumen_renglones.tipo == BONOTRIPLETAZO)) {
                    encontrado = true;
                    resumen_renglones.monto += guardarTicket.montoBono;
                    g_array_index(renglonesDeResumen, ResumenRenglones, r) =
                        resumen_renglones;
                }
            }
            if (not encontrado) {
                resumen_renglones.idSorteo = idSorteo;
                resumen_renglones.tipo = ((not sorteo->isConSigno()))
                                         ? BONO
                                         : BONOTRIPLETAZO;
                resumen_renglones.monto = guardarTicket.montoBono;
                g_array_append_val(renglonesDeResumen, resumen_renglones);
            }
            guardados++;
            {
                boost::uint8_t tipoRenglon = (not sorteo->isConSigno())
                                           ? BONO
                                           : BONOTRIPLETAZO;
                
                boost::format sqlRenglones;
                sqlRenglones.parse("insert into Renglones values (%1%, %2%, %3%, %4%, %5%, %6%, 1)")
                % guardados % bonos[i] % guardarTicket.montoBono
                % unsigned(tipoRenglon) % idTicket % unsigned(idSorteo);
                ejecutar_sql(mysql, sqlRenglones, DEBUG_PROCESAR_TICKET);
                
                boost::format sqlRenglonesJugadas;
                sqlRenglonesJugadas.parse("insert into RenglonesJugadas values(%1%, %2%, %3%, %4%)")
                % guardados % idTicket 
                % numeroJugado(bonos[i], tipoRenglon)
                % idSimbolo(mysql, bonos[i], tipoRenglon, idSorteo);
                ejecutar_sql(mysql, sqlRenglonesJugadas, DEBUG_PROCESAR_TICKET);
            }
        }
        return true;
    } catch (std::runtime_error const&) {
        return false;
    }
}
    
bool
atenderPeticionGuardarTicket_V_2(ConexionActiva& conexionActiva, boost::uint8_t * buff, MYSQL * mysql,
                                 bool manejarTransaccion)
{
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Guardando el Ticket");
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    GuardarTicketV2 guardarTicket = GuardarTicketV2(buff);
    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
    boost::uint32_t idUsuario = conexionActiva.idUsuario;
    boost::uint64_t agente_taquilla = idAgente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += conexionActiva.idTaquilla;
    n_t_preventas_x_taquilla_t * preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    preventas_taquilla->hora_ultimo_renglon = time(NULL);
    preventas_taquilla->hora_ultimo_peticion = time(NULL);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    boost::uint32_t numeroDeRenglones = g_tree_nnodes(preventas_taquilla->preventas);

    if (numeroDeRenglones == 0) {                            /*Ya fue guardado el ticket */
        confirmacion_guardados_V_2(conexionActiva, idAgente, idTaquilla, mysql);
        return false;
    }
    
    if (numeroDeRenglones > guardarTicket.numeroDeRenglones) {
        for (boost::uint32_t renglon = guardarTicket.numeroDeRenglones + 1; renglon <= numeroDeRenglones; ++renglon) {
            modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
        }
        numeroDeRenglones = guardarTicket.numeroDeRenglones;
    }
    
    if (numeroDeRenglones != guardarTicket.numeroDeRenglones) {
        /* abortar ticket */
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error Nro reglones Diferentes servidor %u, cliente %u", numeroDeRenglones, 
                    guardarTicket.numeroDeRenglones);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    std::set<boost::uint8_t> idSorteos;
    boost::int32_t monto = 0;
    boost::uint32_t crc = generCrcDelTicket(guardarTicket.numeroDeRenglones, preventas_taquilla, idSorteos, &monto);
    if (crc != guardarTicket.crc) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error CRC Diferentes <%u>/<%u>", crc, guardarTicket.crc);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }

    if (verificar_ultimo_ticket(conexionActiva, mysql, guardarTicket.ultimoTicket) == false) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        return false;
    }

    GArray * renglonesDePreventa = g_array_new(false, false, sizeof(Preventa));
    for (boost::uint32_t renglon = 1; renglon <= guardarTicket.numeroDeRenglones; ++renglon) {
        Preventa * ppreventa =
            (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
        Preventa preventa = *ppreventa;
        preventa.renglon = renglon;
        g_array_append_val(renglonesDePreventa, preventa);
    }
    GArray * renglonesDeVenta = g_array_new(false, false, sizeof(Preventa));
    {
        boost::uint32_t renglon = 0;
        while (renglonesDePreventa->len > 0) {
            Preventa preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
            boost::uint8_t idSorteo = preventa.idSorteo;
            while (renglon < renglonesDePreventa->len) {
                preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
                if (idSorteo == preventa.idSorteo) {
                    g_array_append_val(renglonesDeVenta, preventa);
                    g_array_remove_index(renglonesDePreventa, renglon);
                } else {
                    renglon++;
                }
            }
            renglon = 0;
        }
    }
    boost::uint16_t cuantos = 0;
    for (boost::uint32_t renglon = 0; renglon < renglonesDeVenta->len; renglon++) {
        Preventa preventa = g_array_index(renglonesDeVenta, Preventa, renglon);
        std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(preventa.idSorteo);
        if (preventa.monto_preventa > 0 and idSorteoPos == idSorteos.end()) {
            cuantos++;
        }
    }
    if (cuantos == 0) {                      
        /*No Hay Renglones para vender */
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        atenderPeticionCancelarPreventa(conexionActiva);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        g_array_free(renglonesDeVenta, true);
        g_array_free(renglonesDePreventa, true);
        return false;
    }

    boost::uint16_t estado = NORMAL;
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "BEGIN", DEBUG_PROCESAR_TICKET);
    }
    boost::uint32_t idTicket;
    {
        std::string nombreApostador;
        std::string ciApostador;
        nombreApostador = guardarTicket.nombreApostador.getValue();
        if (guardarTicket.ciApostador != 0) {
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "%u", guardarTicket.ciApostador);
            ciApostador = buffer;
        } else {
            ciApostador = "0";
        }
        
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Tickets "
                       "VALUES(NULL, FROM_UNIXTIME(%1%), %2%, 0, %3%, %4%, %5%, %6%, NULL, '%7%', '%8%', 0, 0, 1)")
        % time(NULL) % monto % unsigned(estado) % idAgente % idUsuario % unsigned(idTaquilla)
        % nombreApostador % ciApostador;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        idTicket = mysql_insert_id(mysql);
    }    
    boost::int32_t fechaUnix;
    char fecha[BUFFER_SIZE];
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Fecha, UNIX_TIMESTAMP(Fecha) FROM Tickets WHERE Id=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        strcpy(fecha, row[0]);
        fechaUnix = atoi(row[1]);
        mysql_free_result(result);
    }
    
    boost::uint8_t idSorteoBono = 0;
    GArray * renglonesDeResumen = NULL;
    boost::uint16_t guardados = 0;     
    bool ok = guardarRenglonesRegulares(mysql,
                                        idTicket,
                                        renglonesDeVenta, 
                                        numeroDeRenglones,
                                        idSorteos, 
                                        idSorteoBono, 
                                        renglonesDeResumen,
                                        guardados);
    g_array_free(renglonesDeVenta, true);
    g_array_free(renglonesDePreventa, true);
    if (not ok) {
        /* abortar ticket */
        log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesRegulares()");
        g_array_free(renglonesDeResumen, true);
        liberar_venta(conexionActiva, mysql, idTicket);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        if (manejarTransaccion == true) {
            ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    liberar_preventa(conexionActiva);
    boost::uint8_t idSorteo;
    std::vector<boost::uint16_t> bonos;
    boost::uint8_t tipoJugadaBono;
    if (guardarTicket.numeroDeBonos > 0) {                      /*Generar los bonos */
        idSorteo = idSorteoBono;
        {
            std::string mensaje("Sorteo de bonos IdSorteo: ");
            mensaje += boost::lexical_cast<std::string>(unsigned(idSorteo));
            log_clientes(NivelLog::Detallado, &conexionActiva, mensaje.c_str());
        }
        
        Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
        boost::uint32_t numeroRenglon = guardarTicket.numeroDeRenglones + 1;
        bonos = generar_bonos(conexionActiva, idSorteo, guardarTicket.numeroDeBonos,
                              conexionActiva.idZona, idAgente, guardarTicket.montoBono,
                              numeroRenglon, idTaquilla, preventas_taquilla,
                              &tipoJugadaBono);
        
        if (bonos.empty()) {
            /* abortar ticket */
            std::string mensaje("Agencia ");
            mensaje += boost::lexical_cast<std::string>(idAgente) 
                    + " con idSorteo " 
                    + boost::lexical_cast<std::string>(unsigned(idSorteo))
                    + " mal configurada";
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje.c_str());
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }
                                         
        numeroRenglon = guardarTicket.numeroDeRenglones + 1;        
        bool ok = guardarRenglonesBonos(mysql,
                                        idTicket,
                                        guardarTicket,
                                        idSorteo,
                                        sorteo,
                                        bonos,
                                        renglonesDeResumen,
                                        guardados);
        if (not ok) {
            /* abortar ticket */
            log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesBonos()");
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }                                        
    }
    
    for (boost::uint32_t r = 0; r < renglonesDeResumen->len; r++) {
        ResumenRenglones resumen_renglones =
            g_array_index(renglonesDeResumen, ResumenRenglones, r);
        {
            boost::format sqlQuery;
            sqlQuery.parse("INSERT INTO Resumen_Renglones VALUES(%1%, %2%, %3%, %4%)")
            % idTicket 
            % unsigned(resumen_renglones.idSorteo)
            % unsigned(resumen_renglones.tipo)
            % resumen_renglones.monto;
            ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        }
    }
    
    g_array_free(renglonesDeResumen, true);
    
    std::vector<boost::uint8_t> sorteos = liberarIdSorteosSet(idSorteos);

    for (int i = 0, size = sorteos.size(); i < size; ++i) {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Info_Ticket VALUES(%u, %u)")
        % idTicket % unsigned(sorteos[i]);
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    }
    
    ProtocoloExterno::Ticket ticket(idAgente, idTaquilla, guardarTicket.ciApostador, idTicket);
    RealizarVentaExterna ventaExterna(conexionActiva, mysql, ticket);
    if (!ventaExterna()) {
        liberar_venta(conexionActiva, mysql, idTicket);
        atenderPeticionCancelarPreventa(conexionActiva);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    boost::uint32_t correlativo = 0;
    {
        boost::format sqlQuery1;
        sqlQuery1.parse("SELECT SiguienteCorrelativo FROM CorrelativoTickets WHERE IdAgente=%1% FOR UPDATE")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery1, DEBUG_PROCESAR_TICKET);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL || row[0] == NULL) {                        
            liberar_venta(conexionActiva, mysql, idTicket);
            atenderPeticionCancelarPreventa(conexionActiva);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }
        correlativo = strtoul(row[0], NULL, 10);        
        mysql_free_result(result);
        boost::format sqlQuery2;
        sqlQuery2.parse("UPDATE CorrelativoTickets SET SiguienteCorrelativo = SiguienteCorrelativo + 1 "
                        "WHERE IdAgente=%1%")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery2, DEBUG_PROCESAR_TICKET);
    }
    std::string serial = ventaExterna.getSerial();
    {
        boost::uint32_t montoBono = guardarTicket.montoBono * bonos.size();
        unsigned protocolo = ventaExterna.getIdProtocolo();
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET "
                       "Serial='%1%', MontoBono=%2%, Correlativo=%3%, IdProtocolo=%4% "
                       "WHERE Id=%5%")
        % serial % montoBono % correlativo % protocolo % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    }

    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "COMMIT", DEBUG_PROCESAR_TICKET);
    }
        
    enviar_ticket_guardado_V_2(conexionActiva, 
                            fechaUnix, 
                            idTicket, 
                            serial.c_str(), 
                            monto,
                            bonos, 
                            sorteos,
                            idSorteo, 
                            correlativo);
                            
    liberar_preventa(conexionActiva);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void
corregirTicket(MYSQL * /*mysql*/, boost::uint32_t /*idTicket*/, std::vector<RenglonModificado> const& /*renglonesModificados*/)
{
    // TODO
    throw std::runtime_error("corregirTicket no implementado");
}

bool
atenderPeticionGuardarTicket_V_3(ConexionActiva& conexionActiva, boost::uint8_t * buff, MYSQL * mysql,
                                 bool manejarTransaccion)
{
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Guardando el Ticket");
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    GuardarTicketV2 guardarTicket = GuardarTicketV2(buff);
    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
    boost::uint32_t idUsuario = conexionActiva.idUsuario;
    boost::uint64_t agente_taquilla = idAgente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += conexionActiva.idTaquilla;
    n_t_preventas_x_taquilla_t * preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    preventas_taquilla->hora_ultimo_renglon = time(NULL);
    preventas_taquilla->hora_ultimo_peticion = time(NULL);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    boost::uint32_t numeroDeRenglones = g_tree_nnodes(preventas_taquilla->preventas);

    if (numeroDeRenglones == 0) {                            /*Ya fue guardado el ticket */
        confirmacion_guardados_V_3(conexionActiva, idAgente, idTaquilla, mysql);
        return false;
    }
    
    if (numeroDeRenglones > guardarTicket.numeroDeRenglones) {
        for (boost::uint32_t renglon = guardarTicket.numeroDeRenglones + 1; renglon <= numeroDeRenglones; ++renglon) {
            modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
        }
        numeroDeRenglones = guardarTicket.numeroDeRenglones;
    }
    
    if (numeroDeRenglones != guardarTicket.numeroDeRenglones) {
        /* abortar ticket */
        send2cliente(conexionActiva, ErrorException(ERR_TICKETCRCDIFERENTE, 
                                                    "Ticket diferente al del servidor"));
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error Nro reglones Diferentes servidor %u, cliente %u", numeroDeRenglones, 
                    guardarTicket.numeroDeRenglones);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    std::set<boost::uint8_t> idSorteos;
    boost::int32_t monto = 0;
    boost::uint32_t crc = generCrcDelTicket(guardarTicket.numeroDeRenglones, preventas_taquilla, idSorteos, &monto);
    if (crc != guardarTicket.crc) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        send2cliente(conexionActiva, ErrorException(ERR_TICKETCRCDIFERENTE, 
                                                    "Error de crc en el Ticket"));
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error CRC Diferentes <%u>/<%u>", crc, guardarTicket.crc);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
    /*Guargar el ticket al disco */

    if (verificar_ultimo_ticket(conexionActiva, mysql, guardarTicket.ultimoTicket) == false) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        return false;
    }

    GArray * renglonesDePreventa = g_array_new(false, false, sizeof(Preventa));
    for (boost::uint32_t renglon = 1; renglon <= guardarTicket.numeroDeRenglones; ++renglon) {
        Preventa * ppreventa =
            (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
        Preventa preventa = *ppreventa;
        preventa.renglon = renglon;
        g_array_append_val(renglonesDePreventa, preventa);
    }
    GArray * renglonesDeVenta = g_array_new(false, false, sizeof(Preventa));
    {
        boost::uint32_t renglon = 0;
        while (renglonesDePreventa->len > 0) {
            Preventa preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
            boost::uint8_t idSorteo = preventa.idSorteo;
            while (renglon < renglonesDePreventa->len) {
                preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
                if (idSorteo == preventa.idSorteo) {
                    g_array_append_val(renglonesDeVenta, preventa);
                    g_array_remove_index(renglonesDePreventa, renglon);
                } else {
                    renglon++;
                }
            }
            renglon = 0;
        }
    }
    boost::uint16_t cuantos = 0;
    for (boost::uint32_t renglon = 0; renglon < renglonesDeVenta->len; renglon++) {
        Preventa preventa = g_array_index(renglonesDeVenta, Preventa, renglon);
        std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(preventa.idSorteo);
        if (preventa.monto_preventa > 0 and idSorteoPos == idSorteos.end()) {
            cuantos++;
        }
    }
    if (cuantos == 0) {                      
        /*No Hay Renglones para vender */
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        atenderPeticionCancelarPreventa(conexionActiva);
        send2cliente(conexionActiva, ErrorException(ERR_TICKETCRCDIFERENTE, 
                                                    "No hay renglones para vender"));
        g_array_free(renglonesDeVenta, true);
        g_array_free(renglonesDePreventa, true);
        return false;
    }

    boost::uint16_t estado = NORMAL;
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "BEGIN", DEBUG_PROCESAR_TICKET);
    }
    boost::uint32_t idTicket;
    {
        std::string nombreApostador;
        std::string ciApostador;
        nombreApostador = guardarTicket.nombreApostador.getValue();
        if (guardarTicket.ciApostador != 0) {
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "%u", guardarTicket.ciApostador);
            ciApostador = buffer;
        } else {
            ciApostador = "0";
        }
        
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Tickets "
                       "VALUES(NULL, FROM_UNIXTIME(%1%), %2%, 0, %3%, %4%, %5%, %6%, NULL, '%7%', '%8%', 0, 0, 1)")
        % time(NULL) % monto % unsigned(estado) % idAgente % idUsuario % unsigned(idTaquilla)
        % nombreApostador % ciApostador;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        idTicket = mysql_insert_id(mysql);
    }
    boost::int32_t fechaUnix;
    char fecha[BUFFER_SIZE];
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Fecha, UNIX_TIMESTAMP(Fecha) FROM Tickets WHERE Id=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        strcpy(fecha, row[0]);
        fechaUnix = atoi(row[1]);
        mysql_free_result(result);
    }

    boost::uint8_t idSorteoBono = 0;
    GArray * renglonesDeResumen = NULL;
    boost::uint16_t guardados = 0;     
    bool ok = guardarRenglonesRegulares(mysql,
                                        idTicket,
                                        renglonesDeVenta, 
                                        numeroDeRenglones,
                                        idSorteos, 
                                        idSorteoBono, 
                                        renglonesDeResumen,
                                        guardados);
    g_array_free(renglonesDeVenta, true);
    g_array_free(renglonesDePreventa, true);
    if (not ok) {
        /* abortar ticket */
        log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesRegulares()");
        g_array_free(renglonesDeResumen, true);
        liberar_venta(conexionActiva, mysql, idTicket);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        if (manejarTransaccion == true) {
            ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    liberar_preventa(conexionActiva);
    boost::uint8_t idSorteo;
    std::vector<boost::uint16_t> bonos;
    boost::uint8_t tipoJugadaBono;
    if (guardarTicket.numeroDeBonos > 0) {                      /*Generar los bonos */
        idSorteo = idSorteoBono;
        {
            std::string mensaje("Sorteo de bonos IdSorteo: ");
            mensaje += boost::lexical_cast<std::string>(unsigned(idSorteo));
            log_clientes(NivelLog::Detallado, &conexionActiva, mensaje.c_str());
        }
        
        Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
        boost::uint32_t numeroRenglon = guardarTicket.numeroDeRenglones + 1;
        bonos = generar_bonos(conexionActiva, idSorteo, guardarTicket.numeroDeBonos,
                              conexionActiva.idZona, idAgente, guardarTicket.montoBono,
                              numeroRenglon, idTaquilla, preventas_taquilla,
                              &tipoJugadaBono);
        
        if (bonos.empty()) {
            /* abortar ticket */
            std::string mensaje("Agencia ");
            mensaje += boost::lexical_cast<std::string>(idAgente) 
                    + " con idSorteo " 
                    + boost::lexical_cast<std::string>(unsigned(idSorteo))
                    + " mal configurada";
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje.c_str());
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETCRCDIFERENTE, 
                                                    "No se pudo generar los bonos"));
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }
                                         
        numeroRenglon = guardarTicket.numeroDeRenglones + 1;        
        bool ok = guardarRenglonesBonos(mysql,
                                        idTicket,
                                        guardarTicket,
                                        idSorteo,
                                        sorteo,
                                        bonos,
                                        renglonesDeResumen,
                                        guardados);
        if (not ok) {
            /* abortar ticket */
            log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesBonos()");
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }   
    }        
    
    for (boost::uint32_t r = 0; r < renglonesDeResumen->len; r++) {
        ResumenRenglones resumen_renglones =
            g_array_index(renglonesDeResumen, ResumenRenglones, r);
        {
            boost::format sqlQuery;
            sqlQuery.parse("INSERT INTO Resumen_Renglones VALUES(%1%, %2%, %3%, %4%)")
            % idTicket 
            % unsigned(resumen_renglones.idSorteo)
            % unsigned(resumen_renglones.tipo)
            % resumen_renglones.monto;
            ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        }
    }    

    g_array_free(renglonesDeResumen, true);
    std::vector<boost::uint8_t> sorteos(idSorteos.begin(), idSorteos.end());
    
    for (std::set<boost::uint8_t>::const_iterator sorteo = idSorteos.begin();
         sorteo != idSorteos.end(); ++sorteo) {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Info_Ticket VALUES(%u, %u)")
        % idTicket % unsigned(*sorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    }
    ProtocoloExterno::Ticket ticket(idAgente, idTaquilla, guardarTicket.ciApostador, idTicket);
    RealizarVentaExterna ventaExterna(conexionActiva, mysql, ticket);

    if (!ventaExterna()) {
        liberar_venta(conexionActiva, mysql, idTicket);
        atenderPeticionCancelarPreventa(conexionActiva);
        send2cliente(conexionActiva, ErrorException(ventaExterna.getErrorCode(), 
                                                    ventaExterna.getErrorMessage()));
                                              
        if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    
    std::vector<RenglonModificado> renglonesModificados = ventaExterna.getRenglonesModificados();
    if (!renglonesModificados.empty()) {
        corregirTicket(mysql, idTicket, renglonesModificados);
    }
    boost::uint32_t correlativo = 0;
    {
        boost::format sqlQuery1;
        sqlQuery1.parse("SELECT SiguienteCorrelativo FROM CorrelativoTickets WHERE IdAgente=%1% FOR UPDATE")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery1, DEBUG_PROCESAR_TICKET);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL || row[0] == NULL) {                        
            liberar_venta(conexionActiva, mysql, idTicket);
            atenderPeticionCancelarPreventa(conexionActiva);            
            send2cliente(conexionActiva, ErrorException(ERR_TICKETCRCDIFERENTE, 
                                                        "Error en generación de correlativos"));
            
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }
        correlativo = strtoul(row[0], NULL, 10);        
        mysql_free_result(result);
        boost::format sqlQuery2;
        sqlQuery2.parse("UPDATE CorrelativoTickets SET SiguienteCorrelativo = SiguienteCorrelativo + 1 "
                        "WHERE IdAgente=%1%")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery2, DEBUG_PROCESAR_TICKET);
    }

    std::string serial = ventaExterna.getSerial();

    {
        boost::uint32_t montoBono = guardarTicket.montoBono * bonos.size();
        unsigned protocolo = ventaExterna.getIdProtocolo();
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET "
                       "Serial='%1%', MontoBono=%2%, Correlativo=%3%, IdProtocolo=%4% "
                       "WHERE Id=%5%")
        % serial % montoBono % correlativo % protocolo % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    }
    
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "COMMIT", DEBUG_PROCESAR_TICKET);
    }

    enviar_ticket_guardado_V_3(conexionActiva, 
                               fechaUnix, 
                               idTicket,
                               monto,
                               correlativo, 
                               serial,
                               idSorteoBono,                           
                               bonos,
                               renglonesModificados);
                         
    liberar_preventa(conexionActiva);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool
atenderPeticionGuardarTicket_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff, MYSQL * mysql,
                                 bool manejarTransaccion)
{
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Guardando el Ticket");
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    GuardarTicketV1 guardarTicket(buff);
    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
    boost::uint32_t idUsuario = conexionActiva.idUsuario;
    boost::uint64_t agente_taquilla = idAgente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += conexionActiva.idTaquilla;
    n_t_preventas_x_taquilla_t * preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    preventas_taquilla->hora_ultimo_renglon = time(NULL);
    preventas_taquilla->hora_ultimo_peticion = time(NULL);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    boost::uint32_t numeroDeRenglones = g_tree_nnodes(preventas_taquilla->preventas);

    if (numeroDeRenglones == 0) {                            /*Ya fue guardado el ticket */
        confirmacion_guardados_V_1(conexionActiva, idAgente, idTaquilla, mysql);
        return false;
    }
    
    if (numeroDeRenglones > guardarTicket.nrenglon) {  /*Eliminar todos lo tickets que estan de mas */
        for (boost::uint32_t renglon = guardarTicket.nrenglon + 1; renglon <= numeroDeRenglones; renglon++) {
            modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
        }
        numeroDeRenglones = guardarTicket.nrenglon;
    }
    
    if (numeroDeRenglones != guardarTicket.nrenglon) {
        /* abortar ticket */
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error Nro reglones Diferentes servidor %u, cliente %u", numeroDeRenglones, guardarTicket.nrenglon);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    std::set<boost::uint8_t> idSorteos;
    boost::int32_t monto = 0;
    boost::uint32_t crc = generCrcDelTicket(guardarTicket.nrenglon, preventas_taquilla, idSorteos, &monto);
    if (crc != guardarTicket.crcrenglones) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error CRC Diferentes <%u>/<%u>", crc, guardarTicket.crcrenglones);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
    /*Guargar el ticket al disco */

    if (verificar_ultimo_ticket(conexionActiva, mysql, guardarTicket.lastticket) ==
            false) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        return false;
    }

    GArray * renglonesDePreventa = g_array_new(false, false, sizeof(Preventa));
    for (boost::uint32_t renglon = 1; renglon <= guardarTicket.nrenglon; renglon++) {
        Preventa * ppreventa =
            (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
        Preventa preventa = *ppreventa;
        preventa.renglon = renglon;
        g_array_append_val(renglonesDePreventa, preventa);
    }
    GArray * renglonesDeVenta = g_array_new(false, false, sizeof(Preventa));
    {
        boost::uint32_t renglon = 0;
        while (renglonesDePreventa->len > 0) {
            Preventa preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
            boost::uint8_t idSorteo = preventa.idSorteo;
            while (renglon < renglonesDePreventa->len) {
                preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
                if (idSorteo == preventa.idSorteo) {
                    g_array_append_val(renglonesDeVenta, preventa);
                    g_array_remove_index(renglonesDePreventa, renglon);
                } else {
                    renglon++;
                }
            }
            renglon = 0;
        }
    }
    boost::uint16_t cuantos = 0;
    for (boost::uint32_t renglon = 0; renglon < renglonesDeVenta->len; renglon++) {
        Preventa preventa = g_array_index(renglonesDeVenta, Preventa, renglon);
        std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(preventa.idSorteo);
        if (preventa.monto_preventa > 0 and idSorteoPos == idSorteos.end()) {
            cuantos++;
        }
    }
    if (cuantos == 0) {                      
        /*No Hay Renglones para vender */
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        atenderPeticionCancelarPreventa(conexionActiva);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        g_array_free(renglonesDeVenta, true);
        g_array_free(renglonesDePreventa, true);
        return false;
    }

    boost::uint16_t estado = NORMAL;
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "BEGIN", DEBUG_PROCESAR_TICKET);
    }
    boost::uint32_t idTicket;
    {
        char nombreApostador[255], ciApostador[255];
        memset(nombreApostador, '\0', 255);
        memset(ciApostador, '\0', 255);
        strncpy(nombreApostador, guardarTicket.nombreApostador, NOMBRE_APOSTADOR_LENGTH);
        if (guardarTicket.ciApostador != 0) {
            sprintf(ciApostador, "%u", guardarTicket.ciApostador);
        } else {
            sprintf(ciApostador, "%d", 0);
        }
        
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Tickets "
                       "VALUES(NULL, FROM_UNIXTIME(%1%), %2%, 0, %3%, %4%, %5%, %6%, NULL, '%7%', '%8%', 0, 0, 1)")
        % time(NULL) % monto % unsigned(estado) % idAgente % idUsuario % unsigned(idTaquilla)
        % nombreApostador % ciApostador;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        idTicket = mysql_insert_id(mysql);
    }    
    boost::int32_t fechaUnix;
    char fecha[BUFFER_SIZE];
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Fecha, UNIX_TIMESTAMP(Fecha) FROM Tickets WHERE Id=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        strcpy(fecha, row[0]);
        fechaUnix = atoi(row[1]);
        mysql_free_result(result);
    }

    boost::uint8_t idSorteoBono = 0;
    GArray * renglonesDeResumen = NULL;
    boost::uint16_t guardados = 0;     
    bool ok = guardarRenglonesRegulares(mysql,
                                        idTicket,
                                        renglonesDeVenta, 
                                        numeroDeRenglones,
                                        idSorteos, 
                                        idSorteoBono, 
                                        renglonesDeResumen,
                                        guardados);
    g_array_free(renglonesDeVenta, true);
    g_array_free(renglonesDePreventa, true);
    if (not ok) {
        /* abortar ticket */
        log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesRegulares()");
        g_array_free(renglonesDeResumen, true);
        liberar_venta(conexionActiva, mysql, idTicket);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        if (manejarTransaccion == true) {
            ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
    liberar_preventa(conexionActiva);
    boost::uint8_t idSorteo;
    std::vector<boost::uint16_t> bonos;
    boost::uint8_t tipoJugadaBono;
    if (guardarTicket.numeroDeBonos > 0) {                      /*Generar los bonos */
        idSorteo = guardarTicket.idSorteo;
        Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
        if (sorteo != NULL) {
            verificar_sorteo(&conexionActiva, sorteo, idSorteo, false);
            if (not sorteo->isEstadoActivo()) {
                idSorteo = idSorteoBono;
            }
        }
        std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(idSorteo);
        if (idSorteoPos != idSorteos.end() or sorteo == NULL) {
            idSorteo = idSorteoBono;
        }        
        
        {
            std::string mensaje("Sorteo de bonos IdSorteo: ");
            mensaje += boost::lexical_cast<std::string>(unsigned(idSorteo));
            log_clientes(NivelLog::Detallado, &conexionActiva, mensaje.c_str());
        }
        
        sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
        boost::uint32_t renglon = guardarTicket.nrenglon + 1;
        bonos = generar_bonos(conexionActiva, idSorteo, guardarTicket.numeroDeBonos,
                              conexionActiva.idZona, idAgente, guardarTicket.montoBono,
                              renglon, idTaquilla, preventas_taquilla,
                              &tipoJugadaBono);
        
        if (bonos.empty()) {
            /* abortar ticket */
            std::string mensaje("Agencia ");
            mensaje += boost::lexical_cast<std::string>(idAgente) 
                    + " con idSorteo " 
                    + boost::lexical_cast<std::string>(unsigned(idSorteo))
                    + " mal configurada";
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje.c_str());
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }
                                         
        renglon = guardarTicket.nrenglon + 1;        
        bool ok = guardarRenglonesBonos(mysql,
                                        idTicket,
                                        guardarTicket,
                                        idSorteo,
                                        sorteo,
                                        bonos,
                                        renglonesDeResumen,
                                        guardados);
        if (not ok) {
            /* abortar ticket */
            log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesBonos()");
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }   
    }
    
    for (boost::uint32_t r = 0; r < renglonesDeResumen->len; r++) {
        ResumenRenglones resumen_renglones =
            g_array_index(renglonesDeResumen, ResumenRenglones, r);
        {
            boost::format sqlQuery;
            sqlQuery.parse("INSERT INTO Resumen_Renglones VALUES(%1%, %2%, %3%, %4%)")
            % idTicket 
            % unsigned(resumen_renglones.idSorteo)
            % unsigned(resumen_renglones.tipo)
            % resumen_renglones.monto;
            ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
        }
    }
    
    g_array_free(renglonesDeResumen, true);
    
    std::vector<boost::uint8_t> sorteos = liberarIdSorteosSet(idSorteos);

    for (int i = 0, size = sorteos.size(); i < size; ++i) {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Info_Ticket VALUES(%u, %u)")
        % idTicket % unsigned(sorteos[i]);
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    }
    
    ProtocoloExterno::Ticket ticket(idAgente, idTaquilla, guardarTicket.ciApostador, idTicket);
    RealizarVentaExterna ventaExterna(conexionActiva, mysql, ticket);
    if (!ventaExterna()) {
        liberar_venta(conexionActiva, mysql, idTicket);
        atenderPeticionCancelarPreventa(conexionActiva);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    boost::uint32_t correlativo = 0;
    {
        boost::format sqlQuery1;
        sqlQuery1.parse("SELECT SiguienteCorrelativo FROM CorrelativoTickets WHERE IdAgente=%1% FOR UPDATE")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery1, DEBUG_PROCESAR_TICKET);
        MYSQL_RES *result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL || row[0] == NULL) {                        
            liberar_venta(conexionActiva, mysql, idTicket);
            atenderPeticionCancelarPreventa(conexionActiva);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }
        correlativo = strtoul(row[0], NULL, 10);        
        mysql_free_result(result);
        boost::format sqlQuery2;
        sqlQuery2.parse("UPDATE CorrelativoTickets SET SiguienteCorrelativo = SiguienteCorrelativo + 1 "
                        "WHERE IdAgente=%1%")
        % idAgente;
        ejecutar_sql(mysql, sqlQuery2, DEBUG_PROCESAR_TICKET);
    }
    std::string serial = ventaExterna.getSerial();
    {
        boost::uint32_t montoBono = guardarTicket.montoBono * bonos.size();
        unsigned protocolo = ventaExterna.getIdProtocolo();
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET "
                       "Serial='%1%', MontoBono=%2%, Correlativo=%3%, IdProtocolo=%4% "
                       "WHERE Id=%5%")
        % serial % montoBono % correlativo % protocolo % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
    }

    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "COMMIT", DEBUG_PROCESAR_TICKET);
    }
        
    enviar_ticket_guardado_V_1(conexionActiva, 
                            fechaUnix, 
                            idTicket, 
                            serial.c_str(), 
                            monto,
                            bonos, 
                            sorteos,
                            idSorteo, 
                            correlativo);
                            
    liberar_preventa(conexionActiva);
    return true;
}

bool
atenderPeticionGuardarTicket_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff, MYSQL * mysql,
                                 bool manejarTransaccion)
{
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Guardando el Ticket");
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
    
	GuardarTicketV0 guardarTicket(buff);
	boost::uint32_t idAgente = conexionActiva.idAgente;
	boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
	boost::uint32_t idUsuario = conexionActiva.idUsuario;
	boost::uint64_t agente_taquilla = idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += conexionActiva.idTaquilla;
	n_t_preventas_x_taquilla_t * preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	boost::uint32_t numeroDeRenglones = g_tree_nnodes(preventas_taquilla->preventas);

	if (numeroDeRenglones == 0) {                            /*Ya fue guardado el ticket */
		confirmacion_guardados_V_0(conexionActiva, idAgente, idTaquilla, mysql);
		return false;
	}
    
	if (numeroDeRenglones > guardarTicket.nrenglon) {  /*Eliminar todos lo tickets que estan de mas */
		for (boost::uint32_t renglon = guardarTicket.nrenglon + 1; renglon <= numeroDeRenglones; renglon++) {
			modificarRenglon(conexionActiva, idAgente, idTaquilla, renglon, 0);
		}
		numeroDeRenglones = guardarTicket.nrenglon;
	}
    
	if (numeroDeRenglones != guardarTicket.nrenglon) {
        /* abortar ticket */
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error Nro reglones Diferentes servidor %u, cliente %u", numeroDeRenglones, guardarTicket.nrenglon);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    std::set<boost::uint8_t> idSorteos;
	boost::int32_t monto = 0;
	boost::uint32_t crc = generCrcDelTicket(guardarTicket.nrenglon, preventas_taquilla, idSorteos, &monto);
	if (crc != guardarTicket.crcrenglones) {
        sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "Error CRC Diferentes <%u>/<%u>", crc, guardarTicket.crcrenglones);
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        }
        return false;
    }
    
	/*Guargar el ticket al disco */

	if (verificar_ultimo_ticket(conexionActiva, mysql, guardarTicket.lastticket) ==
	        false) {
		sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
		return false;
	}

	GArray * renglonesDePreventa = g_array_new(false, false, sizeof(Preventa));
	for (boost::uint32_t renglon = 1; renglon <= guardarTicket.nrenglon; renglon++) {
		Preventa * ppreventa =
		    (Preventa *) g_tree_lookup(preventas_taquilla->preventas, &renglon);
		Preventa preventa = *ppreventa;
		preventa.renglon = renglon;
		g_array_append_val(renglonesDePreventa, preventa);
	}
	GArray * renglonesDeVenta = g_array_new(false, false, sizeof(Preventa));
	{
		boost::uint32_t renglon = 0;
		while (renglonesDePreventa->len > 0) {
			Preventa preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
			boost::uint8_t idSorteo = preventa.idSorteo;
			while (renglon < renglonesDePreventa->len) {
				preventa = g_array_index(renglonesDePreventa, Preventa, renglon);
				if (idSorteo == preventa.idSorteo) {
					g_array_append_val(renglonesDeVenta, preventa);
					g_array_remove_index(renglonesDePreventa, renglon);
				} else {
					renglon++;
				}
			}
			renglon = 0;
		}
	}
	boost::uint16_t cuantos = 0;
	for (boost::uint32_t renglon = 0; renglon < renglonesDeVenta->len; renglon++) {
		Preventa preventa = g_array_index(renglonesDeVenta, Preventa, renglon);
        std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(preventa.idSorteo);
		if (preventa.monto_preventa > 0 and idSorteoPos == idSorteos.end()) {
			cuantos++;
		}
	}
	if (cuantos == 0) {                      
        /*No Hay Renglones para vender */
		sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
		atenderPeticionCancelarPreventa(conexionActiva);
		send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
		g_array_free(renglonesDeVenta, true);
		g_array_free(renglonesDePreventa, true);
		return false;
	}

	boost::uint16_t estado = NORMAL;
	if (manejarTransaccion == true) {
		ejecutar_sql(mysql, "BEGIN", DEBUG_PROCESAR_TICKET);
	}
	boost::uint32_t idTicket;
	{
		boost::format sqlQuery;
		sqlQuery.parse("INSERT INTO Tickets "
		               "VALUES(NULL, FROM_UNIXTIME(%1%), %2%, 0, %3%, %4%, %5%, %6%, NULL, '', '', 0, 0, 1)")
		% time(NULL) % monto % unsigned(estado) % idAgente % idUsuario % unsigned(idTaquilla);
		ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
		idTicket = mysql_insert_id(mysql);
	}
	boost::int32_t fechaUnix;
	char fecha[BUFFER_SIZE];
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Fecha, UNIX_TIMESTAMP(Fecha) FROM Tickets WHERE Id=%1%")
		% idTicket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
		MYSQL_RES *result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		strcpy(fecha, row[0]);
		fechaUnix = atoi(row[1]);
		mysql_free_result(result);
	}
	boost::uint32_t serial = genera_serial(idTicket, monto, idAgente, idTaquilla, idUsuario, fecha);
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Tickets SET Serial='%1%' WHERE Id=%2%")
		% serial % idTicket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
	}

	boost::uint8_t idSorteoBono = 0;
    GArray * renglonesDeResumen = NULL;
    boost::uint16_t guardados = 0;     
    bool ok = guardarRenglonesRegulares(mysql,
                                        idTicket,
                                        renglonesDeVenta, 
                                        numeroDeRenglones,
                                        idSorteos, 
                                        idSorteoBono, 
                                        renglonesDeResumen,
                                        guardados);
    g_array_free(renglonesDeVenta, true);
    g_array_free(renglonesDePreventa, true);
    if (not ok) {
        /* abortar ticket */
        log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesRegulares()");
        g_array_free(renglonesDeResumen, true);
        liberar_venta(conexionActiva, mysql, idTicket);
        send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        atenderPeticionCancelarPreventa(conexionActiva);
        if (manejarTransaccion == true) {
            ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
        }
        return false;
    }
    
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);
	liberar_preventa(conexionActiva);
	boost::uint8_t idSorteo;
	std::vector<boost::uint16_t> bonos;
	boost::uint8_t tipoJugadaBono;
	if (guardarTicket.numeroDeBonos > 0) {                      /*Generar los bonos */
		idSorteo = guardarTicket.idSorteo;
		Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
		if (sorteo != NULL) {
			verificar_sorteo(&conexionActiva, sorteo, idSorteo, false);
			if (not sorteo->isEstadoActivo()) {
				idSorteo = idSorteoBono;
			}
		}
        std::set<boost::uint8_t>::const_iterator idSorteoPos = idSorteos.find(idSorteo);
		if (idSorteoPos != idSorteos.end() or sorteo == NULL) {
			idSorteo = idSorteoBono;
		}		
		
		{
	    	std::string mensaje("Sorteo de bonos IdSorteo: ");
	    	mensaje += boost::lexical_cast<std::string>(unsigned(idSorteo));
	        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje.c_str());
		}
		
		sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
		boost::uint32_t renglon = guardarTicket.nrenglon + 1;
		bonos = generar_bonos(conexionActiva, idSorteo, guardarTicket.numeroDeBonos,
		                      conexionActiva.idZona, idAgente, guardarTicket.montoBono,
		                      renglon, idTaquilla, preventas_taquilla,
		                      &tipoJugadaBono);
    
    	if (bonos.empty()) {
            /* abortar ticket */
            std::string mensaje("Agencia ");
	    	mensaje += boost::lexical_cast<std::string>(idAgente) 
	    	        + " con idSorteo " 
	    	        + boost::lexical_cast<std::string>(unsigned(idSorteo))
	    	        + " mal configurada";
	        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje.c_str());
            g_array_free(renglonesDeResumen, true);
            
            liberar_venta(conexionActiva, mysql, idTicket);
            
        	send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
        	atenderPeticionCancelarPreventa(conexionActiva);
        	return false;
    	}
		                                 
		renglon = guardarTicket.nrenglon + 1;
        bool ok = guardarRenglonesBonos(mysql,
                                        idTicket,
                                        guardarTicket,
                                        idSorteo,
                                        sorteo,
                                        bonos,
                                        renglonesDeResumen,
                                        guardados);
        if (not ok) {
            /* abortar ticket */
            log_clientes(NivelLog::Bajo, &conexionActiva, "guardarRenglonesBonos()");
            g_array_free(renglonesDeResumen, true);
            liberar_venta(conexionActiva, mysql, idTicket);
            send2cliente(conexionActiva, ERR_TICKETCRCDIFERENTE, NULL, 0);
            atenderPeticionCancelarPreventa(conexionActiva);
            if (manejarTransaccion == true) {
                ejecutar_sql(mysql, "ROLLBACK", DEBUG_PROCESAR_TICKET);
            }
            return false;
        }   
	}
	
	for (boost::uint32_t r = 0; r < renglonesDeResumen->len; r++) {
		ResumenRenglones resumen_renglones =
		    g_array_index(renglonesDeResumen, ResumenRenglones, r);
		{
			boost::format sqlQuery;
			sqlQuery.parse("INSERT INTO Resumen_Renglones VALUES(%1%, %2%, %3%, %4%)")
			% idTicket % unsigned(resumen_renglones.idSorteo)
			% unsigned(resumen_renglones.tipo)
			% resumen_renglones.monto;
			ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
		}
	}
	
	g_array_free(renglonesDeResumen, true);
	
	std::vector<boost::uint8_t> sorteos = liberarIdSorteosSet(idSorteos);

    for (int i = 0, size = sorteos.size(); i < size; ++i) {
		boost::format sqlQuery;
		sqlQuery.parse("INSERT INTO Info_Ticket VALUES(%u, %u)")
		% idTicket % unsigned(sorteos[i]);
		ejecutar_sql(mysql, sqlQuery, DEBUG_PROCESAR_TICKET);
	}

	if (manejarTransaccion == true) {
		ejecutar_sql(mysql, "COMMIT", DEBUG_PROCESAR_TICKET);
	}
	            	
    enviar_ticket_guardado_V_0(conexionActiva, 
                        fechaUnix, 
                        idTicket, 
                        serial, 
                        monto,
                        bonos, 
                        sorteos,
                        idSorteo);
        
	liberar_preventa(conexionActiva);
	return true;
}

void
liberar_venta(ConexionActiva& conexionActiva, MYSQL * mysql, boost::uint32_t ticket)
{
	boost::uint8_t idZona = conexionActiva.idZona;
	n_t_zona_numero_restringido_t * zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	if (zona_num_restringido == NULL) {
		return ;
	}

	boost::uint32_t idAgente = conexionActiva.idAgente;

	TipoDeMontoPorAgencia * tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);

	log_clientes(NivelLog::Detallado, &conexionActiva, "Liberando Venta");

	std::vector<n_a_renglon_t> renglones;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Numero, Monto, IdSorteo, Tipo FROM Renglones WHERE IdTicket=%1%")
		% ticket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_LIBERAR_VENTA);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
			n_a_renglon_t unRenglon;
			unRenglon.numero = atoi(row[0]);
			unRenglon.monto = atoi(row[1]);
			unRenglon.idSorteo = atoi(row[2]);
			unRenglon.tipo = atoi(row[3]);

			renglones.push_back(unRenglon);
		}
		mysql_free_result(result);
	}

	for (std::size_t i = 0, size = renglones.size(); i < size ; ++i) {
		boost::uint16_t numero = renglones[i].numero;
		boost::int32_t monto = renglones[i].monto;
		boost::uint8_t idSorteo = renglones[i].idSorteo;
		boost::uint8_t tipo = renglones[i].tipo;

		Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
		n_t_sorteo_numero_restringido_t * sorteo_num_restringido =
		    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
		                                                         t_sorteo_numero_restringido,
		                                                         &idSorteo);
		TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
		if (tipo == TERMINAL or tipo == TERMINALAZO) {
			boost::uint8_t tipomonto_terminal = tipomonto_sorteo->tipomonto_terminal;
			n_t_tipomonto_t * tipoDeMonto =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(),
			                                         &tipomonto_terminal);
			if (tipoDeMonto == NULL) {
				char mensaje[BUFFER_SIZE];
				sprintf(mensaje,
				         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
				         tipomonto_terminal);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				return ;
			}

			/*-------------------------------------------------------------------------------------------*/
			sem_wait(&tipoDeMonto->sem_t_tipomonto);
			tipoDeMonto->venta_global -= monto;
			limite_tipomonto(tipoDeMonto, tipo);
			n_t_venta_numero_t * venta_numero =
			    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
			venta_numero->venta -= monto;
			sem_post(&tipoDeMonto->sem_t_tipomonto);

			/*-------------------------------------------------------------------------------------------*/

			sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			n_t_numero_restringido_t * num_restringido =
			    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
			                                                  t_numero_restringido_terminal,
			                                                  &numero);
			if (num_restringido != NULL) {
				num_restringido->venta -= monto;
			}
			sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);

		} else if (tipo == TRIPLE or tipo == TRIPLETAZO) {
			boost::uint8_t tipomonto_triple = tipomonto_sorteo->tipomonto_triple;
			n_t_tipomonto_t * tipoDeMonto =
			    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipomonto_triple);
			if (tipoDeMonto == NULL) {
				char mensaje[BUFFER_SIZE];

				sprintf(mensaje,
				         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
				         tipomonto_triple);
				log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
				return ;
			}

			/*-------------------------------------------------------------------------------------------*/
			sem_wait(&tipoDeMonto->sem_t_tipomonto);
			tipoDeMonto->venta_global -= monto;
			limite_tipomonto(tipoDeMonto, tipo);
			n_t_venta_numero_t * venta_numero =
			    (n_t_venta_numero_t *) g_tree_lookup(tipoDeMonto->t_venta_numero, &numero);
			venta_numero->venta -= monto;
			sem_post(&tipoDeMonto->sem_t_tipomonto);

			/*-------------------------------------------------------------------------------------------*/

			sem_wait(&zona_num_restringido->sem_t_sorteo_numero_restringido);
			n_t_numero_restringido_t * num_restringido =
			    (n_t_numero_restringido_t *) g_tree_lookup(sorteo_num_restringido->
			                                                  t_numero_restringido_triple, &numero);
			if (num_restringido != NULL) {
				num_restringido->venta -= monto;
			}
			sem_post(&zona_num_restringido->sem_t_sorteo_numero_restringido);
		}
	}
}

bool
atenderPeticionAnularTicket_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff, MYSQL * mysql,
                                bool manejarTransaccion)
{
	ticket_anular_sol_t ticket_anular = ticket_anular_sol_b2t(buff);

	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Eliminando el Ticket %u", ticket_anular.nticket);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}
    
    std::vector<boost::uint8_t> idSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Renglones WHERE IdTicket=%1%")
        % ticket_anular.nticket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            idSorteos.push_back(atoi(row[0]));
        }
        mysql_free_result(result);
    }
    
    if (idSorteos.empty()) {
            send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
            return false;
    }    

    try {    
        std::auto_ptr<ProtocoloExterno::AnularTicket> 
        anularExterno = ProtocoloExterno::MensajeFactory::getInstance()
                      . crearAnularTicket(mysql, ticket_anular.nticket);            
        (*anularExterno)(mysql);
        
    } catch (ProtocoloExterno::ServiceNoFoundException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, imposible conectar con el servidor local de enlace: %s", 
                e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        return false;
    } catch (ProtocoloExterno::ReponseException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error retornado por el servidor remoto: %s (%i)", 
                         e.what(), e.getCode());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        return false;
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: runtime_error al anular ticket: %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        return false;
    } catch (ProtocoloExterno::InvalidArgumentException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: InvalidArgumentException al anular ticket: %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        return false;                 
    } catch (...) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error desconocido en protocolo externo");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
        return false;
    }
    
	bool ticketExiste = false;
	boost::int32_t fechaPosix = 0;
	boost::int32_t estado = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), Estado FROM Tickets "
		                "WHERE Id=%1% AND Idagente=%2% AND NumTaquilla=%3%")
		% ticket_anular.nticket % conexionActiva.idAgente % unsigned(conexionActiva.idTaquilla);
		ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			fechaPosix = atoi(row[0]);
			estado = atoi(row[1]);
			ticketExiste = true;
		}
		mysql_free_result(result);
	}

	if (not ticketExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, ticket no existe");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_TICKORENGNOEXIS, NULL, 0);
		return false;
	}

	boost::int32_t estadoOld = estado;
	switch (estado) {
		case ANULADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket anulado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_TICKETYAANULADO, NULL, 0);
			return false;
		}
		case PAGADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket pagado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_TICKETYACANCELADO, NULL, 0);
			return false;
		}
		case TACHADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket tachado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_TICKETYAANULADO, NULL, 0);
			return false;
		}
		case ELIMINADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket eliminado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_TICKETYAANULADO, NULL, 0);
			return false;
		}
	}
	time_t fechaServidor = time(NULL);
	boost::int32_t lapsoDeFecha = (fechaServidor - fechaPosix) / 60;
	if (lapsoDeFecha > tiempoDeEliminacion) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, ticket timeout");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		send2cliente(conexionActiva, ERR_TICKETTIEMPOFUERA, NULL, 0);
		return false;
	}

	for (std::size_t i = 0, size = idSorteos.size(); i < size ; ++i) {
		boost::uint8_t idSorteo = idSorteos[i];
		Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
		verificar_sorteo(&conexionActiva, sorteo, idSorteo, false);
		if (not sorteo->isEstadoActivo()) {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, sorteo no activo");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
			send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
			return false;
		}
	}
    estado = ANULADO;
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "BEGIN", DEBUG_ELIMINAR_TICKET);        
    }
    
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Tickets SET Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
		% unsigned(estado) % time(NULL) % ticket_anular.nticket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
	}
	
	{
		boost::format sqlQuery;
		sqlQuery.parse("INSERT INTO Cambio_Ticket VALUES(%1%, NULL, %2%, %3%)")
		% ticket_anular.nticket % estadoOld % conexionActiva.idUsuario;
		ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
	}
    if (manejarTransaccion == true) {
		ejecutar_sql(mysql, "COMMIT", DEBUG_ELIMINAR_TICKET);
		liberar_venta(conexionActiva, mysql, ticket_anular.nticket);
		send2cliente(conexionActiva, ANULAR_TICKET_V_0, NULL, 0);
	}
	return true;
}

bool
atenderPeticionAnularTicket_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff, MYSQL * mysql,
                                bool manejarTransaccion)
{
    ticket_anular_sol_t ticket_anular = ticket_anular_sol_b2t(buff);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Eliminando el Ticket %u", ticket_anular.nticket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }
    
    std::vector<boost::uint8_t> idSorteos;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Renglones WHERE IdTicket=%1%")
        % ticket_anular.nticket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        for (MYSQL_ROW row = mysql_fetch_row(result); NULL != row; row = mysql_fetch_row(result)) {
            idSorteos.push_back(atoi(row[0]));
        }
        mysql_free_result(result);
    }
    
    if (idSorteos.empty()) {
        send2cliente(conexionActiva, ErrorException(ERR_TICKETNOELIM, 
                                                    "Ticket no tiene renglones"));
        return false;
    }    

    try {    
        std::auto_ptr<ProtocoloExterno::AnularTicket> 
        anularExterno = ProtocoloExterno::MensajeFactory::getInstance()
                      . crearAnularTicket(mysql, ticket_anular.nticket);            
        (*anularExterno)(mysql);
        
    } catch (ProtocoloExterno::ServiceNoFoundException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, imposible conectar con el servidor local de enlace: %s", 
                e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_TICKETNOELIM, e.what()));
        return false;
    } catch (ProtocoloExterno::ReponseException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error retornado por el servidor remoto: %s (%i)", 
                         e.what(), e.getCode());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(e.getCode(), e.what()));
        return false;
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: runtime_error al anular ticket: %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_TICKETNOELIM, e.what()));
        return false;
    } catch (ProtocoloExterno::InvalidArgumentException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: InvalidArgumentException al anular ticket: %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_TICKETNOELIM, e.what()));
        return false;                 
    } catch (...) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, error desconocido en protocolo externo");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_TICKETNOELIM,
                                                    "error desconocido en protocolo externo"));
        return false;
    }
    
    bool ticketExiste = false;
    boost::int32_t fechaPosix = 0;
    boost::int32_t estado = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), Estado FROM Tickets "
                        "WHERE Id=%1% AND Idagente=%2% AND NumTaquilla=%3%")
        % ticket_anular.nticket % conexionActiva.idAgente % unsigned(conexionActiva.idTaquilla);
        ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            fechaPosix = atoi(row[0]);
            estado = atoi(row[1]);
            ticketExiste = true;
        }
        mysql_free_result(result);
    }

    if (not ticketExiste) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, ticket no existe");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_TICKORENGNOEXIS, 
                     "Ticket no existe"));
        return false;
    }

    boost::int32_t estadoOld = estado;
    switch (estado) {
        case ANULADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket anulado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETYAANULADO, 
                         "Ticket ya anulado"));
            return false;
        }
        case PAGADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket pagado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETYAANULADO, 
                         "Ticket ya pagado"));
            return false;
        }
        case TACHADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket tachado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETYAANULADO, 
                         "Ticket ya anulado"));
            return false;
        }
        case ELIMINADO: {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, ticket eliminado");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETYAANULADO, 
                         "Ticket ya anulado"));
            return false;
        }
    }
    time_t fechaServidor = time(NULL);
    boost::int32_t lapsoDeFecha = (fechaServidor - fechaPosix) / 60;
    if (lapsoDeFecha > tiempoDeEliminacion) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR: Al anular ticket, ticket timeout");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETTIEMPOFUERA, 
                         "Tiempo para eliminar agotado"));
        return false;
    }

    for (std::size_t i = 0, size = idSorteos.size(); i < size ; ++i) {
        boost::uint8_t idSorteo = idSorteos[i];
        Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
        verificar_sorteo(&conexionActiva, sorteo, idSorteo, false);
        if (not sorteo->isEstadoActivo()) {
            char mensaje[BUFFER_SIZE];
            sprintf(mensaje, "ERROR: Al anular ticket, sorteo no activo");
            log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
            send2cliente(conexionActiva, ErrorException(ERR_TICKETNOELIM, 
                         "Sorteo cerrado"));
            return false;
        }
    }
    estado = ANULADO;
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "BEGIN", DEBUG_ELIMINAR_TICKET);        
    }
    
    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
        % unsigned(estado) % time(NULL) % ticket_anular.nticket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
    }
    
    {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Cambio_Ticket VALUES(%1%, NULL, %2%, %3%)")
        % ticket_anular.nticket % estadoOld % conexionActiva.idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_ELIMINAR_TICKET);
    }
    if (manejarTransaccion == true) {
        ejecutar_sql(mysql, "COMMIT", DEBUG_ELIMINAR_TICKET);
        liberar_venta(conexionActiva, mysql, ticket_anular.nticket);
        send2cliente(conexionActiva, ANULAR_TICKET_V_1, NULL, 0);
    }
    return true;
}

void
atenderPeticionModificarTicket_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
	ModificarTicketV0 ticket_mod(buff);
	boost::uint32_t idAgente = conexionActiva.idAgente;
	boost::uint64_t agente_taquilla = idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += conexionActiva.idTaquilla;
	n_t_preventas_x_taquilla_t * preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	if (preventas_taquilla == NULL) {
		return ;
	}
	sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
	preventas_taquilla->hora_ultimo_renglon = time(NULL);
	preventas_taquilla->hora_ultimo_peticion = time(NULL);
	boost::uint32_t nnodes = g_tree_nnodes(preventas_taquilla->preventas);
	sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);

	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Modificando el Ticket %u", ticket_mod.nticket);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}

	if (nnodes > 0) {

		boost::uint8_t estado = 0;
		bool estadoExiste = true;
		{
			boost::format sqlQuery;
			sqlQuery.parse("SELECT Estado FROM Tickets WHERE Id=%1%")
			% ticket_mod.nticket;
			ejecutar_sql(mysql, sqlQuery, DEBUG_MODIFICAR_TICKET);
			MYSQL_RES * result = mysql_store_result(mysql);
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row == NULL) {
				estado = atoi(row[0]);
				estadoExiste = true;
			}
			mysql_free_result(result);
		}

		if (not estadoExiste) {
			send2cliente(conexionActiva, ERR_TICKORENGNOEXIS, NULL, 0);
			return ;
		}

		ticket_anular_sol_t t;
		t.nticket = ticket_mod.nticket;

		ejecutar_sql(mysql, "BEGIN", DEBUG_MODIFICAR_TICKET);

		boost::uint8_t * buff2 = ticket_anular_sol_t2b(t);
		bool resultado = atenderPeticionAnularTicket_V_0(conexionActiva, buff2, mysql, false);
		delete [] buff2;

		if (resultado) {
			GuardarTicketV0 guardarTicket;
			guardarTicket.numeroDeBonos = ticket_mod.nbonos;
			guardarTicket.idSorteo = ticket_mod.idSorteo;
			guardarTicket.montoBono = ticket_mod.monto;
			guardarTicket.nrenglon = ticket_mod.nrenglon;
			guardarTicket.crcrenglones = ticket_mod.crcrenglones;
			guardarTicket.lastticket = ticket_mod.lastticket;

			std::vector<boost::uint8_t> buff2 = guardarTicket.toRawBuffer();
			resultado = atenderPeticionGuardarTicket_V_0(conexionActiva, &buff2[0], mysql, false);

			if (resultado) {
				ejecutar_sql(mysql, "COMMIT", DEBUG_MODIFICAR_TICKET);
				liberar_venta(conexionActiva, mysql, ticket_mod.nticket);
			} else {
				ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
			}
		} else {
			ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
		}
	} else {
		confirmacion_guardados_V_0(conexionActiva, idAgente, conexionActiva.idTaquilla, mysql);
	}
	atenderPeticionCancelarPreventa(conexionActiva);
}

/*-------------------------------------------------------------------------*/

void
atenderPeticionModificarTicket_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    ModificarTicketV1 ticket_mod(buff);
    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint64_t agente_taquilla = idAgente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += conexionActiva.idTaquilla;
    n_t_preventas_x_taquilla_t * preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    if (preventas_taquilla == NULL) {
        return ;
    }
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    preventas_taquilla->hora_ultimo_renglon = time(NULL);
    preventas_taquilla->hora_ultimo_peticion = time(NULL);
    boost::uint32_t nnodes = g_tree_nnodes(preventas_taquilla->preventas);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Modificando el Ticket %u", ticket_mod.nticket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    if (nnodes > 0) {

        boost::uint8_t estado = 0;
        bool estadoExiste = true;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Estado FROM Tickets WHERE Id=%1%")
            % ticket_mod.nticket;
            ejecutar_sql(mysql, sqlQuery, DEBUG_MODIFICAR_TICKET);
            MYSQL_RES * result = mysql_store_result(mysql);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row == NULL) {
                estado = atoi(row[0]);
                estadoExiste = true;
            }
            mysql_free_result(result);
        }

        if (not estadoExiste) {
            send2cliente(conexionActiva, ERR_TICKORENGNOEXIS, NULL, 0);
            return ;
        }

        ticket_anular_sol_t t;
        t.nticket = ticket_mod.nticket;

        ejecutar_sql(mysql, "BEGIN", DEBUG_MODIFICAR_TICKET);

        boost::uint8_t * buff2 = ticket_anular_sol_t2b(t);
        bool resultado = atenderPeticionAnularTicket_V_0(conexionActiva, buff2, mysql, false);
        delete [] buff2;

        if (resultado) {
            GuardarTicketV1 guardarTicket;
            guardarTicket.numeroDeBonos = ticket_mod.nbonos;
            guardarTicket.idSorteo = ticket_mod.idSorteo;
            guardarTicket.montoBono = ticket_mod.monto;
            guardarTicket.nrenglon = ticket_mod.nrenglon;
            guardarTicket.crcrenglones = ticket_mod.crcrenglones;
            guardarTicket.lastticket = ticket_mod.lastticket;
            guardarTicket.montoPorBono = ticket_mod.montoPorBono;
            guardarTicket.maxBono = ticket_mod.maxBono;
            memcpy(guardarTicket.nombreApostador, ticket_mod.nombreApostador, NOMBRE_APOSTADOR_LENGTH);
            guardarTicket.ciApostador = ticket_mod.ciApostador;
            
            std::vector<boost::uint8_t> buff2 = guardarTicket.toRawBuffer();
            resultado = atenderPeticionGuardarTicket_V_1(conexionActiva, &buff2[0], mysql, false);

            if (resultado) {
                ejecutar_sql(mysql, "COMMIT", DEBUG_MODIFICAR_TICKET);
                liberar_venta(conexionActiva, mysql, ticket_mod.nticket);
            } else {
                ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
            }
        } else {
            ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
        }
    } else {
        confirmacion_guardados_V_1(conexionActiva, idAgente, conexionActiva.idTaquilla, mysql);
    }
    atenderPeticionCancelarPreventa(conexionActiva);
}

void
atenderPeticionModificarTicket_V_2(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    ModificarTicketV2 ticket_mod(buff);
    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint64_t agente_taquilla = idAgente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += conexionActiva.idTaquilla;
    n_t_preventas_x_taquilla_t * preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    if (preventas_taquilla == NULL) {
        return ;
    }
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    preventas_taquilla->hora_ultimo_renglon = time(NULL);
    preventas_taquilla->hora_ultimo_peticion = time(NULL);
    boost::uint32_t nnodes = g_tree_nnodes(preventas_taquilla->preventas);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Modificando el Ticket %u", ticket_mod.idTicket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    if (nnodes > 0) {

        boost::uint8_t estado = 0;
        bool estadoExiste = true;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Estado FROM Tickets WHERE Id=%1%")
            % ticket_mod.idTicket;
            ejecutar_sql(mysql, sqlQuery, DEBUG_MODIFICAR_TICKET);
            MYSQL_RES * result = mysql_store_result(mysql);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row == NULL) {
                estado = atoi(row[0]);
                estadoExiste = true;
            }
            mysql_free_result(result);
        }

        if (not estadoExiste) {
            send2cliente(conexionActiva, ERR_TICKORENGNOEXIS, NULL, 0);
            return ;
        }

        ticket_anular_sol_t t;
        t.nticket = ticket_mod.idTicket;

        ejecutar_sql(mysql, "BEGIN", DEBUG_MODIFICAR_TICKET);
        bool resultado = false;
        {
            boost::uint8_t * buff2 = ticket_anular_sol_t2b(t);
            resultado = atenderPeticionAnularTicket_V_0(conexionActiva, buff2, mysql, false);
            delete [] buff2;
        }

        if (resultado) {
            GuardarTicketV2 guardarTicket;
            guardarTicket.numeroDeBonos = ticket_mod.numeroDeBonos;
            guardarTicket.montoBono = ticket_mod.montoBono;
            guardarTicket.numeroDeRenglones = ticket_mod.numeroDeRenglones;
            guardarTicket.crc = ticket_mod.crc;
            guardarTicket.ultimoTicket = ticket_mod.ultimoTicket;
            guardarTicket.nombreApostador = ticket_mod.nombreApostador;
            guardarTicket.ciApostador = ticket_mod.ciApostador;

            std::vector<boost::uint8_t> buff2 = guardarTicket.toRawBuffer();
            resultado = atenderPeticionGuardarTicket_V_2(conexionActiva, &buff2[0], mysql, false);

            if (resultado) {                
                ejecutar_sql(mysql, "COMMIT", DEBUG_MODIFICAR_TICKET);
                liberar_venta(conexionActiva, mysql, ticket_mod.idTicket);
            } else {
                ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
            }
        } else {
            ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
        }
    } else {
        confirmacion_guardados_V_2(conexionActiva, idAgente, conexionActiva.idTaquilla, mysql);
    }
    atenderPeticionCancelarPreventa(conexionActiva);
}

void
atenderPeticionModificarTicket_V_3(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    ModificarTicketV2 ticket_mod(buff);
    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint64_t agente_taquilla = idAgente;
    agente_taquilla = agente_taquilla << 32;
    agente_taquilla += conexionActiva.idTaquilla;
    n_t_preventas_x_taquilla_t * preventas_taquilla =
        (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
    if (preventas_taquilla == NULL) {
        return ;
    }
    sem_wait(&preventas_taquilla->mutex_preventas_x_taquilla);
    preventas_taquilla->hora_ultimo_renglon = time(NULL);
    preventas_taquilla->hora_ultimo_peticion = time(NULL);
    boost::uint32_t nnodes = g_tree_nnodes(preventas_taquilla->preventas);
    sem_post(&preventas_taquilla->mutex_preventas_x_taquilla);

    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Modificando el Ticket %u", ticket_mod.idTicket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    if (nnodes > 0) {

        boost::uint8_t estado = 0;
        bool estadoExiste = true;
        {
            boost::format sqlQuery;
            sqlQuery.parse("SELECT Estado FROM Tickets WHERE Id=%1%")
            % ticket_mod.idTicket;
            ejecutar_sql(mysql, sqlQuery, DEBUG_MODIFICAR_TICKET);
            MYSQL_RES * result = mysql_store_result(mysql);
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row == NULL) {
                estado = atoi(row[0]);
                estadoExiste = true;
            }
            mysql_free_result(result);
        }

        if (not estadoExiste) {
            send2cliente(conexionActiva, ErrorException(ERR_TICKORENGNOEXIS, 
                                                        "Ticket no existe en el servidor"));
            return ;
        }

        ticket_anular_sol_t t;
        t.nticket = ticket_mod.idTicket;

        ejecutar_sql(mysql, "BEGIN", DEBUG_MODIFICAR_TICKET);
        bool resultado = false;
        {
            boost::uint8_t * buff2 = ticket_anular_sol_t2b(t);
            resultado = atenderPeticionAnularTicket_V_1(conexionActiva, buff2, mysql, false);
            delete [] buff2;
        }

        if (resultado) {
            GuardarTicketV2 guardarTicket;
            guardarTicket.numeroDeBonos = ticket_mod.numeroDeBonos;
            guardarTicket.montoBono = ticket_mod.montoBono;
            guardarTicket.numeroDeRenglones = ticket_mod.numeroDeRenglones;
            guardarTicket.crc = ticket_mod.crc;
            guardarTicket.ultimoTicket = ticket_mod.ultimoTicket;
            guardarTicket.nombreApostador = ticket_mod.nombreApostador;
            guardarTicket.ciApostador = ticket_mod.ciApostador;

            std::vector<boost::uint8_t> buff2 = guardarTicket.toRawBuffer();
            resultado = atenderPeticionGuardarTicket_V_3(conexionActiva, &buff2[0], mysql, false);

            if (resultado) {                
                ejecutar_sql(mysql, "COMMIT", DEBUG_MODIFICAR_TICKET);
                liberar_venta(conexionActiva, mysql, ticket_mod.idTicket);
            } else {
                ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
            }
        } else {
            ejecutar_sql(mysql, "ROLLBACK;", DEBUG_MODIFICAR_TICKET);
        }
    } else {
        confirmacion_guardados_V_3(conexionActiva, idAgente, conexionActiva.idTaquilla, mysql);
    }
    atenderPeticionCancelarPreventa(conexionActiva);
}

void
atenderPeticionRepetirTicket(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
	boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
	boost::uint32_t idAgente = conexionActiva.idAgente;
	boost::uint64_t agente_taquilla = idAgente;
	agente_taquilla = agente_taquilla << 32;
	agente_taquilla += idTaquilla;

	n_t_preventas_x_taquilla_t * preventas_taquilla =
	    (n_t_preventas_x_taquilla_t *) g_tree_lookup(t_preventas_x_taquilla, &agente_taquilla);
	if (preventas_taquilla == NULL) {
		return ;
	}

	TipoDeMontoPorAgencia * tipomonto_agencia = TipoDeMontoPorAgencia::get(idAgente);
	if (tipomonto_agencia == NULL) {
		return ;
	}

	GTree * t_sorteosm =
	    g_tree_new_full(uint8_t_comp_data_func, NULL, uint8_t_destroy_key,
	                     uint8_t_destroy_key);

	repetir_ticket_t repetirticket = repetir_ticket_b2t(buff);

	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Repitiendo el Ticket %u", repetirticket.idTicket);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}

	buff += REPETIR_TICKET_LON;
	for (int i = 0; i < repetirticket.numsorteos; ++i) {
		cambio_sorteo_t cambio_sorteo = cambio_sorteo_b2t(buff);

		boost::uint8_t * sorteo_old = new boost::uint8_t;
		*sorteo_old = cambio_sorteo.sorteo_old;

		boost::uint8_t * sorteo_new = new boost::uint8_t;
		*sorteo_new = cambio_sorteo.sorteo_new;

		g_tree_insert(t_sorteosm, sorteo_old, sorteo_new);
		if (i < repetirticket.numsorteos) {
			buff += CAMBIO_SORTEO_LON;
		}
	}
	bool ticketExiste = false;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Id FROM Tickets WHERE Id=%1% AND IdAgente=%2% AND NumTaquilla=%3%")
		% repetirticket.idTicket % conexionActiva.idAgente % unsigned(conexionActiva.idTaquilla);
		ejecutar_sql(mysql, sqlQuery, DEBUG_REPETIR_TICKET);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			ticketExiste = true;
		}
		mysql_free_result(result);
	}

	if (not ticketExiste) {
		send2cliente(conexionActiva, ERR_TICKORENGNOEXIS, NULL, 0);
		return ;
	}

	std::vector<n_a_renglon_t> renglones;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Numero, Monto, Tipo, IdSorteo FROM Renglones "
		                "WHERE IdTicket=%1% AND Tipo<>%2% AND Tipo<>%3% "
		                "and Monto > 0 ORDER BY Id ASC")
		% repetirticket.idTicket % unsigned(BONO) % unsigned(BONOTRIPLETAZO);
		ejecutar_sql(mysql, sqlQuery, DEBUG_REPETIR_TICKET);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result);
		        NULL != row;
		        row = mysql_fetch_row(result)) {
			n_a_renglon_t renglon;
			renglon.numero = atoi(row[0]);
			renglon.monto = atoi(row[1]);
			renglon.tipo = atoi(row[2]);
			renglon.idSorteo = atoi(row[3]);

			renglones.push_back(renglon);
		}
		mysql_free_result(result);
	}

	boost::uint8_t idZona = conexionActiva.idZona;

	n_t_zona_numero_restringido_t * zona_num_restringido =
	    (n_t_zona_numero_restringido_t *) g_tree_lookup(t_zona_numero_restringido, &idZona);
	if (zona_num_restringido == NULL) {
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "ADVERTENCIA: La zona <%u> no esta en memoria.", idZona);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}

	boost::uint16_t renglon = repetirticket.renglon - 1;
	RespuestaVentaRango respuestaVentaRango;
	respuestaVentaRango.begin = repetirticket.renglon;
	respuestaVentaRango.end = renglon + renglones.size();
	respuestaVentaRango.size = 0;
	boost::uint8_t * buffer = new boost::uint8_t[RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE * respuestaVentaRango.end];

	for (std::size_t j = 0, size = renglones.size(); j < size ; ++j) {
		renglon++;
		boost::uint16_t numero = renglones[j].numero;
		boost::uint32_t monto = renglones[j].monto;
		boost::uint8_t tipo = renglones[j].tipo;
		boost::uint8_t idSorteo = renglones[j].idSorteo;

		boost::uint8_t * psorteo = (boost::uint8_t *) g_tree_lookup(t_sorteosm, &idSorteo);
		if (psorteo != NULL) {
			idSorteo = *psorteo;
		}
		n_t_sorteo_numero_restringido_t * sorteo_num_restringido =
		    (n_t_sorteo_numero_restringido_t *) g_tree_lookup(zona_num_restringido->
		                                                         t_sorteo_numero_restringido,
		                                                         &idSorteo);
		TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
		if (tipomonto_sorteo == NULL) {
			send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo, sizeof(idSorteo));
			delete [] buffer;
			g_tree_destroy(t_sorteosm);
			return ;
		}
		Sorteo * sorteo = (Sorteo *) g_tree_lookup(G_sorteos, &idSorteo);
		verificar_sorteo(&conexionActiva, sorteo, idSorteo, false);
		if (not sorteo->isEstadoActivo()) {
			            
            RespuestaVentaRenglonModificado 
                respuestaVentaRenglonModificado(renglon , 0);
            
            std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
            
            memcpy(buffer + (respuestaVentaRango.size * 
                              RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), 
                   &buff2[0], buff2.size());  
                        
			respuestaVentaRango.size++;

			if ((tipo == TERMINAL or tipo == TERMINALAZO) and (not sorteo->isConSigno())) {
				numero = numero % 100;
				agregar_renglon_vacio_terminal(conexionActiva, renglon, numero, idSorteo,
				                                preventas_taquilla, idAgente, idTaquilla);
			} else if ((tipo == TRIPLE or tipo == TRIPLETAZO) and (not sorteo->isConSigno())) {
				numero = numero % 1000;
				agregar_renglon_vacio_triple(conexionActiva, renglon, numero, idSorteo,
				                              preventas_taquilla, idAgente, idTaquilla, monto);
			} else if (tipo == TERMINALAZO) {
				agregar_renglon_vacio_terminalazo(conexionActiva, renglon, numero, idSorteo,
				                                   preventas_taquilla, idAgente, idTaquilla);
			} else if (tipo == TRIPLETAZO) {
				agregar_renglon_vacio_tripletazo(conexionActiva, renglon, numero, idSorteo,
				                                  preventas_taquilla, idAgente, idTaquilla, monto);
			}
		} else {
			Preventa * preventa = NULL;
			if ((tipo == TERMINAL or tipo == TERMINALAZO) and (not sorteo->isConSigno())) {
				boost::uint8_t tipodemonto = tipomonto_sorteo->tipomonto_terminal;
				n_t_tipomonto_t * tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipodemonto);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];

					send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
					              sizeof(idSorteo));
					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipodemonto);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}
				numero = numero % 100;
				preventa =
				    preventa_terminal(conexionActiva, monto, numero, tipoDeMonto, zona_num_restringido,
				                       sorteo_num_restringido, renglon, idSorteo, preventas_taquilla,
				                       idAgente, idTaquilla, tipomonto_sorteo);

			} else	if ((tipo == TRIPLE or tipo == TRIPLETAZO)
				            and (not sorteo->isConSigno())) {
				boost::uint8_t tipodemonto = tipomonto_sorteo->tipomonto_triple;
				n_t_tipomonto_t * tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipodemonto);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];
					numero = numero % 100;

					send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
					              sizeof(idSorteo));
					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipodemonto);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}
				numero = numero % 1000;
				preventa =
				    preventa_triple(conexionActiva, monto, numero, tipoDeMonto, zona_num_restringido,
				                     sorteo_num_restringido, renglon, idSorteo, preventas_taquilla,
				                     idAgente, idTaquilla, tipomonto_sorteo);
			} else if (tipo == TERMINALAZO or tipo == TERMINAL) {
				boost::uint8_t tipodemonto = tipomonto_sorteo->tipomonto_terminal;
				n_t_tipomonto_t * tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTerminal(), &tipodemonto);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];

					send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
					              sizeof(idSorteo));
					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto terminal <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipodemonto);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}
				preventa =
				    preventa_terminalazo(conexionActiva, monto, numero, tipoDeMonto, zona_num_restringido,
				                          sorteo_num_restringido, renglon, idSorteo, preventas_taquilla,
				                          idAgente, idTaquilla, tipomonto_sorteo);

			} else if (tipo == TRIPLETAZO or tipo == TRIPLE) {
				boost::uint8_t tipodemonto = tipomonto_sorteo->tipomonto_triple;
				n_t_tipomonto_t * tipoDeMonto =
				    (n_t_tipomonto_t *) g_tree_lookup(sorteo->getTiposMontoTriple(), &tipodemonto);
				if (tipoDeMonto == NULL) {
					char mensaje[BUFFER_SIZE];

					send2cliente(conexionActiva, ERR_TIPOSMONTOXAGENNOEX, &idSorteo,
					              sizeof(idSorteo));
					sprintf(mensaje,
					         "ADVERTENCIA: El tipo de Monto triple <%u> no esta en Memoria 'debe reiniciar el servidor'.",
					         tipodemonto);
					log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
					return ;
				}
				preventa =
				    preventa_tripletazo(conexionActiva, monto, numero, tipoDeMonto, zona_num_restringido,
				                         sorteo_num_restringido, renglon, idSorteo, preventas_taquilla,
				                         idAgente, idTaquilla, tipomonto_sorteo);
			}

			if (NULL != preventa) {
                
                RespuestaVentaRenglonModificado 
                    respuestaVentaRenglonModificado(renglon , preventa->monto_preventa);
                
                std::vector<boost::uint8_t> buff2 = respuestaVentaRenglonModificado.toRawBuffer();
                
                memcpy(buffer + (respuestaVentaRango.size * 
                                  RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE), 
                       &buff2[0], buff2.size());
                
                
				respuestaVentaRango.size++;
			}
		}
	}

	boost::uint32_t lon1 = RespuestaVentaRango::CONST_RAW_BUFFER_SIZE;
	boost::uint32_t lon2 = respuestaVentaRango.size * RespuestaVentaRenglonModificado::CONST_RAW_BUFFER_SIZE;
	boost::uint32_t lon = lon1 + lon2;
	std::vector<boost::uint8_t> buff2 = respuestaVentaRango.toRawBuffer();
	boost::uint8_t * buffenviar = new boost::uint8_t[lon];
	memcpy(buffenviar, &buff2[0], lon1);
	memcpy(buffenviar + lon1, buffer, lon2);
	send2cliente(conexionActiva, RESPVENTAXRANGO, buffenviar, lon);
	
	delete [] buffenviar;
	delete [] buffer;
	g_tree_destroy(t_sorteosm);

}

void
atenderPeticionPagarTicket_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
	pagar_ticket_V_0_t pagar_ticket = pagar_ticket_V_0_b2t(buff);
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Pagando el Ticket %u", pagar_ticket.idTicket);
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}

	bool ticketExiste = false;
	boost::uint8_t estado = NORMAL;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT Id, Estado FROM Tickets WHERE Id=%1% AND IdAgente=%2%")
		% pagar_ticket.idTicket % conexionActiva.idAgente;
		ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			estado = atoi(row[1]);
			ticketExiste = true;
		}
		mysql_free_result(result);
	}

	if (not ticketExiste) {
		char mensaje[BUFFER_SIZE];
		send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
		sprintf(mensaje, "ADVERTENCIA - El Ticket %u No existe o no pertenece a la Agencia",
		         pagar_ticket.idTicket);
		log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
		return ;
	}

	if (estado == PAGADO) {
		send2cliente(conexionActiva, PAGAR_TICKET_V_0, NULL, 0);
		return ;
	}

	boost::uint8_t numeroTicketsPremiados = 0;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT COUNT(IdTicket) FROM Tickets_Premiados WHERE IdTicket=%1%")
		% pagar_ticket.idTicket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			numeroTicketsPremiados = atoi(row[0]);
		}
		mysql_free_result(result);
	}

	if (numeroTicketsPremiados == 0) {
		send2cliente(conexionActiva, ERR_TICKETNOELIM, NULL, 0);
		return ;
	}

	ejecutar_sql(mysql, "BEGIN", DEBUG_PAGAR_TICKET);
	{
		boost::format sqlQuery;
		sqlQuery.parse("UPDATE Tickets SET Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
		% unsigned(PAGADO) % time(NULL) % pagar_ticket.idTicket;
		ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
	}
	{
		boost::format sqlQuery;
		sqlQuery.parse("INSERT INTO Cambio_Ticket VALUES(%1%, NULL, %2%, %3%)")
		% pagar_ticket.idTicket % unsigned(estado) % conexionActiva.idUsuario;
		ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
	}
	ejecutar_sql(mysql, "COMMIT", DEBUG_PAGAR_TICKET);

	send2cliente(conexionActiva, PAGAR_TICKET_V_0, NULL, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void
atenderPeticionPagarTicket_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    pagar_ticket_V_1_t pagar_ticket = pagar_ticket_V_1_b2t(buff);
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Pagando el Ticket %u", pagar_ticket.idTicket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    bool ticketExiste = false;
    boost::uint8_t estado = NORMAL;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Id, Estado FROM Tickets WHERE Id=%1% AND IdAgente=%2%")
        % pagar_ticket.idTicket % conexionActiva.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            estado = atoi(row[1]);
            ticketExiste = true;
        }
        mysql_free_result(result);
    }

    if (not ticketExiste) {
        char mensaje[BUFFER_SIZE];
        send2cliente(conexionActiva, ERR_PAGAR_TICKET, NULL, 0);
        sprintf(mensaje, "ADVERTENCIA - El Ticket %u No existe o no pertenece a la Agencia",
                 pagar_ticket.idTicket);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        return ;
    }

    if (estado == PAGADO) {
        send2cliente(conexionActiva, PAGAR_TICKET_V_1, NULL, 0);
        return ;
    }

    boost::uint8_t numeroTicketsPremiados = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT COUNT(IdTicket) FROM Tickets_Premiados WHERE IdTicket=%1%")
        % pagar_ticket.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            numeroTicketsPremiados = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    if (numeroTicketsPremiados == 0) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA - El Ticket %u esta premiado",
                 pagar_ticket.idTicket);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_PAGAR_TICKET, NULL, 0);
        return ;
    }

    boost::uint8_t idSorteo = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Renglones WHERE IdTicket=%1% limit 1")
        % pagar_ticket.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result); 
        if (NULL != row) {
            idSorteo = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    if (idSorteo == 0) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA - El Ticket %u no tiene sorteo",
                 pagar_ticket.idTicket);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_PAGAR_TICKET, NULL, 0);
        return ;
    }
    
    std::auto_ptr<ProtocoloExterno::PremiarTicket> 
    premiarExterno = ProtocoloExterno::MensajeFactory::getInstance()
                   . crearPremiarTicket(mysql, pagar_ticket.idTicket);
    
    try {
        (*premiarExterno)(mysql);
        
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR al pagar ticket - %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_PAGAR_TICKET, NULL, 0);
        return ;      
    } catch (ProtocoloExterno::InvalidArgumentException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR al pagar ticket - %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_PAGAR_TICKET, NULL, 0);
        return ;                
    } catch (...) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR al pagar ticket - excepcion desconocida");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ERR_PAGAR_TICKET, NULL, 0);
        return ;
    }
        
    ejecutar_sql(mysql, "BEGIN", DEBUG_PAGAR_TICKET);
    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET "
                       "Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
        % unsigned(PAGADO) % time(NULL) % pagar_ticket.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO TicketsPagados VALUES(%1%, %2%, %3%, %4%)")
        % pagar_ticket.idTicket % pagar_ticket.montoNeto % pagar_ticket.impuesto
        % configuracion.impuesto_ganancia_fortuita;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Cambio_Ticket VALUES(%1%, NULL, %2%, %3%)")
        % pagar_ticket.idTicket % unsigned(estado) % conexionActiva.idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
    }
    ejecutar_sql(mysql, "COMMIT", DEBUG_PAGAR_TICKET);

    send2cliente(conexionActiva, PAGAR_TICKET_V_1, NULL, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void
atenderPeticionPagarTicket_V_2(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    pagar_ticket_V_1_t pagar_ticket = pagar_ticket_V_1_b2t(buff);
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Pagando el Ticket %u", pagar_ticket.idTicket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    bool ticketExiste = false;
    boost::uint8_t estado = NORMAL;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Id, Estado FROM Tickets WHERE Id=%1% AND IdAgente=%2%")
        % pagar_ticket.idTicket % conexionActiva.idAgente;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            estado = atoi(row[1]);
            ticketExiste = true;
        }
        mysql_free_result(result);
    }

    if (not ticketExiste) {
        char mensaje[BUFFER_SIZE];        
        send2cliente(conexionActiva, ErrorException(ERR_PAGAR_TICKET, 
                                                    "Ticket existe o no pertenece a la Agencia"));
        sprintf(mensaje, "ADVERTENCIA - El Ticket %u No existe o no pertenece a la Agencia",
                 pagar_ticket.idTicket);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        return ;
    }

    if (estado == PAGADO) {
        send2cliente(conexionActiva, PAGAR_TICKET_V_2, NULL, 0);
        return ;
    }

    boost::uint8_t numeroTicketsPremiados = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT COUNT(IdTicket) FROM Tickets_Premiados WHERE IdTicket=%1%")
        % pagar_ticket.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            numeroTicketsPremiados = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    if (numeroTicketsPremiados == 0) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA - El Ticket %u no esta premiado",
                 pagar_ticket.idTicket);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);     
        send2cliente(conexionActiva, ErrorException(ERR_PAGAR_TICKET, 
                                                    "Ticket no esta premiado"));        
        return ;
    }

    boost::uint8_t idSorteo = 0;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT IdSorteo FROM Renglones WHERE IdTicket=%1% limit 1")
        % pagar_ticket.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result); 
        if (NULL != row) {
            idSorteo = atoi(row[0]);
        }
        mysql_free_result(result);
    }

    if (idSorteo == 0) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ADVERTENCIA - El Ticket %u no tiene sorteo",
                 pagar_ticket.idTicket);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_PAGAR_TICKET, 
                                                    "Ticket no esta premiado"));
        return ;
    }
    
    std::auto_ptr<ProtocoloExterno::PremiarTicket> 
    premiarExterno = ProtocoloExterno::MensajeFactory::getInstance()
                   . crearPremiarTicket(mysql, pagar_ticket.idTicket);
    
    try {
        (*premiarExterno)(mysql);
        
    } catch (std::runtime_error const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR al pagar ticket - %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_PAGAR_TICKET, e.what()));
        return ;      
    } catch (ProtocoloExterno::InvalidArgumentException const& e) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR al pagar ticket - %s", e.what());
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_PAGAR_TICKET, e.what()));
        return ;                
    } catch (...) {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "ERROR al pagar ticket - excepcion desconocida");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        send2cliente(conexionActiva, ErrorException(ERR_PAGAR_TICKET, "error desconocido"));
        return ;
    }
        
    ejecutar_sql(mysql, "BEGIN", DEBUG_PAGAR_TICKET);
    {
        boost::format sqlQuery;
        sqlQuery.parse("UPDATE Tickets SET "
                       "Estado=%1%, FechaAct=FROM_UNIXTIME(%2%) WHERE Id=%3%")
        % unsigned(PAGADO) % time(NULL) % pagar_ticket.idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO TicketsPagados VALUES(%1%, %2%, %3%, %4%)")
        % pagar_ticket.idTicket % pagar_ticket.montoNeto % pagar_ticket.impuesto
        % configuracion.impuesto_ganancia_fortuita;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
    }
    {
        boost::format sqlQuery;
        sqlQuery.parse("INSERT INTO Cambio_Ticket VALUES(%1%, NULL, %2%, %3%)")
        % pagar_ticket.idTicket % unsigned(estado) % conexionActiva.idUsuario;
        ejecutar_sql(mysql, sqlQuery, DEBUG_PAGAR_TICKET);
    }
    ejecutar_sql(mysql, "COMMIT", DEBUG_PAGAR_TICKET);

    send2cliente(conexionActiva, PAGAR_TICKET_V_2, NULL, 0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void
atenderPeticionDescargarTicketPremiado_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    descargar_ticket_premiado_t descargarTicketPremiado = descargar_ticket_premiado_b2t(buff);
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Consultando si el Ticket %u esta premiado", descargarTicketPremiado.idTicket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    boost::uint32_t idTicket = descargarTicketPremiado.idTicket;
    bool ticketEsPremiado = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT SUM(Monto) FROM Tickets_Premiados WHERE IdTicket=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row && NULL != row[0]) {
            if (atoi(row[0]) > 0) {
                ticketEsPremiado = true;
            }
        }
        mysql_free_result(result);
    }

    if (not ticketEsPremiado) {    
        send2cliente(conexionActiva, ERR_DESCARGAR_TICKET_PREM, NULL, 0);
        return;    
    }
    
    bool ticketExiste = false;
    respuesta_descargar_ticket_premiado_V_0_t r;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), ApostadorNombre, Monto, Serial, ApostadorCI, Estado, Correlativo "
                       "FROM Tickets WHERE Estado<>%1% AND Id=%2%") 
        % unsigned(PAGADO) % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            ticketExiste = true;
            r.idTicket = idTicket;    
            r.fechaHora = atoi(row[0]) - 1800;
            memset(r.nombreApostador, ' ', NOMBRE_APOSTADOR_LENGTH);
            memcpy(r.nombreApostador, row[1], MIN(NOMBRE_APOSTADOR_LENGTH, strlen(row[1])));            
            r.monto = strtoul(row[2], NULL, 10);            
            memset(r.serial, '0', SERIAL_LENGTH);
            int const ROW_3_LON = MIN(SERIAL_LENGTH, strlen(row[3]));
            memcpy(r.serial + SERIAL_LENGTH - ROW_3_LON, row[3], ROW_3_LON);                        
            r.ciApostador = strtoul(row[4], NULL, 10);
            r.estado = atoi(row[5]);
            r.correlativo = strtoul(row[6], NULL, 10);
        }
        mysql_free_result(result);
    }
    
    if (not ticketExiste) {    
        send2cliente(conexionActiva, ERR_DESCARGAR_TICKET_PREM, NULL, 0);
        return;    
    }
    
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Renglones.Id, Renglones.IdSorteo, Renglones.Numero, "
                       "Renglones.Tipo, Renglones.Monto "
                       "FROM Renglones, Tickets_Premiados, Premios "
                       "WHERE Renglones.IdTicket = Tickets_Premiados.IdTicket and "
                       "Tickets_Premiados.IdSorteo = Premios.IdSorteo and "
                       "Tickets_Premiados.Fecha = Premios.Fecha and "
                       "Tickets_Premiados.IdSorteo = Renglones.IdSorteo and "
                       "((Renglones.Numero = Premios.Triple and Renglones.Tipo in(3,5,7,8)) or "
                       "(Renglones.Numero = SUBSTRING(Premios.Triple,2,2) and Renglones.Tipo = 2) or "
                       "(Renglones.Numero = CONCAT(SUBSTRING(LPAD(Premios.Triple,5,'00000'),1,2),SUBSTRING(LPAD(Premios.Triple,5,'00000'),4,2)) "
                       "and Renglones.Tipo = 4)) and "
                       "Renglones.IdTicket = %1%") 
                       % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        while (NULL != row) {
            renglon_premiado_t renglon;
            renglon.numeroRenglon = atoi(row[0]);
            renglon.idSorteo      = atoi(row[1]);
            renglon.numero        = atoi(row[2]);
            renglon.tipo          = atoi(row[3]);
            renglon.monto         = strtoul(row[4], NULL, 10);
            r.renglones.push_back(renglon);
            row = mysql_fetch_row(result);
        }          
        mysql_free_result(result);                
    }
    
    r.numeroRenglones = r.renglones.size();
    
    if (not r.numeroRenglones) {    
        send2cliente(conexionActiva, ERR_DESCARGAR_TICKET_PREM, NULL, 0);
        return;    
    }
        
    boost::uint8_t * buffer = respuesta_descargar_ticket_premiado_V_0_t2b(r);    
    
    send2cliente(conexionActiva, RESP_DESCARGAR_TICKET_PREM_V_0, buffer, 
                 RESPUESTA_DESCARGAR_TICKET_PREMIADO_V_0_LON + r.numeroRenglones * RENGLON_PREMIADO_LON);
    
    delete [] buffer;    
}

void
atenderPeticionDescargarTicketPremiado_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    descargar_ticket_premiado_t descargarTicketPremiado = descargar_ticket_premiado_b2t(buff);
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Consultando si el Ticket %u esta premiado", descargarTicketPremiado.idTicket);
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    boost::uint32_t idTicket = descargarTicketPremiado.idTicket;
    bool ticketEsPremiado = false;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT SUM(Monto) FROM Tickets_Premiados WHERE IdTicket=%1%")
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row && NULL != row[0]) {
            if (atoi(row[0]) > 0) {
                ticketEsPremiado = true;
            }
        }
        mysql_free_result(result);
    }

    if (not ticketEsPremiado) {    
        send2cliente(conexionActiva, ERR_DESCARGAR_TICKET_PREM, NULL, 0);
        return;    
    }
    
    bool ticketExiste = false;
    respuesta_descargar_ticket_premiado_V_1_t r;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT UNIX_TIMESTAMP(Fecha), ApostadorNombre, Monto, Serial, "
                       "ApostadorCI, Estado, Correlativo "
                       "FROM Tickets, TicketsPagados WHERE Id=%1%") 
        % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            ticketExiste = true;
            r.idTicket = idTicket;    
            r.fechaHora = atoi(row[0]) - 1800;
            memset(r.nombreApostador, ' ', NOMBRE_APOSTADOR_LENGTH);
            memcpy(r.nombreApostador, row[1], MIN(NOMBRE_APOSTADOR_LENGTH, strlen(row[1])));            
            r.monto = strtoul(row[2], NULL, 10);            
            memset(r.serial, '0', SERIAL_LENGTH);
            int const ROW_3_LON = MIN(SERIAL_LENGTH, strlen(row[3]));
            memcpy(r.serial + SERIAL_LENGTH - ROW_3_LON, row[3], ROW_3_LON);                        
            r.ciApostador = strtoul(row[4], NULL, 10);
            r.estado = atoi(row[5]);
            r.correlativo = strtoul(row[6], NULL, 10);
        }
        mysql_free_result(result);
    }    
    
    if (not ticketExiste) {    
        send2cliente(conexionActiva, ERR_DESCARGAR_TICKET_PREM, NULL, 0);
        return;    
    }
    
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Renglones.Id, Renglones.IdSorteo, Renglones.Numero, "
                       "Renglones.Tipo, Renglones.Monto "
                       "FROM Renglones, Tickets_Premiados, Premios "
                       "WHERE Renglones.IdTicket = Tickets_Premiados.IdTicket and "
                       "Tickets_Premiados.IdSorteo = Premios.IdSorteo and "
                       "Tickets_Premiados.Fecha = Premios.Fecha and "
                       "Tickets_Premiados.IdSorteo = Renglones.IdSorteo and "
                       "((Renglones.Numero = Premios.Triple and Renglones.Tipo in(3,5,7,8)) or "
                       "(Renglones.Numero = SUBSTRING(Premios.Triple,2,2) and Renglones.Tipo = 2) or "
                       "(Renglones.Numero = CONCAT(SUBSTRING(LPAD(Premios.Triple,5,'00000'),1,2),SUBSTRING(LPAD(Premios.Triple,5,'00000'),4,2)) "
                       "and Renglones.Tipo = 4)) and "
                       "Renglones.IdTicket = %1%") 
                       % idTicket;
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_TICKET);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        while (NULL != row) {
            renglon_premiado_t renglon;
            renglon.numeroRenglon = atoi(row[0]);
            renglon.idSorteo      = atoi(row[1]);
            renglon.numero        = atoi(row[2]);
            renglon.tipo          = atoi(row[3]);
            renglon.monto         = strtoul(row[4], NULL, 10);
            r.renglones.push_back(renglon);
            row = mysql_fetch_row(result);
        }          
        mysql_free_result(result);                
    }
    
    r.numeroRenglones = r.renglones.size();
    
    if (not r.numeroRenglones) {    
        send2cliente(conexionActiva, ERR_DESCARGAR_TICKET_PREM, NULL, 0);
        return;    
    }
        
    boost::uint8_t * buffer = respuesta_descargar_ticket_premiado_V_1_t2b(r);    
    
    send2cliente(conexionActiva, RESP_DESCARGAR_TICKET_PREM_V_1, buffer, 
                 RESPUESTA_DESCARGAR_TICKET_PREMIADO_V_1_LON + r.numeroRenglones * RENGLON_PREMIADO_LON);
    
    delete [] buffer;    
}

void
atenderPeticionSaldo_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    {
        char mensaje[BUFFER_SIZE];
        sprintf(mensaje, "Consultando saldo V1");
        log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
    }

    boost::uint32_t idAgente = conexionActiva.idAgente;
    boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
    boost::uint32_t fecha = consultar_saldo_b2t(buff).fecha;

    boost::uint16_t fechas[3];
    for (boost::uint8_t i = 0; i < 2; i++) {
        fechas[i] = fecha % 100;
        fecha = fecha / 100;
    }
    fechas[2] = fecha;

    bool hayResumen = false;
    resp_consultar_saldo_V_1_t r;
    {
        boost::format sqlQuery;
        sqlQuery.parse("SELECT SUM(Venta), SUM(Porcentaje), SUM(Premios), SUM(RetencionEstimada) "
                       "FROM Resumen_Ventas "
                        "WHERE IdAgente=%u AND NumTaquilla=%u AND Fecha='%d-%02d-%02d' "
                        "AND Tipo IN (%d, %d, %d, %d)")
        % idAgente % unsigned(idTaquilla) % fechas[2] % fechas[1] % fechas[0]
        % unsigned(TERMINAL) % unsigned(TRIPLE) % unsigned(TERMINALAZO) % unsigned(TRIPLETAZO);

        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_SALDO);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            hayResumen = true;
            r.venta = (row[0] != NULL)
                      ? strtoul(row[0], NULL, 10)
                      : 0;
            r.porcentaje = (row[1] != NULL)
                           ? strtoul(row[1], NULL, 10)
                           : 0;
            r.premios = (row[2] != NULL)
                        ? strtoul(row[2], NULL, 10)
                        : 0;
            r.retencionEstimada = (row[3] != NULL)
                                ? strtoul(row[3], NULL, 10)
                                : 0;                        
        }
        mysql_free_result(result);
    }

    if (not hayResumen) {
        send2cliente(conexionActiva, ERR_NOINFORMACION, NULL, 0);
        return ;
    }

    boost::uint8_t * buffer = resp_consultar_saldo_V_1_t2b(r);
    send2cliente(conexionActiva, SALDO_V_1, buffer, RESP_CONSULTAR_SALDO_V_1_LON);
    delete [] buffer;
}

void
atenderPeticionSaldo_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Consultando saldo V0");
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}

	boost::uint32_t idAgente = conexionActiva.idAgente;
	boost::uint8_t idTaquilla = conexionActiva.idTaquilla;
	boost::uint32_t fecha = consultar_saldo_b2t(buff).fecha;

	boost::uint16_t fechas[3];
	for (boost::uint8_t i = 0; i < 2; i++) {
		fechas[i] = fecha % 100;
		fecha = fecha / 100;
	}
	fechas[2] = fecha;

	bool hayResumen = false;
	resp_consultar_saldo_V_0_t r;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT SUM(Venta), SUM(Porcentaje), SUM(Premios) FROM Resumen_Ventas "
		                "WHERE IdAgente=%u AND NumTaquilla=%u AND Fecha='%d-%02d-%02d' "
		                "AND Tipo IN (%d, %d, %d, %d)")
		% idAgente % unsigned(idTaquilla) % fechas[2] % fechas[1] % fechas[0]
		% unsigned(TERMINAL) % unsigned(TRIPLE) % unsigned(TERMINALAZO) % unsigned(TRIPLETAZO);

		ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_SALDO);
		MYSQL_RES * result = mysql_store_result(mysql);
		MYSQL_ROW row = mysql_fetch_row(result);
		if (NULL != row) {
			hayResumen = true;
			r.venta = (row[0] != NULL)
			          ? strtoul(row[0], NULL, 10)
			          : 0;
			r.porcentaje = (row[1] != NULL)
			               ? strtoul(row[1], NULL, 10)
			               : 0;
			r.premios = (row[2] != NULL)
			            ? strtoul(row[2], NULL, 10)
			            : 0;
		}
		mysql_free_result(result);
	}

	if (not hayResumen) {
		send2cliente(conexionActiva, ERR_NOINFORMACION, NULL, 0);
		return ;
	}

	boost::uint8_t * buffer = resp_consultar_saldo_V_0_t2b(r);
	send2cliente(conexionActiva, SALDO_V_0, buffer, RESP_CONSULTAR_SALDO_V_0_LON);
	delete [] buffer;
}

boost::uint8_t
getFormaDePago(ConexionActiva& conexionActiva, boost::uint8_t idSorteo)
{
    TipoDeMontoPorAgencia * tipomonto_agencia = TipoDeMontoPorAgencia::get(conexionActiva.idAgente);
    if (tipomonto_agencia == NULL) {
        char mensaje[BUFFER_SIZE];
        send2cliente(conexionActiva, ERR_NOINFORMACION, NULL, 0);
        sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de montos.");
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        return 100;
    }

    TipoDeMonto * tipomonto_sorteo = tipomonto_agencia->getTipoDeMonto(idSorteo);
    if (tipomonto_sorteo == NULL) {
        char mensaje[BUFFER_SIZE];
        send2cliente(conexionActiva, ERR_NOINFORMACION, NULL, 0);
        sprintf(mensaje, "ADVERTENCIA: La Agencia no tiene tipo de monto para el sorteo <%u>.",
                 idSorteo);
        log_clientes(NivelLog::Bajo, &conexionActiva, mensaje);
        return 100;
    }
    return tipomonto_sorteo->forma_pago;
}

std::string
formatearFecha(boost::uint32_t fecha)
{
    boost::uint16_t fechas[3];
    for (boost::uint8_t i = 0; i < 2; i++) {
        fechas[i] = fecha % 100;
        fecha = fecha / 100;
    }
    fechas[2] = fecha;
    
    boost::format format;
    format.parse("%d-%02d-%02d") % fechas[2] % fechas[1] % fechas[0];
    return format.str();
}

struct PremioNoExisteException : public std::exception {};

resp_consultar_premios_V_0_t 
consultarPremios_V_0(ConexionActiva& conexionActiva, boost::uint8_t idSorteo, boost::uint32_t fecha, MYSQL* mysql)
{
    datos_premios_t datoPremio;
    bool premiosExisten = false;
    {
        std::string fechaStr = formatearFecha(fecha);
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Triple, Term1ro, Term2do, Term3ro, Comodin, MontoAdd "
                        "FROM Premios WHERE Fecha='%s' AND IdSorteo=%u")
        % fechaStr % unsigned(idSorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_PREMIOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            premiosExisten = true;
            for (int i = 0; i < 4; i++) {
                datoPremio.numeros[i] = (row[i] == NULL) ? -1 : atoi(row[i]);
            }
            datoPremio.comodin = (row[4] == NULL) ? 0 : atoi(row[4]);
            datoPremio.montoAdicional = (row[5] == NULL) ? 0 : atoi(row[5]);
        }
        mysql_free_result(result);
    }
    if (not premiosExisten) {
        throw PremioNoExisteException();
    }

    resp_consultar_premios_V_0_t r;
    r.idSorteo = idSorteo;
    r.triple = datoPremio.numeros[0];

    char cnumero[4];
    sprintf(cnumero, "%03d", r.triple);

    switch (getFormaDePago(conexionActiva, idSorteo)) {
        case 0: {
                r.term1 = datoPremio.numeros[1];
                r.term2 = datoPremio.numeros[2];
                r.term3 = datoPremio.numeros[3];
            }
            break;

        case 1: {
                char terminal[3];
                sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
                r.term1 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
                r.term2 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[2]);
                r.term3 = atoi(terminal);
            }
            break;

        case 2: {
                char terminal[3];
                sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
                r.term1 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
                r.term2 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
                r.term3 = atoi(terminal);
            }
            break;

        case 3: {
                char terminal[3];
                sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
                r.term1 = atoi(terminal);
                r.term2 = (r.term1 + 1 > 99 ? 00 : r.term1 + 1);
                r.term3 = (r.term1 - 1 < 00 ? 99 : r.term1 - 1);
            }
            break;

        case 4: {
                sprintf(cnumero, "%05d", r.triple);
                char terminal[3];
                sprintf(terminal,
                         "%c%c%c%c",
                         cnumero[0],
                         cnumero[1],
                         cnumero[3],
                         cnumero[4]);
                r.term1 = atoi(terminal);
                r.term2 = -1;
                r.term3 = -1;
            }
            break;
        case 100: 
        default:
            throw std::runtime_error("sorteo sin forma de pago para el agente");
            break;           
    }

    r.comodin = datoPremio.comodin;
    r.monto = datoPremio.montoAdicional;

    return r;
}


resp_consultar_premios_V_1_t 
consultarPremios_V_1(ConexionActiva& conexionActiva, boost::uint8_t idSorteo, boost::uint32_t fecha, MYSQL* mysql)
{
    datos_premios_t datoPremio;
    resp_consultar_premios_V_1_t r;
    bool premiosExisten = false;
    {
        std::string fechaStr = formatearFecha(fecha);
        boost::format sqlQuery;
        sqlQuery.parse("SELECT Triple, Term1ro, Term2do, Term3ro, Comodin, MontoAdd, SorteoNo "
                        "FROM Premios WHERE Fecha='%s' AND IdSorteo=%u")
        % fechaStr % unsigned(idSorteo);
        ejecutar_sql(mysql, sqlQuery, DEBUG_CONSULTAR_PREMIOS);
        MYSQL_RES * result = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (NULL != row) {
            premiosExisten = true;
            for (int i = 0; i < 4; i++) {
                datoPremio.numeros[i] = (row[i] == NULL) ? -1 : atoi(row[i]);
            }
            datoPremio.comodin = (row[4] == NULL) ? 0 : atoi(row[4]);
            datoPremio.montoAdicional = (row[5] == NULL) ? 0 : atoi(row[5]);
            
            memset(r.serialSorteo, 0, SERIAL_SORTEO_LON);
            if (row[6] != NULL) {
                strncpy(r.serialSorteo, row[6], SERIAL_SORTEO_LON);
            }
        }
        mysql_free_result(result);
    }
    if (not premiosExisten) {
        throw PremioNoExisteException();
    }
    
    r.idSorteo = idSorteo;
    r.triple = datoPremio.numeros[0];

    char cnumero[4];
    sprintf(cnumero, "%03d", r.triple);

    switch (getFormaDePago(conexionActiva, idSorteo)) {
        case 0: {
                r.term1 = datoPremio.numeros[1];
                r.term2 = datoPremio.numeros[2];
                r.term3 = datoPremio.numeros[3];
            }
            break;

        case 1: {
                char terminal[3];
                sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
                r.term1 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
                r.term2 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[2]);
                r.term3 = atoi(terminal);
            }
            break;

        case 2: {
                char terminal[3];
                sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
                r.term1 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
                r.term2 = atoi(terminal);
                sprintf(terminal, "%c%c", cnumero[0], cnumero[1]);
                r.term3 = atoi(terminal);
            }
            break;

        case 3: {
                char terminal[3];
                sprintf(terminal, "%c%c", cnumero[1], cnumero[2]);
                r.term1 = atoi(terminal);
                r.term2 = (r.term1 + 1 > 99 ? 00 : r.term1 + 1);
                r.term3 = (r.term1 - 1 < 00 ? 99 : r.term1 - 1);
            }
            break;

        case 4: {
                sprintf(cnumero, "%05d", r.triple);
                char terminal[3];
                sprintf(terminal,
                         "%c%c%c%c",
                         cnumero[0],
                         cnumero[1],
                         cnumero[3],
                         cnumero[4]);
                r.term1 = atoi(terminal);
                r.term2 = -1;
                r.term3 = -1;
            }
            break;
        case 100: 
        default:
            throw std::runtime_error("sorteo sin forma de pago para el agente");
            break;           
    }

    r.comodin = datoPremio.comodin;
    r.monto = datoPremio.montoAdicional;
    
    return r;
}

void
atenderPeticionPremios_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
	log_clientes(NivelLog::Detallado, &conexionActiva, "Consultando Premios: atenderPeticionPremios");

	consultar_premio_t consultarsaldo = consultar_premio_b2t(buff);
    
    try {
        resp_consultar_premios_V_0_t r = consultarPremios_V_0(conexionActiva, consultarsaldo.idSorteo,
                                                              consultarsaldo.fecha, mysql);
        boost::uint8_t * buffer = resp_consultar_premios_V_0_t2b(r);
        send2cliente(conexionActiva, PREMIOS_V_0, buffer, RESP_CONSULTAR_PREMIOS_V_0_LON);
        delete [] buffer;
    
    } catch (PremioNoExisteException const&) {
        send2cliente(conexionActiva, ERR_NOINFORMACION, NULL, 0);
        return ;
    } catch (std::runtime_error const& e) {
    }
}

void
atenderPeticionPremios_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    log_clientes(NivelLog::Detallado, &conexionActiva, "Consultando Premios: atenderPeticionPremios");

    consultar_premio_t consultarsaldo = consultar_premio_b2t(buff);
    
    try {
        resp_consultar_premios_V_1_t r = consultarPremios_V_1(conexionActiva, consultarsaldo.idSorteo,
                                                              consultarsaldo.fecha, mysql);
        boost::uint8_t * buffer = resp_consultar_premios_V_1_t2b(r);
        send2cliente(conexionActiva, PREMIOS_V_1, buffer, RESP_CONSULTAR_PREMIOS_V_1_LON);
        
        delete [] buffer;
    
    } catch (PremioNoExisteException const&) {
        send2cliente(conexionActiva, ERR_NOINFORMACION, NULL, 0);
        return ;
    } catch (std::runtime_error const& e) {
    }
}

void
atenderPeticionTodosPremios_V_0(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    log_clientes(NivelLog::Detallado, &conexionActiva, "Consultando Todos los Premios");
    consultar_todos_premios_t consulta = consultar_todos_premios_b2t(buff);
    
    boost::format sqlQuery;
    sqlQuery.parse("SELECT Sorteos.Id "
                   "FROM Sorteos, LimitesPorc, TiposMontoTriple, TiposMontoTerminal "
                   "WHERE Sorteos.Id=LimitesPorc.IdSorteo AND "
                   "LimitesPorc.IdMontoTriple=TiposMontoTriple.Id AND "
                   "LimitesPorc.IdMontoTerminal=TiposMontoTerminal.Id AND IdAgente=%1%")
    % conexionActiva.idAgente;
    
    ejecutar_sql(mysql, sqlQuery, DEBUG_TODOS_PREMIOS);
    MYSQL_RES * result = mysql_store_result(mysql);
    std::vector<boost::uint8_t> idSorteos;
    for (MYSQL_ROW row = mysql_fetch_row(result); row != NULL; row = mysql_fetch_row(result)) {
        idSorteos.push_back(atoi(row[0]));        
    }
    mysql_free_result(result);
    
    
    resp_consultar_todos_premios_V_0_t respuesta;
    for (std::size_t i = 0, size = idSorteos.size(); i < size; ++i) {
        try {
            respuesta.premios.push_back(consultarPremios_V_0(conexionActiva, idSorteos[i],
                                                             consulta.fecha, mysql));
        } catch (PremioNoExisteException const&) {
        } catch (std::runtime_error const& e) {
            return;
        }
    }
    
    respuesta.numeroPremios = respuesta.premios.size();    
    
    boost::uint8_t * buffer = resp_consultar_todos_premios_V_0_t2b(respuesta);
    std::size_t bufferSize = RESP_CONSULTAR_TODOS_PREMIOS_LON 
                           + (RESP_CONSULTAR_PREMIOS_V_0_LON * respuesta.numeroPremios);
    send2cliente(conexionActiva, TODOS_PREMIOS_V_0, buffer, bufferSize);
    
    delete [] buffer;
}

void
atenderPeticionTodosPremios_V_1(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
 
    log_clientes(NivelLog::Detallado, &conexionActiva, "Consultando Todos los Premios");
    consultar_todos_premios_t consulta = consultar_todos_premios_b2t(buff);
    
    boost::format sqlQuery;
    sqlQuery.parse("SELECT Sorteos.Id "
                   "FROM Sorteos, LimitesPorc, TiposMontoTriple, TiposMontoTerminal "
                   "WHERE Sorteos.Id=LimitesPorc.IdSorteo AND "
                   "LimitesPorc.IdMontoTriple=TiposMontoTriple.Id AND "
                   "LimitesPorc.IdMontoTerminal=TiposMontoTerminal.Id AND IdAgente=%1%")
    % conexionActiva.idAgente;
    
    ejecutar_sql(mysql, sqlQuery, DEBUG_TODOS_PREMIOS);
    MYSQL_RES * result = mysql_store_result(mysql);
    std::vector<boost::uint8_t> idSorteos;
    for (MYSQL_ROW row = mysql_fetch_row(result); row != NULL; row = mysql_fetch_row(result)) {
        idSorteos.push_back(atoi(row[0]));        
    }
    mysql_free_result(result);
    
    
    resp_consultar_todos_premios_V_1_t respuesta;
    for (std::size_t i = 0, size = idSorteos.size(); i < size; ++i) {
        try {
            respuesta.premios.push_back(consultarPremios_V_1(conexionActiva, idSorteos[i],
                                                             consulta.fecha, mysql));
        } catch (PremioNoExisteException const&) {
        } catch (std::runtime_error const& e) {
            return;
        }
    }
    
    respuesta.numeroPremios = respuesta.premios.size();    
    
    boost::uint8_t * buffer = resp_consultar_todos_premios_V_1_t2b(respuesta);
    std::size_t bufferSize = RESP_CONSULTAR_TODOS_PREMIOS_LON 
                           + (RESP_CONSULTAR_PREMIOS_V_1_LON * respuesta.numeroPremios);
    send2cliente(conexionActiva, TODOS_PREMIOS_V_1, buffer, bufferSize);
    
    delete [] buffer;
}

void
atenderPeticionConsultarPremios(ConexionActiva& conexionActiva, boost::uint8_t * buff)
{
    DataBaseConnetion dbConnetion;
    MYSQL* mysql = dbConnetion.get();
                
	{
		char mensaje[BUFFER_SIZE];
		sprintf(mensaje, "Resumen de tickets Premiados");
		log_clientes(NivelLog::Detallado, &conexionActiva, mensaje);
	}

	boost::uint32_t idAgente = conexionActiva.idAgente;
	boost::uint8_t idTaquilla = conexionActiva.idTaquilla;

	boost::uint32_t fecha = consultar_saldo_b2t(buff).fecha;
	boost::uint16_t fechas[3];
	for (boost::uint32_t i = 0; i < 2; i++) {
		fechas[i] = fecha % 100;
		fecha = fecha / 100;
	}
	fechas[2] = fecha;


	std::vector<std::pair<boost::uint32_t, boost::uint32_t> > premiosPorTicket;
	{
		boost::format sqlQuery;
		sqlQuery.parse("SELECT SUM(Monto), IdTicket FROM Tickets_Premiados "
		                "WHERE IdAgente=%u AND NumTaquilla=%u AND Fecha='%d-%02d-%02d' "
		                "GROUP BY IdTicket")
		% idAgente % unsigned(idTaquilla) % fechas[2] % fechas[1] % fechas[0];
		ejecutar_sql(mysql, sqlQuery, DEBUG_RESUMEN_TICKETS_PREMIADOS);
		MYSQL_RES * result = mysql_store_result(mysql);
		for (MYSQL_ROW row = mysql_fetch_row(result);
		        NULL != row;
		        row = mysql_fetch_row(result)) {
			boost::uint32_t monto = (row[0] != NULL)
			                   ? strtoul(row[0], NULL, 10)
			                   : 0;
			boost::uint32_t idTicket = strtoul(row[1], NULL, 10);

			premiosPorTicket.push_back(std::make_pair(monto, idTicket));
		}
		mysql_free_result(result);
	}

	if (premiosPorTicket.size() == 0) {
		boost::uint8_t cuantos = 0;
		send2cliente(conexionActiva, CONSULTAR_PREMIOS, &cuantos, 1);
		return ;
	}

	boost::uint8_t cuantos = 0;
	boost::uint8_t * buffer = NULL;
	for (std::size_t size = premiosPorTicket.size(); cuantos < size ; ++cuantos) {
		boost::uint32_t monto = premiosPorTicket[cuantos].first;
		boost::uint32_t idTicket = premiosPorTicket[cuantos].second;

		if (cuantos == 0) {
			boost::uint32_t lon1 = sizeof(idTicket);
			boost::uint32_t lon2 = sizeof(monto);
			boost::uint32_t lon = lon1 + lon2;
			buffer = new boost::uint8_t[lon];
			memcpy(&buffer[0], &idTicket, lon1);
			memcpy(&buffer[lon1], &monto, lon2);
		} else {
			boost::uint32_t lon2 = sizeof(idTicket);
			boost::uint32_t lon3 = sizeof(monto);
			boost::uint32_t lon1 = (lon2 + lon3) * cuantos;
			boost::uint32_t lon = lon1 + lon2 + lon3;
			boost::uint8_t * tmpbuff = new boost::uint8_t[lon];
			memcpy(&tmpbuff[0], buffer, lon1);
			memcpy(&tmpbuff[lon1], &idTicket, lon2);
			memcpy(&tmpbuff[lon1 + lon2], &monto, lon3);
			delete [] buffer;
			buffer = tmpbuff;
		}
	}

	boost::uint32_t lon2 = sizeof(boost::uint32_t); // monto
	boost::uint32_t lon3 = sizeof(boost::uint32_t); // idTicket
	boost::uint32_t lon1 = lon2 + lon3;
	lon2 = lon1 * cuantos;
	lon1 = sizeof(cuantos);
	boost::uint32_t lon = lon1 + lon2;
	boost::uint8_t * tmpbuff = new boost::uint8_t[lon];
	memcpy(&tmpbuff[0], &cuantos, lon1);
	memcpy(&tmpbuff[lon1], buffer, lon2);
	delete [] buffer;
	send2cliente(conexionActiva, CONSULTAR_PREMIOS, tmpbuff, lon);
	delete [] tmpbuff;
}
