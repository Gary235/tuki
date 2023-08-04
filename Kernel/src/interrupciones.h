#ifndef KERNEL_INTERRUPCIONES_H
#define KERNEL_INTERRUPCIONES_H

#include <shared_tuki.h>

#include "comunicacion.h"
#include "planificacion.h"
#include "kernel.h"


// HANDLERS

void handle_wait(t_interrupcion_cpu* interrupcion, bool* liberar_pcb);
void handle_signal(t_interrupcion_cpu* interrupcion, bool* liberar_pcb);
void handle_io(t_interrupcion_cpu* interrupcion);
void handle_yield(t_interrupcion_cpu* interrupcion);
void handle_segmento_mayor_a_max(t_interrupcion_cpu* interrupcion);
void handle_create_segment(t_interrupcion_cpu* interrupcion, bool* liberar_pcb);
void handle_delete_segment(t_interrupcion_cpu* interrupcion);
void handle_f_open(t_interrupcion_cpu* interrupcion);
void handle_f_close(t_interrupcion_cpu* interrupcion, bool* liberar_pcb);
void handle_f_truncate(t_interrupcion_cpu* interrupcion);
void handle_f_seek(t_interrupcion_cpu* interrupcion);
void handle_f_read(t_interrupcion_cpu* interrupcion);
void handle_f_write(t_interrupcion_cpu* interrupcion);
void handle_exit(t_interrupcion_cpu* interrupcion, bool* liberar_pcb);

// HELPERS

void cerrar_archivo(t_pcb* pcb, char* nombre_archivo);
void exec_a_exit(char* motivo, t_pcb* pcb, bool* liberar_pcb);

#endif /* KERNEL_INTERRUPCIONES_H */
