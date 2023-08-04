#include "comunicacion.h"

int pid_actual = 0;

void atender_consola(void* arg) {
	int* cliente_fd = (int*) arg;

	recibir_operacion(*cliente_fd); // recibe modulo
	int cod_op = recibir_operacion(*cliente_fd);

	if (cod_op == MENSAJE) {
		recibir_mensaje(*cliente_fd, logger, 1);
		enviar_mensaje("Mensaje Leido", *cliente_fd, KERNEL, MENSAJE);
	}

	if (cod_op == PAQUETE) {
		t_list* instrucciones = deserializar_instrucciones(*cliente_fd, NULL, NULL, 0);
		enviar_mensaje("Instrucciones procesadas", *cliente_fd, KERNEL, MENSAJE); // a la consola

	    pthread_mutex_lock(&mx_pid);
		t_pcb* pcb = alocar_pcb();
		setear_pcb(
			pcb,
			*cliente_fd,
			pid_actual,
			config_get_double_value(config, "ESTIMACION_INICIAL") / 1000,
			instrucciones
		);

	    log_info(logger, "\n");
	    log_info(logger, "Se crea el proceso %i en NEW", pid_actual);

		pid_actual++;
	    pthread_mutex_unlock(&mx_pid);

	    pthread_mutex_lock(&mx_acceso_cola_new);
	    queue_push(cola_new, pcb);
	    pthread_mutex_unlock(&mx_acceso_cola_new);

	    sem_post(&sem_cont_cola_new);
	}

	free(cliente_fd);
}

// MEMORIA

void solicitar_tabla_de_segmentos_inicial(t_pcb* pcb) {
	char* parsed_pid = string_itoa(*pcb->pid);
	enviar_mensaje(parsed_pid, conexion_memoria, KERNEL, SEGMENTOS_INICIAL);
	free(parsed_pid);

	log_info(logger, "PID: %i - Tabla de Segmentos Inicial Solicitada", *pcb->pid);

	// Esperar respuesta
	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op

	int* size = malloc(sizeof(int));
	uint32_t offset = 0;
	t_list* tabla_segmentos_inicial = list_create();

	void* buffer = recibir_buffer(size, conexion_memoria);
	deserializar_tabla_segmentos(buffer, &offset, tabla_segmentos_inicial);

	free(buffer);
	free(size);

	list_add_all(pcb->tabla_segmentos, tabla_segmentos_inicial);

	log_info(logger, "PID: %i - Tabla de Segmentos Inicial Recibida", *pcb->pid);

	pthread_mutex_lock(&mx_acceso_tabla_segmentos);
	if (list_is_empty(tabla_segmentos)) {
		// Se agrega a `tabla_segmentos` el segmento 0, la primera vez que es solicitado
		t_segmento* segmento_0 = list_get(tabla_segmentos_inicial, 0);
		t_segmento* copia_segmento_0 = alocar_segmento();
		*copia_segmento_0->id = *segmento_0->id;
		*copia_segmento_0->pid = *segmento_0->pid;
		*copia_segmento_0->tamanio = *segmento_0->tamanio;
		*copia_segmento_0->direccion_base = *segmento_0->direccion_base;

		list_add(tabla_segmentos, copia_segmento_0);
	}
	pthread_mutex_unlock(&mx_acceso_tabla_segmentos);

	list_destroy(tabla_segmentos_inicial);
}

