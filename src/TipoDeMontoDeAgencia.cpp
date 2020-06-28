#include <TipoDeMontoDeAgencia.h>

TipoDeMontoPorAgencia::TipoDeMontoPorAgencias TipoDeMontoPorAgencia::tipoDeMontoPorAgencias;
ACE_Recursive_Thread_Mutex TipoDeMontoPorAgencia::tipoDeMontoPorAgenciasMutex;

TipoDeMontoPorAgencia::TipoDeMontoPorAgencia()
{
}

TipoDeMontoPorAgencia::MutexGuard::MutexGuard()
{
    tipoDeMontoPorAgenciasMutex.acquire();
}
 
TipoDeMontoPorAgencia::MutexGuard::~MutexGuard()
{
    tipoDeMontoPorAgenciasMutex.release();
}
    
TipoDeMontoPorAgencia* 
TipoDeMontoPorAgencia::get(boost::uint32_t idAgente)
{
    MutexGuard g;
    TipoDeMontoPorAgencias::iterator iter = tipoDeMontoPorAgencias.find(idAgente);
    return (iter == tipoDeMontoPorAgencias.end()) ? NULL : iter->second;
}

void 
TipoDeMontoPorAgencia::add(boost::uint32_t idAgente, TipoDeMontoPorAgencia* tipoDeMontoDeAgencia)
{
    MutexGuard g;
    tipoDeMontoPorAgencias.insert(std::make_pair(idAgente, tipoDeMontoDeAgencia));
}

TipoDeMonto* 
TipoDeMontoPorAgencia::getTipoDeMonto(boost::uint8_t idSorteo)
{
    TiposDeMonto::iterator iter = tiposDeMonto.find(idSorteo);
    return (iter == tiposDeMonto.end()) ? NULL : iter->second;
}

void 
TipoDeMontoPorAgencia::updateTipoDeMonto(boost::uint8_t idSorteo, TipoDeMonto* tipoDeMonto)
{    
    TiposDeMonto::iterator iter = tiposDeMonto.find(idSorteo);
    if (iter == tiposDeMonto.end()) {
        tiposDeMonto.insert(std::make_pair(idSorteo, tipoDeMonto));
    } else {
        delete iter->second;
        iter->second = tipoDeMonto;
    }
}
