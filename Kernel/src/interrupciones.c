#include "interrupciones.h"

// HANDLERS

void handle_wait(t_interrupcion_cpu* interrupcion, bool* liberar_pcb) {
    if (!dictionary_has_key(dic_recursos, (char*) interrupcion->parametros)) {
        log_error(logger, "No existe el recurso: %s", (char*) interrupcion->parametros);
        exec_a_exit((char*) interrupcion->parametros, interrupcion->pcb, liberar_pcb);
        return;
    }

    pthread_mutex_lock(&mx_acceso_dic_recursos);
    t_data_recurso* recurso = (t_data_recurso*) dictionary_get(dic_recursos, (char*) interrupcion->parametros);
    recurso->instancias_libres -= 1;
    log_info(
        logger,
        "PID: %i - Wait: %s - Instancias: %i",
        *interrupcion->pcb->pid, (char*) interrupcion->parametros, recurso->instancias_libres
    );

    if (recurso->instancias_libres < 0) {
        log_info(logger, "PID: %i - Bloqueado por: %s", *interrupcion->pcb->pid, (char*) interrupcion->parametros);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", *interrupcion->pcb->pid);
        queue_push(recurso->cola_bloqueo, interrupcion->pcb);
    } else {
        int* pid_proceso = malloc(sizeof(int));
	    *pid_proceso = *interrupcion->pcb->pid;
        list_add(recurso->procesos_con_recurso, pid_proceso);

        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
        agregar_a_ready(interrupcion->pcb, 0);
    }
    pthread_mutex_unlock(&mx_acceso_dic_recursos);
}

void handle_signal(t_interrupcion_cpu* interrupcion, bool* liberar_pcb) {
    if (!dictionary_has_key(dic_recursos, (char*) interrupcion->parametros)) {
        log_error(logger, "No existe el recurso: %s", (char*) interrupcion->parametros);
        exec_a_exit((char*) interrupcion->parametros, interrupcion->pcb, liberar_pcb);
        return;
    }

    pthread_mutex_lock(&mx_acceso_dic_recursos);
    t_data_recurso* recurso = (t_data_recurso*) dictionary_get(dic_recursos, (char*) interrupcion->parametros);
    recurso->instancias_libres += 1;

    log_info(logger, "PID: %i - Signal: %s - Instancias: %i", *interrupcion->pcb->pid, (char*) interrupcion->parametros, recurso->instancias_libres);

    bool es_proceso_interrumpido(void* pid) {
        int* parsed_pid = (int*) pid;
        return *parsed_pid == *interrupcion->pcb->pid;
    }

    list_remove_and_destroy_by_condition(recurso->procesos_con_recurso, es_proceso_interrumpido, free);

    if (!queue_is_empty(recurso->cola_bloqueo)) {
        t_pcb* proximo_proceso_a_utilizar_recurso = (t_pcb*) queue_pop(recurso->cola_bloqueo);

        int* pid_proximo = malloc(sizeof(int));
        *pid_proximo = *proximo_proceso_a_utilizar_recurso->pid;
        list_add(recurso->procesos_con_recurso, pid_proximo);
        recurso->instancias_libres -= 1;

        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", *proximo_proceso_a_utilizar_recurso->pid);
        agregar_a_ready(proximo_proceso_a_utilizar_recurso, 0);
    }

    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
    agregar_a_ready(interrupcion->pcb, 0);
    pthread_mutex_unlock(&mx_acceso_dic_recursos);

    pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
    seguir_ejecutando = *interrupcion->pcb->pid;
    pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
}

void handle_io(t_interrupcion_cpu* interrupcion) {
    log_info(logger, "PID: %i - Bloqueado por: IO", *interrupcion->pcb->pid);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", *interrupcion->pcb->pid);

    t_io_blocked_args* io_blocked_args = malloc(sizeof(t_io_blocked_args));
    io_blocked_args->pcb = interrupcion->pcb;
    io_blocked_args->segundos = (int*) interrupcion->parametros;

    pthread_t thread_io_blocked;
    pthread_create(&thread_io_blocked, NULL, (void*) proceso_bloqueado_por_IO, io_blocked_args);
    pthread_detach(thread_io_blocked);
}

void handle_yield(t_interrupcion_cpu* interrupcion) {
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
    agregar_a_ready(interrupcion->pcb, 0);
}

