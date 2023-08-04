
#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include <shared_tuki.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>

#include "comunicacion.h"
#include "estructuras.h"


void iniciar_estructuras();
void terminar_programa();
void signal_catch();

extern t_config *config, *superbloque;
extern t_log* logger;
extern int conexion_memoria, server_fd;
extern int tamanio_bloque, cantidad_bloques, tamanio_file_system;

extern t_list* fcbs;
extern t_bitarray* bitmap_bloques;
extern void *bloques_en_memoria, *bitmap_en_memoria;
extern char* base_archivo;

#endif /* FILE_SYSTEM_H_ */
