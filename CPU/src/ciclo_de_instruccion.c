#include "ciclo_de_instruccion.h"

char* OPERACIONES_CON_MMU[] = {OP_MOV_IN, OP_MOV_OUT, OP_F_READ, OP_F_WRITE, NULL};

t_interrupcion_cpu* ejecutar_instrucciones(t_pcb* pcb) {
	t_interrupcion_cpu* interrupcion = alocar_interrupcion();
	while(*interrupcion->codigo == NO_OP) {
		ejecutar_ciclo_de_instruccion(pcb, interrupcion);
	}

	return interrupcion;
}

void ejecutar_ciclo_de_instruccion(t_pcb* pcb, t_interrupcion_cpu* interrupcion) {
	t_instruccion* instruccion = fetch(pcb->instrucciones, pcb->pc);

	int num_segmento;
	intptr_t* direccion_fisica = decode(instruccion, pcb, interrupcion, &num_segmento);
	if (*interrupcion->codigo == EXIT) return;

	log_info(logger, "------------");
	log_info(
		logger,
		"PID: %i - Ejecutando: %s - %s %s %s",
		*pcb->pid, instruccion->operacion, instruccion->parametro_0, instruccion->parametro_1, instruccion->parametro_2
	);

	execute(instruccion, pcb, interrupcion, direccion_fisica, &num_segmento);
	*pcb->pc += 1;
}

t_instruccion* fetch(t_list* instrucciones, int* pc) {
	return (t_instruccion*) list_get(instrucciones, *pc);
}

intptr_t* decode(
		t_instruccion* instruccion,
		t_pcb* pcb,
		t_interrupcion_cpu* interrupcion,
		int* num_segmento
) {
	if (strcmp(instruccion->operacion, OP_SET) == 0) {
		sleep(config_get_int_value(config, "RETARDO_INSTRUCCION") / 1000);
		return NULL;
	}

	if (string_en_string_array(OPERACIONES_CON_MMU, instruccion->operacion)) {
		char* par_direccion_logica;

		if (strcmp(instruccion->operacion, OP_MOV_OUT) == 0) par_direccion_logica = instruccion->parametro_0;
		else par_direccion_logica = instruccion->parametro_1;

		int direccion_logica = strtol(par_direccion_logica, NULL, 10);

		int tam_max_segmento = config_get_int_value(config, "TAM_MAX_SEGMENTO");
		int desplazamiento = direccion_logica % tam_max_segmento;
		*num_segmento = direccion_logica / tam_max_segmento;

		t_segmento* segmento = list_get(pcb->tabla_segmentos, *num_segmento);

		bool escribe_o_lee = string_en_string_array((char*[]) {OP_F_READ, OP_F_WRITE, NULL}, instruccion->operacion);
		
		int cantidad_de_bytes;
		if (escribe_o_lee)
			cantidad_de_bytes = strtol(instruccion->parametro_2, NULL, 10); // F_READ - F_WRITE

		else if (strcmp(instruccion->operacion, OP_MOV_OUT) == 0) 
			cantidad_de_bytes = get_tam_registro(instruccion->parametro_1); // MOV_OUT

		else 
			cantidad_de_bytes = get_tam_registro(instruccion->parametro_0); // MOV_IN


		bool hay_segmentation_fault = desplazamiento + cantidad_de_bytes > *segmento->tamanio;
		if (hay_segmentation_fault) {
			log_info(
				logger,
				"PID: %i - Error SEG_FAULT - Segmento: %i - Offset: %i - Tamaño: %i",
				*pcb->pid, *num_segmento, desplazamiento, *segmento->tamanio
			);

			char* mensaje = malloc(10);
			strcpy(mensaje, "SEG_FAULT");

			setear_interrupcion(interrupcion, EXIT, mensaje, 10, pcb);
			return NULL;
		}

		intptr_t* direccion_fisica = malloc(sizeof(intptr_t));
		*direccion_fisica = *segmento->direccion_base + (intptr_t) desplazamiento;

		log_info(
			logger,
			"PID: %i - MMU - Direccion Logica: %s - Direccion Fisica: %ld",
			*pcb->pid, par_direccion_logica, *direccion_fisica
		);
		return direccion_fisica;
	}

	return NULL;
}

void execute(
		t_instruccion* instruccion,
		t_pcb* pcb,
		t_interrupcion_cpu* interrupcion,
		intptr_t* direccion_fisica,
		int* num_segmento
) {
	bool operacion_es(char* op) { return strcmp(instruccion->operacion, op) == 0; }

	t_op_args* args = malloc(sizeof(t_op_args));
	args->pcb = pcb;
	args->instruccion = instruccion;
	args->interrupcion = interrupcion;

	t_mmu_op_args* mmu_args = malloc(sizeof(t_mmu_op_args));
	mmu_args->direccion_fisica = direccion_fisica;
	mmu_args->num_segmento = num_segmento;

	if (operacion_es(OP_SET)) handle_set(args);
	if (operacion_es(OP_YIELD)) handle_yield(args);
	if (operacion_es(OP_EXIT)) handle_exit(args);
	if (operacion_es(OP_WAIT)) handle_wait(args);
	if (operacion_es(OP_SIGNAL)) handle_signal(args);
	if (operacion_es(OP_IO)) handle_io(args);
	if (operacion_es(OP_MOV_IN)) handle_mov_in(args, mmu_args);
	if (operacion_es(OP_MOV_OUT)) handle_mov_out(args, mmu_args);
	if (operacion_es(OP_CREATE_SEGMENT)) handle_create_segment(args);
	if (operacion_es(OP_DELETE_SEGMENT)) handle_delete_segment(args);
	if (operacion_es(OP_F_OPEN)) handle_f_open(args);
	if (operacion_es(OP_F_CLOSE)) handle_f_close(args);
	if (operacion_es(OP_F_TRUNCATE)) handle_f_truncate(args);
	if (operacion_es(OP_F_SEEK)) handle_f_seek(args);
	if (operacion_es(OP_F_READ)) handle_f_read(args, mmu_args);
	if (operacion_es(OP_F_WRITE)) handle_f_write(args, mmu_args);

	if (mmu_args->direccion_fisica != NULL) free(mmu_args->direccion_fisica);
	free(mmu_args);
	free(args);
}
