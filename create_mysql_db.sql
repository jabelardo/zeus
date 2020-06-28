START TRANSACTION;

USE mysql;
INSERT INTO user VALUES ('192.168.%.%', 'zeus','701850a5037d65fc','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','N','','','','',0,0,0);

INSERT INTO db VALUES ('192.168.%.%','zeus','zeus','Y','Y','Y','Y','Y','Y','N','Y','Y','Y','Y','Y');

CREATE DATABASE zeus;
USE zeus;

CREATE TABLE Medios
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Descripcion VARCHAR(15) NOT NULL,
   PuedeActualizar TINYINT UNSIGNED DEFAULT 0 NOT NULL,
   PRIMARY KEY (Id)
)TYPE=INNODB;

CREATE TABLE Zonas
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Nombre VARCHAR(30) NOT NULL,
   Descripcion VARCHAR(50) NOT NULL,
   PRIMARY KEY (Id)
)TYPE=INNODB;

CREATE TABLE TiposAgente
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Descripcion VARCHAR(15) NOT NULL,
   UNIQUE (Descripcion),
   PRIMARY KEY (Id)
)TYPE=INNODB;


CREATE TABLE Agentes
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   Nombre VARCHAR(62) NOT NULL,
   -- esto deberia formar una tabla aparte de Representantes
   Representante VARCHAR(40) NOT NULL,
   CI INT NOT NULL,
   Telefono VARCHAR(16) NOT NULL,
   Celular VARCHAR(16),
   Direccion VARCHAR(100) NOT NULL,
   Estado TINYINT UNSIGNED NOT NULL,
   IdRecogedor INT UNSIGNED,
   IdMedio TINYINT UNSIGNED NOT NULL,
   IdTipoAgente TINYINT UNSIGNED NOT NULL,
   IdZona TINYINT UNSIGNED NOT NULL,
   NroAbonado VARCHAR(30) NOT NULL,
   Rif VARCHAR(12) NOT NULL,
   CupoMaximo INT UNSIGNED NOT NULL DEFAULT 0,
   UsarProductos TINYINT UNSIGNED NOT NULL DEFAULT 0,
   IdBanca INT UNSIGNED NOT NULL default 0,
   Email varchar(40) not null default '',
   PRIMARY KEY (Id),
   INDEX IDX_Agentes_Recogedor (IdRecogedor),
   INDEX IDX_Agentes_Medio (IdMedio),
   INDEX IDX_Agentes_TipoAgente (IdTipoAgente),
   INDEX IDX_Agentes_Zonas (IdZona),
   INDEX IDX_Agentes_IdBanca (IdBanca),
   FOREIGN KEY (IdRecogedor) REFERENCES Agentes (Id),
   FOREIGN KEY (IdMedio) REFERENCES Medios (Id),
   FOREIGN KEY (IdTipoAgente) REFERENCES TiposAgente (Id),
   FOREIGN KEY (IdZona) REFERENCES Zonas (Id)
)TYPE=INNODB;

--> INICIO NUEVAS TABLAS <--
CREATE TABLE NCC01
(
   IdBanca INT UNSIGNED NOT NULL,
   Valor   INT UNSIGNED NOT NULL,
   PRIMARY KEY (IdBanca),
   FOREIGN KEY (IdBanca) REFERENCES Agentes (IdBanca)
)TYPE=INNODB;
--> FIN NUEVAS TABLAS <--

CREATE TABLE Taquillas
(
   Serial INT UNSIGNED DEFAULT 0 NOT NULL,
   IdAgente INT UNSIGNED NOT NULL,
   Numero TINYINT UNSIGNED NOT NULL,
   Hilo INT UNSIGNED,
   Hora_Ult_Conexion DATETIME,
   Hora_Ult_Desconexion DATETIME,
   PRIMARY KEY (IdAgente,Numero),
   INDEX IDX_Taquillas_Agente (IdAgente),
   INDEX IDX_Agente_Numero(IdAgente,Numero),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id)
)TYPE=INNODB;

Create table Protocolos
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Nombre Varchar(40) NOT NULL,
   PRIMARY KEY (Id),
   UNIQUE (Nombre)
)TYPE=INNODB;
insert into Protocolos values (1,'NINGUNO');
insert into Protocolos values (2,'SELCO');
insert into Protocolos values (3,'TRANSAX');

-- alter table Loterias add CodigoTelecom INT UNSIGNED default 720 NOT NULL
create table Loterias 
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   Nombre VARCHAR(40) NOT NULL,   
   IdProtocolo TINYINT UNSIGNED DEFAULT 1 NOT NULL,
   CodigoTelecom INT UNSIGNED default 720 NOT NULL,
   PRIMARY KEY (Id),
   UNIQUE (Nombre),
   INDEX IDX_IdProtocolo(IdProtocolo),
   FOREIGN KEY (IdProtocolo) REFERENCES Protocolos(Id)
)TYPE=INNODB;
INSERT INTO Loterias(Id,Nombre) VALUES(1,'Todas');
INSERT INTO Loterias(Id,Nombre) VALUES(2,'No Asignada');

CREATE TABLE Beneficiencias 
(
  Id INT UNSIGNED NOT NULL auto_increment,
  RIF varchar(15) NOT NULL,
  Nombre varchar(100) NOT NULL,
  PRIMARY KEY  (id)
) TYPE=InnoDB;

CREATE TABLE Juegos 
(
  Id INT UNSIGNED NOT NULL auto_increment,
  IdLoteria INT UNSIGNED NOT NULL,
  Codigo INT UNSIGNED NOT NULL,
  Nombre varchar(100) NOT NULL,
  IdBeneficiencia INT UNSIGNED NOT NULL,
  PRIMARY KEY  (Id),
  INDEX IDX_Juegos_IdLoteria (IdLoteria),
  INDEX IDX_Juegos_IdBeneficiencia (IdBeneficiencia),
  FOREIGN KEY (IdLoteria) REFERENCES Loterias(Id),
  FOREIGN KEY (IdBeneficiencia) REFERENCES Beneficiencias(Id)
) TYPE=InnoDB;

create table CuposProductos 
(
  IdJuego INT UNSIGNED NOT NULL,
  IdAgente INT UNSIGNED NOT NULL,
  Cupo INT UNSIGNED NOT NULL,
  INDEX IDX_CuposProductos_IdJuego (IdJuego),
  INDEX IDX_CuposProductos_IdAgente (IdAgente),
  FOREIGN KEY (IdJuego) REFERENCES Juegos(Id),
  FOREIGN KEY (IdAgente) REFERENCES Agentes(Id)
);

create table DetallesJuegos
(
  Id INT UNSIGNED NOT NULL auto_increment,
  IdJuego INT UNSIGNED NOT NULL,
  Modalidades VARCHAR(40),
  Premiaciones VARCHAR(70),
  Caducidad VARCHAR(40),
  Estado TINYINT(1),
  Hora TIME,
  Lunes TINYINT(1),
  Martes TINYINT(1),
  Miercoles  TINYINT(1),
  Jueves TINYINT(1),
  Viernes TINYINT(1),
  Sabado TINYINT(1),
  Domingo TINYINT(1),
  HoraCierre TIME,
  PRIMARY KEY  (Id),
  INDEX IDX_CuposProductos_IdJuego (IdJuego),
  FOREIGN KEY (IdJuego) REFERENCES Juegos(Id)
);