t_segmento* deserializar_respuesta_crear_segmento() {
	int* size = malloc(sizeof(int));
	uint32_t offset = 0;
	void* buffer = recibir_buffer(size, conexion_memoria);

	t_segmento* segmento = alocar_segmento();

	memcpy(segmento->id, buffer + offset, sizeof(int));
	offset += sizeof(int);

	memcpy(segmento->pid, buffer + offset, sizeof(int));
	offset += sizeof(int);

	memcpy(segmento->direccion_base, buffer + offset, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(segmento->tamanio, buffer + offset, sizeof(int));
	offset += sizeof(int);

	free(buffer);
	free(size);
	return segmento;
}

op_code solicitar_crear_segmento(int id_segmento, int tamanio_segmento, t_pcb* pcb) {
	size_t tamanio_int = sizeof(int);
	t_paquete *paquete = crear_paquete(KERNEL, CREATE_SEGMENT);

	uint32_t tamanio_paquete = tamanio_int * 3;
	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	memcpy(stream + offset, &id_segmento, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, &tamanio_segmento, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, pcb->pid, tamanio_int);
	offset += tamanio_int;

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);

	// Esperar Respuesta
	recibir_operacion(conexion_memoria); // recibe modulo
	op_code status = recibir_operacion(conexion_memoria);

	if (status == CREATE_SEGMENT) {
		t_segmento* segmento = deserializar_respuesta_crear_segmento();

		log_info(logger, "PID: %i - Segmento Creado - Id: %i - Tamaño: %i", *pcb->pid, *segmento->id, *segmento->tamanio);

		pthread_mutex_lock(&mx_acceso_tabla_segmentos);
		list_add(tabla_segmentos, segmento);
		pthread_mutex_unlock(&mx_acceso_tabla_segmentos);

		t_segmento* copia_segmento = alocar_segmento();
		*copia_segmento->id = *segmento->id;
		*copia_segmento->pid = *segmento->pid;
		*copia_segmento->tamanio = *segmento->tamanio;
		*copia_segmento->direccion_base = *segmento->direccion_base;
		list_add(pcb->tabla_segmentos, copia_segmento);

		return CREATE_SEGMENT;
	}

	log_warning(logger, "PID: %i - No se pudo crear el segmento %i - Motivo: %s", *pcb->pid, id_segmento, get_nombre_por_codigo(status));
	recibir_mensaje(conexion_memoria, logger, 1);
	return status;
}

bool verificar_solicitud_crear_segmento(int id_segmento, t_pcb* pcb) {
	if (id_segmento == 0) {
		log_error(logger, "PID: %i - Solicitud de Creacion de Segmento Rechazada - Motivo: Id igual a 0", *pcb->pid);
		return false;
	}

	if (list_size(pcb->tabla_segmentos) + 1 > max_segmentos_por_proceso) {
		log_error(logger, "PID: %i - Solicitud de Creacion de Segmento Rechazada - Motivo: Id mayor al maximo", *pcb->pid);
		return false;
	}

	bool es_segmento_a_crear(void* param) {
		t_segmento* seg = (t_segmento*) param;
		return *seg->id == id_segmento && *seg->pid == *pcb->pid;
	}

	t_segmento* segmento_con_id_nuevo = (t_segmento*) list_find(tabla_segmentos, (void*) es_segmento_a_crear);
	if (segmento_con_id_nuevo != NULL) {
		log_error(
			logger,
			"PID: %i - Solicitud de Creacion de Segmento Rechazada - Motivo: Segmento #%i ya existe",
			*pcb->pid, *segmento_con_id_nuevo->id
		);
		return false;
	}

	return true;
}

op_code solicitar_crear_segmento_en_memoria(int id_segmento, int tamanio_segmento, t_pcb *pcb) {
	if (!verificar_solicitud_crear_segmento(id_segmento, pcb)) return NO_OP;

	log_info(logger, "PID: %i - Crear Segmento - Id: %i - Tamaño: %i", *pcb->pid, id_segmento, tamanio_segmento);

	op_code status = solicitar_crear_segmento(id_segmento, tamanio_segmento, pcb);
	return status;
}

void solicitar_borrar_segmento(int id_segmento, t_pcb* pcb) {
	size_t tamanio_int = sizeof(int);
	t_paquete *paquete = crear_paquete(KERNEL, DELETE_SEGMENT);

	uint32_t tamanio_paquete = tamanio_int * 2;
	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	memcpy(stream + offset, &id_segmento, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, pcb->pid, tamanio_int);
	offset += tamanio_int;

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);

	// Esperar Respuesta
	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op
	char* respuesta = recibir_mensaje(conexion_memoria, logger, 0);

	if (strcmp(respuesta, OK) == 0) {
		bool es_segmento_a_borrar(void* param) {
			t_segmento* segmento_en_tabla = (t_segmento*) param;
			return *segmento_en_tabla->id == id_segmento && *segmento_en_tabla->pid == *pcb->pid;
		}

		pthread_mutex_lock(&mx_acceso_tabla_segmentos);
		list_remove_and_destroy_by_condition(tabla_segmentos, (void*) es_segmento_a_borrar, (void*) liberar_segmento);
		pthread_mutex_unlock(&mx_acceso_tabla_segmentos);

		list_remove_and_destroy_by_condition(pcb->tabla_segmentos, (void*) es_segmento_a_borrar, (void*) liberar_segmento);
	} else {
		log_error(logger, "Fallo al borrar segmento");
	}

	free(respuesta);
}

