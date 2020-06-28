#include "Agente.h"

namespace dominio
{

Agente::Agente()
		: idAgente_( 0 )
		, nombre_( "" )
		, direccion_( "" )
		, telefono_( "" )
		, identificadorDeMedio_( "" )
		, idPadre_( 0 )
{
}

Agente::~Agente()
{
}

boost::uint32_t
Agente::idAgente() const
{
	return idAgente_;
}

std::string
Agente::nombre() const
{
	return nombre_;
}

std::string
Agente::direccion() const
{
	return direccion_;
}

std::string
Agente::telefono() const
{
	return telefono_;
}

std::string
Agente::identificadorDeMedio() const
{
	return identificadorDeMedio_;
}

boost::uint32_t
Agente::idPadre() const
{
	return idPadre_;
}

void
Agente::idAgente( boost::uint32_t idAgente )
{
	idAgente_ = idAgente;
}

void
Agente::nombre( std::string const& nombre )
{
	nombre_ = nombre;
}

void
Agente::direccion( std::string const& direccion )
{
	direccion_ = direccion;
}

void
Agente::telefono( std::string const& telefono )
{
	telefono_ = telefono;
}

void
Agente::identificadorDeMedio( std::string const& identificadorDeMedio )
{
	identificadorDeMedio_ = identificadorDeMedio;
}

void
Agente::idPadre( boost::uint32_t idPadre )
{
	//if ( padre_ != NULL ) {
	//	desvincularPadre();
	//}
	idPadre_ = idPadre;
}

}

