
#ifndef MEMORIA_OPERACIONES_MODULOS_H_
#define MEMORIA_OPERACIONES_MODULOS_H_

#include <shared_tuki.h>

#include "memoria.h"

#define BEST "BEST"
#define FIRST "FIRST"
#define WORST "WORST"

// ---- HANDLERS ----

void handle_kernel(op_code operacion);
void handle_cpu(op_code operacion);
void handle_fs(op_code operacion);

void handle_kernel_nuevo_proceso();
void handle_kernel_create_segment();
void handle_kernel_delete_segment();
void handle_kernel_compactacion();

// ---- HELPERS ----

void escribir_en_memoria(int conexion, char* origen);
void leer_memoria(int conexion, char* origen);

char* deserializar_escribir_memoria(
	int socket_cliente,
	int* tamanio_valor,
	intptr_t* direccion_fisica,
	int* pid
);
void deserializar_leer_memoria(
		int socket_cliente,
		intptr_t* direccion_fisica,
		int* tamanio_a_leer,
		int* pid
);
void deserializar_crear_segmento(
		int socket_cliente,
		int* id_segmento,
		int* tamanio_segmento,
		int* pid
);
void deserializar_borrar_segmento(int socket_cliente, int* id_segmento, int* pid);

bool comparar_direccion_base(void* param_a, void* param_b);
void unir_huecos();
void insertar_segmento(
	t_segmento* nuevo_segmento,
	t_segmento* hueco,
	int id,
	int tamanio,
	intptr_t direccion_base,
	bool(*condition)(void*),
	int pid
);
op_code crear_segmento(int tamanio_segmento, int id_segmento, t_segmento* nuevo_segmento, int pid);
op_code borrar_segmento(int id_a_borrar, int pid);

void enviar_respuesta_crear_segmento(t_segmento* segmento);
void compactar_segmentos();

#endif /* MEMORIA_OPERACIONES_MODULOS_H_ */
