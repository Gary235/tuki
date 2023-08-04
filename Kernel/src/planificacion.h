
#ifndef KERNEL_PLANIFICACION_H_
#define KERNEL_PLANIFICACION_H_

#include <shared_tuki.h>

#include "comunicacion.h"
#include "interrupciones.h"
#include "kernel.h"


#define FIFO "FIFO"
#define HRRN "HRRN"
#define BILLION 1000000000L;


typedef struct {
	int* segundos;
	t_pcb* pcb;
} t_io_blocked_args;


typedef struct {
	t_pcb* pcb;
	op_code operacion;
} t_fs_blocked_args;

void iniciar_planificacion();

double get_response_ratio(t_pcb* proceso, double ahora);
void* comparar_response_ratio(void* param_a, void* param_b);
void actualizar_estimacion_de_rafagas(double alfa, t_log* logger);

t_pcb* elegir_proximo_proceso();
char* get_lista_pids();
void agregar_a_ready(t_pcb* pcb, int desde_new);
void proceso_bloqueado_por_IO(t_io_blocked_args* args);
void proceso_bloqueado_por_FS(t_fs_blocked_args* args);

void new_a_ready();
void ready_a_exec();
void escuchar_respuestas_cpu();

extern sem_t sem_multiprogramacion_actual,
	sem_cont_cola_new,
	sem_cont_lista_ready,
	sem_ready_exec,
	sem_proceso_en_cpu,
	sem_proceso_en_fs
;

extern pthread_mutex_t mx_pid,
	mx_acceso_cola_new,
	mx_acceso_lista_ready,
	mx_acceso_tabla_segmentos,
	mx_acceso_tabla_archivos_abiertos,
	mx_acceso_cpu,
	mx_acceso_dic_recursos,
	mx_acceso_dic_archivos,
	mx_acceso_seguir_ejecutando
;

extern t_queue* cola_new;
extern t_list *lista_ready, *tabla_segmentos, *tabla_archivos_abiertos;
extern t_dictionary *dic_recursos, *dic_archivos;

extern int seguir_ejecutando, ultimo_ejecutado;
extern int cantidad_procesos_fs;
extern struct timespec tiempo_de_envio;


#endif /* KERNEL_PLANIFICACION_H_ */