create table EstadosTickets 
(
	Id integer not null,
	Estado varchar(40),
	Detalle varchar(60)
); 

CREATE TABLE  TiposJugadas 
(
  Id INT UNSIGNED NOT NULL auto_increment,
  Tipo INT UNSIGNED default NULL,
  Valor char(2) default NULL,
  Nombre varchar(32) default NULL,
  NombResumido char(3) default NULL,
  PRIMARY KEY  (Id)
) TYPE=InnoDB;

-- alter table TipoModalidad add Tipo Integer;
-- alter table TipoModalidad add TipoResumen Integer;
CREATE TABLE  TipoModalidad
(
  Id int(11) default NULL,
  Nombre varchar(30) default NULL,
  Tipo Integer,
  TipoResumen Integer,
) TYPE=INNODB;
-- Update TipoModalidad set Tipo = 1 where id in (7,3,8,5);
-- Update TipoModalidad set Tipo = 0 where id in (2,4);
-- Update TipoModalidad set TipoResumen = id;
-- Update TipoModalidad set TipoResumen = 3 where id = 7;
-- Update TipoModalidad set TipoResumen = 5 where id = 8;

------- ALTER TABLE Productos ADD FOREIGN KEY(IdJuego) REFERENCES Juegos(id);
create table Productos
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   IdLoteria INT UNSIGNED NOT NULL,
   Nombre varchar (40) NOT NULL,
   IdJuego INT UNSIGNED NOT NULL,
   PRIMARY KEY (Id),
   INDEX IDX_Productos_IdLoteria (IdLoteria),
   INDEX IDX_Productos_IdJuego (IdJuego),
   FOREIGN KEY (IdLoteria) REFERENCES  Loterias (id),
   FOREIGN KEY(IdJuego) REFERENCES Juegos(id)
)TYPE=INNODB;

CREATE TABLE Sorteos
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Nombre VARCHAR(20) NOT NULL,
   Dia TINYINT UNSIGNED NOT NULL,
   HoraCierre TIME NOT NULL,
   Estado TINYINT UNSIGNED NOT NULL,
   FechaSuspHasta DATE,
   FechaModHoraHasta DATE,
   ModHora TIME,
   TipoJugada TINYINT UNSIGNED DEFAULT 0,
   IdLoteria INT UNSIGNED DEFAULT 2 NOT NULL,
   IdProducto TINYINT UNSIGNED NOT NULL,
   HoraSorteo Time default '00:00:00',
   PRIMARY KEY (Id),   
   INDEX IDX_Sorteos_IdProducto (IdProducto),
   FOREIGN KEY (IdProducto) REFERENCES Productos (Id),
   INDEX IDX_Sorteos_HoraCierreId (HoraCierre, Id),
   INDEX IDX_Sorteos_IdLoteria(IdLoteria),
   FOREIGN KEY (IdLoteria) REFERENCES Loterias (Id)
)TYPE=INNODB;

CREATE TABLE Usuarios
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   IdAgente INT UNSIGNED NOT NULL,
   Login VARCHAR(20) NOT NULL,
   Password VARCHAR(32) NOT NULL,
   CI INT NOT NULL,
   Nombre VARCHAR(40) NOT NULL,
   Direccion VARCHAR(60) NOT NULL,
   Telefono VARCHAR(12) NOT NULL,
   Prioridad TINYINT UNSIGNED NOT NULL,
   Estado TINYINT UNSIGNED NOT NULL,
   Hilo INT UNSIGNED DEFAULT NULL,
   IdLoteria INT UNSIGNED DEFAULT 1 NOT NULL,
   PRIMARY KEY (Id),
   UNIQUE (CI),
   UNIQUE (Login),
   INDEX IDX_Usuarios_Agente (IdAgente),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id)
)TYPE=INNODB;

CREATE TABLE Tickets
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   Fecha DATETIME NOT NULL,
   Monto INT UNSIGNED NOT NULL,
   Serial VARCHAR(26) DEFAULT "" NOT NULL,
   Estado TINYINT UNSIGNED NOT NULL,
   IdAgente INT UNSIGNED NOT NULL,
   IdUsuario INT UNSIGNED NOT NULL,
   NumTaquilla TINYINT UNSIGNED NOT NULL,
   FechaAct DATETIME,
   ApostadorNombre VARCHAR(30) DEFAULT "" NOT NULL,
   ApostadorCI VARCHAR(12) DEFAULT "" NOT NULL, 
   Correlativo INT UNSIGNED NOT NULL DEFAULT 0,
   MontoBono INT UNSIGNED NOT NULL DEFAULT 0,
   IdProtocolo TINYINT UNSIGNED NOT NULL DEFAULT 1,
   PRIMARY KEY (Id),
   INDEX IDX_Tickets_Agente (IdAgente),
   INDEX IDX_Tickets_Agente_Taquilla (IdAgente,NumTaquilla),
   INDEX IDX_Tickets_Usuario (IdUsuario),
   INDEX IDX_Tickets_Fecha (Fecha),
   INDEX IDX_Tickets_Fecha_Agente (Fecha,IdAgente),
   INDEX IDX_Tickets_Id_Agente (Id,IdAgente),
   INDEX IDX_Tickets_Fecha_Estado (Fecha,Estado),
   INDEX IDX_Tickets_Fecha_Estado_Agente (Fecha,Estado,IdAgente),
   INDEX IDX_IdProtocolo(IdProtocolo),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id),
   FOREIGN KEY (IdAgente,NumTaquilla) REFERENCES Taquillas (IdAgente,Numero),
   FOREIGN KEY (IdUsuario) REFERENCES Usuarios (Id),
   FOREIGN KEY (IdProtocolo) REFERENCES Protocolos(Id)
)TYPE=INNODB;

CREATE TABLE CorrelativoTickets
(
	IdAgente INT UNSIGNED NOT NULL,
	SiguienteCorrelativo INT UNSIGNED NOT NULL DEFAULT 1,
	PRIMARY KEY (IdAgente),
	FOREIGN KEY (IdAgente) REFERENCES Agentes (Id) 
)TYPE=INNODB;

CREATE TABLE TicketsPagados
(
   IdTicket INT UNSIGNED NOT NULL,
   MontoNeto INT UNSIGNED NOT NULL,
   Impuesto  INT UNSIGNED NOT NULL,
   PorcRetencion INT UNSIGNED NOT NULL,
   PRIMARY KEY (IdTicket),
   FOREIGN KEY (IdTicket) REFERENCES Tickets (Id)
)TYPE=INNODB;

CREATE TABLE Configuracion
(
	llave varchar(50) NOT NULL,
	valor varchar(200),
	PRIMARY KEY(llave)
)TYPE=INNODB;
INSERT INTO Configuracion VALUES ('IGF','1600');

