#include "operaciones_modulos.h"

// ---- HANDLERS ----

void handle_kernel(op_code operacion) {
	log_info(logger, "Comunicacion desde el Kernel");

	switch(operacion) {
		case SEGMENTOS_INICIAL:
			handle_kernel_nuevo_proceso();
			break;
		case CREATE_SEGMENT:
			handle_kernel_create_segment();
			break;
		case DELETE_SEGMENT:
			handle_kernel_delete_segment();
			break;
		case COMPACTACION:
			handle_kernel_compactacion();
			break;
		default:
			break;
	}
}

void handle_cpu(op_code operacion) {
	log_info(logger, "Comunicacion desde la CPU");

	switch(operacion) {
		case ESCRIBIR_MEMORIA:
			escribir_en_memoria(conexion_cpu, "CPU");
			break;
		case LEER_MEMORIA:
			leer_memoria(conexion_cpu, "CPU");
			break;
		default:
			break;
	}
}

void handle_fs(op_code operacion) {
	log_info(logger, "Comunicacion desde el FS");

	switch(operacion) {
		case F_READ:
			escribir_en_memoria(conexion_fs, "FS");
			break;
		case F_WRITE:
			leer_memoria(conexion_fs, "FS");
			break;
		default:
			break;
	}
}

void handle_kernel_nuevo_proceso() {
	char* pid = recibir_mensaje(conexion_kernel, logger, 0);
	log_info(logger, "Creación de Proceso PID: %s", pid);
	log_info(logger, "PID: %s - Tabla de Segmentos Inicial Solicitada", pid);

	t_list* tabla_segmentos_inicial = list_create();
	t_segmento* segmento_0 = (t_segmento*) list_get(tabla_segmentos, 0);
	t_segmento* copia_segmento_0 = alocar_segmento();
	*copia_segmento_0->id = *segmento_0->id;
	*copia_segmento_0->pid = *segmento_0->pid;
	*copia_segmento_0->tamanio = *segmento_0->tamanio;
	*copia_segmento_0->direccion_base = *segmento_0->direccion_base;
	list_add(tabla_segmentos_inicial, copia_segmento_0);

	t_paquete *paquete = crear_paquete(MEMORIA, PAQUETE);

	uint32_t tamanio = calcular_tamanio_tabla_segmentos(tabla_segmentos_inicial);
	paquete->buffer->size = tamanio;

	void* stream = malloc(tamanio);
	uint32_t offset = 0;

	serializar_tabla_segmentos(stream, &offset, tabla_segmentos_inicial);
	paquete->buffer->stream = stream;

	enviar_paquete(paquete, conexion_kernel);
	eliminar_paquete(paquete);

	list_clean_and_destroy_elements(tabla_segmentos_inicial, (void*) liberar_segmento);
	list_destroy(tabla_segmentos_inicial);

	log_info(logger, "PID: %s - Tabla de Segmentos Inicial Enviada", pid);
	free(pid);
}

void handle_kernel_create_segment() {
	int tamanio_nuevo_segmento, id_nuevo_segmento, pid;

	deserializar_crear_segmento(conexion_kernel, &id_nuevo_segmento, &tamanio_nuevo_segmento, &pid);

	t_segmento* nuevo_segmento = alocar_segmento();
	op_code resultado = crear_segmento(tamanio_nuevo_segmento, id_nuevo_segmento, nuevo_segmento, pid);

	if (resultado == CREATE_SEGMENT) {
		log_info(
			logger, 
			"PID: %i - Crear Segmento: %i - Base: %ld - TAMAÑO: %i",
			pid, *nuevo_segmento->id, *nuevo_segmento->direccion_base, *nuevo_segmento->tamanio
		);
		enviar_respuesta_crear_segmento(nuevo_segmento);
	} else {
		liberar_segmento((void*) nuevo_segmento);
		log_warning(
			logger,
			"PID: %i - No se pudo crear el segmento %i - Motivo: %s",
			pid, id_nuevo_segmento, get_nombre_por_codigo(resultado)
		);
		enviar_mensaje("No se pudo crear el segmento", conexion_kernel, MEMORIA, resultado);
	}
}

