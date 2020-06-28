#include <ConexionActiva.h>
    
ConexionActiva::ConexionActiva()
    : sd(0)
    , idTaquilla(0)
    , idAgente(0)
    , idUsuario(0)
    , idZona(0)
    , mutex()
    , login()
    , usarProductos(false)
    , ipAddress()
{
    memset(login, 0, 12);
}
 
ConexionActiva::~ConexionActiva()
{
}

std::auto_ptr<ConexionActiva> 
ConexionActiva::create(int sd, char const* ipAddress_)
{
    std::auto_ptr<ConexionActiva> result = create(sd);
    result->ipAddress = ipAddress_;   
    return result;
}

std::auto_ptr<ConexionActiva> 
ConexionActiva::create(int sd)
{
    ConexionActiva* conexionActiva = new ConexionActiva;
    std::auto_ptr<ConexionActiva> result(conexionActiva);
    
    conexionActiva->sd = sd;
    return result;
}