CREATE TABLE SelcoLoterias
(
	CodLoteria INT UNSIGNED NOT NULL,
	IdLoteria INT UNSIGNED NOT NULL,
	INDEX IDX_IdLoteria(IdLoteria),
	PRIMARY KEY(CodLoteria),
	FOREIGN KEY(IdLoteria) REFERENCES Loterias(Id)
)TYPE=INNODB;

Create Table SelcoSorteos
(
	IdSorteo TINYINT unsigned NOT NULL PRIMARY KEY,
	CodLoteria SMALLINT NOT NULL,
	CodSorteo SMALLINT NOT NULL,
	TripleJugadaAB TINYINT NOT NULL,
	EspecialCantado tinyint default 0,
	FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;


Create Table SelcoAgentes
(
	IdAgente INT UNSIGNED NOT NULL,
	IdTaquilla TINYINT UNSIGNED NOT NULL,
	CodComercializador SMALLINT UNSIGNED NOT NULL,
	CodTaquilla SMALLINT UNSIGNED NOT NULL,
	CodCentroApuesta SMALLINT UNSIGNED NOT NULL,
	Primary Key (IdAgente, IdTaquilla),	
	FOREIGN KEY (IdAgente, IdTaquilla) REFERENCES Taquillas (IdAgente,Numero)
)TYPE=INNODB;

CREATE TABLE TransaxSorteos
(
    IdSorteo TINYINT UNSIGNED NOT NULL PRIMARY KEY,
    CodigoSorteo varchar(3) NOT NULL,
    CodigoLoteria varchar(2) NOT NULL,
    FOREIGN KEY (IdSorteo) REFERENCES Sorteos(Id)
)TYPE=INNODB;

CREATE TABLE TransaxComercializador
(
  IdComercializadora INT UNSIGNED NOT NULL,
  CodigoNCC varchar(5) NOT NULL default '',
  PRIMARY KEY(IdComercializadora),
  FOREIGN KEY (IdComercializadora) REFERENCES Agentes(Id)
)TYPE=INNODB;

CREATE TABLE Renglones
(
   Id SMALLINT UNSIGNED NOT NULL,
   Numero SMALLINT UNSIGNED NOT NULL,
   Monto INT UNSIGNED NOT NULL,
   Tipo TINYINT UNSIGNED NOT NULL,
   IdTicket INT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   EsBono TINYINT UNSIGNED DEFAULT 0 NOT NULL,
   PRIMARY KEY (Id,IdTicket),
   INDEX IDX_Renglones_Ticket (IdTicket),
   INDEX IDX_Renglones_Sorteo (IdSorteo),
   INDEX IDX_Renglones_Ticket_Sorteo (IdTicket,IdSorteo),
   INDEX IDX_Renglones_Num_Tipo (Numero,Tipo),
   INDEX IDX_Renglones_Num_Tipo_Tick_Sort (Numero,Tipo,IdTicket,IdSorteo),
   FOREIGN KEY (IdTicket) REFERENCES Tickets (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;

CREATE TABLE TiposMontoTriple
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Monto SMALLINT UNSIGNED NOT NULL,
   ValorInicial SMALLINT UNSIGNED NOT NULL,
   Proporcion TINYINT UNSIGNED NOT NULL,
   Incremento SMALLINT UNSIGNED NOT NULL,
   ValorAdicional SMALLINT UNSIGNED NOT NULL,
   ValorFinal INT UNSIGNED NOT NULL,
   Observacion VARCHAR(100) NOT NULL,
   PRIMARY KEY (Id)
)TYPE=INNODB;

CREATE TABLE TiposMontoTerminal
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Monto1ro SMALLINT UNSIGNED NOT NULL,
   Monto2do SMALLINT UNSIGNED NOT NULL,
   Monto3ro SMALLINT UNSIGNED NOT NULL,
   ValorInicial SMALLINT UNSIGNED NOT NULL,
   Proporcion TINYINT UNSIGNED NOT NULL,
   Incremento SMALLINT UNSIGNED NOT NULL,
   ValorAdicional SMALLINT UNSIGNED NOT NULL,
   ValorFinal INT UNSIGNED NOT NULL,
   Observacion VARCHAR(100) NOT NULL,
   PRIMARY KEY (Id)
)TYPE=INNODB;

CREATE TABLE Equipos_Comunicacion
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,,
   Serial VARCHAR(12) NOT NULL,
   IdAgente INT UNSIGNED,
   Descripcion VARCHAR(50) NOT NULL,
   PRIMARY KEY (Id),
   INDEX IDX_Equipos_Com_Agente (IdAgente),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id)
)TYPE=INNODB;

CREATE TABLE LimitesPorc
(
   IdAgente INT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   IdMontoTriple TINYINT UNSIGNED NOT NULL,
   IdMontoTerminal TINYINT UNSIGNED NOT NULL,
   IdZona TINYINT UNSIGNED NOT NULL,
   IdFormaPago TINYINT UNSIGNED NOT NULL,
   PorcTrip SMALLINT NOT NULL,
   PorcTerm SMALLINT NOT NULL,
   LimDiscTrip SMALLINT UNSIGNED NOT NULL,
   LimDiscTerm SMALLINT UNSIGNED NOT NULL,
   Comodin BOOL NOT NULL,
   MontoAdd SMALLINT UNSIGNED NOT NULL,
   CHECK ( IdFormaPago>= 0 AND IdFormaPago <= 3),
   PRIMARY KEY (IdAgente,IdSorteo,IdMontoTriple), -- creo deberia ser IdZona em vez de IdMontoTriple
   INDEX IDX_LimitesPorc_Agente (IdAgente),
   INDEX IDX_LimitesPorc_Sorteo (IdSorteo),
   INDEX IDX_LimitesPorc_FormaPago (IdFormaPago),
   INDEX IDX_LimitesPorc_MontoTriple (IdMontoTriple),
   INDEX IDX_LimitesPorc_MontoTerminal (IdMontoTerminal),
   INDEX IDX_LimitesPorc_Zona (IdZona),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id),
   FOREIGN KEY (IdMontoTriple) REFERENCES TiposMontoTriple (Id),
   FOREIGN KEY (IdMontoTerminal) REFERENCES TiposMontoTerminal (Id),
   FOREIGN KEY (IdZona) REFERENCES Zonas (Id)
)TYPE=INNODB;

