#include "planificacion.h"

sem_t sem_multiprogramacion_actual,
	sem_cont_cola_new,
	sem_cont_lista_ready,
	sem_ready_exec,
	sem_proceso_en_cpu,
	sem_proceso_en_fs
;

pthread_mutex_t mx_pid = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_cola_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_lista_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_tabla_segmentos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_tabla_archivos_abiertos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_dic_recursos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_dic_archivos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_cpu = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_acceso_seguir_ejecutando = PTHREAD_MUTEX_INITIALIZER;

t_queue* cola_new;
t_list *lista_ready, *tabla_segmentos, *tabla_archivos_abiertos;
t_dictionary *dic_recursos, *dic_archivos;

int seguir_ejecutando = -1, ultimo_ejecutado = -1; // pid
struct timespec tiempo_de_envio;

void iniciar_planificacion() {
	dic_recursos = dictionary_create();
	dic_archivos = dictionary_create();
	cola_new = queue_create();
	lista_ready = list_create();
	tabla_segmentos = list_create();
	tabla_archivos_abiertos = list_create();

	char** recursos = string_get_string_as_array(config_get_string_value(config, "RECURSOS"));
	char** instacias_recursos = string_get_string_as_array(config_get_string_value(config, "INSTANCIAS_RECURSOS"));

	// Comprueba si la cantidad de recursos coincide con la cantidad de instancias por recurso
	if (cantidad_elementos(recursos) != cantidad_elementos(instacias_recursos)) {
		log_error(logger, "Cantidad de recursos no matchea la cantidad de instancias por recurso");
		exit(EXIT_FAILURE);
	}

	// Por cada recurso, se crea una estructura que almacena información relacionada
	for (int i = 0; i < cantidad_elementos(recursos); i++) {
		t_data_recurso* data = malloc(sizeof(t_data_recurso));
		data->recurso = malloc(string_length(recursos[i]) + 1);

		strcpy(data->recurso, recursos[i]);
		data->instancias_libres = strtol(instacias_recursos[i], NULL, 10);
		data->cola_bloqueo = queue_create();
		data->procesos_con_recurso = list_create();

		dictionary_put(dic_recursos, data->recurso, data);
	}

	for (int i = 0; i < cantidad_elementos(recursos); i++) {
		free(recursos[i]);
		free(instacias_recursos[i]);
	}

	free(recursos);
	free(instacias_recursos);

	// Inicializa semáforos
	sem_init(&sem_multiprogramacion_actual, 0, config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION"));   //controla la cantidad máxima de procesos en ejecución
	sem_init(&sem_cont_cola_new, 0, 0);
	sem_init(&sem_cont_lista_ready, 0, 0);
	sem_init(&sem_ready_exec, 0, 0);
	sem_init(&sem_proceso_en_cpu, 0, 0);
	sem_init(&sem_proceso_en_fs, 0, 1);

	pthread_t thread_new_a_ready;
	pthread_create(&thread_new_a_ready, NULL, (void*) new_a_ready, NULL);
	pthread_detach(thread_new_a_ready);

	pthread_t thread_ready_a_exec;
	pthread_create(&thread_ready_a_exec, NULL, (void*) ready_a_exec, NULL);
	pthread_detach(thread_ready_a_exec);

	pthread_t thread_respuesta_cpu;
	pthread_create(&thread_respuesta_cpu, NULL, (void*) escuchar_respuestas_cpu, NULL);
	pthread_detach(thread_respuesta_cpu);
}

// Calcular el ratio de respuesta de un proceso
double get_response_ratio(t_pcb* proceso, double ahora) {
	double waiting = ahora - *proceso->tiempo_llegada_ready;
	return (waiting + *proceso->estimacion_proxima_rafaga) / *proceso->estimacion_proxima_rafaga;
}

// Compara el ratio de respuesta de dos procesos y devuelve el proceso con un ratio de respuesta más alto
void* comparar_response_ratio(void* param_a, void* param_b) {
	t_pcb* proceso_a = (t_pcb*) param_a;
	t_pcb* proceso_b = (t_pcb*) param_b;

	struct timespec ahora;
	clock_gettime(CLOCK_REALTIME, &ahora);  // Se obtiene el tiempo actual

	// Se calcula convirtiendo el tiempo actual en segundos y sumando los nanosegundos correspondientes divididos por mil millones
	double tiempo_ahora = ahora.tv_sec + (double) ahora.tv_nsec / (double) BILLION;

	double rr_a = get_response_ratio(proceso_a, tiempo_ahora);
	double rr_b = get_response_ratio(proceso_b, tiempo_ahora);

	if (rr_a > rr_b) return proceso_a;
	return proceso_b;
}

// Actualiza la estimación de las ráfagas de los procesos en la lista lista_ready
void actualizar_estimacion_de_rafagas(double alfa, t_log* logger) {
	void actualizar_rafaga(void* param) {
		t_pcb* proceso = (t_pcb*) param;

		if (*proceso->pid != ultimo_ejecutado) return;

		*proceso->estimacion_proxima_rafaga =
			(double) alfa * (*proceso->tiempo_de_ejecucion) +
			(double) (1 - alfa) * (*proceso->estimacion_proxima_rafaga);

		log_info(logger, "PID %i - Nueva estimacion: %f", *proceso->pid, *proceso->estimacion_proxima_rafaga);
	}

	list_iterate(lista_ready, (void*) actualizar_rafaga);
}

t_pcb* elegir_proximo_proceso() {
	if (seguir_ejecutando >= 0) {
		// Sigue ejecutando el que estaba en EXEC

		bool es_proceso_que_sigue_ejecutando(void* proceso) {
			t_pcb* parsed_proceso = (t_pcb*) proceso;
			return *parsed_proceso->pid == seguir_ejecutando;
		}

		pthread_mutex_lock(&mx_acceso_lista_ready);
		t_pcb* pcb = (t_pcb*) list_find(lista_ready, (void*) es_proceso_que_sigue_ejecutando);
		list_remove_by_condition(lista_ready, (void*) es_proceso_que_sigue_ejecutando);
		pthread_mutex_unlock(&mx_acceso_lista_ready);

		pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
		seguir_ejecutando = -1;
		pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
		return pcb;
	}

	char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	if (strcmp(algoritmo_planificacion, FIFO) == 0) {
		pthread_mutex_lock(&mx_acceso_lista_ready);
		t_pcb* pcb = (t_pcb*) list_get(lista_ready, 0);
		list_remove(lista_ready, 0);
		pthread_mutex_unlock(&mx_acceso_lista_ready);

		return pcb;
	}

	// Planificacion por HRRN
	pthread_mutex_lock(&mx_acceso_lista_ready);
	actualizar_estimacion_de_rafagas(config_get_double_value(config, "HRRN_ALFA"), logger);

	t_pcb* highest_rr_pcb = list_get_maximum(lista_ready, comparar_response_ratio);
	log_info(logger, "PID Elejido: %i", *highest_rr_pcb->pid);

	bool es_proceso_hrrn(void* proceso) {
		t_pcb* parsed_proceso = (t_pcb*) proceso;
		return *parsed_proceso->pid == *highest_rr_pcb->pid;
	}

	list_remove_by_condition(lista_ready, es_proceso_hrrn);
	pthread_mutex_unlock(&mx_acceso_lista_ready);

	return highest_rr_pcb;
}

char* get_lista_pids() {
	char* lista_pids = string_new();
	string_append(&lista_pids, "[");

	void concatenar_pid_a_lista(void* arg) {
		t_pcb* pcb = (t_pcb*) arg;
		char* parsed_pid = string_itoa(*pcb->pid);

		string_append(&lista_pids, parsed_pid);
		string_append(&lista_pids, ", ");

		free(parsed_pid);
	}

	list_iterate(lista_ready, (void*) concatenar_pid_a_lista);
	string_append(&lista_pids, "]");
	
	return lista_pids;
}

void agregar_a_ready(t_pcb* pcb, int desde_new) {
    pthread_mutex_lock(&mx_acceso_lista_ready);
    list_add(lista_ready, pcb);

	char* lista_pids = get_lista_pids();
    log_info(logger, "Cola Ready %s: %s", config_get_string_value(config, "ALGORITMO_PLANIFICACION"), lista_pids);
	free(lista_pids);

    struct timespec llegada_ready;
    clock_gettime(CLOCK_REALTIME, &llegada_ready);

    *pcb->tiempo_llegada_ready = llegada_ready.tv_sec + (double) llegada_ready.tv_nsec / (double) BILLION;
    pthread_mutex_unlock(&mx_acceso_lista_ready);

	sem_post(&sem_cont_lista_ready);
	if (desde_new) sem_post(&sem_ready_exec);
}

void new_a_ready(){
	while(1) {
		sem_wait(&sem_cont_cola_new); // chequear que haya procesos en new
		sem_wait(&sem_multiprogramacion_actual); // chequear que haya menos procesos en ready que la maxima

	    pthread_mutex_lock(&mx_acceso_cola_new);
	    t_pcb* pcb = queue_pop(cola_new);
	    pthread_mutex_unlock(&mx_acceso_cola_new);

	    solicitar_tabla_de_segmentos_inicial(pcb);

		log_info(logger, "PID: %i - Estado Anterior: NEW - Estado Actual: READY", *pcb->pid);
	    agregar_a_ready(pcb, 1);
	}
}

void ready_a_exec(){
	while(1) {
	  sem_wait(&sem_ready_exec);
	  sem_wait(&sem_cont_lista_ready); // chequear que haya procesos en ready

	  pthread_mutex_lock(&mx_acceso_cpu); // chequear cpu desocupado
	  t_pcb* pcb = elegir_proximo_proceso(config, logger);

	  log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", *pcb->pid);

	  ultimo_ejecutado = *pcb->pid;
	  enviar_proceso(conexion_cpu, pcb, KERNEL);

	  clock_gettime(CLOCK_REALTIME, &tiempo_de_envio);

	  sem_post(&sem_proceso_en_cpu);
	}
}

void proceso_bloqueado_por_IO(t_io_blocked_args* args) {
	log_info(logger, "PID: %i - Ejecuta IO: %i", *args->pcb->pid, *args->segundos);
	sleep(*args->segundos);
	agregar_a_ready(args->pcb, 0);
}

void proceso_bloqueado_por_FS(t_fs_blocked_args* args) {
	sem_wait(&sem_proceso_en_fs);

	recibir_operacion(conexion_fs); // recibe modulo
	int cod_op = recibir_operacion(conexion_fs);
	loggear_codigo(logger, cod_op);
	char* respuesta = recibir_mensaje(conexion_fs, logger, 0);

	sem_post(&sem_proceso_en_fs);

	char* operacion = get_nombre_por_codigo(cod_op);
	if (strcmp(respuesta, OK) != 0) {
		log_error(logger, "PID: %i - %s Termino con error", *args->pcb->pid, operacion);
	} else {
		log_info(logger, "PID: %i - %s Termino exitosamente", *args->pcb->pid, operacion);
		log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", *args->pcb->pid);
	}

	agregar_a_ready(args->pcb, 0);

	free(respuesta);
	free(args);
}

void escuchar_respuestas_cpu() {
	while(1) {
	    sem_wait(&sem_proceso_en_cpu);

		recibir_operacion(conexion_cpu); // recibe modulo
		recibir_operacion(conexion_cpu); // recibe cod_op

		bool liberar_pcb = false;
		t_interrupcion_cpu* interrupcion = deserializar_interrupcion(conexion_cpu);

		struct timespec ahora;
		clock_gettime(CLOCK_REALTIME, &ahora);

		*interrupcion->pcb->tiempo_de_ejecucion =
			(ahora.tv_sec - tiempo_de_envio.tv_sec) +
			(double) (ahora.tv_nsec - tiempo_de_envio.tv_nsec) / (double) BILLION;

		loggear_codigo(logger, *interrupcion->codigo);
		log_info(logger, "PID: %i - Tiempo de ejecucion: %lf", *interrupcion->pcb->pid, *interrupcion->pcb->tiempo_de_ejecucion);

		switch (*interrupcion->codigo) {
			case WAIT:
				handle_wait(interrupcion, &liberar_pcb);
                break;
			case SIGNAL:
				handle_signal(interrupcion, &liberar_pcb);
                break;
			case IO:
				handle_io(interrupcion);
                break;
			case YIELD:
				handle_yield(interrupcion);
                break;
			case SEGMENTO_MAYOR_A_MAX:
				handle_segmento_mayor_a_max(interrupcion);
				break;
			case CREATE_SEGMENT:
				handle_create_segment(interrupcion, &liberar_pcb);
				break;
			case DELETE_SEGMENT:
				handle_delete_segment(interrupcion);
				break;
			case F_OPEN:
				handle_f_open(interrupcion);
				break;
			case F_CLOSE:
				handle_f_close(interrupcion, &liberar_pcb);
				break;
			case F_TRUNCATE:
				handle_f_truncate(interrupcion);
				break;
			case F_SEEK:
				handle_f_seek(interrupcion);
				break;
			case F_READ:
				handle_f_read(interrupcion);
				break;
			case F_WRITE:
				handle_f_write(interrupcion);
				break;
			case EXIT:
				handle_exit(interrupcion, &liberar_pcb);
				break;
			default:
                break;
		}

		liberar_interrupcion(interrupcion, liberar_pcb);
		pthread_mutex_unlock(&mx_acceso_cpu);
		sem_post(&sem_ready_exec);
	}
}
