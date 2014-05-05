/*
 * Jefe.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: ezequielgenender
 */

#include "Jefe.h"

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "constantesArchivos.h"
#include "Log.h"

Jefe::Jefe(std::string n, const PipeAutos& gen, const PipeAutos& aten) {
	nombre = n;
	cantidadAtendida = 0;
	cantidadDespachada = 0;

	arribos = gen;
	envios = aten;
}

Jefe::~Jefe(){}

bool Jefe::leerAuto(Auto* autito){
	bool status = arribos.leerAuto(autito);
	Log::enviarMensaje("Lei un auto.");
	return status;
}

void Jefe::atenderAutos(){
	pid_t id = fork();
	if (id != 0)
		return;

	Log::abrir_log();
	Log::setEscritor("Jefe " + nombre);
	Auto autito;

	Log::enviarMensaje("Se ha iniciado el Proceso.");

	cantEmpleadosDisponibles.crear(ARCHIVO_CANTIDAD_EMPLEADOS, DISPONIBILIDAD_EMPLEADOS);
	Log::enviarMensaje("Se creo la memoria compartida: cantidad de empleados disponibles.");

	while (leerAuto(&autito)){
		Log::enviarMensaje("Hay auto para ser atendido, patente " + string(autito.getPatente()));
		if (!hayEmpleados()){
			cantidadDespachada++;
			Log::enviarMensaje("La cantidad de autos despachados es de: ", cantidadDespachada);
			mensajeDespachante(autito.getPatente());
		}else{
			cantidadAtendida++;
			tomarEmpleado();
			enviarAutoAEmpleado(autito);
			Log::enviarMensaje("La cantidad de autos que atendi es de: ", cantidadAtendida);
		}
	}

	arribos.cerrar();
	Log::enviarMensaje("Cierro el pipe de lectura arribos");
	envios.cerrar();
	Log::enviarMensaje("Cierro el pipe de escritura envios");

	cantEmpleadosDisponibles.liberar();
	Log::enviarMensaje("Libero la memoria compartida: cantidad de Empleados disponibles.");

	Log::enviarMensaje("Fin del proceso.");
	Log::cerrar_log();
	exit(0);
}

bool Jefe::hayEmpleados(){
	int cant_empleados = cantEmpleadosDisponibles.leer();
	Log::enviarMensaje("La cantidad de empleados disponibles es ", cant_empleados);
	return (cant_empleados > 0);
}

void Jefe::tomarEmpleado(){
	Log::enviarMensaje("Tomo un empleado disponible.");
	cantEmpleadosDisponibles.incrementar(-1);
}

void Jefe::mensajeDespachante(std::string patente){
	Log::enviarMensaje("Se ha despachado al auto de patente " + patente + " por no haber empleados libres");
}

bool Jefe::enviarAutoAEmpleado(const Auto& autito){
	Log::enviarMensaje("Envio el auto por el pipe hacia empleados");
	return envios.escribirAuto(autito);
}