-- alter table Premios add estaResumido BOOL NOT NULL DEFAULT 0;
-- Update Premios, Resumen_Ventas set Premios.estaResumido = 1
-- 		where Premios.fecha = Resumen_Ventas.Fecha 
-- 		and Premios.IdSorteo= Resumen_Ventas.IdSorteo;
CREATE TABLE Premios
(
   IdSorteo TINYINT UNSIGNED NOT NULL,
   Fecha DATE NOT NULL,
   Triple SMALLINT UNSIGNED NOT NULL,
   Term1ro TINYINT UNSIGNED,
   Term2do TINYINT UNSIGNED,
   Term3ro TINYINT UNSIGNED,
   Comodin TINYINT NOT NULL,
   MontoAdd SMALLINT UNSIGNED,
   SorteoNo VARCHAR(8) NOT NULL DEFAULT '',
   TipoJugada TINYINT UNSIGNED NOT NULL default 0,
   HoraSorteo TIME NOT NULL default '00:00:00',
   estaResumido BOOL NOT NULL DEFAULT 0,
   PRIMARY KEY (IdSorteo, Fecha),
   INDEX IDX_Premios_Sorteo (IdSorteo),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;

CREATE TABLE Restringidos
(
    Numero SMALLINT UNSIGNED NOT NULL,
    Tipo TINYINT UNSIGNED NOT NULL,
    PorcVenta INT NOT NULL DEFAULT 10000,
    FechaHasta DATE NOT NULL,
    IdSorteo TINYINT UNSIGNED NOT NULL,
    IdZona TINYINT UNSIGNED NOT NULL,
    Tope INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (Numero, Tipo, IdSorteo, IdZona),
    INDEX IDX_Restringidos_Sorteo (IdSorteo),
    INDEX IDX_Restringidos_Zona (IdZona),
    FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id),
    FOREIGN KEY (IdZona) REFERENCES Zonas (Id)
)TYPE=INNODB;

CREATE TABLE Grupos
(
    Id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    Nombre VARCHAR(40) NOT NULL,
    Descripcion VARCHAR(60) NOT NULL,
    PRIMARY KEY (Id),
    UNIQUE (Nombre)
)TYPE=INNODB;

CREATE TABLE Usuarios_Grupos_R
(
    IdUsuario INT UNSIGNED NOT NULL,
    IdGrupo SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (IdUsuario, IdGrupo),
    INDEX IDX_Usuarios_Grupos_R_Grupo (IdGrupo),
    INDEX IDX_Usuarios_Grupos_R_Usuario (IdUsuario),
    FOREIGN KEY (IdGrupo) REFERENCES Grupos (Id),
    FOREIGN KEY (IdUsuario) REFERENCES Usuarios (Id)
)TYPE=INNODB;

CREATE TABLE Nodos
(
    Id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    Nombre VARCHAR(40) NOT NULL,
    IdPadre SMALLINT UNSIGNED,
    Tipo VARCHAR(1) NOT NULL,
    Formulario VARCHAR(30),
    Posicion INT,
    Icono TINYINT UNSIGNED,
    PRIMARY KEY (Id),
    INDEX IDX_Nodos_IdPadre (IdPadre),
    FOREIGN KEY (IdPadre) REFERENCES Nodos (Id)
)TYPE=INNODB;

CREATE TABLE Nodos_Grupos_R
(
    IdNodo SMALLINT UNSIGNED NOT NULL,
    IdGrupo SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (IdNodo, IdGrupo),
    INDEX IDX_Nodos_Grupos_R_Grupo (IdGrupo),
    INDEX IDX_Nodos_Grupos_R_Nodo (IdNodo),
    FOREIGN KEY (IdGrupo) REFERENCES Grupos (Id),
    FOREIGN KEY (IdNodo) REFERENCES Nodos (Id)
)TYPE=INNODB;

CREATE TABLE Tareas
(
    Id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    Nombre VARCHAR(30) NOT NULL,
    Descripcion VARCHAR(30) NOT NULL,
    LlaveTarea VARCHAR(15),
    Icono VARCHAR(15) NOT NULL,
    PRIMARY KEY (Id),
    UNIQUE (LlaveTarea)
)TYPE=INNODB;

CREATE TABLE Tareas_Nodos_R
(
    IdTarea SMALLINT UNSIGNED NOT NULL,
    IdNodo SMALLINT UNSIGNED NOT NULL,
    Posicion TINYINT UNSIGNED,
    PRIMARY KEY (IdTarea, IdNodo),
    INDEX IDX_Tareas_Nodos_R_Nodo (IdNodo),
    INDEX IDX_Tareas_Nodos_R_Tarea (IdTarea),
    FOREIGN KEY (IdNodo) REFERENCES Nodos (Id),
    FOREIGN KEY (IdTarea) REFERENCES Tareas (Id)
)TYPE=INNODB;

CREATE TABLE Tareas_Nodos_Grupos_R
(
    IdTarea SMALLINT UNSIGNED NOT NULL,
    IdNodo SMALLINT UNSIGNED,
    IdGrupo SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (IdTarea, IdGrupo, IdNodo),
    INDEX IDX_Tareas_Nodos_Grupos_R_Grupo (IdGrupo),
    INDEX IDX_Tareas_Nodos_Grupos_R_Nodo (IdNodo),
    INDEX IDX_Tareas_Nodos_Grupos_R_Tarea (IdTarea),
    FOREIGN KEY (IdGrupo) REFERENCES Grupos (Id),
    FOREIGN KEY (IdNodo) REFERENCES Nodos (Id),
    FOREIGN KEY (IdTarea) REFERENCES Tareas (Id)
)TYPE=INNODB;

CREATE TABLE Cambio_Ticket
(
   IdTicket_Old INT UNSIGNED NOT NULL,
   IdTicket_New INT UNSIGNED ,
   Estatus_Old  TINYINT UNSIGNED NOT NULL,
   IdUsuario INT UNSIGNED NOT NULL,
   INDEX IDX_IdTicket_Old (IdTicket_Old),
   INDEX IDX_IdTicket_New (IdTicket_New),
   FOREIGN KEY (IdTicket_Old) REFERENCES Tickets (Id),
   FOREIGN KEY (IdTicket_New) REFERENCES Tickets (Id)
)TYPE=INNODB;

