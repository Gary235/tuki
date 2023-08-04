#include "comunicacion.h"


void recibir_kernel(char* puerto) {
	int* cliente_fd = esperar_cliente(server_fd, logger);

	recibir_operacion(*cliente_fd); // recibe modulo
	recibir_operacion(*cliente_fd); // recibe cod_op
	recibir_mensaje(*cliente_fd, logger, 1);
	enviar_mensaje(OK, *cliente_fd, FILE_SYSTEM, MENSAJE);

	bool should_continue = true;
	while (should_continue) {
		recibir_operacion(*cliente_fd); // recibe modulo
		int cod_op = recibir_operacion(*cliente_fd);

		loggear_codigo(logger, cod_op);
		switch (cod_op) {
			case EXISTE_ARCHIVO:
				handle_existe_archivo(*cliente_fd);
				break;
			case F_TRUNCATE:
				handle_f_truncate(*cliente_fd);
				break;
			case F_READ:
				handle_f_read(*cliente_fd);
				break;
			case F_WRITE:
				handle_f_write(*cliente_fd);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				should_continue = false;
				break;
			default:
				log_warning(logger, "Operacion desconocida.");
				break;
		}
	}

	free(cliente_fd);
}


void enviar_archivo(t_archivo_abierto* archivo, int cliente_fd) {
	t_paquete *paquete = crear_paquete(FILE_SYSTEM, EXISTE_ARCHIVO);

	uint32_t tamanio_archivo =
		string_length(archivo->nombre) + 1 +
		string_length(archivo->path) + 1 +
		sizeof(int) * 3;

	paquete->buffer->size = tamanio_archivo;

	void *stream = malloc(tamanio_archivo);
	uint32_t offset = 0;

	serializar_archivo_abierto(stream, &offset, archivo);

	paquete->buffer->stream = stream;

	enviar_paquete(paquete, cliente_fd);
	eliminar_paquete(paquete);
	liberar_archivo((void*) archivo);
}

void solicitar_escritura_de_archivo(char* info_en_archivo, int tamanio_info, intptr_t* direccion_fisica, int pid) {
	t_paquete *paquete = crear_paquete(FILE_SYSTEM, F_READ);

	uint32_t tamanio_archivo = tamanio_info + sizeof(int) * 2 + sizeof(intptr_t);
	paquete->buffer->size = tamanio_archivo;

	void *stream = malloc(tamanio_archivo);
	uint32_t offset = 0;

	memcpy(stream + offset, direccion_fisica, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(stream + offset, &tamanio_info, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, info_en_archivo, tamanio_info);
	offset += tamanio_info;

	memcpy(stream + offset, &pid, sizeof(int));
	offset += sizeof(int);

	paquete->buffer->stream = stream;

	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);
}

void solicitar_lectura_de_memoria(int tamanio_info, intptr_t* direccion_fisica, int pid) {
	t_paquete* paquete = crear_paquete(FILE_SYSTEM, F_WRITE);

	uint32_t tamanio_archivo = sizeof(int) * 2 + sizeof(intptr_t);
	paquete->buffer->size = tamanio_archivo;

	void* stream = malloc(tamanio_archivo);
	uint32_t offset = 0;

	memcpy(stream + offset, direccion_fisica, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(stream + offset, &tamanio_info, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, &pid, sizeof(int));
	offset += sizeof(int);

	paquete->buffer->stream = stream;

	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);
}


void handle_existe_archivo(int cliente_fd) {
	char* nombre_archivo = recibir_mensaje(cliente_fd, logger, 0);
	char* path = string_new();
	string_append(&path, base_archivo);
	string_append(&path, nombre_archivo);

	bool es_archivo(void* param) {
		t_fcb* fcb = (t_fcb*) param;
		return strcmp(fcb->nombre_archivo, nombre_archivo) == 0;
	}

	log_info(logger, "Abrir Archivo: %s", nombre_archivo);

	t_fcb* fcb = list_find(fcbs, (void*) es_archivo);
	if (fcb == NULL) crear_fcb(nombre_archivo, path);

	t_archivo_abierto* archivo = alocar_archivo(string_length(nombre_archivo), string_length(path));
	strcpy(archivo->nombre, nombre_archivo);
	strcpy(archivo->path, path);
	*archivo->offset = 0;

	enviar_archivo(archivo, cliente_fd);

	free(nombre_archivo);
	free(path);
}

