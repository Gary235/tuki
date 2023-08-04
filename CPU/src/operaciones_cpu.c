#include "operaciones_cpu.h"

// ---- HELPERS ----

int escribir_memoria(intptr_t* direccion_fisica, char* valor, int tamanio_valor, int* pid) {
	t_paquete *paquete = crear_paquete(CPU, ESCRIBIR_MEMORIA);

	uint32_t tamanio_paquete =
		sizeof(intptr_t) + // df
		tamanio_valor + // valor
		sizeof(int) * 2; // tamanio_valor y pid

	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	memcpy(stream + offset, direccion_fisica, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(stream + offset, &tamanio_valor, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, valor, tamanio_valor);
	offset += tamanio_valor;

	memcpy(stream + offset, pid, sizeof(int));
	offset += sizeof(int);

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);

	// Esperar OK
	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op
	char* respuesta = recibir_mensaje(conexion_memoria, logger, 0);

	int return_value = 0;
	if (strcmp(respuesta, OK) != 0) return_value = -1;
	
	free(respuesta);
	return return_value;
}

char* leer_memoria(intptr_t* direccion_fisica, int* pid, int tamanio_a_leer) {
	t_paquete *paquete = crear_paquete(CPU, LEER_MEMORIA);

	uint32_t tamanio_paquete = sizeof(intptr_t) + sizeof(int) * 2; // df, pid y tam
	paquete->buffer->size = tamanio_paquete;

	void* stream = malloc(tamanio_paquete);
	u_int32_t offset = 0;

	memcpy(stream + offset, direccion_fisica, sizeof(intptr_t));
	offset += sizeof(intptr_t);

	memcpy(stream + offset, &tamanio_a_leer, sizeof(int));
	offset += sizeof(int);

	memcpy(stream + offset, pid, sizeof(int));
	offset += sizeof(int);

	paquete->buffer->stream = stream;
	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);

	// Esperar OK
	recibir_operacion(conexion_memoria); // recibe modulo
	recibir_operacion(conexion_memoria); // recibe cod_op
	char* respuesta = recibir_mensaje(conexion_memoria, logger, 0);

	return respuesta;
}


// ---- HANDLERS ----

void handle_set(t_op_args* args) {
	char* registro = get_val_registro(args->pcb, args->instruccion->parametro_0);

	if (registro != NULL) {
		char* parsed_param = (char*) malloc(string_length(args->instruccion->parametro_1) + 1);
		strcpy(parsed_param, args->instruccion->parametro_1);

		int tam_registro = get_tam_registro(args->instruccion->parametro_0);
		agregar_padding(parsed_param, "7", tam_registro);

		memcpy(registro, parsed_param, tam_registro);
		// strcpy(registro, parsed_param);

		free(parsed_param);
	} else {
		char* mensaje = malloc(22);
		strcpy(mensaje, "Registro no existente");
		setear_interrupcion(args->interrupcion, EXIT, mensaje, 22, args->pcb);
	}
}

void handle_yield(t_op_args* args) {
	char* mensaje = malloc(6);
	strcpy(mensaje, "YIELD");
	setear_interrupcion(args->interrupcion, YIELD, mensaje, 6, args->pcb);
}

void handle_exit(t_op_args* args) {
	char* mensaje = malloc(8);
	strcpy(mensaje, "SUCCESS");
	setear_interrupcion(args->interrupcion, EXIT, mensaje, 8, args->pcb);
}

void handle_wait(t_op_args* args) {
	int tamanio_parametro = string_length(args->instruccion->parametro_0) + 1;
	char* recurso = malloc(tamanio_parametro);
	strcpy(recurso, args->instruccion->parametro_0);

	setear_interrupcion(args->interrupcion, WAIT, recurso, tamanio_parametro, args->pcb);
}

void handle_signal(t_op_args* args) {
	int tamanio_parametro = string_length(args->instruccion->parametro_0) + 1;
	char* recurso = malloc(tamanio_parametro);
	strcpy(recurso, args->instruccion->parametro_0);

	setear_interrupcion(args->interrupcion, SIGNAL, recurso, tamanio_parametro, args->pcb);
}

void handle_io(t_op_args* args) {
	int* segundos_de_bloqueo = malloc(sizeof(int));
	*segundos_de_bloqueo = strtol(args->instruccion->parametro_0, NULL, 10);

	setear_interrupcion(args->interrupcion, IO, segundos_de_bloqueo, sizeof(int), args->pcb);
}