void handle_kernel_delete_segment() {
	int id, pid;

	deserializar_borrar_segmento(conexion_kernel, &id, &pid);
	op_code status = borrar_segmento(id, pid);

	if (status == DELETE_SEGMENT) enviar_mensaje(OK, conexion_kernel, MEMORIA, MENSAJE);
	else {
		enviar_mensaje("No se pudo eliminar el segmento", conexion_kernel, MEMORIA, MENSAJE);
	}
}

void handle_kernel_compactacion() {
	char* pid = recibir_mensaje(conexion_kernel, logger, 0);

	log_info(logger, "PID: %s - Solicitud de Compactación", pid);

	compactar_segmentos();

	sleep(config_get_int_value(config, "RETARDO_COMPACTACION") / 1000);

	log_info(logger, "PID: %s - Compactación Realizada", pid);

	t_paquete* paquete = crear_paquete(MEMORIA, PAQUETE);
	uint32_t tamanio = calcular_tamanio_tabla_segmentos(tabla_segmentos);
	paquete->buffer->size = tamanio;

	void* stream = malloc(tamanio);
	uint32_t offset = 0;

	serializar_tabla_segmentos(stream, &offset, tabla_segmentos);
	paquete->buffer->stream = stream;

	enviar_paquete(paquete, conexion_kernel);
	eliminar_paquete(paquete);
	free(pid);
}


// ---- HELPERS ----

void escribir_en_memoria(int conexion, char* origen) {
	int tamanio_valor, pid;
	intptr_t* df_a_escribir = malloc(sizeof(intptr_t));

	char* valor_a_escribir = deserializar_escribir_memoria(
		conexion,
		&tamanio_valor,
		df_a_escribir,
		&pid
	);

	log_info(
		logger, 
		"PID: %i - Acción: ESCRIBIR - Dirección física: %ld - Tamaño: %i - Origen: %s",
		pid, *df_a_escribir, tamanio_valor, origen
	);
	memcpy((void*) *df_a_escribir, valor_a_escribir, tamanio_valor);

	if (valor_a_escribir[tamanio_valor] == '\0') {
		log_info(logger, "valor a escribir(1): %s", valor_a_escribir);
	} else {
		char* valor_loggeable = malloc(tamanio_valor + 1);
		memcpy(valor_loggeable, valor_a_escribir, tamanio_valor);
		valor_loggeable[tamanio_valor + 1] = '\0';

		log_info(logger, "valor a escribir(2): %s", valor_loggeable);

		free(valor_loggeable);
	}

	sleep(config_get_int_value(config, "RETARDO_MEMORIA") / 1000);

	enviar_mensaje(OK, conexion, MEMORIA, MENSAJE);

	free(df_a_escribir);
	free(valor_a_escribir);
}

void leer_memoria(int conexion, char* origen) {
	int tamanio_a_leer, pid;
	intptr_t* df_a_leer = malloc(sizeof(intptr_t));

	deserializar_leer_memoria(conexion, df_a_leer, &tamanio_a_leer, &pid);

	log_info(
		logger, 
		"PID: %i - Acción: LEER - Dirección física: %ld - Tamaño: %i - Origen: %s",
		pid, *df_a_leer, tamanio_a_leer, origen
	);

	char* valor_a_leer = malloc(tamanio_a_leer);

	memcpy(valor_a_leer, (void*) *df_a_leer, tamanio_a_leer);

	if (valor_a_leer[tamanio_a_leer] == '\0') {
		log_info(logger, "valor a leer(1): %s", valor_a_leer);
	} else {
		char* valor_loggeable = malloc(tamanio_a_leer + 1);
		memcpy(valor_loggeable, valor_a_leer, tamanio_a_leer);
		valor_loggeable[tamanio_a_leer + 1] = '\0';

		log_info(logger, "valor a leer(2): %s", valor_loggeable);

		free(valor_loggeable);
	}

	sleep(config_get_int_value(config, "RETARDO_MEMORIA") / 1000);

	enviar_mensaje(valor_a_leer, conexion, MEMORIA, MENSAJE);

	free(df_a_leer);
	free(valor_a_leer);
}


