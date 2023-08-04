#ifndef SHARED_TUKI_H_
#define SHARED_TUKI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <math.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>

#include <pthread.h>
#include <semaphore.h>

#define CONFIG_PATH_BASE "/home/utnso/tp-2023-1c-Los-Magios/archivos_config/"
#define HOLA "HOLA"
#define OK "OK"

typedef enum {
//  -------------- Tipos de stream
	MENSAJE,
	PAQUETE,
//  -------------- Modulos
	CONSOLA,
	KERNEL,
	CPU,
	FILE_SYSTEM,
	MEMORIA,
	INSTRUCCIONES, // Usado como modulo para generar el config_path
//  -------------- Tipo de interrupcion CPU
	EXIT,
	IO,
	WAIT,
	YIELD,
	SIGNAL,
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
	CREATE_SEGMENT,
	DELETE_SEGMENT,
	NO_OP,
	SEGMENTO_MAYOR_A_MAX,
//  -------------- Tipo de interrupcion Memoria
	OUT_OF_MEMORY,
	COMPACTACION,
//  -------------- Tipo de solicitud
	SEGMENTOS_INICIAL,
	ESCRIBIR_MEMORIA,
	LEER_MEMORIA,
	EXISTE_ARCHIVO
} op_code;

typedef struct {
	int size;
	void *stream;
} t_buffer;

typedef struct {
	op_code modulo_origen;
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

typedef struct {
	char* operacion;
	char* parametro_0;
	char* parametro_1;
	char* parametro_2;
} t_instruccion;

typedef struct {
	char* ip;
	char* puerto;
	t_log *logger;
	t_config *config;
	int modulo;
} t_thread_modulo_args;

typedef struct {
	int cliente_fd;
	t_log* logger;
} t_thread_cliente_args;

typedef struct {
	char* AX;  char* BX;  char* CX;  char* DX;
	char* EAX; char* EBX; char* ECX; char* EDX;
	char* RAX; char* RBX; char* RCX; char* RDX;
} t_registros_cpu;

typedef struct {
	int* id;
	int* pid;
	intptr_t* direccion_base;
	int* tamanio;
} t_segmento;

typedef struct {
	op_code* status;
	t_segmento* segmento;
} t_respuesta_crear_segmento;

typedef struct {
	int* id_segmento;
	int* tamanio_segmento;
} t_interrupcion_cpu_crear_segmento;

typedef struct {
	char* nombre_archivo;
	int* tamanio_nombre;
	int* entero;
} t_interrupcion_cpu_truncate_seek;

typedef struct {
	char* nombre_archivo;
	int* tamanio_nombre;
	intptr_t* direccion_fisica;
	int* cantidad_bytes;
} t_interrupcion_cpu_read_write;

typedef struct {
	char* nombre_archivo;
	int* tamanio;
	uint32_t* puntero_directo;
	uint32_t* puntero_indirecto;
} t_fcb;

typedef struct {
	char* nombre;
	char* path;
	int* offset;
} t_archivo_abierto;

typedef struct {
	int* socket_cliente;
	int* pid;
	int* pc;
	double* estimacion_proxima_rafaga;
	double* tiempo_de_ejecucion;
	double* tiempo_llegada_ready;
	t_registros_cpu* registros;
	t_list* instrucciones;
	t_list* tabla_segmentos;
	t_list* archivos_abiertos;
} t_pcb;

typedef struct {
	char* recurso;
	int instancias_libres;
	t_queue* cola_bloqueo;
	t_list* procesos_con_recurso;
} t_data_recurso;

typedef struct {
	op_code* codigo;
	void* parametros;
	int* tamanio_parametros;
	t_pcb* pcb;
} t_interrupcion_cpu;


// ---- UTILS ----

char* get_nombre_por_codigo(op_code codigo);
void loggear_codigo(t_log* logger, op_code codigo);
char* get_config_path(op_code modulo, char* archivo);
int cantidad_elementos(char** elementos);
bool string_en_string_array(char** array, char* elemento);
bool contiene_new_line(char* texto);
char* remover_ultimo_caracter(char* texto);
void agregar_padding(char* texto, char* pad, int maximo);
t_log* iniciar_logger(char* logger_name, int show_in_console);
t_config* iniciar_config(char* config_name);
char* get_val_registro(t_pcb* pcb, char* registro);
int get_tam_registro(char* registro);

// ---- CONEXION ----

int crear_conexion(char *ip, char *puerto);
int iniciar_servidor(t_log *logger, char *ip, char *puerto);
int* esperar_cliente(int socket_servidor, t_log *logger);
void recibir_clientes(t_log* logger, int server_fd, char *puerto, void (*atencion_al_cliente)(void*));
int conectar_con_modulo(char* ip, char* puerto, op_code modulo_origen, t_log* logger, char* buffer, int chequear_OK);

// ---- SERIALIZACION ----

void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(op_code modulo_origen, op_code codigo_operacion);
void enviar_mensaje(char *mensaje, int socket_cliente, op_code modulo_origen, op_code operacion);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int *size, int socket_cliente);
char* recibir_mensaje(int socket_cliente, t_log *logger, int liberar_mensaje);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void* serializar_paquete(t_paquete *paquete, int bytes);
void eliminar_paquete(t_paquete *paquete);

