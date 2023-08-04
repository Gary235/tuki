
#ifndef CPU_COMUNICACION_H_
#define CPU_COMUNICACION_H_

#include <shared_tuki.h>

#include "ciclo_de_instruccion.h"
#include "cpu.h"

void recibir_proceso(int cliente_fd);
void recibir_kernel(char *puerto);

#endif /* CPU_COMUNICACION_H_ */