CREATE TABLE Info_Ticket
(
   IdTicket INT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   PRIMARY KEY (IdTicket, IdSorteo),
   INDEX IDX_IdTicket (IdTicket),
   INDEX IDX_IdSorteo (IdSorteo),
   FOREIGN KEY (IdTicket) REFERENCES Tickets (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;
 
CREATE TABLE Resumen_Ventas
(
   IdAgente INT UNSIGNED NOT NULL,
   NumTaquilla TINYINT UNSIGNED NOT NULL,
   Fecha DATE NOT NULL,
   Tipo TINYINT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   Venta INT UNSIGNED NOT NULL,
   Porcentaje INT UNSIGNED NOT NULL,
   Premios INT UNSIGNED NOT NULL,
   Saldo INT NOT NULL,
   TipoMonto TINYINT UNSIGNED NOT NULL,
   Ganancia INT NOT NULL,
   RetencionEstimada INT UNSIGNED NOT NULL,
   PorcRetencion INT UNSIGNED NOT NULL,
   PRIMARY KEY (IdAgente,NumTaquilla, Fecha, Tipo, IdSorteo),
   INDEX IDX_Resumen_Agente_Fecha_Sorteo (IdAgente,Fecha,IdSorteo),
   INDEX IDX_Resumen_Agente_Fecha_Tipo_Sorteo (IdAgente,Fecha,Tipo,IdSorteo),
   INDEX IDX_Resumen_Fecha_Sorteo(Fecha,IdSorteo),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;

CREATE TABLE Tickets_Premiados
(
   IdAgente INT UNSIGNED NOT NULL,
   NumTaquilla TINYINT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   Fecha DATE NOT NULL,
   IdTicket INT UNSIGNED NOT NULL,
   Monto INT NOT NULL,
   PRIMARY KEY (IdAgente,NumTaquilla,IdSorteo,Fecha,IdTicket),
   INDEX IDX_Premiados_Agente (IdAgente),
   INDEX IDX_Premiados_Tickets (IdTicket),
   INDEX IDX_Premiados_Sorteo (IdSorteo),
   INDEX IDX_Premiados_Sorteo_Fecha (IdSorteo,Fecha),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id),
   FOREIGN KEY (IdTicket) REFERENCES Tickets (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;

CREATE TABLE Mensajeria
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   IdUsuarioR INT UNSIGNED NOT NULL, -- Remitemnte
   IdUsuarioD INT UNSIGNED NOT NULL, -- Destinatario
   FechaHora DATETIME NOT NULL,
   Mensaje VARCHAR(255) NOT NULL,
   Leido TINYINT DEFAULT 0 NOT NULL,
   Tipo TINYINT DEFAULT 0 NOT NULL,
   PRIMARY KEY (Id),
   INDEX IDX_FechaHora (FechaHora),
   INDEX IDX_FechaHoraLeido (FechaHora,Leido)
)TYPE=INNODB;

CREATE TABLE Total_General
(
   IdAgente INT UNSIGNED NOT NULL,
   Fecha DATE NOT NULL,
   Tipo INT UNSIGNED NOT NULL,
   IdMonto INT UNSIGNED NOT NULL,
   IdSorteo INT UNSIGNED NOT NULL,
   Venta INT UNSIGNED NOT NULL,
   Porcentaje INT UNSIGNED NOT NULL,
   Premios INT UNSIGNED NOT NULL,
   Saldo INT NOT NULL,
   INDEX IDX_TotalGeneral_Agente (IdAgente),
   INDEX IDX_TotalGeneral_Sorteo (IdSorteo),
   INDEX IDX_TotalGeneral_AgenteTipoMonto (IdAgente,Tipo,IdMonto),
)TYPE=INNODB;

CREATE TABLE Resumen_Renglones
(
   IdTicket INT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   Tipo TINYINT UNSIGNED NOT NULL,
   Monto INT NOT NULL,
   PRIMARY KEY (IdTicket,IdSorteo,Tipo),
   INDEX IDX_Resumen_Renglones_IdTicket (IdTicket),
   INDEX IDX_Resumen_Renglones_IdSorteo (IdSorteo),
   INDEX IDX_Resumen_Renglones_IdTicketIdSorteo (IdTicket,IdSorteo),
   INDEX IDX_Resumen_Renglones_Todo (IdTicket,IdSorteo,Tipo)
)TYPE=INNODB;

CREATE TABLE WebLog
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   FechaHora DATETIME NOT NULL,
   Estado TINYINT NOT NULL DEFAULT 0,
   Titulo VARCHAR(60) NOT NULL,
   Contenido TEXT NOT NULL,
   PRIMARY KEY (Id)
)TYPE=INNODB;

CREATE TABLE Resumen_Tickets
(
   Id INT UNSIGNED NOT NULL ,
   Fecha DATETIME NOT NULL,
   Monto INT UNSIGNED NOT NULL,
   Serial INT UNSIGNED NOT NULL,
   Estado TINYINT UNSIGNED NOT NULL,
   IdAgente INT UNSIGNED NOT NULL,
   IdUsuario INT UNSIGNED NOT NULL,
   NumTaquilla TINYINT UNSIGNED NOT NULL,
   FechaAct DATETIME ,
   NumRenglon INT UNSIGNED NOT NULL,
   ApostadorNombre VARCHAR(30) DEFAULT "" NOT NULL,
   ApostadorCI VARCHAR(12) DEFAULT "" NOT NULL
)TYPE=INNODB;

CREATE TABLE Bitacora_Admin
(
    Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    Forma VARCHAR(50) NOT NULL,
    Operacion TINYINT UNSIGNED NOT NULL,
    IdUsuario INT UNSIGNED NOT NULL,
    FechaHora DATETIME NOT NULL,
    Descripcion VARCHAR(255) NOT NULL,
    PRIMARY KEY (Id)
)TYPE=INNODB;

CREATE TABLE Parametros_Prev
(
    Numeros INT UNSIGNED NOT NULL,
    Monto INT UNSIGNED NOT NULL,
    Promedio INT UNSIGNED NOT NULL,
    Tiempo INT UNSIGNED NOT NULL
)TYPE=INNODB;

CREATE TABLE TiposStatus
(
   Id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
   Descripcion VARCHAR(15) NOT NULL,
   PRIMARY KEY (Id)
)TYPE=INNODB;

-------------------------------------------------------------------------------

CREATE TABLE Historico_Tickets
(
   Id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   Fecha DATETIME NOT NULL,
   Monto INT UNSIGNED NOT NULL,
   Serial VARCHAR(26) DEFAULT "" NOT NULL,
   Estado TINYINT UNSIGNED NOT NULL,
   IdAgente INT UNSIGNED NOT NULL,
   IdUsuario INT UNSIGNED NOT NULL,
   NumTaquilla TINYINT UNSIGNED NOT NULL,
   FechaAct DATETIME,
   ApostadorNombre VARCHAR(30) DEFAULT "" NOT NULL,
   ApostadorCI VARCHAR(12) DEFAULT "" NOT NULL, 
   Correlativo INT UNSIGNED NOT NULL DEFAULT 0,
   MontoBono INT UNSIGNED NOT NULL DEFAULT 0,
   IdProtocolo TINYINT UNSIGNED NOT NULL DEFAULT 1,
   PRIMARY KEY (Id),
   INDEX IDX_Tickets_Agente (IdAgente),
   INDEX IDX_Tickets_Agente_Taquilla (IdAgente,NumTaquilla),
   INDEX IDX_Tickets_Usuario (IdUsuario),
   INDEX IDX_Tickets_Fecha (Fecha),
   INDEX IDX_Tickets_Fecha_Agente (Fecha,IdAgente),
   INDEX IDX_Tickets_Id_Agente (Id,IdAgente),
   INDEX IDX_Tickets_Fecha_Estado (Fecha,Estado),
   INDEX IDX_Tickets_Fecha_Estado_Agente (Fecha,Estado,IdAgente),
   INDEX IDX_IdProtocolo(IdProtocolo),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id),
   FOREIGN KEY (IdAgente,NumTaquilla) REFERENCES Taquillas (IdAgente,Numero),
   FOREIGN KEY (IdUsuario) REFERENCES Usuarios (Id),
   FOREIGN KEY (IdProtocolo) REFERENCES Protocolos(Id)
)TYPE=INNODB;

CREATE TABLE Historico_Tickets_Premiados
(
   IdAgente INT UNSIGNED NOT NULL,
   NumTaquilla TINYINT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   Fecha DATE NOT NULL,
   IdTicket INT UNSIGNED NOT NULL,
   Monto INT NOT NULL,
   PRIMARY KEY (IdAgente,NumTaquilla,IdSorteo,Fecha,IdTicket),
   INDEX IDX_Premiados_Agente (IdAgente),
   INDEX IDX_Premiados_Tickets (IdTicket),
   INDEX IDX_Premiados_Sorteo (IdSorteo),
   INDEX IDX_Premiados_Sorteo_Fecha (IdSorteo,Fecha),
   FOREIGN KEY (IdAgente) REFERENCES Agentes (Id),
   FOREIGN KEY (IdTicket) REFERENCES Historico_Tickets (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;

CREATE TABLE Historico_Renglones
(
   Id SMALLINT UNSIGNED NOT NULL,
   Numero SMALLINT UNSIGNED NOT NULL,
   Monto INT UNSIGNED NOT NULL,
   Tipo TINYINT UNSIGNED NOT NULL,
   IdTicket INT UNSIGNED NOT NULL,
   IdSorteo TINYINT UNSIGNED NOT NULL,
   EsBono TINYINT UNSIGNED DEFAULT 0 NOT NULL,
   PRIMARY KEY (Id,IdTicket),
   INDEX IDX_Renglones_Ticket (IdTicket),
   INDEX IDX_Renglones_Sorteo (IdSorteo),
   INDEX IDX_Renglones_Ticket_Sorteo (IdTicket,IdSorteo),
   INDEX IDX_Renglones_Num_Tipo (Numero,Tipo),
   INDEX IDX_Renglones_Num_Tipo_Tick_Sort (Numero,Tipo,IdTicket,IdSorteo),
   FOREIGN KEY (IdTicket) REFERENCES Historico_Tickets (Id),
   FOREIGN KEY (IdSorteo) REFERENCES Sorteos (Id)
)TYPE=INNODB;

-------------------------------------------------------------------------------

INSERT INTO Medios VALUES (1,"TCP/IP",0), (2,"MODEM",0), (3,"RADIO",0);

INSERT INTO TiposAgente VALUES (1,"Agencia"), (2,"Recogedor"), (3,"Banca");

INSERT INTO Zonas VALUES (1,"Nombre","Descripcion");

INSERT INTO Agentes VALUES (1,"Nombre","Representante",0,1,NULL,0,3,0,"NroAbonado",NULL,NULL,NULL);

INSERT INTO Usuarios VALUES (1,1,"master","e10adc3949ba59abbe56e057f20f883e",0,"master","Direccion","Telefono",6,1,NULL,1);

INSERT INTO Grupos VALUES (1,"root","Administrador del sistema");

INSERT INTO Grupos VALUES (2,"infobug","Mensajes de error del punto de venta");

INSERT INTO Usuarios_Grupos_R VALUES (1,1);

INSERT INTO Usuarios_Grupos_R VALUES (1,2);

INSERT INTO Tareas VALUES
('1','nuevo','Nuevo Registro','nuevo','nuevo'),
('2','editar','Editar Registro','editar','editar'),
('3','eliminar','Eliminar Registro','eliminar','eliminar'),
('4','consultar','Consultar','consultar','consultar'),
('5','imprimir','Imprimir','imprimir','imprimir'),
('6','sorteo_mod_st','Servidor: Act/Des Sorteo','sorteo_mod_st','t0'),
('7','sorteo_mod_h','Servidor: Cambiar Hora Cierre','sorteo_mod_h','t0'),
('8','taquilla_conect','Servidor:Taquillas Conectadas','taquilla_conect','t0'),
('9','agencia_mod_st','Servidor: Act/Des Agencia','agencia_mod_st','t0'),
('10','desactivar','Activar/Desactivar','desactivar','act_des'),
('11','horasorteo','Horario Temporal','horasorteo','updatehora'),
('12','numero_rest','Servidor:Restringir Nmeros','numero_rest','t0'),
('13','ticket_elim','Servidor:Eliminar Tickets','ticket_elim','t0'),
('14','ticket_proceso','Tickets en Proceso','ticket_proceso','ticket_proceso'),
('15','elim_ticket','Eliminar Tickets','elim_ticket','elim_ticket'),
('16','actualizar','Actualizar','actualizar','refresh'),
('17','calc_totales','Calcular Totales','calc_totales','calcular'),
('18','limite','Servidor: L?ites Actuales','limite','t0'),
('19','calcular_resumen','Servidor:Resumen de Ventas','calcular_resum','t0'),
('20','calcular_preventa_taq','Servidor:Renglones x Taquilla','renglon_taq','t0'),
('21','mostrar_preventa_taq','Servidor:Detalles Rengln','detalle_renglon','t0'),
('22','nuevosubmenu','Nuevo SubMen','nuevosubmenu','nuevosubmenu'),
('23','nuevoform','Nuevo Formulario','nuevoform','nuevoform'),
('24','actualizar_tiposmonto','Servidor: Act Tipos Monto','act_tipomonto','t0'),
('25','mensajeria','Servidor: Mensajeria','mensajeria','t0');

INSERT INTO Nodos Values
('1','Sistema Administrativo','1','S',NULL,'1','1'),
('2','Seguridad Funcional','1','F','frmSegFun','17','3'),
('4','Grupos','1','F','frmGrupos','5','3'),
('5','Usuarios x Grupo','1','F','frmUsuariosGrupos','28','3'),
('6','Premios','1','F','frmPremios','12','3'),
('7','Tickets','1','F','frmTicket','22','3'),
('8','Mensajes','1','F','frmMensajes','10','3'),
('9','Usuarios','1','F','frmUsuarios','27','3'),
('10','Lista Diaria','1','F','frmCriterios','8','3'),
('11','Taquillas','1','F','frmTaquillas','20','3'),
('13','Limites y Porcentajes','1','F','frmLimPorc','7','3'),
('14','Sorteos','1','F','frmSorteos','19','3'),
('15','Agentes','1','F','frmAgentes','1','3'),
('17','Servidor Base de Datos','1','F','frmConfigBD','18','3'),
('18','Zonas','1','F','frmZona','29','3'),
('19','Tipos de Montos','1','F','frmTiposdeMonto','24','3'),
('20','Taquillas en Linea','1','F','frmTaqConect','21','3'),
('21','Limite Actual','1','F','frmLimiteActual','6','3'),
('22','Salir del Sistema','1','F','frmSalir','16','3'),
('23','Totales de Venta','1','F','frmCriResVen','25','3'),
('24','Radios','1','F','frmRadios','15','3'),
('25','Nros Limitados','1','F','frmRestringidos','11','3'),
('26','Tickets Premiados','1','F','frmTicketsPremiados','23','3'),
('27','Preventas','1','F','frmPreventa','13','3'),
('28','Medios','1','F','frmMedios','9','3'),
('29','Procesar Totales','1','F','frmProcesarTotales','14','3'),
('30','TotalVentas','1','F','FrmTotalVentas','26','3'),
('31','Cambio Clave Local','1','F','frmCambioClaveLocal','2','3'),
('32','Lista Diaria','1','F','FrmListaDiaria','30','3'),
('33','Saldo Diario por Loteria','1','F','FrmRepCuadre','31','3'),
('34','Tickets Vendidos','1','F','FrmRepTicketsVend','32','3'),
('35','Consulta de Numeros Vendidos','1','F','frmConsultaVendidos','3','3'),
('36','Eliminar Tickets','1','F','frmEliminarTickets','4','3'),
('37','Equipos de Comunicación','1','F','frmEquiposCom','33','3'),
('38','Bitacora','1','F','FrmBitacora','34','3'),
('39','Ganancias y Perdidas','1','F','FrmRepGanancias','35','3'),
('40','Medios de Comunicación','1','F','FrmRepMedios','36','3'),
('41','Limites y Porcentajes','1','F','FrmRepLimites','37','3');

INSERT INTO Tareas_Nodos_R VALUES
('1','4','1'),('1','8','2'),('1','9','1'),('1','11','1'),
('1','14','1'),('1','15','1'),('1','18','1'),('1','19','1'),
('1','24','1'),('1','25','1'),('1','28','1'),('1','37','1'),
('2','2','1'),('2','4','2'),('2','5','1'),('2','9','2'),
('2','11','2'),('2','14','2'),('2','15','2'),('2','18','2'),
('2','19','2'),('2','24','2'),('2','28','2'),('2','37','2'),
('3','2','2'),('3','4','3'),('3','6','3'),('3','8','3'),('3','9','3'),
('3','11','3'),('3','13','2'),('3','14','3'),('3','15','3'),('3','18','3'),
('3','19','3'),('3','24','3'),('3','25','2'),('3','28','3'),('3','37','3'),
('3','38','2'),('4','4','4'),('4','9','4'),('4','13','3'),('4','14','4'),
('4','15','4'),('4','18','4'),('4','19','4'),('4','24','4'),('4','25','3'),
('4','28','4'),('4','37','4'),('5','8','4'),('6','14','8'),('7','14','7'),
('8','20','4'),('9','20','2'),('10','14','5'),('10','20','3'),('11','14','6'),
('12','25','4'),('13','7','5'),('15','7','3'),('16','6','1'),('16','7','1'),
('16','8','1'),('16','20','1'),('16','27','1'),('16','38','1'),('17','23','1'),
('18','21','1'),('19','6','2'),('19','29','1'),('20','7','2'),('21','7','4'),
('22','2','4'),('23','2','3'),('24','13','1'),('25','8','5');

INSERT INTO Nodos_Grupos_R VALUES
('1','1'),('2','1'),('4','1'),('5','1'),('6','1'),('7','1'),('8','1'),('9','1'),
('10','1'),('11','1'),('13','1'),('14','1'),('15','1'),('17','1'),('18','1'),
('19','1'),('20','1'),('21','1'),('22','1'),('23','1'),('24','1'),('25','1'),
('26','1'),('27','1'),('28','1'),('29','1'),('30','1'),('31','1'),('32','1'),
('33','1'),('34','1'),('35','1'),('37','1'),('38','1'),('39','1'),('40','1'),
('41','1');

INSERT INTO Tareas_Nodos_Grupos_R VALUES
('1','4','1'),('1','8','1'),('1','9','1'),('1','11','1'),('1','14','1'),('1','15','1'),
('1','18','1'),('1','19','1'),('1','24','1'),('1','25','1'),('1','28','1'),('1','37','1'),
('2','2','1'),('2','4','1'),('2','5','1'),('2','9','1'),('2','11','1'),('2','14','1'),
('2','15','1'),('2','18','1'),('2','19','1'),('2','24','1'),('2','28','1'),('2','37','1'),
('3','2','1'),('3','4','1'),('3','6','1'),('3','8','1'),('3','9','1'),('3','11','1'),
('3','13','1'),('3','14','1'),('3','15','1'),('3','18','1'),('3','19','1'),('3','24','1'),
('3','25','1'),('3','28','1'),('3','37','1'),('3','38','1'),('4','4','1'),('4','9','1'),
('4','13','1'),('4','14','1'),('4','15','1'),('4','18','1'),('4','19','1'),('4','24','1'),
('4','25','1'),('4','28','1'),('4','37','1'),('5','8','1'),('6','14','1'),('7','14','1'),
('8','20','1'),('9','20','1'),('10','14','1'),('10','20','1'),('11','14','1'),('12','25','1'),
('13','7','1'),('15','7','1'),('16','6','1'),('16','7','1'),('16','8','1'),('16','20','1'),
('16','27','1'),('16','38','1'),('18','21','1'),('19','6','1'),('19','29','1'),('20','7','1'),
('21','7','1'),('22','2','1'),('23','2','1'),('24','13','1'),('25','8','1');

insert into TiposStatus (Id,Descripcion) values (1,'ACTIVO');
insert into TiposStatus (Id,Descripcion) values (2,'INACTIVO');
insert into TiposStatus (Id,Descripcion) values (3,'POR INSTALAR');
insert into TiposStatus (Id,Descripcion) values (4,'FUERA DEL SISTEMA');
insert into TiposStatus (Id,Descripcion) values (5,'AUXILIAR');

-- Para Registrar el Formulario de Productos....
insert into Nodos values ('51','Productos','1','F','frmProductos','47','3');
insert into Nodos_Grupos_R values ('51','1');
insert into Tareas_Nodos_R values ('1','51','1');
insert into Tareas_Nodos_R values ('3','51','3');
insert into Tareas_Nodos_R values ('4','51','4');
insert into Tareas_Nodos_R values ('2','51','2');
insert into Tareas_Nodos_Grupos_R values ('1','51','1');
insert into Tareas_Nodos_Grupos_R values ('3','51','1');
insert into Tareas_Nodos_Grupos_R values ('4','51','1');
insert into Tareas_Nodos_Grupos_R values ('2','51','1');

--Insertar el Formulario de Selco
insert into Nodos values (52,'Protocolos',1,'F','frmProtocolo',48,3);
insert into Nodos_Grupos_R values (52,1);
insert into Tareas_Nodos_R values (1,52,1);
insert into Tareas_Nodos_R values (3,52,3);
insert into Tareas_Nodos_R values (4,52,4);
insert into Tareas_Nodos_R values (2,52,2);
insert into Tareas_Nodos_Grupos_R values (1,52,1);
insert into Tareas_Nodos_Grupos_R values (3,52,1);
insert into Tareas_Nodos_Grupos_R values (4,52,1);
insert into Tareas_Nodos_Grupos_R values (2,52,1);

insert into Nodos values (53,'Protocolos',1,'F','frmSelcoSorteos',48,3);
insert into Nodos_Grupos_R values (53,1);
insert into Tareas_Nodos_R values (1,53,1);
insert into Tareas_Nodos_R values (3,53,3);
insert into Tareas_Nodos_R values (4,53,4);
insert into Tareas_Nodos_R values (2,53,2);
insert into Tareas_Nodos_Grupos_R values (1,53,1);
insert into Tareas_Nodos_Grupos_R values (3,53,1);
insert into Tareas_Nodos_Grupos_R values (4,53,1);
insert into Tareas_Nodos_Grupos_R values (2,53,1);

INSERT INTO Configuracion(llave,valor) VALUES("Transax_CodigoServidor","102");
INSERT INTO Configuracion(llave,valor) VALUES("Transax_Ip","201.234.230.148");
INSERT INTO Configuracion(llave,valor) VALUES("Transax_LoginDataBase","transax.sql");
INSERT INTO Configuracion(llave,valor) VALUES("Transax_NameDataBase","transax");
INSERT INTO Configuracion(llave,valor) VALUES("Transax_PasswordDataBase","tx56521480");
INSERT INTO Configuracion(llave,valor) VALUES("Transax_Port","1433");


-- Creacion sorteos Mycrocom
CREATE TABLE MycrocomSorteos(
    IdSorteo TINYINT UNSIGNED NOT NULL,
    CodigoSorteo varchar(3) NOT NULL default '',
    PRIMARY KEY(IdSorteo ),
    FOREIGN KEY (IdSorteo) REFERENCES Sorteos(Id)
)TYPE=INNODB;

-- Taquillas configuradas para Mycrocom
CREATE TABLE MycrocomTaquillas
(
  IdTaquilla TINYINT UNSIGNED NOT NULL,
  IdAgente INT UNSIGNED NOT NULL,
  CodigoBanca INT UNSIGNED NOT NULL,
  CodigoTaquilla INT UNSIGNED NOT NULL,
  PRIMARY KEY(IdAgente,IdTaquilla),
  FOREIGN KEY (IdAgente,IdTaquilla) REFERENCES Taquillas(IdAgente,Numero)  
)TYPE=INNODB;

INSERT INTO Configuracion(llave,valor) VALUES("Mycrocom_URL","http://admin.mycrolottery.com/webSale.jsp");

-------------------------------------------------------------------------------

INSERT INTO `Beneficiencias` VALUES  
 (3,'','Lotería del Zulia.'),
 (4,'','Lotería del Tachira.'),
 (5,'','Lotería de Caraca'),
 (7,'','Lotería  de Oriente.'),
 (8,'','Lotería de Falcon'),
 (9,'','Lotería de Margarita'),
 (10,'','Loteria de  Aragua.'),
 (12,'','Loteria de Cojedes'),
 (13,'','Loteria de Carabobo');
 
INSERT INTO `TiposJugadas` VALUES  
(1,1,'01','Aries', 'ARI'),
(2,1,'02','Tauro', 'TAU'),
(3,1,'03','Geminis', 'GEM'),
(4,1,'04','Cancer', 'CAN'),
(5,1,'05','Leo', 'LEO'),
(6,1,'06','Virgo', 'VIR'),
(7,1,'07','Libra', 'LIB'),
(8,1,'08','Escorpio', 'ESC'),
(9,1,'09','Sagitario', 'SAG'),
(10,1,'10','Capricornio', 'CAP'),
(11,1,'11','Acuario', 'ACU'),
(12,1,'12','Picis', 'PIC'),
(13,2,'01','Enero', 'ENE'),
(14,2,'02','Febrero', 'FEB'),
(15,2,'03','Marzo', 'MAR'),
(16,2,'04','Abril', 'ABR'),
(17,2,'05','Mayo', 'MAY'),
(18,2,'06','Junio', 'JUN'),
(19,2,'07','Julio', 'JUL'),
(20,2,'08','Agosto', 'AGO'),
(21,2,'09','Septiembre', 'SEP'),
(22,2,'10','Octubre', 'OCT'),
(23,2,'11','Noviembre', 'NOV'),
(24,2,'12','Diciembre', 'DIC');

INSERT INTO `Juegos` VALUES   
 ( 2,10, 2,'Calendario Millonario', 10),
 ( 3, 6, 3,'Chance en Linea', 4),
 ( 9,17, 9,'Nuestro Triple', 13),
 (11, 7,11,'Oriente Tu Triple', 9),
 (13,12,13,'Triple Coro', 8),
 (14, 4,14,'Triple en Li­nea Tachira', 4),
 (15, 9,15,'Triple Gallo', 9),
 (16,13,16,'Triple Gordo On Line', 7),
 (17, 5,17,'Triple Leon', 5),
 (18,16,18,'Triple Los Animalitos', 10),
 (19,13,19,'Triple Oriente', 7),
 (20,14,20,'Triple Popular', 5),
 (23, 3,23,'Triple Zodiacal', 3);

INSERT INTO `TipoModalidad` VALUES  
(2,'Terminal'),
(3,'Triple'),
(4,'Terminal + Simbolo'),
(5,'Triple + Simbolo'),
(7,'Triple'),
(8,'Triple + Simbolo');
 
insert into EstadosTickets values 
 (1,'Jugado','Normal'), 
 (2,'Anulado','Modificado'), 
 (3,'Anulado','Taquilla'), 
 (4,'Jugado','Generado'), 
 (5,'Pagado','Pagado'), 
 (6,'Anulado','Servidor'), 
 (7,'Anulado','Administrativo');

insert into Nodos values (54,'Beneficiencias',1,'F','frmBeneficiencias',48,3);
insert into Nodos_Grupos_R values (54,1);
insert into Tareas_Nodos_R values (1,54,1);
insert into Tareas_Nodos_R values (3,54,3);
insert into Tareas_Nodos_R values (4,54,4);
insert into Tareas_Nodos_R values (2,54,2);
insert into Tareas_Nodos_Grupos_R values (1,54,1);
insert into Tareas_Nodos_Grupos_R values (3,54,1);
insert into Tareas_Nodos_Grupos_R values (4,54,1);
insert into Tareas_Nodos_Grupos_R values (2,54,1);

insert into Nodos values (55,'Juegos',1,'F','frmJuegos',48,3);
insert into Nodos_Grupos_R values (55,1);
insert into Tareas_Nodos_R values (1,55,1);
insert into Tareas_Nodos_R values (3,55,3);
insert into Tareas_Nodos_R values (4,55,4);
insert into Tareas_Nodos_R values (2,55,2);
insert into Tareas_Nodos_Grupos_R values (1,55,1);
insert into Tareas_Nodos_Grupos_R values (3,55,1);
insert into Tareas_Nodos_Grupos_R values (4,55,1);
insert into Tareas_Nodos_Grupos_R values (2,55,1);

insert into Nodos values (56,'DetalleJuego',1,'F','frmDetalleJuego',48,3);
insert into Nodos_Grupos_R values (56,1);
insert into Tareas_Nodos_R values (1,56,1);
insert into Tareas_Nodos_R values (3,56,3);
insert into Tareas_Nodos_R values (4,56,4);
insert into Tareas_Nodos_R values (2,56,2);
insert into Tareas_Nodos_Grupos_R values (1,56,1);
insert into Tareas_Nodos_Grupos_R values (3,56,1);
insert into Tareas_Nodos_Grupos_R values (4,56,1);
insert into Tareas_Nodos_Grupos_R values (2,56,1);

INSERT INTO NCC01 VALUES
   (2,95),
   (7,27),
   (9,203),
   (11,223),
   (12,8),
   (13,90),
   (14,26),
   (15,209),
   (16,340);


COMMIT;
