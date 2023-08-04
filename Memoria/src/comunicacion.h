
#ifndef MEMORIA_COMUNICACION_H_
#define MEMORIA_COMUNICACION_H_

#include <shared_tuki.h>

#include "memoria.h"
#include "operaciones_modulos.h"

void aceptar_modulos();
void escuchar_modulos();
void handle_comunicacion(int cliente_fd);

#endif /* MEMORIA_COMUNICACION_H_ */