char* deserializar_escribir_memoria(
	int socket_cliente,
	int* tamanio_valor,
	intptr_t* direccion_fisica,
	int* pid
) {
	int* size = malloc(sizeof(int));
	size_t tamanio_int = sizeof(int);
	uint32_t offset = 0;
	void* buffer = recibir_buffer(size, socket_cliente);

	memcpy(direccion_fisica, buffer + offset, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(tamanio_valor, buffer + offset, tamanio_int);
	offset += tamanio_int;

	char* valor = malloc(*tamanio_valor);
	memcpy(valor, buffer + offset, *tamanio_valor);
	offset += *tamanio_valor;

	memcpy(pid, buffer + offset, tamanio_int);
	offset += tamanio_int;

	free(buffer);
	free(size);

	return valor;
}

void deserializar_leer_memoria(
	int socket_cliente,
	intptr_t* direccion_fisica,
	int* tamanio_a_leer,
	int* pid
) {
	int* size = malloc(sizeof(int));
	size_t tamanio_int = sizeof(int);
	uint32_t offset = 0;
	void* buffer = recibir_buffer(size, socket_cliente);

	memcpy(direccion_fisica, buffer + offset, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(tamanio_a_leer, buffer + offset, tamanio_int);
	offset += tamanio_int;

	memcpy(pid, buffer + offset, tamanio_int);
	offset += tamanio_int;

	free(buffer);
	free(size);
}

void deserializar_crear_segmento(
	int socket_cliente,
	int* id_segmento,
	int* tamanio_segmento,
	int* pid
) {
	int* size = malloc(sizeof(int));
	size_t tamanio_int = sizeof(int);
	uint32_t offset = 0;
	void* buffer = recibir_buffer(size, socket_cliente);

	memcpy(id_segmento, buffer + offset, tamanio_int);
	offset += tamanio_int;

	memcpy(tamanio_segmento, buffer + offset, tamanio_int);
	offset += tamanio_int;

	memcpy(pid, buffer + offset, tamanio_int);
	offset += tamanio_int;

	free(buffer);
	free(size);
}

void deserializar_borrar_segmento(int socket_cliente, int* id_segmento, int* pid) {
	int* size = malloc(sizeof(int));
	size_t tamanio_int = sizeof(int);
	uint32_t offset = 0;
	void* buffer = recibir_buffer(size, socket_cliente);

	memcpy(id_segmento, buffer + offset, tamanio_int);
	offset += tamanio_int;

	memcpy(pid, buffer + offset, tamanio_int);
	offset += tamanio_int;

	free(buffer);
	free(size);
}

bool comparar_direccion_base(void* param_a, void* param_b) {
	t_segmento* a = (t_segmento*) param_a;
	t_segmento* b = (t_segmento*) param_b;

	return *a->direccion_base < *b->direccion_base;
}

void unir_huecos() {
	log_info(logger, "Empieza a unir huecos: %i", huecos_libres->elements_count);

	// Ordenar huecos libres por direccion asi los adyacentes quedan juntos
	list_sort(huecos_libres, comparar_direccion_base);

	t_link_element* elemento = huecos_libres->head;
	while (elemento != NULL) {
		t_segmento* hueco = elemento->data;
		t_list* huecos_a_unir = list_create();

		t_link_element* siguiente_elemento = elemento->next;
		while (siguiente_elemento != NULL) {
			t_segmento* siguiente_hueco = siguiente_elemento->data;

			bool es_siguiente_hueco(void* param) {
				t_segmento* hueco_en_lista = (t_segmento*) param;
				return *hueco_en_lista->id == *siguiente_hueco->id;
			}

			// Al comparar, la primera iteracion `huecos_a_unir` esta vacia, por lo que comparamos con `hueco`
			// Si no esta vacia comparamos con el ultimo elemento de `huecos_a_unir`
			t_segmento* hueco_a_comparar;
			if (list_is_empty(huecos_a_unir)) hueco_a_comparar = hueco;
			else hueco_a_comparar = (t_segmento*) list_get(huecos_a_unir, huecos_a_unir->elements_count - 1);

			log_info(
				logger,
				"(dir) siguiente_hueco: %ld - (dir + tam) hueco_a_comparar: %ld",
				*siguiente_hueco->direccion_base, *hueco_a_comparar->direccion_base + *hueco_a_comparar->tamanio
			);

			// Si el siguiente hueco es adayacente, se lo agrega a `huecos_a_unir` y se lo saca de `huecos_libres`
			if (*siguiente_hueco->direccion_base == *hueco_a_comparar->direccion_base + *hueco_a_comparar->tamanio) {
				list_remove_by_condition(huecos_libres, (void*) es_siguiente_hueco);
				list_add(huecos_a_unir, siguiente_hueco);
			} else {
				break; // Si no es adyacente, significa que ya no puede compactar mas
			}

			siguiente_elemento = siguiente_elemento->next;
		}

		log_info(logger, "huecos_a_unir: %i", huecos_a_unir->elements_count);

		if (list_is_empty(huecos_a_unir)) {
			list_destroy(huecos_a_unir);
			elemento = elemento->next;
			continue;
		}

		int acumulador_tamanio = 0;

		void sumar_tamanio(void* param) {
			t_segmento* hueco = (t_segmento*) param;
			acumulador_tamanio += *hueco->tamanio;
		}

		list_iterate(huecos_a_unir, (void*) sumar_tamanio);

		*hueco->tamanio += acumulador_tamanio; // Al sumar el tamanio, el hueco "padre", se "come" a los otros

		list_clean_and_destroy_elements(huecos_a_unir, (void*) liberar_segmento);
		list_destroy(huecos_a_unir);
		elemento = elemento->next;
	}

	log_info(logger, "Termino de unir huecos: %i", huecos_libres->elements_count);
}

void insertar_segmento(
	t_segmento* nuevo_segmento,
	t_segmento* hueco,
	int id,
	int tamanio,
	intptr_t direccion_base,
	bool(*condition)(void*),
	int pid
) {
	*nuevo_segmento->id = id;
	*nuevo_segmento->pid = pid;
	*nuevo_segmento->tamanio = tamanio;
	*nuevo_segmento->direccion_base = direccion_base;

	list_add(tabla_segmentos, nuevo_segmento);

	if (*hueco->tamanio == tamanio) {
		list_remove_and_destroy_by_condition(huecos_libres, condition, (void*) liberar_segmento);
	} else {
		*hueco->direccion_base = *hueco->direccion_base + tamanio;
		*hueco->tamanio = *hueco->tamanio - tamanio;
	}
}

op_code crear_segmento(int tamanio_segmento, int id_segmento, t_segmento* nuevo_segmento, int pid) {
	char* algoritmo_asignacion = config_get_string_value(config, "ALGORITMO_ASIGNACION");

	bool algoritmo_es(char* algo) { return strcmp(algoritmo_asignacion, algo) == 0; }

	if (algoritmo_es(FIRST)) list_sort(huecos_libres, comparar_direccion_base);

	bool out_of_memory = true;
	int acc_tamanio_huecos = 0,
		menor_diferencia = 1000000000, id_hueco_menor_diferencia = -1, // Best
		tamanio_mayor_hueco = 0, id_mayor_hueco = -1 // Worst
	;

	log_info(logger, "Empieza el proceso de creacion, ALGORITMO: %s", algoritmo_asignacion);

	t_link_element* elemento = huecos_libres->head;
	while (elemento != NULL) {
		t_segmento* hueco = elemento->data;

		if (*hueco->tamanio < tamanio_segmento) {
			if (out_of_memory) acc_tamanio_huecos += *hueco->tamanio;

			elemento = elemento->next;
			continue;
		}

		out_of_memory = false;
		acc_tamanio_huecos = 0;

		if (algoritmo_es(FIRST)) {
			bool es_first_hueco(void* param) {
				t_segmento* hueco_en_lista = (t_segmento*) param;
				return *hueco_en_lista->id == *hueco->id;
			}

			insertar_segmento(
				nuevo_segmento,
				hueco,
				id_segmento,
				tamanio_segmento,
				*hueco->direccion_base,
				es_first_hueco,
				pid
			);
			break;
		}

		if (algoritmo_es(BEST)) {
			int diferencia_actual = abs(*hueco->tamanio - tamanio_segmento);
			if (diferencia_actual < menor_diferencia) {
				menor_diferencia = diferencia_actual;
				id_hueco_menor_diferencia = *hueco->id;
			}

			if (menor_diferencia == 0) break; // Si ya encontro a uno perfecto puede dejar de buscar
		}

		if (algoritmo_es(WORST)) {
			if (*hueco->tamanio > tamanio_mayor_hueco) {
				tamanio_mayor_hueco = *hueco->tamanio;
				id_mayor_hueco = *hueco->id;
			}
		}

		elemento = elemento->next;
	}

	if (acc_tamanio_huecos >= tamanio_segmento) return COMPACTACION;
	if (out_of_memory) return OUT_OF_MEMORY;

	if (algoritmo_es(BEST)) {
		bool es_best_hueco(void* param) {
			t_segmento* hueco = (t_segmento*) param;
			return *hueco->id == id_hueco_menor_diferencia;
		}

		t_segmento* hueco_menor_diferencia = list_find(huecos_libres, (void*) es_best_hueco);
		insertar_segmento(
			nuevo_segmento,
			hueco_menor_diferencia,
			id_segmento,
			tamanio_segmento,
			*hueco_menor_diferencia->direccion_base,
			es_best_hueco,
			pid
		);
	}

	if (algoritmo_es(WORST)) {
		bool es_worst_hueco(void* param) {
			t_segmento* hueco = (t_segmento*) param;
			return *hueco->id == id_mayor_hueco;
		}

		t_segmento* mayor_hueco = list_find(huecos_libres, (void*) es_worst_hueco);
		insertar_segmento(
			nuevo_segmento,
			mayor_hueco,
			id_segmento,
			tamanio_segmento,
			*mayor_hueco->direccion_base,
			es_worst_hueco,
			pid
		);
	}

	return CREATE_SEGMENT;
}

op_code borrar_segmento(int id_a_borrar, int pid) {
	bool es_segmento_a_borrar(void* param) {
		t_segmento* segmento_en_tabla = (t_segmento*) param;
		return *segmento_en_tabla->id == id_a_borrar && *segmento_en_tabla->pid == pid;
	}

	t_segmento* segmento_a_borrar = list_find(tabla_segmentos, (void*) es_segmento_a_borrar);
	if (segmento_a_borrar == NULL) {
		log_info(logger, "PID: %i - El Segmento %i no existe", pid, id_a_borrar);
		return NO_OP;
	}

	log_info(
		logger, 
		"PID: %i - Eliminar Segmento: %i - Base: %ld - TAMAÑO: %i",
		pid, *segmento_a_borrar->id, *segmento_a_borrar->direccion_base, *segmento_a_borrar->tamanio
	);

	bool es_hueco_adyacente(void* param) {
		t_segmento* hueco_en_lista = (t_segmento*) param;

		bool hueco_es_anterior =
			*hueco_en_lista->direccion_base == *segmento_a_borrar->direccion_base - *hueco_en_lista->tamanio;
		bool hueco_es_siguiente =
			*hueco_en_lista->direccion_base == *segmento_a_borrar->direccion_base + *segmento_a_borrar->tamanio;

		return hueco_es_anterior || hueco_es_siguiente;
	}

	t_segmento* hueco_adyacente = (t_segmento*) list_find(huecos_libres, (void*) es_hueco_adyacente);
	if (hueco_adyacente != NULL) {
		// Si es siguiente hay que mover para atras la direccion base
		if (*hueco_adyacente->direccion_base == *segmento_a_borrar->direccion_base + *segmento_a_borrar->tamanio) {
			*hueco_adyacente->direccion_base = *segmento_a_borrar->direccion_base;
		}

		*hueco_adyacente->tamanio += *segmento_a_borrar->tamanio;
	} else {
		list_remove_by_condition(tabla_segmentos, (void*) es_segmento_a_borrar);

		*segmento_a_borrar->pid = -1;
		*segmento_a_borrar->id = id_hueco_actual;
		id_hueco_actual++;

		list_add(huecos_libres, segmento_a_borrar);
	}

	// Borrar contenido del segmento en el espacio usuario
	memset((void*) *segmento_a_borrar->direccion_base, 0, *segmento_a_borrar->tamanio);

	unir_huecos();

	return DELETE_SEGMENT;
}

void enviar_respuesta_crear_segmento(t_segmento* segmento) {
	size_t tamanio_int = sizeof(int);
	t_paquete* paquete = crear_paquete(MEMORIA, CREATE_SEGMENT);

	uint32_t tamanio_paquete = tamanio_int * 3 + sizeof(intptr_t);
	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	memcpy(stream + offset, segmento->id, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, segmento->pid, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, segmento->direccion_base, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(stream + offset, segmento->tamanio, tamanio_int);
	offset += tamanio_int;

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_kernel);
	eliminar_paquete(paquete);
}

void compactar_segmentos() {
	list_sort(tabla_segmentos, comparar_direccion_base);

	int i = 0;
	t_link_element* elemento = tabla_segmentos->head;
	while (elemento != NULL) {
		t_segmento* segmento = elemento->data;

		if (i == 0) {
			log_info(
				logger, 
				"Segmento: %i - Base: %ld - Tamaño %i",
				*segmento->id, *segmento->direccion_base, *segmento->tamanio
			);
			i++;
			elemento = elemento->next;
			continue;
		}

		t_segmento* anterior_segmento = list_get(tabla_segmentos, i - 1);
		intptr_t direccion_base_previa = *segmento->direccion_base;

		*segmento->direccion_base = *anterior_segmento->direccion_base + *anterior_segmento->tamanio;
		memcpy((void*) *segmento->direccion_base, (void*) direccion_base_previa, *segmento->tamanio);

		log_info(
			logger, 
			"PID: %i - Segmento: %i - Base: %ld - Tamaño %i",
			*segmento->pid, *segmento->id, *segmento->direccion_base, *segmento->tamanio
		);

		i++;
		elemento = elemento->next;
	}

	list_clean_and_destroy_elements(huecos_libres, (void*) liberar_segmento);
	int tamanio_memoria = config_get_int_value(config, "TAM_MEMORIA");
	int tamanio_ocupado = 0;

	void sumar_tamanio(void* param) {
		t_segmento* segmento = (t_segmento*) param;
		tamanio_ocupado += *segmento->tamanio;
	}

	list_iterate(tabla_segmentos, (void*) sumar_tamanio);

	t_segmento* ultimo_segmento = list_get(tabla_segmentos, tabla_segmentos->elements_count - 1);
	t_segmento* primer_segmento = list_get(tabla_segmentos, 0);

	if (*primer_segmento->direccion_base + tamanio_memoria == *ultimo_segmento->direccion_base + *ultimo_segmento->tamanio)
		return;

	t_segmento* hueco = alocar_segmento();
	*hueco->tamanio = tamanio_memoria - tamanio_ocupado;
	*hueco->direccion_base = *ultimo_segmento->direccion_base + *ultimo_segmento->tamanio;
	*hueco->id = id_hueco_actual;
	list_add(huecos_libres, hueco);

	log_info(
		logger, 
		"Hueco restante: %i - Base: %ld - Tamaño %i",
		*hueco->id, *hueco->direccion_base, *hueco->tamanio
	);

	id_hueco_actual++;
}