void handle_segmento_mayor_a_max(t_interrupcion_cpu* interrupcion) {
    log_error(
        logger, 
        "PID: %i - Solicitud de Creacion de Segmento Rechazada - Motivo: Tamaño mayor al máximo configurado",
        *interrupcion->pcb->pid
    );
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
}

void handle_create_segment(t_interrupcion_cpu* interrupcion, bool* liberar_pcb) {
    int id_seg = *((t_interrupcion_cpu_crear_segmento*) interrupcion->parametros)->id_segmento;
    int tam_seg = *((t_interrupcion_cpu_crear_segmento*) interrupcion->parametros)->tamanio_segmento;

    op_code status_crear_segmento = solicitar_crear_segmento_en_memoria(id_seg, tam_seg, interrupcion->pcb);

    if (status_crear_segmento == OUT_OF_MEMORY) {
        exec_a_exit("OUT OF MEMORY", interrupcion->pcb, liberar_pcb);
        return;
    }
    if (status_crear_segmento == COMPACTACION) {
        solicitar_compactacion_segmentos(*interrupcion->pcb->pid);
        *interrupcion->pcb->pc = *interrupcion->pcb->pc - 1;
    }

    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
    agregar_a_ready(interrupcion->pcb, 0);

    pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
    seguir_ejecutando = *interrupcion->pcb->pid;
    pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
}

void handle_delete_segment(t_interrupcion_cpu* interrupcion) {
    int id_segmento = *((int*) interrupcion->parametros);

    solicitar_borrar_segmento_en_memoria(id_segmento, interrupcion->pcb);

    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
    agregar_a_ready(interrupcion->pcb, 0);

    pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
    seguir_ejecutando = *interrupcion->pcb->pid;
    pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
}

void handle_f_open(t_interrupcion_cpu* interrupcion) {
    log_info(logger, "PID: %i - Abrir Archivo: %s", *interrupcion->pcb->pid, (char*) interrupcion->parametros);

    bool es_archivo_a_abrir(void* param) {
        t_archivo_abierto* archivo = (t_archivo_abierto*) param;
        return strcmp(archivo->nombre, (char*) interrupcion->parametros) == 0;
    }

    t_archivo_abierto* archivo_en_tabla = list_find(tabla_archivos_abiertos, (void*) es_archivo_a_abrir);

    if (archivo_en_tabla != NULL) {
        log_info(logger, "PID: %i - Ya estaba en la tabla", *interrupcion->pcb->pid);

        pthread_mutex_lock(&mx_acceso_dic_archivos);
        t_data_recurso* archivo_a_abrir = (t_data_recurso*) dictionary_get(dic_archivos, (char*) interrupcion->parametros);

        log_info(logger, "PID: %i - Bloqueado por: %s", *interrupcion->pcb->pid, (char*) interrupcion->parametros);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", *interrupcion->pcb->pid);
        queue_push(archivo_a_abrir->cola_bloqueo, interrupcion->pcb);
        pthread_mutex_unlock(&mx_acceso_dic_archivos);
    } else {
        log_info(logger, "PID: %i - No estaba en la tabla", *interrupcion->pcb->pid);

        solicitar_existencia_de_archivo((char*) interrupcion->parametros, interrupcion->pcb);

        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
        agregar_a_ready(interrupcion->pcb, 0);

        pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
        seguir_ejecutando = *interrupcion->pcb->pid;
        pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
    }
}

void handle_f_close(t_interrupcion_cpu* interrupcion, bool* liberar_pcb) {
    if (!dictionary_has_key(dic_archivos, (char*) interrupcion->parametros)) {
        log_error(logger, "No existe el archivo: %s", (char*) interrupcion->parametros);
        exec_a_exit((char*) interrupcion->parametros, interrupcion->pcb, liberar_pcb);
        return;
    }

    pthread_mutex_lock(&mx_acceso_dic_archivos);
    cerrar_archivo(interrupcion->pcb, (char*) interrupcion->parametros);

    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
    agregar_a_ready(interrupcion->pcb, 0);
    pthread_mutex_unlock(&mx_acceso_dic_archivos);

    pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
    seguir_ejecutando = *interrupcion->pcb->pid;
    pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
}