void handle_mov_in(t_op_args* args, t_mmu_op_args* mmu_args) {
	char* registro = args->instruccion->parametro_0;
	int tam_registro = get_tam_registro(registro);

	char* valor_en_memoria = leer_memoria(
		mmu_args->direccion_fisica,
		args->pcb->pid,
		tam_registro
	);

	char* val_registro = get_val_registro(args->pcb, registro);
	memcpy(val_registro, valor_en_memoria, tam_registro);

	if (val_registro[tam_registro] == '\0') {
		log_info(
			logger,
			"PID: %i - Acción: LEER(1) - Segmento: %i - Dirección Física: %ld - Valor: %s",
			*args->pcb->pid, *mmu_args->num_segmento, *mmu_args->direccion_fisica, val_registro
		);
	} else {
		char* valor_loggeable = malloc(tam_registro + 1);
		memcpy(valor_loggeable, val_registro, tam_registro);
		valor_loggeable[tam_registro + 1] = '\0';

		log_info(
			logger,
			"PID: %i - Acción: LEER(2) - Segmento: %i - Dirección Física: %ld - Valor: %s",
			*args->pcb->pid, *mmu_args->num_segmento, *mmu_args->direccion_fisica, valor_loggeable
		);

		free(valor_loggeable);
	}


	free(valor_en_memoria);
}

void handle_mov_out(t_op_args* args, t_mmu_op_args* mmu_args) {
	char* val_registro = get_val_registro(args->pcb, args->instruccion->parametro_1);
	int tam_registro = get_tam_registro(args->instruccion->parametro_1);

	int respuesta = escribir_memoria(
		mmu_args->direccion_fisica,
		val_registro,
		tam_registro,
		args->pcb->pid
	);

	if (respuesta == -1) {
		char* mensaje = malloc(26);
		strcpy(mensaje, "Error escribiendo memoria");
		setear_interrupcion(args->interrupcion, EXIT, mensaje, 26, args->pcb);
		return;
	}

	if (val_registro[tam_registro] == '\0') {
		log_info(
			logger,
			"PID: %i - Acción: ESCRIBIR(1) - Segmento: %i - Dirección Física: %ld - Valor: %s",
			*args->pcb->pid, *mmu_args->num_segmento, *mmu_args->direccion_fisica, val_registro
		);
		log_info(logger, "valor a leer: %s", val_registro);
	} else {
		char* valor_loggeable = malloc(tam_registro + 1);
		memcpy(valor_loggeable, val_registro, tam_registro);
		valor_loggeable[tam_registro + 1] = '\0';

		log_info(
			logger,
			"PID: %i - Acción: ESCRIBIR(2) - Segmento: %i - Dirección Física: %ld - Valor: %s",
			*args->pcb->pid, *mmu_args->num_segmento, *mmu_args->direccion_fisica, valor_loggeable
		);

		free(valor_loggeable);
	}
}

void handle_create_segment(t_op_args* args) {
	int id_segmento = strtol(args->instruccion->parametro_0, NULL, 10);
	int tamanio_segmento = strtol(args->instruccion->parametro_1, NULL, 10);

	int tamanio_parametros = sizeof(int) * 2;
	t_interrupcion_cpu_crear_segmento* respuesta = malloc(sizeof(t_interrupcion_cpu_crear_segmento));
	respuesta->id_segmento = malloc(sizeof(int));
	respuesta->tamanio_segmento = malloc(sizeof(int));

	*respuesta->id_segmento = id_segmento;
	*respuesta->tamanio_segmento = tamanio_segmento;

	op_code codigo = CREATE_SEGMENT;
	if (tamanio_segmento > config_get_int_value(config, "TAM_MAX_SEGMENTO")) {
		codigo = SEGMENTO_MAYOR_A_MAX;
	}

	setear_interrupcion(args->interrupcion, codigo, respuesta, tamanio_parametros, args->pcb);
}

void handle_delete_segment(t_op_args* args) {
	int* id_segmento = malloc(sizeof(int));
	*id_segmento = strtol(args->instruccion->parametro_0, NULL, 10);
	setear_interrupcion(args->interrupcion, DELETE_SEGMENT, id_segmento, sizeof(int), args->pcb);
}

void handle_f_open(t_op_args* args) {
	int tamanio_nombre = string_length(args->instruccion->parametro_0) + 1;
	char* nombre_archivo = malloc(tamanio_nombre);
	strcpy(nombre_archivo, args->instruccion->parametro_0);

	setear_interrupcion(args->interrupcion, F_OPEN, nombre_archivo, tamanio_nombre, args->pcb);
}

void handle_f_close(t_op_args* args) {
	int tamanio_nombre = string_length(args->instruccion->parametro_0) + 1;
	char* nombre_archivo = malloc(tamanio_nombre);
	strcpy(nombre_archivo, args->instruccion->parametro_0);

	setear_interrupcion(args->interrupcion, F_CLOSE, nombre_archivo, tamanio_nombre, args->pcb);
}

