#ifndef ZEUS_TIPOSDEMONTODEAGENCIA_H_
#define ZEUS_TIPOSDEMONTODEAGENCIA_H_

#include <boost/noncopyable.hpp>
#include <semaphore.h>
#include <map>
#include <boost/cstdint.hpp>
#include <ace/Recursive_Thread_Mutex.h>

struct TipoDeMonto : boost::noncopyable
{
    boost::uint8_t tipomonto_triple;
    boost::uint8_t tipomonto_terminal;
    boost::uint8_t forma_pago;
    double porctriple;
    double porcterminal;
    double limdisctrip;
    double limdiscterm;
    boost::uint8_t comodin;
    boost::uint32_t montoAdicional;
};

class TipoDeMontoPorAgencia : boost::noncopyable
{
public:
    TipoDeMontoPorAgencia();   
    
    struct MutexGuard
    {
        MutexGuard();
        ~MutexGuard();
    };
    
    static TipoDeMontoPorAgencia* get(boost::uint32_t idAgente);
    static void add(boost::uint32_t idAgente, TipoDeMontoPorAgencia* tipoDeMontoDeAgencia);
    
    TipoDeMonto* getTipoDeMonto(boost::uint8_t idSorteo);
    void updateTipoDeMonto(boost::uint8_t idSorteo, TipoDeMonto* tipoDeMonto);
    
private:   
    typedef std::map<boost::uint8_t, TipoDeMonto*> TiposDeMonto;
    TiposDeMonto tiposDeMonto;
    
    typedef std::map<boost::uint32_t, TipoDeMontoPorAgencia*> TipoDeMontoPorAgencias;
    static TipoDeMontoPorAgencias tipoDeMontoPorAgencias;
    static ACE_Recursive_Thread_Mutex tipoDeMontoPorAgenciasMutex;
   
    
};

#endif /*ZEUS_TIPOSDEMONTODEAGENCIA_H_*/
