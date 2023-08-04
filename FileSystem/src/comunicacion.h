
#ifndef FS_COMUNICACION_H_
#define FS_COMUNICACION_H_

#include <shared_tuki.h>

#include "file_system.h"
#include "estructuras.h"

void recibir_kernel(char *puerto);

void solicitar_escritura_de_archivo(char* info_en_archivo, int tamanio_info, intptr_t* direccion_fisica, int pid);
void enviar_archivo(t_archivo_abierto* archivo, int cliente_fd);

void handle_existe_archivo(int cliente_fd);
void handle_f_truncate(int cliente_fd);
void handle_f_read(int cliente_fd);
void handle_f_write(int cliente_fd);

#endif /* FS_COMUNICACION_H_ */