void handle_f_truncate(t_op_args* args) {
	int tamanio_nombre = string_length(args->instruccion->parametro_0) + 1;
	char* nombre_archivo = args->instruccion->parametro_0;

	int tamanio_a_truncar = strtol(args->instruccion->parametro_1, NULL, 10);

	int tamanio_parametros = sizeof(int) * 2 + tamanio_nombre;
	t_interrupcion_cpu_truncate_seek* respuesta = malloc(sizeof(t_interrupcion_cpu_truncate_seek));
	respuesta->nombre_archivo = malloc(tamanio_nombre);
	respuesta->tamanio_nombre = malloc(sizeof(int));
	respuesta->entero = malloc(sizeof(int));

	strcpy(respuesta->nombre_archivo, nombre_archivo);
	*respuesta->tamanio_nombre = tamanio_nombre;
	*respuesta->entero = tamanio_a_truncar;

	setear_interrupcion(args->interrupcion, F_TRUNCATE, respuesta, tamanio_parametros, args->pcb);
}

void handle_f_seek(t_op_args* args) {
	int tamanio_nombre = string_length(args->instruccion->parametro_0) + 1;
	char* nombre_archivo = args->instruccion->parametro_0;

	int offset = strtol(args->instruccion->parametro_1, NULL, 10);

	int tamanio_parametros = sizeof(int) * 2 + tamanio_nombre;
	t_interrupcion_cpu_truncate_seek* respuesta = malloc(sizeof(t_interrupcion_cpu_truncate_seek));
	respuesta->nombre_archivo = malloc(tamanio_nombre);
	respuesta->tamanio_nombre = malloc(sizeof(int));
	respuesta->entero = malloc(sizeof(int));

	strcpy(respuesta->nombre_archivo, nombre_archivo);
	*respuesta->tamanio_nombre = tamanio_nombre;
	*respuesta->entero = offset;

	setear_interrupcion(args->interrupcion, F_SEEK, respuesta, tamanio_parametros, args->pcb);
}

void handle_f_read(t_op_args* args, t_mmu_op_args* mmu_args) {
	int tamanio_nombre = string_length(args->instruccion->parametro_0) + 1;
	char* nombre_archivo = args->instruccion->parametro_0;

	int cantidad_bytes = strtol(args->instruccion->parametro_2, NULL, 10);

	int tamanio_parametros = sizeof(int) * 2 + tamanio_nombre + sizeof(intptr_t);
	t_interrupcion_cpu_read_write* respuesta = malloc(sizeof(t_interrupcion_cpu_read_write));
	respuesta->nombre_archivo = malloc(tamanio_nombre);
	respuesta->tamanio_nombre = malloc(sizeof(int));
	respuesta->cantidad_bytes = malloc(sizeof(int));
	respuesta->direccion_fisica = malloc(sizeof(intptr_t));

	strcpy(respuesta->nombre_archivo, nombre_archivo);
	*respuesta->tamanio_nombre = tamanio_nombre;
	*respuesta->cantidad_bytes = cantidad_bytes;
	*respuesta->direccion_fisica = *mmu_args->direccion_fisica;

	setear_interrupcion(args->interrupcion, F_READ, respuesta, tamanio_parametros, args->pcb);
}

void handle_f_write(t_op_args* args, t_mmu_op_args* mmu_args) {
	int tamanio_nombre = string_length(args->instruccion->parametro_0) + 1;
	char* nombre_archivo = args->instruccion->parametro_0;

	int cantidad_bytes = strtol(args->instruccion->parametro_2, NULL, 10);

	int tamanio_parametros = sizeof(int) * 2 + tamanio_nombre + sizeof(intptr_t);
	t_interrupcion_cpu_read_write* respuesta = malloc(sizeof(t_interrupcion_cpu_read_write));
	respuesta->nombre_archivo = malloc(tamanio_nombre);
	respuesta->tamanio_nombre = malloc(sizeof(int));
	respuesta->cantidad_bytes = malloc(sizeof(int));
	respuesta->direccion_fisica = malloc(sizeof(intptr_t));

	strcpy(respuesta->nombre_archivo, nombre_archivo);
	*respuesta->tamanio_nombre = tamanio_nombre;
	*respuesta->cantidad_bytes = cantidad_bytes;
	*respuesta->direccion_fisica = *mmu_args->direccion_fisica;

	setear_interrupcion(args->interrupcion, F_WRITE, respuesta, tamanio_parametros, args->pcb);
}