void handle_f_truncate(t_interrupcion_cpu* interrupcion) {
    int tamanio = *((t_interrupcion_cpu_truncate_seek*) interrupcion->parametros)->entero;
    char* nombre_archivo = ((t_interrupcion_cpu_truncate_seek*) interrupcion->parametros)->nombre_archivo;

    log_info(logger, "PID: %i - Archivo: %s - Tamaño: %i", *interrupcion->pcb->pid, nombre_archivo, tamanio);

    log_info(logger, "PID: %i - Bloqueado por: FS", *interrupcion->pcb->pid);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", *interrupcion->pcb->pid);

    t_fs_blocked_args* args = malloc(sizeof(t_fs_blocked_args));
    args->pcb = interrupcion->pcb;
    args->operacion = F_TRUNCATE;

    pthread_t thread_fs_blocked;
    pthread_create(&thread_fs_blocked, NULL, (void*) proceso_bloqueado_por_FS, args);
    pthread_detach(thread_fs_blocked);

    solicitar_truncamiento_de_archivo(interrupcion);
}

void handle_f_seek(t_interrupcion_cpu* interrupcion) {
    int seek_offset = *((t_interrupcion_cpu_truncate_seek*) interrupcion->parametros)->entero;
    char* nombre_archivo = ((t_interrupcion_cpu_truncate_seek*) interrupcion->parametros)->nombre_archivo;

    bool es_archivo_a_seek(void* param) {
        t_archivo_abierto* archivo = (t_archivo_abierto*) param;
        return strcmp(archivo->nombre, nombre_archivo) == 0;
    }

    t_archivo_abierto* archivo_a_seek = (t_archivo_abierto*) list_find(tabla_archivos_abiertos, (void*) es_archivo_a_seek);

    if (archivo_a_seek == NULL) log_error(logger, "No existe archivo %s, en la tabla de archivos", nombre_archivo);
    else {
        t_archivo_abierto* archivo_a_seek_en_pcb = (t_archivo_abierto*) list_find(interrupcion->pcb->archivos_abiertos, (void*) es_archivo_a_seek);

        *archivo_a_seek->offset = seek_offset;
        *archivo_a_seek_en_pcb->offset = seek_offset;

        log_info(logger, "PID: %i - Actualizar puntero Archivo: %s - Puntero %i", *interrupcion->pcb->pid, nombre_archivo, seek_offset);
    }

    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
    agregar_a_ready(interrupcion->pcb, 0);

    pthread_mutex_lock(&mx_acceso_seguir_ejecutando);
    seguir_ejecutando = *interrupcion->pcb->pid;
    pthread_mutex_unlock(&mx_acceso_seguir_ejecutando);
}

void handle_f_read(t_interrupcion_cpu* interrupcion) {
    char* nombre_archivo = ((t_interrupcion_cpu_read_write*) interrupcion->parametros)->nombre_archivo;
    intptr_t* direccion_fisica = ((t_interrupcion_cpu_read_write*) interrupcion->parametros)->direccion_fisica;
    int* cant_bytes = ((t_interrupcion_cpu_read_write*) interrupcion->parametros)->cantidad_bytes;

    bool es_archivo_a_leer(void* param) {
        t_archivo_abierto* archivo = (t_archivo_abierto*) param;
        return strcmp(archivo->nombre, nombre_archivo) == 0;
    }

    t_archivo_abierto* archivo_a_leer = list_find(tabla_archivos_abiertos, (void*) es_archivo_a_leer);

    if (archivo_a_leer == NULL) {
        log_error(logger, "No existe archivo %s, en la tabla de archivos", nombre_archivo);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
        agregar_a_ready(interrupcion->pcb, 0);
        return;
    }

    log_info(
        logger, 
        "PID: %i - Leer Archivo: %s - Puntero: %i - Dirección Memoria: %ld - Tamaño: %i",
        *interrupcion->pcb->pid, nombre_archivo, *archivo_a_leer->offset, *direccion_fisica, *cant_bytes
    );

    log_info(logger, "PID: %i - Bloqueado por: FS", *interrupcion->pcb->pid);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", *interrupcion->pcb->pid);

    t_fs_blocked_args* args = malloc(sizeof(t_fs_blocked_args));
    args->pcb = interrupcion->pcb;
    args->operacion = F_READ;

    pthread_t thread_fs_blocked;
    pthread_create(&thread_fs_blocked, NULL, (void*) proceso_bloqueado_por_FS, args);
    pthread_detach(thread_fs_blocked);

    solicitar_escritura_lectura_de_archivo(interrupcion, archivo_a_leer->offset);
}

