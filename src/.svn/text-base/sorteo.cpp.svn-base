#include <sorteo.h>
#include <utils.h>
#include <ventas.h>
#include <stdexcept>
#include <ace/SOCK_Stream.h>

GTree *G_sorteos = NULL;

Sorteo::Sorteo(boost::uint8_t id)
    : id_(id)
    , estadoActivo_(false)
    , estadoInicialActivo_(false)
    , horaCierre_(0)
    , diaActual_(diadehoy())
    , conSigno_(false)
    , tiposMontoTriple_(g_tree_new(uint8_t_comp_func))
    , tiposMontoTerminal_(g_tree_new(uint8_t_comp_func))
{
}

Sorteo::Sorteo(boost::uint8_t id, bool estadoActivo, boost::int32_t horaCierre, 
               bool conSigno)
    : id_(id)
    , estadoActivo_(estadoActivo)
    , estadoInicialActivo_(estadoActivo_)
    , horaCierre_(horaCierre)
    , diaActual_(diadehoy())
    , conSigno_(conSigno)
    , tiposMontoTriple_(g_tree_new(uint8_t_comp_func))
    , tiposMontoTerminal_(g_tree_new(uint8_t_comp_func))
{
}

boost::uint8_t 
Sorteo::getId() const
{
    return id_;
}

bool 
Sorteo::isEstadoActivo() const
{
    return estadoActivo_;
}

bool 
Sorteo::isEstadoInicialActivo() const
{
    return estadoInicialActivo_;
}

boost::int32_t 
Sorteo::getHoraCierre() const
{
    return horaCierre_;
}

boost::uint8_t 
Sorteo::getDiaActual() const
{
    return diaActual_;
}

bool 
Sorteo::isConSigno() const
{
    return conSigno_;
}

GTree* 
Sorteo::getTiposMontoTriple() const
{
    return tiposMontoTriple_;
}

GTree* 
Sorteo::getTiposMontoTerminal() const
{
    return tiposMontoTerminal_;
}

void 
Sorteo::setHoraCierre(boost::int32_t horaCierre)
{
    horaCierre_ = horaCierre;
}

void 
Sorteo::setEstadoActivo(bool estadoActivo)
{    
    if (!estadoInicialActivo_) return;
    
    estadoActivo_ = estadoActivo;      
}
