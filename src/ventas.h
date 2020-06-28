#ifndef VENTAS_H
#define VENTAS_H

#include <global.h>
#include <vector>
#include <ConexionActiva.h>
#include <ProtocoloZeus/Protocolo.h>

void liberar_venta(ConexionActiva& conexionActiva, MYSQL * mysql_con, 
    boost::uint32_t ticket);

void atenderPeticionCancelarPreventa(ConexionActiva& conexionActiva);

void atenderPeticionVenta(ConexionActiva& conexionActiva, boost::uint8_t * buffer);

void atenderPeticionEliminarRenglon(ConexionActiva& conexionActiva, 
    ProtocoloZeus::renglon_t renglon);

void atenderPeticionTerminales(ConexionActiva& conexionActiva, boost::uint8_t * buff);

void atenderPeticionRepetirJugada(ConexionActiva& conexionActiva, boost::uint8_t * buff);

bool atenderPeticionGuardarTicket_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff, MYSQL * mysql_con, bool manejarTransaccion);
    
bool atenderPeticionGuardarTicket_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff, MYSQL * mysql_con, bool manejarTransaccion);    

bool atenderPeticionGuardarTicket_V_2(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff, MYSQL * mysql_con, bool manejarTransaccion);    

bool atenderPeticionGuardarTicket_V_3(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff, MYSQL * mysql_con, bool manejarTransaccion); 
            
void atenderPeticionModificarTicket_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionModificarTicket_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionModificarTicket_V_2(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionModificarTicket_V_3(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);
        
bool atenderPeticionAnularTicket_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff, MYSQL * mysql_con,  bool manejarTransaccion);

bool atenderPeticionAnularTicket_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff, MYSQL * mysql_con,  bool manejarTransaccion);
    
void atenderPeticionRepetirTicket(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionPagarTicket_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionPagarTicket_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);    

void atenderPeticionPagarTicket_V_2(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);    
        
void atenderPeticionDescargarTicketPremiado_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionDescargarTicketPremiado_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);
        
void atenderPeticionSaldo_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionSaldo_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);
    
void atenderPeticionPremios_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionPremios_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionConsultarPremios(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

unsigned long long getFechaHora(std::time_t t = std::time(0));
    
void atenderPeticionTodosPremios_V_0(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);

void atenderPeticionTodosPremios_V_1(ConexionActiva& conexionActiva, 
    boost::uint8_t * buff);
            
#endif
