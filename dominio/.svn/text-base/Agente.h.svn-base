#ifndef AGENTE_H_
#define AGENTE_H_

#include <boost/cstdint.hpp>
#include <string>

namespace dominio
{

class Agente
{
public:
	Agente();
	virtual ~Agente();
	
	boost::uint32_t idAgente() const;
	void idAgente( boost::uint32_t idAgente );

	std::string nombre() const;
	void nombre( std::string const& nombre );

	std::string direccion() const;
	void direccion( std::string const& direccion );

	//Agente::Estado estado() const;
	//void estado( Agente::Estado estado );

	std::string telefono() const;
	void telefono( std::string const& telefono );

	std::string identificadorDeMedio() const;
	void identificadorDeMedio( std::string const& identificadorDeMedio );

	//Persona representante() const;
	//void representante( Persona const& representante );

	//Zona zona() const;
	//void zona( Zona const& zona );

	//Medio medio() const;
	//void medio( Medio const& medio );

	boost::uint32_t idPadre() const;
	void idPadre( boost::uint32_t idPadre );
	
private:
	boost::uint32_t idAgente_;
	std::string nombre_;
	std::string direccion_;
	std::string telefono_;
	std::string identificadorDeMedio_;
	boost::uint32_t idPadre_;
};

}

#endif /*AGENTE_H_*/