bool verificar_solicitud_borrar_segmento(int id_segmento, int pid) {
	if (id_segmento == 0) {
		log_error(logger, "PID: %i - Solicitud de Eliminacion de Segmento Rechazada - Motivo: Id igual a 0", pid);
		return false;
	}

	bool es_segmento_a_borrar(void* param) {
		t_segmento* seg = (t_segmento*) param;
		return *seg->id == id_segmento && *seg->pid == pid;
	}

	void* ptr_a_segmento = list_find(tabla_segmentos, (void*) es_segmento_a_borrar);
	if (ptr_a_segmento == NULL) {
		log_error(
			logger,
			"PID: %i - Solicitud de Eliminacion de Segmento Rechazada - Motivo: Segmento #%i no existe",
			pid, id_segmento
		);
		return false;
	}

	return true;
}

void solicitar_borrar_segmento_en_memoria(int id_segmento, t_pcb* pcb) {
	if (!verificar_solicitud_borrar_segmento(id_segmento, *pcb->pid)) return;

	log_info(logger, "PID: %i - Eliminar Segmento - Id: %i", *pcb->pid, id_segmento);

	solicitar_borrar_segmento(id_segmento, pcb);
}

void solicitar_compactacion_segmentos(int pid) {
	log_info(logger, "PID: %i - Compactación: Esperando Fin de Operaciones de FS", pid);
	sem_wait(&sem_proceso_en_fs);

	char* parsed_pid = string_itoa(pid);
	enviar_mensaje(parsed_pid, conexion_memoria, KERNEL, COMPACTACION);
	free(parsed_pid);

	log_info(logger, "PID: %i - Compactación: Se solicitó compactación", pid);

	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op
	sem_post(&sem_proceso_en_fs);

	int* size = malloc(sizeof(int));
	uint32_t offset = 0;
	void* buffer = recibir_buffer(size, conexion_memoria);

	t_list* nueva_tabla_segmentos = list_create();
	deserializar_tabla_segmentos(buffer, &offset, nueva_tabla_segmentos);

	free(buffer);
	free(size);

	pthread_mutex_lock(&mx_acceso_tabla_segmentos);
	list_clean_and_destroy_elements(tabla_segmentos, (void*) liberar_segmento);
	list_add_all(tabla_segmentos, nueva_tabla_segmentos);
	list_destroy(nueva_tabla_segmentos);
	pthread_mutex_unlock(&mx_acceso_tabla_segmentos);

	void actualizar_segmentos(void* arg) {
		t_pcb* pcb = (t_pcb*) arg;
		if (list_size(pcb->tabla_segmentos) <= 1) return;

		int i = 0;
		t_link_element* elemento = pcb->tabla_segmentos->head;
		while (elemento != NULL) {
			if (i == 0) {
				i++;
				elemento = elemento->next;
				continue;
			}

			t_segmento* segmento = (t_segmento*) elemento->data;

			bool es_segmento(void* arg) {
				t_segmento* segmento_en_tabla = (t_segmento*) arg;
				return *segmento_en_tabla->id == *segmento->id && *segmento_en_tabla->pid == *pcb->pid;
			}

			t_segmento* segmento_matcheado = (t_segmento*) list_find(tabla_segmentos, (void*) es_segmento);
			*segmento->direccion_base = *segmento_matcheado->direccion_base;

			i++;
			elemento = elemento->next;
		}
	}

	pthread_mutex_lock(&mx_acceso_lista_ready);
	list_iterate(lista_ready, (void*) actualizar_segmentos);
	pthread_mutex_unlock(&mx_acceso_lista_ready);

	void actualizar_segmentos_diccionario(void* arg){
		t_data_recurso* data = (t_data_recurso*) arg;
		if (queue_is_empty(data->cola_bloqueo)) return;

		list_iterate(data->cola_bloqueo->elements, (void*) actualizar_segmentos);
	}

    pthread_mutex_lock(&mx_acceso_dic_recursos);
	t_list* valores_dic_recurso = dictionary_elements(dic_recursos);
	list_iterate(valores_dic_recurso, (void*) actualizar_segmentos_diccionario);
	list_destroy(valores_dic_recurso);
    pthread_mutex_unlock(&mx_acceso_dic_recursos);

    pthread_mutex_lock(&mx_acceso_dic_archivos);
	t_list* valores_dic_archivo = dictionary_elements(dic_archivos);
	list_iterate(valores_dic_archivo, (void*) actualizar_segmentos_diccionario);
	list_destroy(valores_dic_archivo);
    pthread_mutex_unlock(&mx_acceso_dic_archivos);

	log_info(logger, "Se finalizó el proceso de compactación");
}

