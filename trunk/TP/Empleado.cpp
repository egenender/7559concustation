/*
 * Empleado.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: jenniferandrea
 */

#include "Empleado.h"

#include <unistd.h>
#include <stdlib.h>


#include "constantesArchivos.h"

const bool Empleado::USO;
const bool Empleado::DESUSO;

Empleado::Empleado(std::string name, const PipeAutos& pipe, const Semaforo& semaforo) {
	nombre = name;
	cantidadAtendidos = 0;
	arribos = pipe;
	accesoSurtidores = semaforo;
	log.setTipo(Log::ENTRADA_PERSONAJE);
}

Empleado::~Empleado() {
}

void Empleado::atenderUnAuto(Auto& autito){
	log.escribirEntrada("Hay auto para ser atendido, patente " + string(autito.getPatente()));
	int surtidor = tomarSurtidor();
	log.escribirEntrada("Logre tomar el surtidor ",surtidor);

	int litros = autito.llenar();
	log.escribirEntrada("Llene el tanque. Cantidad de litros: ",litros);
	devolverSurtidor(surtidor);
	log.escribirEntrada("He devuelto el surtidor numero ",surtidor);

	float plata = litros * PRECIO_POR_LITRO;
	caja.depositar(plata);
	log.escribirEntrada("Deposito en caja $", plata);

	log.escribirEntrada("Termine de atender el auto, cuya patente es " + string(autito.getPatente()));
	autito.imprimir(); //Imprimo los datos del auto que fue atendido satisfactoriamente
	disponibilidad.incrementar(1);
	log.escribirEntrada("Estoy disponible para el jefe");

}

bool Empleado::comenzarDia(){
	log.setEscritor("Empleado " + nombre);
	log.escribirEntrada("Se ha iniciado el Proceso.");

	bool abriCaja = caja.abrir();
	if(!abriCaja){
		log.escribirEntrada("No pude abrir la caja.");
		return false;
	}
	log.escribirEntrada("Abri la caja.");

	arribos.setearModo(Pipe::LECTURA);

	try{
		disponibilidad.crear(ARCHIVO_CANTIDAD_EMPLEADOS, DISPONIBILIDAD_EMPLEADOS);
	}catch(std::string &e){
		cout << e << endl;
		log.escribirEntrada("No pude crear la memoria compartida: cantidad de empleados disponibles");
		cierreDeCaja();
		return false;
	}
	disponibilidad.incrementar(1);
	log.escribirEntrada("Me pongo a disposicion del Jefe.");

	return true;
}

void Empleado::cierreDeCaja(){
	bool cerre = caja.cerrar();
	if(!cerre)
		log.escribirEntrada("No pude cerrar la caja.");
	else
		log.escribirEntrada("Cerre la caja.");
}

void Empleado::finalizarDia(){
	cierreDeCaja();

	try{
		disponibilidad.liberar();
		log.escribirEntrada("Libero la memoria compartida: cantidad de Empleados disponibles.");
	}catch(std::string &e){
		cout << e << endl;
		log.escribirEntrada("No fue posible liberar la memoria compartida: cantidad de empleados disponibles");
	}

	for (int i = 0; i < int(surtidores.size()); i++){
		try{
			this->surtidores.at(i).liberar();
			log.escribirEntrada("Libere el surtido numero ", i);
		}catch(std::string &e){
			cout << e << endl;
			log.escribirEntrada("No pude liberar el surtidor numero ", i);
		}
	}

	log.escribirEntrada("Finalice mi proceso.");
}

bool Empleado::crearSurtidores(int cantidadSurtidores){
	for (int i = 0; i < cantidadSurtidores; i++){
		try{
			MemoriaCompartida<bool> surtidor (ARCHIVO_SURTIDORES, SURTIDOR+i);
			this->surtidores.push_back(surtidor);
			//log.escribirEntrada("Asocio surtidor numero: ",i);
		}catch(std::string &e){
			cout << e << endl;
			log.escribirEntrada("No se pudo crear la memoria compartida: Surtidor numero ",i);
			return false;
		}
	}
	return true;
}

pid_t Empleado::atenderAutos(int cantidadSurtidores){
	pid_t id = fork();
	if (id != 0)
		return id;

	bool comienzo = comenzarDia();
	if(!comienzo){
		log.escribirEntrada("Finalizo Proceso por ERROR.");
		return id;
	}

	Auto autito;

	bool creacion = crearSurtidores(cantidadSurtidores);
	if (! creacion){
		finalizarDia();
		return id;
	}

	while(leerAuto(&autito)){
		atenderUnAuto(autito);
	}

	finalizarDia();

	return id;
}

bool Empleado::leerAuto(Auto* autito){
	log.escribirEntrada("Voy a leer un auto. Si no hay, me duermo.");
	bool status = arribos.leerAuto(autito);
	return status;
}

int Empleado::tomarSurtidor(){
	//Espero a que me dejen solicitar el surtidor. Me habilitan cuando hay un surtidor que pueda tomar
	accesoSurtidores.wait();
	log.escribirEntrada("Estoy buscando un surtidor libre.");

	//busco surtidor que NO este en uso
	for (unsigned int i = 0; i < surtidores.size(); i++){
		if (surtidores.at(i).modificarValor(USO))
			return i;
		else
			log.escribirEntrada("No logre tomar el surtidor ", int(i));
	}

	return -1;
}

void Empleado::devolverSurtidor(int surtidor){
	log.escribirEntrada("Estoy por devolver el surtidor");
	surtidores.at(surtidor).modificarValor(DESUSO);
	accesoSurtidores.signal();
}
