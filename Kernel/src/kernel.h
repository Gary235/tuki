
#ifndef KERNEL_H_
#define KERNEL_H_


#include<shared_tuki.h>

#include "planificacion.h"
#include "comunicacion.h"

void terminar_programa();
void signal_catch();

extern t_config* config;
extern t_log* logger;
extern int conexion_cpu, conexion_fs, conexion_memoria, server_fd, max_segmentos_por_proceso;

#endif /* KERNEL_H_ */