void handle_f_truncate(int cliente_fd) {
	int* size = malloc(sizeof(int));
	uint32_t offset = 0;
	t_interrupcion_cpu* interrupcion = alocar_interrupcion();

	void* buffer = recibir_buffer(size, cliente_fd);
	deserializar_f_truncate_seek(buffer, &offset, interrupcion);
	free(buffer);
	free(size);

	t_interrupcion_cpu_truncate_seek* data = (t_interrupcion_cpu_truncate_seek*) interrupcion->parametros;

	char* path = string_new();
	string_append(&path, base_archivo);
	string_append(&path, data->nombre_archivo);

	bool es_fcb(void* param) {
		t_fcb* fcb = (t_fcb*) param;
		return strcmp(fcb->nombre_archivo, data->nombre_archivo) == 0;
	}
	t_fcb* fcb = (t_fcb*) list_find(fcbs, (void*) es_fcb);

	log_info(logger, "Truncar Archivo: %s - Tamaño: %i", fcb->nombre_archivo, *data->entero);

	if (*data->entero == *fcb->tamanio) {
		log_info(logger, "El tamaño a truncar y el actual son iguales");
		enviar_mensaje(OK, cliente_fd, FILE_SYSTEM, F_TRUNCATE);
		liberar_interrupcion(interrupcion, 0);
		free(path);
		return;
	}

	bool hay_que_agrandar = *data->entero > *fcb->tamanio;

	int cant_bloques_actual = ceil((double) *fcb->tamanio / (double) tamanio_bloque);
	int cant_bloques_con_truncado = ceil((double) *data->entero / (double) tamanio_bloque);
	int diferencia_de_bloques = abs(cant_bloques_actual - cant_bloques_con_truncado);

	*fcb->tamanio = *data->entero;
	t_config* config_fcb = iniciar_config(path);
	char* parsed_tamanio = string_itoa((int) *data->entero);
	config_set_value(config_fcb, "TAMANIO_ARCHIVO", parsed_tamanio);
	config_save(config_fcb);
	config_destroy(config_fcb);
	free(parsed_tamanio);

	if (diferencia_de_bloques == 0) {
		log_info(logger, "No hace falta asignar o desasignar bloques");
		enviar_mensaje(OK, cliente_fd, FILE_SYSTEM, F_TRUNCATE);
		liberar_interrupcion(interrupcion, 0);
		free(path);
		return;
	}

	if (hay_que_agrandar) {
		asignar_bloques(diferencia_de_bloques, fcb);
	} else {
		desasignar_bloques(diferencia_de_bloques, fcb);
	}

	enviar_mensaje(OK, cliente_fd, FILE_SYSTEM, F_TRUNCATE);
	liberar_interrupcion(interrupcion, 0);
	free(path);
}

void handle_f_read(int cliente_fd) {
	int* size = malloc(sizeof(int));
	int offset_archivo, pid;
	uint32_t offset = 0;
	t_interrupcion_cpu* interrupcion = alocar_interrupcion();

	void* buffer = recibir_buffer(size, cliente_fd);
	deserializar_f_read_write(buffer, &offset, interrupcion);

	memcpy(&offset_archivo, buffer + offset, sizeof(int));
	offset += sizeof(int);

	memcpy(&pid, buffer + offset, sizeof(int));
	offset += sizeof(int);

	free(buffer);
	free(size);

	t_interrupcion_cpu_read_write* data = (t_interrupcion_cpu_read_write*) interrupcion->parametros;

	log_info(
		logger,
		"Leer Archivo: %s - Puntero: %i - Memoria: %ld - Tamaño: %i",
		data->nombre_archivo, offset_archivo, *data->direccion_fisica, *data->cantidad_bytes
	);

	bool es_fcb(void* param) {
		t_fcb* fcb = (t_fcb*) param;
		return strcmp(fcb->nombre_archivo, data->nombre_archivo) == 0;
	}
	t_fcb* fcb = (t_fcb*) list_find(fcbs, (void*) es_fcb);

	int indice = offset_archivo / tamanio_bloque;
	int offset_bloque = offset_archivo % tamanio_bloque;
	char* info_en_archivo = malloc(*data->cantidad_bytes);

	get_info_de_bloque(fcb, indice, offset_bloque, *data->cantidad_bytes, info_en_archivo);

	solicitar_escritura_de_archivo(info_en_archivo, *data->cantidad_bytes, data->direccion_fisica, pid);
	liberar_interrupcion(interrupcion, 0);
	free(info_en_archivo);

	// Esperar respuesta
	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op
	char* respuesta = recibir_mensaje(conexion_memoria, logger, 0);

	if (strcmp(respuesta, OK) != 0) {
		enviar_mensaje("No se pudo escribir el valor en memoria", cliente_fd, FILE_SYSTEM, F_READ);
	} else {
		enviar_mensaje(OK, cliente_fd, FILE_SYSTEM, F_READ);
	}

	free(respuesta);
}

void handle_f_write(int cliente_fd) {
	int* size = malloc(sizeof(int));
	int offset_archivo, pid;
	uint32_t offset = 0;
	t_interrupcion_cpu* interrupcion = alocar_interrupcion();

	void* buffer = recibir_buffer(size, cliente_fd);
	deserializar_f_read_write(buffer, &offset, interrupcion);

	memcpy(&offset_archivo, buffer + offset, sizeof(int));
	offset += sizeof(int);

	memcpy(&pid, buffer + offset, sizeof(int));
	offset += sizeof(int);

	free(buffer);
	free(size);

	t_interrupcion_cpu_read_write* data = (t_interrupcion_cpu_read_write*) interrupcion->parametros;

	log_info(
		logger,
		"Escribir Archivo: %s - Puntero: %i - Memoria: %ld - Tamaño: %i",
		data->nombre_archivo, offset_archivo, *data->direccion_fisica, *data->cantidad_bytes
	);

	solicitar_lectura_de_memoria(*data->cantidad_bytes, data->direccion_fisica, pid);

	// Esperar respuesta
	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op
	char* valor_leido = recibir_mensaje(conexion_memoria, logger, 0);

	bool es_fcb(void* param) {
		t_fcb* fcb = (t_fcb*) param;
		return strcmp(fcb->nombre_archivo, data->nombre_archivo) == 0;
	}
	t_fcb* fcb = (t_fcb*) list_find(fcbs, (void*) es_fcb);

	int indice = offset_archivo / tamanio_bloque;
	int offset_bloque = offset_archivo % tamanio_bloque;

	set_info_en_bloque(fcb, indice, offset_bloque, *data->cantidad_bytes, valor_leido);

	enviar_mensaje(OK, cliente_fd, FILE_SYSTEM, F_WRITE);
	liberar_interrupcion(interrupcion, 0);
	free(valor_leido);
}
