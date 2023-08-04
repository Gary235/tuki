
#ifndef CPU_OPERACIONES_H_
#define CPU_OPERACIONES_H_

#include <shared_tuki.h>
#include "ciclo_de_instruccion.h"
#include "cpu.h"

// ---- HELPERS ----

int escribir_memoria(intptr_t* direccion_fisica, char* valor, int tamanio_valor, int* pid);
char* leer_memoria(intptr_t* direccion_fisica, int* pid, int tamanio_a_leer);


// ---- HANDLERS ----

void handle_set(t_op_args* args);
void handle_yield(t_op_args* args);
void handle_exit(t_op_args* args);
void handle_wait(t_op_args* args);
void handle_signal(t_op_args* args);
void handle_io(t_op_args* args);
void handle_mov_in(t_op_args* args, t_mmu_op_args* mmu_args);
void handle_mov_out(t_op_args* args, t_mmu_op_args* mmu_args);
void handle_create_segment(t_op_args* args);
void handle_delete_segment(t_op_args* args);
void handle_f_open(t_op_args* args);
void handle_f_close(t_op_args* args);
void handle_f_truncate(t_op_args* args);
void handle_f_seek(t_op_args* args);
void handle_f_read(t_op_args* args, t_mmu_op_args* mmu_args);
void handle_f_write(t_op_args* args, t_mmu_op_args* mmu_args);

#endif /* CPU_OPERACIONES_H_ */
