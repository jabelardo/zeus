#include "DatabaseAccessor.h"
#include <cassert>
#include <boost/format.hpp>
#include <boost/static_assert.hpp>
#include "utils.h"
#include <ProtocoloZeus/Protocolo.h>

using namespace ProtocoloZeus;

namespace database
{

DatabaseAccessor::DatabaseAccessor( MYSQL * dbConnection )
	: dbConnection_( dbConnection )
{
	assert( dbConnection_ && "puntero nulo" );
}

DatabaseAccessor::~DatabaseAccessor()
{
}

template <class T>
T extraer( MYSQL_ROW )
{
	//BOOST_STATIC_ASSERT( false );
	T result;
	return result;
}

template <typename T>
T DatabaseAccessor::obtenerEntidadDeQuery( boost::format const& query )
{
	ejecutar_sql( dbConnection_, query, /*debug_*/ false );
	MYSQL_RES * mySqlResult = mysql_store_result( dbConnection_ );
	MYSQL_ROW row = mysql_fetch_row( mySqlResult );

	T result;
	if ( NULL != row ) {
		result = extraer<T>( row );
	}
	mysql_free_result( mySqlResult );
	return result;
}

//-----[AGENTES]---------------------------------------------------------------
std::string const
SELECT_FROM_AGENTES = "SELECT "
                      "IdTipoAgente, "
                      "Id, Nombre, Direccion, Estado, Telefono, NroAbonado, "
                      "IdRecogedor, IdMedio, IdZona, "
                      "Representante, CI, Celular "
                      "FROM Agentes ";
                      
template <>
dominio::Agente extraer( MYSQL_ROW row )
{
	assert( row != NULL );

	//FIXME: este codigo es sensible (no es seguro) a exepciones
	
	dominio::Agente result;
	dominio::Agente* agente = &result;

	//if ( negocio::Agente::tipoAgencia == atoi( row[ 0 ] ) ) {
	//	agente = new negocio::Agencia;
	//} else {
	//	agente = new negocio::Recogedor;
	//}
	agente->idAgente( atoi( row[ 1 ] ) );
	agente->nombre( getString( row[ 2 ], NOMBRE_AGENTE_V_1_LON ) );
	agente->direccion( getString( row[ 3 ], DIRECCION_LON ) );
	//agente->estado( negocio::Agente::Estado( atoi( row[ 4 ] ) ) );
	agente->telefono( getString( row[ 5 ], TELEFONO_LON ) );
	agente->identificadorDeMedio( getString( row[ 6 ], 30 ) );

	// IdRecogedor puede ser nulo
	agente->idPadre( ( row[ 7 ] != NULL ) ? atoi( row[ 7 ] ) : 0 );
	//agente->medio( negocio::Medio( atoi( row[ 8 ] ) ) );
	//agente->zona( negocio::Zona( atoi( row[ 9 ] ) ) );

	/*agente->representante(
	    negocio::Persona( getString( row[ 10 ], NOMBRE_LON ),
	                      atoi( row[ 11 ] ),
	                      // Celular puede ser nulo
	                      ( row[ 12 ] != NULL )
	                      ? getString( row[ 12 ], TELEFONO_LON )
	                      : "NO TIENE" ) );*/
	return result;	
}                      
                      
dominio::Agente DatabaseAccessor::obtenerAgentePorId( boost::uint32_t idAgente )
{
	boost::format query;
	query.parse( SELECT_FROM_AGENTES + "WHERE Id=%1%" ) % idAgente;
	return obtenerEntidadDeQuery<dominio::Agente>( query );
}

}