void handle_f_write(t_interrupcion_cpu* interrupcion) {
    char* nombre_archivo = ((t_interrupcion_cpu_read_write*) interrupcion->parametros)->nombre_archivo;
    intptr_t* direccion_fisica = ((t_interrupcion_cpu_read_write*) interrupcion->parametros)->direccion_fisica;
    int* cant_bytes = ((t_interrupcion_cpu_read_write*) interrupcion->parametros)->cantidad_bytes;

    bool es_archivo_a_escribir(void* param) {
        t_archivo_abierto* archivo = (t_archivo_abierto*) param;
        return strcmp(archivo->nombre, nombre_archivo) == 0;
    }

    t_archivo_abierto* archivo_a_escribir = list_find(tabla_archivos_abiertos, (void*) es_archivo_a_escribir);

    if (archivo_a_escribir == NULL) {
        log_error(logger, "No existe archivo %s, en la tabla de archivos", nombre_archivo);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: READY", *interrupcion->pcb->pid);
        agregar_a_ready(interrupcion->pcb, 0);
        return;
    }

    log_info(
        logger, 
        "PID: %i - Escribir Archivo: %s - Puntero: %i - Dirección Memoria: %ld - Tamaño: %i",
        *interrupcion->pcb->pid, nombre_archivo, *archivo_a_escribir->offset, *direccion_fisica, *cant_bytes
    );

    log_info(logger, "PID: %i - Bloqueado por: FS", *interrupcion->pcb->pid);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", *interrupcion->pcb->pid);

    t_fs_blocked_args* args = malloc(sizeof(t_fs_blocked_args));
    args->pcb = interrupcion->pcb;
    args->operacion = F_WRITE;

    pthread_t thread_fs_blocked;
    pthread_create(&thread_fs_blocked, NULL, (void*) proceso_bloqueado_por_FS, args);
    pthread_detach(thread_fs_blocked);

    solicitar_escritura_lectura_de_archivo(interrupcion, archivo_a_escribir->offset);
}

void handle_exit(t_interrupcion_cpu* interrupcion, bool* liberar_pcb) {
	exec_a_exit((char*) interrupcion->parametros, interrupcion->pcb, liberar_pcb);
}

// HELPERS

void cerrar_archivo(t_pcb* pcb, char* nombre_archivo) {
    log_info(logger, "PID: %i - Cerrar Archivo: %s", *pcb->pid, nombre_archivo);

    t_data_recurso* archivo_a_cerrar = (t_data_recurso*) dictionary_get(dic_archivos, nombre_archivo);

    bool es_archivo_a_cerrar(void* param) {
        t_archivo_abierto* archivo = (t_archivo_abierto*) param;
        return strcmp(archivo->nombre, archivo_a_cerrar->recurso) == 0;
    }

    bool es_proceso(void* pid) {
        return *((int*) pid) == *pcb->pid;
    }

    void* pid_usando_archivo = list_find(archivo_a_cerrar->procesos_con_recurso, (void*) es_proceso);
    log_info(logger, "PID: %i - pid_usando_archivo: %p", *pcb->pid, pid_usando_archivo);
    if (pid_usando_archivo == NULL) return;

    list_remove_and_destroy_by_condition(pcb->archivos_abiertos, (void*) es_archivo_a_cerrar, (void*) liberar_archivo);
    list_remove_and_destroy_by_condition(archivo_a_cerrar->procesos_con_recurso, (void*) es_proceso, free);
    archivo_a_cerrar->instancias_libres += 1;

    log_info(logger, "PID: %i - queue_is_empty: %i", *pcb->pid, queue_is_empty(archivo_a_cerrar->cola_bloqueo));
    if (!queue_is_empty(archivo_a_cerrar->cola_bloqueo)) {
        t_pcb* proximo_proceso_a_utilizar_archivo = (t_pcb*) queue_pop(archivo_a_cerrar->cola_bloqueo);

        t_archivo_abierto* archivo_en_tabla = (t_archivo_abierto*) list_find(tabla_archivos_abiertos, (void*) es_archivo_a_cerrar);
        t_archivo_abierto* copia_archivo_en_tabla = alocar_archivo(string_length(archivo_en_tabla->nombre), string_length(archivo_en_tabla->path));
        strcpy(copia_archivo_en_tabla->nombre, archivo_en_tabla->nombre);
        strcpy(copia_archivo_en_tabla->path, archivo_en_tabla->path);
        *copia_archivo_en_tabla->offset = *archivo_en_tabla->offset;
        list_add(proximo_proceso_a_utilizar_archivo->archivos_abiertos, copia_archivo_en_tabla);

        int* pid_proximo = malloc(sizeof(int));
        *pid_proximo = *proximo_proceso_a_utilizar_archivo->pid;
        list_add(archivo_a_cerrar->procesos_con_recurso, pid_proximo);
        archivo_a_cerrar->instancias_libres -= 1;

        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", *proximo_proceso_a_utilizar_archivo->pid);
        agregar_a_ready(proximo_proceso_a_utilizar_archivo, 0);
    } else {
        list_remove_and_destroy_by_condition(tabla_archivos_abiertos, es_archivo_a_cerrar, (void*) liberar_archivo);
        dictionary_remove_and_destroy(dic_archivos, archivo_a_cerrar->recurso, (void*) liberar_recurso);
    }
}

