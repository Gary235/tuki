
#ifndef CONSOLA_H_
#define CONSOLA_H_

#include<shared_tuki.h>

t_list* leer_instrucciones(char *file_path);
void handle_exit();

void terminar_programa();
void signal_catch();

extern t_config* config;
extern t_log* logger;
extern int conexion_kernel;

#endif /* CONSOLA_H_ */
