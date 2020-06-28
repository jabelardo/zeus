#ifndef DATABASEACCESSOR_H_
#define DATABASEACCESSOR_H_

#include <mysql/mysql.h>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <dominio/Agente.h>

namespace database
{

class DatabaseAccessor
{
public:
	DatabaseAccessor(MYSQL * dbConnection);
	virtual ~DatabaseAccessor();
	
	//-----[AGENTES]-----------------------------------------------------------
	dominio::Agente obtenerAgentePorId( boost::uint32_t idAgente );
	
private:
	template <typename T> 
	T obtenerEntidadDeQuery( boost::format const& query );
	
	MYSQL * dbConnection_;
};

}

#endif /*DATABASEACCESSOR_H_*/