// ---- INSTRUCCIONES ----

uint32_t calcular_tamanio_instrucciones(t_list *lista);
void* serializar_instrucciones(uint32_t size, t_list *lista);
t_list* deserializar_instrucciones(int socket_cliente, void* buffer_inicializado, uint32_t* offset, int size_inicializado);
t_instruccion* parsear_instruccion(char *linea_instruccion);
t_instruccion* alocar_instruccion(int op_len, int par0_len, int par1_len, int par2_len);
void liberar_instruccion(void* param);
void enviar_instrucciones(int socket_fd, t_list *lista);

// ---- PCB ----

t_archivo_abierto* alocar_archivo(int len_nombre, int len_path);
t_segmento* alocar_segmento();
t_registros_cpu* alocar_registros_cpu();
t_pcb* alocar_pcb();
t_interrupcion_cpu* alocar_interrupcion();

void liberar_recurso(void* param);
void liberar_archivo(void* param);
void liberar_segmento(void* param);
void liberar_registros_cpu(t_pcb* pcb);
void liberar_pcb(t_pcb* pcb);
void liberar_create_segment(t_interrupcion_cpu* interrupcion);
void liberar_f_truncate_seek(t_interrupcion_cpu* interrupcion);
void liberar_f_read_write(t_interrupcion_cpu* interrupcion);
void liberar_parametros_interrupcion(t_interrupcion_cpu* interrupcion);
void liberar_interrupcion(t_interrupcion_cpu* interrupcion, bool libera_pcb);

void setear_pcb(
		t_pcb* pcb,
		int socket_cliente,
		int pid,
		double estimacion_proxima_rafaga,
		t_list* instrucciones
);
void setear_interrupcion(t_interrupcion_cpu* interrupcion, op_code codigo, void* parametros, int tamanio_parametros, t_pcb* pcb);

uint32_t calcular_tamanio_archivos_abiertos(t_list* archivos_abiertos);
uint32_t calcular_tamanio_tabla_segmentos(t_list* tabla_segmentos);
uint32_t calcular_tamanio_pcb(t_pcb* pcb);
uint32_t calcular_tamanio_interrupcion(t_interrupcion_cpu* interrupcion);

void serializar_registros_cpu(void* stream, uint32_t* offset, t_registros_cpu* registros);
void serializar_tabla_segmentos(void* stream, uint32_t* offset, t_list* tabla_segmentos);
void serializar_archivo_abierto(void* stream, uint32_t* offset, t_archivo_abierto* archivo);
void serializar_archivos_abiertos(void* stream, uint32_t* offset, t_list* archivos_abiertos);
void* serializar_pcb(uint32_t size, t_pcb* pcb);

void serializar_un_parametro(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void serializar_create_segment(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void serializar_f_truncate_seek(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void serializar_f_read_write(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void serializar_parametros_interrupcion(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void* serializar_interrupcion(uint32_t size, t_interrupcion_cpu* interrupcion);

void deserializar_tabla_segmentos(void* stream, uint32_t* offset, t_list* tabla_segmentos);
void deserializar_archivo_abierto(void* buffer, uint32_t* offset, t_archivo_abierto* archivo);
void deserializar_archivos_abiertos(void* stream, uint32_t* offset, t_list* archivos_abiertos);
void deserializar_registros_cpu(void* buffer, uint32_t* offset, t_registros_cpu* registros);
t_pcb* deserializar_pcb(int socket_cliente,  void* buffer_inicializado, uint32_t* offset_inicializado, int size_inicializado);

void deserializar_un_parametro(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void deserializar_create_segment(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void deserializar_f_truncate_seek(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void deserializar_f_read_write(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion);
void deserializar_parametros_interrupcion(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion);
t_interrupcion_cpu* deserializar_interrupcion(int socket_cliente);

void enviar_proceso(int socket_fd, t_pcb* pcb, op_code modulo_origen);
void enviar_interrupcion(int socket_fd, t_interrupcion_cpu* interrupcion, op_code modulo_origen);

#endif /* SHARED_TUKI_H_ */
