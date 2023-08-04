
#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <shared_tuki.h>

#include "comunicacion.h"

void iniciar_estructuras();
void terminar_programa();
void signal_catch();

extern t_config* config;
extern t_log* logger;

extern void* espacio_usuario;
extern t_list *huecos_libres, *tabla_segmentos;

extern int conexion_kernel, conexion_cpu, conexion_fs, server_fd, id_hueco_actual;

#endif /* MEMORIA_H_ */