void exec_a_exit(char* motivo, t_pcb* pcb, bool* liberar_pcb) {
	log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: EXIT", *pcb->pid);
	log_info(logger, "Finaliza el proceso %i - Motivo: %s", *pcb->pid, motivo);

	void remover_proceso_de_recurso(void* param) {
		t_data_recurso* recurso = (t_data_recurso*) param;

		bool es_proceso(void* pid) {
			int* parsed_pid = (int*) pid;
			return *parsed_pid == *pcb->pid;
		}

        void* pid_usando_recurso = list_find(recurso->procesos_con_recurso, (void*) es_proceso);
        if (pid_usando_recurso == NULL) return;

		list_remove_and_destroy_by_condition(recurso->procesos_con_recurso, (void*) es_proceso, free);
        recurso->instancias_libres += 1;

        if (!queue_is_empty(recurso->cola_bloqueo)) {
            t_pcb* proximo_proceso_a_utilizar_recurso = (t_pcb*) queue_pop(recurso->cola_bloqueo);

            int* pid_proximo = malloc(sizeof(int));
            *pid_proximo = *proximo_proceso_a_utilizar_recurso->pid;
            list_add(recurso->procesos_con_recurso, pid_proximo);
            recurso->instancias_libres -= 1;

            log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", *proximo_proceso_a_utilizar_recurso->pid);
            agregar_a_ready(proximo_proceso_a_utilizar_recurso, 0);
        }
	}

	pthread_mutex_lock(&mx_acceso_dic_recursos);
	t_list* valores_dic = dictionary_elements(dic_recursos);
	list_iterate(valores_dic, (void*) remover_proceso_de_recurso);
    list_destroy(valores_dic);
	pthread_mutex_unlock(&mx_acceso_dic_recursos);

	t_link_element* elemento = pcb->tabla_segmentos->head;
	while (elemento != NULL) {
		t_segmento* segmento = elemento->data;
		if (*segmento->id == 0) {
			elemento = elemento->next;
			continue;
		}

		elemento = elemento->next;
		solicitar_borrar_segmento_en_memoria(*segmento->id, pcb);
	}

	bool es_segmento_de_proceso(void* arg) {
		t_segmento* segmento = (t_segmento*) arg;
		return *segmento->pid == *pcb->pid;
	}
	list_remove_and_destroy_all_by_condition(tabla_segmentos, es_segmento_de_proceso, (void*) liberar_segmento);

	elemento = pcb->archivos_abiertos->head;
	while (elemento != NULL) {
		t_archivo_abierto* archivo_abierto = elemento->data;

		elemento = elemento->next;
		cerrar_archivo(pcb, archivo_abierto->nombre);
	}

	enviar_mensaje(motivo, *pcb->socket_cliente, KERNEL, EXIT);
	sem_post(&sem_multiprogramacion_actual);
	*liberar_pcb = true;
}