// FILE SYSTEM

void solicitar_existencia_de_archivo(char* nombre, t_pcb* pcb) {
	sem_wait(&sem_proceso_en_fs);

	enviar_mensaje(nombre, conexion_fs, KERNEL, EXISTE_ARCHIVO);

	// Esperar respuesta
	recibir_operacion(conexion_fs); // recibe modulo
	recibir_operacion(conexion_fs); // recibe cod_op

	log_info(logger, "PID: %i - Archivo abierto con exito", *pcb->pid);

	int* size = malloc(sizeof(int));
	uint32_t offset = 0;
	t_archivo_abierto* archivo = alocar_archivo(0, 0);

	void* buffer = recibir_buffer(size, conexion_fs);
	deserializar_archivo_abierto(buffer, &offset, archivo);

	sem_post(&sem_proceso_en_fs);
	free(buffer);
	free(size);

	pthread_mutex_lock(&mx_acceso_tabla_archivos_abiertos);
	list_add(tabla_archivos_abiertos, archivo);
	pthread_mutex_unlock(&mx_acceso_tabla_archivos_abiertos);

	t_data_recurso* data = malloc(sizeof(t_data_recurso));
	data->recurso = malloc(string_length(archivo->nombre) + 1);
	strcpy(data->recurso, archivo->nombre);
	data->instancias_libres = 0; // Arranca el proceso con el archivo
	data->cola_bloqueo = queue_create();
	data->procesos_con_recurso = list_create();

	int* pid_proceso = malloc(sizeof(int));
	*pid_proceso = *pcb->pid;
	list_add(data->procesos_con_recurso, pid_proceso);

	pthread_mutex_lock(&mx_acceso_dic_archivos);
	dictionary_put(dic_archivos, data->recurso, data);
	pthread_mutex_unlock(&mx_acceso_dic_archivos);

	t_archivo_abierto* copia_archivo = alocar_archivo(string_length(archivo->nombre), string_length(archivo->path));
	strcpy(copia_archivo->nombre, archivo->nombre);
	strcpy(copia_archivo->path, archivo->path);
	*copia_archivo->offset = *archivo->offset;

	list_add(pcb->archivos_abiertos, copia_archivo);
}

void solicitar_truncamiento_de_archivo(t_interrupcion_cpu* interrupcion) {
	size_t tamanio_int = sizeof(int);
	t_paquete *paquete = crear_paquete(KERNEL, F_TRUNCATE);

	uint32_t tamanio_paquete =
			tamanio_int * 2 +
			string_length(((t_interrupcion_cpu_truncate_seek*) interrupcion->parametros)->nombre_archivo) + 1;
	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	serializar_f_truncate_seek(stream, &offset, interrupcion);

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_fs);
	eliminar_paquete(paquete);
}

void solicitar_escritura_lectura_de_archivo(t_interrupcion_cpu* interrupcion, int* offset_archivo) {
	size_t tamanio_int = sizeof(int);
	t_paquete *paquete = crear_paquete(KERNEL, *interrupcion->codigo);

	uint32_t tamanio_paquete =
			tamanio_int * 4 +
			sizeof(intptr_t) +
			string_length(((t_interrupcion_cpu_read_write*) interrupcion->parametros)->nombre_archivo) + 1;
	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	serializar_f_read_write(stream, &offset, interrupcion);

	memcpy(stream + offset, offset_archivo, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, interrupcion->pcb->pid, tamanio_int);
	offset += tamanio_int;

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_fs);
	eliminar_paquete(paquete);
}
