#ifndef KERNEL_COMUNICACION_H_
#define KERNEL_COMUNICACION_H_

#include <shared_tuki.h>

#include "planificacion.h"
#include "kernel.h"

extern int pid_actual;

void atender_consola(void* args);
void solicitar_tabla_de_segmentos_inicial(t_pcb* pcb);

t_segmento* deserializar_respuesta_crear_segmento();
op_code solicitar_crear_segmento(int id_segmento, int tamanio_segmento, t_pcb* pcb);
bool verificar_solicitud_crear_segmento(int id_segmento, t_pcb* pcb);
op_code solicitar_crear_segmento_en_memoria(int id_segmento, int tamanio_segmento, t_pcb *pcb);

void solicitar_borrar_segmento(int id_segmento, t_pcb* pcb);
bool verificar_solicitud_borrar_segmento(int id_segmento, int pid);
void solicitar_borrar_segmento_en_memoria(int id_segmento, t_pcb *pcb);
void solicitar_compactacion_segmentos(int pid);

void solicitar_existencia_de_archivo(char* nombre, t_pcb* pcb);
void solicitar_truncamiento_de_archivo(t_interrupcion_cpu* interrupcion);
void solicitar_escritura_lectura_de_archivo(t_interrupcion_cpu* interrupcion, int* offset_archivo);

#endif /* KERNEL_COMUNICACION_H_ */
