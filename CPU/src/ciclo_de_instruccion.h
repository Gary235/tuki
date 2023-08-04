
#ifndef CPU_CICLO_INSTRUCCION_H_
#define CPU_CICLO_INSTRUCCION_H_

#include <shared_tuki.h>

typedef struct {
	t_pcb* pcb;
	t_instruccion* instruccion;
	t_interrupcion_cpu* interrupcion;
} t_op_args;

typedef struct {
	int* num_segmento;
	intptr_t* direccion_fisica;
} t_mmu_op_args;

#include "operaciones_cpu.h"
#include "cpu.h"

#define OP_CREATE_SEGMENT "CREATE_SEGMENT"
#define OP_DELETE_SEGMENT "DELETE_SEGMENT"
#define OP_EXIT "EXIT"
#define OP_F_CLOSE "F_CLOSE"
#define OP_F_OPEN "F_OPEN"
#define OP_F_READ "F_READ"
#define OP_F_SEEK "F_SEEK"
#define OP_F_TRUNCATE "F_TRUNCATE"
#define OP_F_WRITE "F_WRITE"
#define OP_IO "I/O"
#define OP_MOV_IN "MOV_IN"
#define OP_MOV_OUT "MOV_OUT"
#define OP_SET "SET"
#define OP_SIGNAL "SIGNAL"
#define OP_WAIT "WAIT"
#define OP_YIELD "YIELD"

t_interrupcion_cpu* ejecutar_instrucciones(t_pcb* pcb);
void ejecutar_ciclo_de_instruccion(t_pcb* pcb, t_interrupcion_cpu* interrupcion);

t_instruccion* fetch(t_list* instrucciones, int* pc);
intptr_t* decode(
		t_instruccion* instruccion,
		t_pcb* pcb,
		t_interrupcion_cpu* interrupcion,
		int* num_segmento
);
void execute(
		t_instruccion* instruccion,
		t_pcb* pcb,
		t_interrupcion_cpu* interrupcion,
		intptr_t* direccion_fisica,
		int* num_segmento
);
#endif /* CPU_CICLO_INSTRUCCION_H_ */
