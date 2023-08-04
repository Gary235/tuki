
#ifndef CPU_H_
#define CPU_H_

#include <shared_tuki.h>

#include "comunicacion.h"

void terminar_programa();
void signal_catch();

extern t_config* config;
extern t_log* logger;
extern int server_fd, conexion_memoria;

#endif /* CPU_H_ */
