#ifndef SORTEO_H_
#define SORTEO_H_

#include <glib.h>
#include <boost/cstdint.hpp>
#include <mysql/mysql.h>
#include <memory>
#include <utility>
#include <vector>
#include <ConexionActiva.h>

/*
 * Nodo del arbol de los sorteos
 */
class Sorteo
{
friend void cargar_sorteos();

private:
    Sorteo(boost::uint8_t id);
        
public:    
    
    Sorteo(boost::uint8_t id, bool estadoActivo, boost::int32_t horaCierre, bool conSigno);
    
    boost::uint8_t getId() const;
    bool isEstadoActivo() const;
    bool isEstadoInicialActivo() const;
    boost::int32_t getHoraCierre() const;
    boost::uint8_t getDiaActual() const;
    bool isConSigno() const;
    GTree* getTiposMontoTriple() const;
    GTree* getTiposMontoTerminal() const;
    
    void setHoraCierre(boost::int32_t horaCierre);
    
    void setEstadoActivo(bool estadoActivo = true);    
            
private:
    boost::uint8_t id_;
    bool estadoActivo_;
    bool estadoInicialActivo_;
    boost::int32_t horaCierre_;
    boost::uint8_t diaActual_;
    bool conSigno_;
    GTree *tiposMontoTriple_;   /* llave = idtipomontotriple */
    GTree *tiposMontoTerminal_; /* llave = idtipomontoterminal */
};

extern GTree *G_sorteos;      /* llave = idSorteo */

#endif /*SORTEO_H_*/
