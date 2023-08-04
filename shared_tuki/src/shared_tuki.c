#include "shared_tuki.h"


// ---- UTILS ----

char* get_nombre_por_codigo(op_code codigo) {
	if (codigo == CONSOLA) return "CONSOLA";
	if (codigo == KERNEL) return "KERNEL";
	if (codigo == CPU) return "CPU";
	if (codigo == FILE_SYSTEM) return "FILE_SYSTEM";
	if (codigo == INSTRUCCIONES) return "INSTRUCCIONES";
	if (codigo == EXIT) return "EXIT";
	if (codigo == IO) return "IO";
	if (codigo == WAIT) return "WAIT";
	if (codigo == YIELD) return "YIELD";
	if (codigo == SIGNAL) return "SIGNAL";
	if (codigo == F_OPEN) return "F_OPEN";
	if (codigo == F_CLOSE) return "F_CLOSE";
	if (codigo == F_SEEK) return "F_SEEK";
	if (codigo == F_READ) return "F_READ";
	if (codigo == F_WRITE) return "F_WRITE";
	if (codigo == F_TRUNCATE) return "F_TRUNCATE";
	if (codigo == CREATE_SEGMENT) return "CREATE_SEGMENT";
	if (codigo == DELETE_SEGMENT) return "DELETE_SEGMENT";
	if (codigo == NO_OP) return "NO_OP";
	if (codigo == SEGMENTO_MAYOR_A_MAX) return "SEGMENTO_MAYOR_A_MAX";
	if (codigo == OUT_OF_MEMORY) return "OUT_OF_MEMORY";
	if (codigo == COMPACTACION) return "COMPACTACION";
	if (codigo == SEGMENTOS_INICIAL) return "SEGMENTOS_INICIAL";
	if (codigo == ESCRIBIR_MEMORIA) return "ESCRIBIR_MEMORIA";
	if (codigo == LEER_MEMORIA) return "LEER_MEMORIA";
	if (codigo == EXISTE_ARCHIVO) return "EXISTE_ARCHIVO";

	return "";
}

void loggear_codigo(t_log* logger, op_code codigo) {
	log_info(logger, "\n");
	log_info(logger, "Codigo: %s", get_nombre_por_codigo(codigo));
}

char* get_config_path(op_code modulo, char* archivo) {
    char* carpeta = string_new();

	switch(modulo) {
		case FILE_SYSTEM:
			string_append(&carpeta, "file_system/");
			break;
		case KERNEL:
			string_append(&carpeta, "kernel/");
			break;
		case CPU:
			string_append(&carpeta, "cpu/");
			break;
		case CONSOLA:
			string_append(&carpeta, "consola/");
			break;
		case MEMORIA:
			string_append(&carpeta, "memoria/");
			break;
		case INSTRUCCIONES:
			string_append(&carpeta, "instrucciones/");
			break;
		default:
			break;
	}

	char* result = string_new();
	string_append(&result, CONFIG_PATH_BASE);
	string_append(&result, carpeta);
	string_append(&result, archivo);

	free(carpeta);

	return result;
}

int cantidad_elementos(char** elementos){
	int i = 0;
	while (elementos[i] != NULL) i++;
	return i;
}

bool string_en_string_array(char** array, char* elemento) {
	int i = 0;
    while (array[i] != NULL) {
   		if (strcmp(array[i], elemento) == 0) return true;
        i++;
    }
	return false;
}

bool contiene_new_line(char* texto) { return string_contains(texto, "\n"); }

char* remover_ultimo_caracter(char* texto) { return string_substring_until(texto, string_length(texto) - 1); }

void agregar_padding(char* texto, char* pad, int maximo) {
	int cant_padding = maximo - string_length(texto);

	if (cant_padding <= 0) return;

	string_n_append(&texto, pad, cant_padding);
}

t_log* iniciar_logger(char* logger_name, int show_in_console) {
	t_log* nuevo_logger;
	nuevo_logger = log_create(logger_name, "TUKILOG", show_in_console, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(char* config_name) {
	t_config* nuevo_config;
	nuevo_config = config_create(config_name);
	return nuevo_config;
}

char* get_val_registro(t_pcb* pcb, char* registro) {
	bool registro_es(char* reg) { return strcmp(reg, registro) == 0; }

	if (registro_es("AX")) return pcb->registros->AX;
	else if (registro_es("BX")) return pcb->registros->BX;
	else if (registro_es("CX")) return pcb->registros->CX;
	else if (registro_es("DX")) return pcb->registros->DX;
	else if (registro_es("EAX")) return pcb->registros->EAX;
	else if (registro_es("EBX")) return pcb->registros->EBX;
	else if (registro_es("ECX")) return pcb->registros->ECX;
	else if (registro_es("EDX")) return pcb->registros->EDX;
	else if (registro_es("RAX")) return pcb->registros->RAX;
	else if (registro_es("RBX")) return pcb->registros->RBX;
	else if (registro_es("RCX")) return pcb->registros->RCX;
	else if (registro_es("RDX")) return pcb->registros->RDX;

	return NULL;
}

int get_tam_registro(char* registro) {
	char* regitros_04_bytes[] = {"AX", "BX", "CX", "DX", NULL};
	char* regitros_08_bytes[] = {"EAX", "EBX", "ECX", "EDX", NULL};
	char* regitros_16_bytes[] = {"RAX", "RBX", "RCX", "RDX", NULL};

	if (string_en_string_array(regitros_04_bytes, registro)) return 4;
	if (string_en_string_array(regitros_08_bytes, registro)) return 8;
	if (string_en_string_array(regitros_16_bytes, registro)) return 16;

	return 0;
}


// ---- CONEXION ----

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente  = socket(
		server_info->ai_family,
	    server_info->ai_socktype,
	    server_info->ai_protocol
	);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);
	return socket_cliente;
}

int iniciar_servidor(t_log* logger, char *ip, char* puerto) {
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int* esperar_cliente(int socket_servidor, t_log *logger) {
	// Aceptamos un nuevo cliente
	int* socket_cliente = malloc(sizeof(int));
	*socket_cliente = accept(socket_servidor, NULL, NULL);

	if (*socket_cliente != -1) log_info(logger, "Se conecto un cliente! (fd: %i)", *socket_cliente);

	return socket_cliente;
}

void recibir_clientes(t_log* logger, int server_fd, char *puerto, void (*atencion_al_cliente)(void*)) {
	while (1) {
		int* cliente_fd = esperar_cliente(server_fd, logger);
		if (*cliente_fd == -1) {
			log_error(logger, "No se pudo aceptar al cliente");
			break;
		}

		pthread_t thread;
		pthread_create(&thread, NULL, (void*) atencion_al_cliente, cliente_fd);
		pthread_detach(thread);
	}
}

int conectar_con_modulo(char* ip, char* puerto, op_code modulo_origen, t_log* logger, char* buffer, int chequear_OK) {
	int conexion = crear_conexion(ip, puerto);
	if (conexion == -1) return -1;

	enviar_mensaje(HOLA, conexion, modulo_origen, MENSAJE);

	// Esperar respuesta
	recibir_operacion(conexion); // recibe modulo
	recibir_operacion(conexion); // recibe cod_op

	char* respuesta = recibir_mensaje(conexion, logger, 0);

	if (chequear_OK == 0) strcpy(buffer, respuesta);

	if (strcmp(respuesta, OK) != 0 && chequear_OK == 1) return -1;

	free(respuesta);
	return conexion;
}


// ---- SERIALIZACION ----

void crear_buffer(t_paquete* paquete) {
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(op_code modulo_origen, op_code codigo_operacion) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = codigo_operacion;
	paquete->modulo_origen = modulo_origen;
	crear_buffer(paquete);

	return paquete;
}

void enviar_mensaje(char* mensaje, int socket_cliente, op_code modulo_origen, op_code operacion) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = operacion;
	paquete->modulo_origen = modulo_origen;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = string_length(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 3 * sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente) {
	int bytes = paquete->buffer->size + 3 * sizeof(int); // size, op_code y modulo
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

int recibir_operacion(int socket_cliente) {
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0) return cod_op;

	close(socket_cliente);
	return -1;
}

void* recibir_buffer(int* size, int socket_cliente) {
	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);

	void* buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

char* recibir_mensaje(int socket_cliente, t_log* logger, int liberar_mensaje) {
	int* size = malloc(sizeof(int));
	*size = 0;

	char* buffer = recibir_buffer(size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s (len: %i)", buffer, *size);

	free(size);

	if (liberar_mensaje) {
		free(buffer);
		return NULL;
	}

	return buffer;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio) {
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void* serializar_paquete(t_paquete* paquete, int bytes) {
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->modulo_origen), sizeof(int));
	desplazamiento += sizeof(int);
	
	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

void eliminar_paquete(t_paquete* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}


// ---- INSTRUCCIONES ----

uint32_t calcular_tamanio_instrucciones(t_list *lista) {
	uint32_t tamanio = 0;
	size_t tamanio_int = sizeof(int);

	void calcular_tamanio_por_instruccion(void* elem) {
		t_instruccion* parsed_elem = (t_instruccion*) elem;
		tamanio += string_length(parsed_elem->operacion) + 1;
		tamanio += string_length(parsed_elem->parametro_0) + 1;
		tamanio += string_length(parsed_elem->parametro_1) + 1;
		tamanio += string_length(parsed_elem->parametro_2) + 1;

		tamanio += tamanio_int * 4;
	}

	list_iterate(lista, (void*) calcular_tamanio_por_instruccion);

	return tamanio;
}

t_instruccion* parsear_instruccion(char *linea_instruccion) {

	char **elementos = string_split(linea_instruccion, " ");
	int length = cantidad_elementos(elementos);

	char *op = elementos[0];
	char *par_0 = "", *par_1 = "", *par_2 = "";

	if (1 < length) par_0 = elementos[1];
	if (2 < length) par_1 = elementos[2];
	if (3 < length) par_2 = elementos[3];

	if(contiene_new_line(op)) op = remover_ultimo_caracter(op);
	if(contiene_new_line(par_0)) par_0 = remover_ultimo_caracter(par_0);
	if(contiene_new_line(par_1)) par_1 = remover_ultimo_caracter(par_1);
	if(contiene_new_line(par_2)) par_2 = remover_ultimo_caracter(par_2);

	t_instruccion *instruccion = alocar_instruccion(
		string_length(op),
		string_length(par_0),
		string_length(par_1),
		string_length(par_2)
	);

	strcpy(instruccion->operacion, op);
	strcpy(instruccion->parametro_0, par_0);
	strcpy(instruccion->parametro_1, par_1);
	strcpy(instruccion->parametro_2, par_2);

	return instruccion;
}

void* serializar_instrucciones(uint32_t size, t_list *lista) {
	void *stream = malloc(size);
	uint32_t offset = 0;
	size_t tamanio_int = sizeof(int);

	t_link_element *elemento = lista->head;
	while (elemento != NULL) {
		t_instruccion *instruccion = elemento->data;

	 	int tamanio_op = string_length(instruccion->operacion) + 1;
	 	int tamanio_0  = string_length(instruccion->parametro_0) + 1;
	 	int tamanio_1  = string_length(instruccion->parametro_1) + 1;
	 	int tamanio_2  = string_length(instruccion->parametro_2) + 1;

		memcpy(stream+offset, &tamanio_op, tamanio_int);
		offset += tamanio_int;

		memcpy(stream+offset, &tamanio_0, tamanio_int);
		offset += tamanio_int;

		memcpy(stream+offset, &tamanio_1, tamanio_int);
		offset += tamanio_int;

		memcpy(stream+offset, &tamanio_2, tamanio_int);
		offset += tamanio_int;

		memcpy(stream+offset, instruccion->operacion, tamanio_op);
		offset += tamanio_op;

		memcpy(stream+offset, instruccion->parametro_0, tamanio_0);
		offset += tamanio_0;

		memcpy(stream+offset, instruccion->parametro_1, tamanio_1);
		offset += tamanio_1;

		memcpy(stream+offset, instruccion->parametro_2, tamanio_2);
		offset += tamanio_2;

		elemento = elemento->next;
	}

	free(elemento);
	return stream;
}

t_list* deserializar_instrucciones(int socket_cliente, void* buffer_inicializado, uint32_t* offset_inicializado, int size_inicializado) {
	size_t tamanio_int = sizeof(int);

	int* size = malloc(sizeof(int));
	void* buffer;
	uint32_t* offset;
	if (buffer_inicializado == NULL) {
		buffer = recibir_buffer(size, socket_cliente);
		offset = malloc(sizeof(uint32_t));
		*offset = 0;
	} else {
		buffer = buffer_inicializado;
		offset = offset_inicializado;
		*size = size_inicializado + *offset;
	}

	t_list* valores = list_create();
	while(*offset < *size) {
		int tamanio_op, tamanio_0, tamanio_1, tamanio_2;

		memcpy(&tamanio_op, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		memcpy(&tamanio_0, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		memcpy(&tamanio_1, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		memcpy(&tamanio_2, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		t_instruccion* instruccion = alocar_instruccion(tamanio_op, tamanio_0, tamanio_1, tamanio_2);

		memcpy(instruccion->operacion, buffer + *offset, tamanio_op);
		*offset += tamanio_op;

		memcpy(instruccion->parametro_0, buffer + *offset, tamanio_0);
		*offset += tamanio_0;

		memcpy(instruccion->parametro_1, buffer + *offset, tamanio_1);
		*offset += tamanio_1;

		memcpy(instruccion->parametro_2, buffer + *offset, tamanio_2);
		*offset += tamanio_2;

		list_add(valores, instruccion);
	}

	if (buffer_inicializado == NULL) {
		free(buffer);
		free(offset);
	}
	free(size);
	return valores;
}

t_instruccion* alocar_instruccion(int op_len, int par0_len, int par1_len, int par2_len)
{
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));

	instruccion->operacion   = malloc(op_len + 1);
	instruccion->parametro_0 = malloc(par0_len + 1);
	instruccion->parametro_1 = malloc(par1_len + 1);
	instruccion->parametro_2 = malloc(par2_len + 1);

	return instruccion;
}

void liberar_instruccion(void* param) {
	t_instruccion* instruccion = (t_instruccion*) param;

	free(instruccion->operacion);
	free(instruccion->parametro_0);
	free(instruccion->parametro_1);
	free(instruccion->parametro_2);
	free(instruccion);
}

void enviar_instrucciones(int socket_fd, t_list *lista) {
	t_paquete *paquete = crear_paquete(CONSOLA, PAQUETE);

	uint32_t tamanio_instrucciones = calcular_tamanio_instrucciones(lista);
	paquete->buffer->size = tamanio_instrucciones;

	void *stream = serializar_instrucciones(tamanio_instrucciones, lista);
	paquete->buffer->stream = stream;

	enviar_paquete(paquete, socket_fd);
	eliminar_paquete(paquete);
	list_clean_and_destroy_elements(lista, (void*) liberar_instruccion);
	list_destroy(lista);
}


// ---- PCB ----

t_archivo_abierto* alocar_archivo(int len_nombre, int len_path) {
	t_archivo_abierto* archivo = malloc(sizeof(t_archivo_abierto));

	archivo->offset = malloc(sizeof(int));

	if (len_nombre > 0) {
		archivo->nombre = malloc(len_nombre + 1);
	} else {
		archivo->nombre = NULL;
	}

	if (len_path > 0) {
		archivo->path = malloc(len_path + 1);
	} else {
		archivo->path = NULL;
	}

	return archivo;
}

t_segmento* alocar_segmento() {
	t_segmento* segmento = malloc(sizeof(t_segmento));

	segmento->id = malloc(sizeof(int));
	segmento->pid = malloc(sizeof(int));
	segmento->tamanio = malloc(sizeof(int));
	segmento->direccion_base = malloc(sizeof(intptr_t));

	return segmento;
}

t_registros_cpu* alocar_registros_cpu() {
	t_registros_cpu* registros = malloc(sizeof(t_registros_cpu));
	int tam_4 = 4, tam_8 = 8, tam_16 = 16;

	registros->AX  = malloc(tam_4);
	registros->BX  = malloc(tam_4);
	registros->CX  = malloc(tam_4);
	registros->DX  = malloc(tam_4);
	registros->EAX = malloc(tam_8);
	registros->EBX = malloc(tam_8);
	registros->ECX = malloc(tam_8);
	registros->EDX = malloc(tam_8);
	registros->RAX = malloc(tam_16);
	registros->RBX = malloc(tam_16);
	registros->RCX = malloc(tam_16);
	registros->RDX = malloc(tam_16);

	char* val_inicial_4 = string_repeat('7', tam_4);
	char* val_inicial_8 = string_repeat('7', tam_8);
	char* val_inicial_16 = string_repeat('7', tam_16);

	memcpy(registros->AX, val_inicial_4, tam_4);
	memcpy(registros->BX, val_inicial_4, tam_4);
	memcpy(registros->CX, val_inicial_4, tam_4);
	memcpy(registros->DX, val_inicial_4, tam_4);
	memcpy(registros->EAX, val_inicial_8, tam_8);
	memcpy(registros->EBX, val_inicial_8, tam_8);
	memcpy(registros->ECX, val_inicial_8, tam_8);
	memcpy(registros->EDX, val_inicial_8, tam_8);
	memcpy(registros->RAX, val_inicial_16, tam_16);
	memcpy(registros->RBX, val_inicial_16, tam_16);
	memcpy(registros->RCX, val_inicial_16, tam_16);
	memcpy(registros->RDX, val_inicial_16, tam_16);
	
	free(val_inicial_4);
	free(val_inicial_8);
	free(val_inicial_16);

	return registros;
}

t_pcb* alocar_pcb() {
	t_pcb* pcb = malloc(sizeof(t_pcb));

	pcb->socket_cliente = malloc(sizeof(int));
	pcb->pid = malloc(sizeof(int));
	pcb->pc = malloc(sizeof(int));
	pcb->estimacion_proxima_rafaga = malloc(sizeof(double));
	pcb->tiempo_de_ejecucion = malloc(sizeof(double));
	pcb->tiempo_llegada_ready = malloc(sizeof(double));

	pcb->registros = alocar_registros_cpu();
	pcb->instrucciones = list_create();
	pcb->tabla_segmentos = list_create();
	pcb->archivos_abiertos = list_create();

	return pcb;
}

t_interrupcion_cpu* alocar_interrupcion() {
	t_interrupcion_cpu* interrupcion = malloc(sizeof(t_interrupcion_cpu));

	interrupcion->codigo = malloc(sizeof(op_code));
	interrupcion->tamanio_parametros = malloc(sizeof(int));

	*interrupcion->codigo = NO_OP;

	return interrupcion;
}


void liberar_recurso(void* param) {
	t_data_recurso* data_recurso = (t_data_recurso*) param;

	if (!queue_is_empty(data_recurso->cola_bloqueo)) {
		queue_clean_and_destroy_elements(data_recurso->cola_bloqueo, (void*) liberar_pcb);
	}
	if (!list_is_empty(data_recurso->procesos_con_recurso)) {
		list_clean_and_destroy_elements(data_recurso->procesos_con_recurso, free);
	}

	queue_destroy(data_recurso->cola_bloqueo);
	list_destroy(data_recurso->procesos_con_recurso);
	free(data_recurso->recurso);
	free(data_recurso);
}

void liberar_archivo(void* param) {
	t_archivo_abierto* archivo = (t_archivo_abierto*) param;

	free(archivo->offset);
	free(archivo->nombre);
	free(archivo->path);
	free(archivo);
}

void liberar_segmento(void* param) {
	t_segmento* segmento = (t_segmento*) param;

	free(segmento->id);
	free(segmento->pid);
	free(segmento->tamanio);
	free(segmento->direccion_base);
	free(segmento);
}

void liberar_registros_cpu(t_pcb* pcb) {
	free(pcb->registros->AX);
	free(pcb->registros->BX);
	free(pcb->registros->CX);
	free(pcb->registros->DX);
	free(pcb->registros->EAX);
	free(pcb->registros->EBX);
	free(pcb->registros->ECX);
	free(pcb->registros->EDX);
	free(pcb->registros->RAX);
	free(pcb->registros->RBX);
	free(pcb->registros->RCX);
	free(pcb->registros->RDX);
	free(pcb->registros);
}

void liberar_pcb(t_pcb* pcb) {
	free(pcb->socket_cliente);
	free(pcb->pid);
	free(pcb->pc);
	free(pcb->estimacion_proxima_rafaga);
	free(pcb->tiempo_de_ejecucion);
	free(pcb->tiempo_llegada_ready);

	liberar_registros_cpu(pcb);

	list_clean_and_destroy_elements(pcb->instrucciones, (void*) liberar_instruccion);
	list_clean_and_destroy_elements(pcb->tabla_segmentos, (void*) liberar_segmento);
	list_clean_and_destroy_elements(pcb->archivos_abiertos, (void*) liberar_archivo);

	list_destroy(pcb->instrucciones);
	list_destroy(pcb->tabla_segmentos);
	list_destroy(pcb->archivos_abiertos);

	free(pcb);
}

void liberar_create_segment(t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_crear_segmento* parametros = (t_interrupcion_cpu_crear_segmento*) interrupcion->parametros;
	free(parametros->id_segmento);
	free(parametros->tamanio_segmento);
}

void liberar_f_truncate_seek(t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_truncate_seek* parametros = (t_interrupcion_cpu_truncate_seek*) interrupcion->parametros;
	free(parametros->nombre_archivo);
	free(parametros->tamanio_nombre);
	free(parametros->entero);
}

void liberar_f_read_write(t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_read_write* parametros = (t_interrupcion_cpu_read_write*) interrupcion->parametros;
	free(parametros->direccion_fisica);
	free(parametros->cantidad_bytes);
	free(parametros->nombre_archivo);
	free(parametros->tamanio_nombre);
}

void liberar_parametros_interrupcion(t_interrupcion_cpu* interrupcion) {
	switch (*interrupcion->codigo) {
		case CREATE_SEGMENT:
			liberar_create_segment(interrupcion);
			break;
		case F_TRUNCATE:
		case F_SEEK:
			liberar_f_truncate_seek(interrupcion);
			break;
		case F_READ:
		case F_WRITE:
			liberar_f_read_write(interrupcion);
			break;
		default:
			break;
	}

	free(interrupcion->parametros);
}


void liberar_interrupcion(t_interrupcion_cpu* interrupcion, bool libera_pcb) {
	liberar_parametros_interrupcion(interrupcion);

	free(interrupcion->tamanio_parametros);
	free(interrupcion->codigo);

	if (libera_pcb) liberar_pcb(interrupcion->pcb);

	free(interrupcion);
}


void setear_pcb(
	t_pcb* pcb,
	int socket_cliente,
	int pid,
	double estimacion_proxima_rafaga,
	t_list* instrucciones
) {
	*pcb->socket_cliente = socket_cliente;
	*pcb->pid = pid;
	*pcb->pc = 0;
	*pcb->estimacion_proxima_rafaga = estimacion_proxima_rafaga;
	*pcb->tiempo_de_ejecucion = 0;

	list_destroy(pcb->instrucciones); // Lista vacia alocada anteriormente
	pcb->instrucciones = instrucciones;
}

void setear_interrupcion(
	t_interrupcion_cpu* interrupcion,
	op_code codigo,
	void* parametros,
	int tamanio_parametros,
	t_pcb* pcb
) {
	*interrupcion->codigo = codigo;
	*interrupcion->tamanio_parametros = tamanio_parametros;
	interrupcion->parametros = parametros;
	interrupcion->pcb = pcb;
}


uint32_t calcular_tamanio_tabla_segmentos(t_list* tabla_segmentos) {
	uint32_t tamanio = 0;
	uint32_t tamanio_segmento = sizeof(int) * 3 + sizeof(intptr_t);

	tamanio += sizeof(int);
	tamanio += (uint32_t) list_size(tabla_segmentos) * tamanio_segmento;

	return tamanio;
}

uint32_t calcular_tamanio_archivos_abiertos(t_list* archivos_abiertos) {
	uint32_t tamanio = 0;
	size_t tamanio_int = sizeof(int);

	void calcular_tamanio_por_archivo(void* elem) {
		t_archivo_abierto* parsed_elem = (t_archivo_abierto*) elem;
		tamanio += string_length(parsed_elem->nombre) + 1;
		tamanio += string_length(parsed_elem->path) + 1;

		tamanio += tamanio_int * 3;
	}

	tamanio += tamanio_int;
	list_iterate(archivos_abiertos, (void*) calcular_tamanio_por_archivo);

	return tamanio;
}

uint32_t calcular_tamanio_pcb(t_pcb* pcb) {
	uint32_t tamanio = 0;

	tamanio += sizeof(int) * 3;
	tamanio += sizeof(double) * 3;

	tamanio += 4 * 4 + 8 * 4 + 16 * 4;

	tamanio += sizeof(int);
	tamanio += calcular_tamanio_instrucciones(pcb->instrucciones);

	tamanio += calcular_tamanio_tabla_segmentos(pcb->tabla_segmentos);
	tamanio += calcular_tamanio_archivos_abiertos(pcb->archivos_abiertos);

	return tamanio;
}

uint32_t calcular_tamanio_interrupcion(t_interrupcion_cpu* interrupcion) {
	uint32_t tamanio = 0;

	tamanio += sizeof(op_code);

	tamanio += sizeof(int);
	tamanio += *interrupcion->tamanio_parametros;

	tamanio += sizeof(int);
	tamanio += calcular_tamanio_pcb(interrupcion->pcb);

	return tamanio;
}


void serializar_registros_cpu(void* stream, uint32_t* offset, t_registros_cpu* registros) {
	memcpy(stream + *offset, registros->AX, 4);
	*offset += 4;
	memcpy(stream + *offset, registros->BX, 4);
	*offset += 4;
	memcpy(stream + *offset, registros->CX, 4);
	*offset += 4;
	memcpy(stream + *offset, registros->DX, 4);
	*offset += 4;

	memcpy(stream + *offset, registros->EAX, 8);
	*offset += 8;
	memcpy(stream + *offset, registros->EBX, 8);
	*offset += 8;
	memcpy(stream + *offset, registros->ECX, 8);
	*offset += 8;
	memcpy(stream + *offset, registros->EDX, 8);
	*offset += 8;

	memcpy(stream + *offset, registros->RAX, 16);
	*offset += 16;
	memcpy(stream + *offset, registros->RBX, 16);
	*offset += 16;
	memcpy(stream + *offset, registros->RCX, 16);
	*offset += 16;
	memcpy(stream + *offset, registros->RDX, 16);
	*offset += 16;
}

void serializar_tabla_segmentos(void* stream, uint32_t* offset, t_list* tabla_segmentos) {
	size_t tamanio_int = sizeof(int);
	int tamanio_tabla = list_size(tabla_segmentos);

	memcpy(stream + *offset, &tamanio_tabla, tamanio_int);
	*offset += tamanio_int;

	t_link_element* elemento = tabla_segmentos->head;
	while (elemento != NULL) {
		t_segmento* segmento = elemento->data;

		memcpy(stream + *offset, segmento->id, tamanio_int);
		*offset += tamanio_int;

		memcpy(stream + *offset, segmento->pid, tamanio_int);
		*offset += tamanio_int;

		memcpy(stream + *offset, segmento->direccion_base, sizeof(intptr_t));
		*offset += sizeof(intptr_t);

		memcpy(stream + *offset, segmento->tamanio, tamanio_int);
		*offset += tamanio_int;

		elemento = elemento->next;
	}
}

void serializar_archivo_abierto(void* stream, uint32_t* offset, t_archivo_abierto* archivo) {
	int len_nombre = string_length(archivo->nombre) + 1;
	int len_path = string_length(archivo->path) + 1;

	memcpy(stream + *offset, &len_nombre, sizeof(int));
	*offset += sizeof(int);

	memcpy(stream + *offset, archivo->nombre, len_nombre);
	*offset += len_nombre;

	memcpy(stream + *offset, &len_path, sizeof(int));
	*offset += sizeof(int);

	memcpy(stream + *offset, archivo->path, len_path);
	*offset += len_path;

	memcpy(stream + *offset, archivo->offset, sizeof(int));
	*offset += sizeof(int);
}

void serializar_archivos_abiertos(void* stream, uint32_t* offset, t_list* archivos_abiertos) {
	int tamanio_lista = list_size(archivos_abiertos);

	memcpy(stream + *offset, &tamanio_lista, sizeof(int));
	*offset += sizeof(int);

	t_link_element* elemento = archivos_abiertos->head;
	while (elemento != NULL) {
		t_archivo_abierto* archivo = elemento->data;

		serializar_archivo_abierto(stream, offset, archivo);

		elemento = elemento->next;
	}
}

void* serializar_pcb(uint32_t size, t_pcb* pcb) {
	void *stream = malloc(size);
	uint32_t offset = 0;
	size_t tamanio_int = sizeof(int);

	memcpy(stream + offset, pcb->socket_cliente, tamanio_int);
	offset += tamanio_int;
	memcpy(stream + offset, pcb->pid, tamanio_int);
	offset += tamanio_int;
	memcpy(stream + offset, pcb->pc, tamanio_int);
	offset += tamanio_int;
	memcpy(stream + offset, pcb->estimacion_proxima_rafaga, sizeof(double));
	offset += sizeof(double);
	memcpy(stream + offset, pcb->tiempo_de_ejecucion, sizeof(double));
	offset += sizeof(double);
	memcpy(stream + offset, pcb->tiempo_llegada_ready, sizeof(double));
	offset += sizeof(double);

	serializar_registros_cpu(stream, &offset, pcb->registros);

	uint32_t tamanio_instrucciones = calcular_tamanio_instrucciones(pcb->instrucciones);
	void* stream_instrucciones = serializar_instrucciones(tamanio_instrucciones, pcb->instrucciones);

	memcpy(stream + offset, &tamanio_instrucciones, tamanio_int);
	offset += tamanio_int;

	memcpy(stream + offset, stream_instrucciones, tamanio_instrucciones);
	offset += tamanio_instrucciones;

	free(stream_instrucciones);

	serializar_tabla_segmentos(stream, &offset, pcb->tabla_segmentos);
	serializar_archivos_abiertos(stream, &offset, pcb->archivos_abiertos);

	return stream;
}

void serializar_un_parametro(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	memcpy(stream + *offset, interrupcion->parametros, *interrupcion->tamanio_parametros);
	*offset += *interrupcion->tamanio_parametros;
}

void serializar_create_segment(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_crear_segmento* parametros = (t_interrupcion_cpu_crear_segmento*) interrupcion->parametros;

	memcpy(stream + *offset, parametros->id_segmento, sizeof(int));
	*offset += sizeof(int);

	memcpy(stream + *offset, parametros->tamanio_segmento, sizeof(int));
	*offset += sizeof(int);
}

void serializar_f_truncate_seek(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_truncate_seek* parametros = (t_interrupcion_cpu_truncate_seek*) interrupcion->parametros;

	memcpy(stream + *offset, parametros->tamanio_nombre, sizeof(int));
	*offset += sizeof(int);

	memcpy(stream + *offset, parametros->nombre_archivo, *parametros->tamanio_nombre);
	*offset += *parametros->tamanio_nombre;

	memcpy(stream + *offset, parametros->entero, sizeof(int));
	*offset += sizeof(int);
}

void serializar_f_read_write(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_read_write* parametros = (t_interrupcion_cpu_read_write*) interrupcion->parametros;

	memcpy(stream + *offset, parametros->tamanio_nombre, sizeof(int));
	*offset += sizeof(int);

	memcpy(stream + *offset, parametros->nombre_archivo, *parametros->tamanio_nombre);
	*offset += *parametros->tamanio_nombre;

	memcpy(stream + *offset, parametros->cantidad_bytes, sizeof(int));
	*offset += sizeof(int);

	memcpy(stream + *offset, parametros->direccion_fisica, sizeof(intptr_t));
	*offset += sizeof(intptr_t);
}

void serializar_parametros_interrupcion(void* stream, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	memcpy(stream + *offset, interrupcion->tamanio_parametros, sizeof(int));
	*offset += sizeof(int);

	switch (*interrupcion->codigo) {
		case YIELD:
		case EXIT:
		case WAIT:
		case SIGNAL:
		case F_OPEN:
		case F_CLOSE:
		case IO:
		case DELETE_SEGMENT:
			serializar_un_parametro(stream, offset, interrupcion);
			break;
		case CREATE_SEGMENT:
			serializar_create_segment(stream, offset, interrupcion);
			break;
		case F_TRUNCATE:
		case F_SEEK:
			serializar_f_truncate_seek(stream, offset, interrupcion);
			break;
		case F_READ:
		case F_WRITE:
			serializar_f_read_write(stream, offset, interrupcion);
			break;
		default:
			break;
	}
}

void* serializar_interrupcion(uint32_t size, t_interrupcion_cpu* interrupcion) {
	size_t tamanio_int = sizeof(int);
	void *stream = malloc(size);

	uint32_t* offset = malloc(sizeof(uint32_t));
	*offset = 0;

	memcpy(stream + *offset, interrupcion->codigo, tamanio_int);
	*offset += tamanio_int;

	serializar_parametros_interrupcion(stream, offset, interrupcion);

	uint32_t tamanio_pcb = calcular_tamanio_pcb(interrupcion->pcb);
	void* stream_pcb = serializar_pcb(tamanio_pcb, interrupcion->pcb);

	memcpy(stream + *offset, &tamanio_pcb, tamanio_int);
	*offset += tamanio_int;

	memcpy(stream + *offset, stream_pcb, tamanio_pcb);
	*offset += tamanio_pcb;

	free(stream_pcb);
	free(offset);
	return stream;
}


void deserializar_tabla_segmentos(void* buffer, uint32_t* offset, t_list* tabla_segmentos) {
	size_t tamanio_int = sizeof(int);
	int tamanio_tabla;

	memcpy(&tamanio_tabla, buffer + *offset, tamanio_int);
	*offset += tamanio_int;

	int i = 0;
	while (i < tamanio_tabla) {
		t_segmento* segmento = alocar_segmento();

		memcpy(segmento->id, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		memcpy(segmento->pid, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		memcpy(segmento->direccion_base, buffer + *offset, sizeof(intptr_t));
		*offset += sizeof(intptr_t);

		memcpy(segmento->tamanio, buffer + *offset, tamanio_int);
		*offset += tamanio_int;

		list_add(tabla_segmentos, segmento);
		i++;
	}
}

void deserializar_archivo_abierto(void* buffer, uint32_t* offset, t_archivo_abierto* archivo) {
	int len_nombre, len_path;

	memcpy(&len_nombre, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	if (archivo->nombre == NULL) archivo->nombre = malloc(len_nombre);

	memcpy(archivo->nombre, buffer + *offset, len_nombre);
	*offset += len_nombre;

	memcpy(&len_path, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	if (archivo->path == NULL) archivo->path = malloc(len_path);

	memcpy(archivo->path, buffer + *offset, len_path);
	*offset += len_path;

	memcpy(archivo->offset, buffer + *offset, sizeof(int));
	*offset += sizeof(int);
}

void deserializar_archivos_abiertos(void* buffer, uint32_t* offset, t_list* archivos_abiertos) {
	int tamanio_lista;

	memcpy(&tamanio_lista, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	int i = 0;
	while (i < tamanio_lista) {
		t_archivo_abierto* archivo = alocar_archivo(0, 0);

		deserializar_archivo_abierto(buffer, offset, archivo);

		list_add(archivos_abiertos, archivo);
		i++;
	}
}

void deserializar_registros_cpu(void* buffer, uint32_t* offset, t_registros_cpu* registros) {
	memcpy(registros->AX, buffer + *offset, 4);
	*offset += 4;
	memcpy(registros->BX, buffer + *offset, 4);
	*offset += 4;
	memcpy(registros->CX, buffer + *offset, 4);
	*offset += 4;
	memcpy(registros->DX, buffer + *offset, 4);
	*offset += 4;

	memcpy(registros->EAX, buffer + *offset, 8);
	*offset += 8;
	memcpy(registros->EBX, buffer + *offset, 8);
	*offset += 8;
	memcpy(registros->ECX, buffer + *offset, 8);
	*offset += 8;
	memcpy(registros->EDX, buffer + *offset, 8);
	*offset += 8;

	memcpy(registros->RAX, buffer + *offset, 16);
	*offset += 16;
	memcpy(registros->RBX, buffer + *offset, 16);
	*offset += 16;
	memcpy(registros->RCX, buffer + *offset, 16);
	*offset += 16;
	memcpy(registros->RDX, buffer + *offset, 16);
	*offset += 16;
}

t_pcb* deserializar_pcb(int socket_cliente,  void* buffer_inicializado, uint32_t* offset_inicializado, int size_inicializado) {
	size_t tamanio_int = sizeof(int);
	t_pcb* pcb = alocar_pcb();

	int* size = malloc(sizeof(int));
	void* buffer;
	uint32_t* offset;
	if (buffer_inicializado == NULL) {
		buffer = recibir_buffer(size, socket_cliente);
		offset = malloc(sizeof(uint32_t));
		*offset = 0;
	} else {
		buffer = buffer_inicializado;
		offset = offset_inicializado;
		*size = size_inicializado + *offset;
	}

	memcpy(pcb->socket_cliente, buffer + *offset, tamanio_int);
	*offset += tamanio_int;
	memcpy(pcb->pid, buffer + *offset, tamanio_int);
	*offset += tamanio_int;
	memcpy(pcb->pc, buffer + *offset, tamanio_int);
	*offset += tamanio_int;
	memcpy(pcb->estimacion_proxima_rafaga, buffer + *offset, sizeof(double));
	*offset += sizeof(double);
	memcpy(pcb->tiempo_de_ejecucion, buffer + *offset, sizeof(double));
	*offset += sizeof(double);
	memcpy(pcb->tiempo_llegada_ready, buffer + *offset, sizeof(double));
	*offset += sizeof(double);

	deserializar_registros_cpu(buffer, offset, pcb->registros);

	int tamanio_instrucciones;
	memcpy(&tamanio_instrucciones, buffer + *offset, tamanio_int);
	*offset += tamanio_int;

	list_destroy(pcb->instrucciones);
	pcb->instrucciones = deserializar_instrucciones(-1, buffer, offset, tamanio_instrucciones);

	deserializar_tabla_segmentos(buffer, offset, pcb->tabla_segmentos);
	deserializar_archivos_abiertos(buffer, offset, pcb->archivos_abiertos);

	if (buffer_inicializado == NULL) {
		free(buffer);
		free(offset);
	}
	free(size);
	return pcb;
}

void deserializar_un_parametro(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	interrupcion->parametros = malloc(*interrupcion->tamanio_parametros);

	memcpy(interrupcion->parametros, buffer + *offset, *interrupcion->tamanio_parametros);
	*offset += *interrupcion->tamanio_parametros;
}

void deserializar_create_segment(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	t_interrupcion_cpu_crear_segmento* parametros = malloc(sizeof(t_interrupcion_cpu_crear_segmento));
	parametros->id_segmento = malloc(sizeof(int));
	parametros->tamanio_segmento = malloc(sizeof(int));

	memcpy(parametros->id_segmento, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	memcpy(parametros->tamanio_segmento, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	interrupcion->parametros = (void*) parametros;
}

void deserializar_f_truncate_seek(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	int tamanio_nombre;

	memcpy(&tamanio_nombre, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	t_interrupcion_cpu_truncate_seek* parametros = malloc(sizeof(t_interrupcion_cpu_truncate_seek));
	parametros->nombre_archivo = malloc(tamanio_nombre);
	parametros->tamanio_nombre = malloc(sizeof(int));
	parametros->entero = malloc(sizeof(int));

	*parametros->tamanio_nombre = tamanio_nombre;

	memcpy(parametros->nombre_archivo, buffer + *offset, tamanio_nombre);
	*offset += tamanio_nombre;

	memcpy(parametros->entero, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	interrupcion->parametros = (void*) parametros;
}

void deserializar_f_read_write(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	int tamanio_nombre;

	memcpy(&tamanio_nombre, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	t_interrupcion_cpu_read_write* parametros = malloc(sizeof(t_interrupcion_cpu_read_write));
	parametros->nombre_archivo = malloc(tamanio_nombre);
	parametros->tamanio_nombre = malloc(sizeof(int));
	parametros->cantidad_bytes = malloc(sizeof(int));
	parametros->direccion_fisica = malloc(sizeof(intptr_t));

	*parametros->tamanio_nombre = tamanio_nombre;

	memcpy(parametros->nombre_archivo, buffer + *offset, tamanio_nombre);
	*offset += tamanio_nombre;

	memcpy(parametros->cantidad_bytes, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	memcpy(parametros->direccion_fisica, buffer + *offset, sizeof(intptr_t));
	*offset += sizeof(intptr_t);

	interrupcion->parametros = (void*) parametros;
}

void deserializar_parametros_interrupcion(void* buffer, uint32_t* offset, t_interrupcion_cpu* interrupcion) {
	memcpy(interrupcion->tamanio_parametros, buffer + *offset, sizeof(int));
	*offset += sizeof(int);

	switch (*interrupcion->codigo) {
		case YIELD:
		case EXIT:
		case WAIT:
		case SIGNAL:
		case F_OPEN:
		case F_CLOSE:
		case IO:
		case DELETE_SEGMENT:
			deserializar_un_parametro(buffer, offset, interrupcion);
			break;
		case CREATE_SEGMENT:
			deserializar_create_segment(buffer, offset, interrupcion);
			break;
		case F_TRUNCATE:
		case F_SEEK:
			deserializar_f_truncate_seek(buffer, offset, interrupcion);
			break;
		case F_READ:
		case F_WRITE:
			deserializar_f_read_write(buffer, offset, interrupcion);
			break;
		default:
			break;
	}
}

t_interrupcion_cpu* deserializar_interrupcion(int socket_cliente) {
	size_t tamanio_int = sizeof(int);
	int* size = malloc(sizeof(int));

	uint32_t* offset = malloc(sizeof(uint32_t));
	*offset = 0;

	void* buffer = recibir_buffer(size, socket_cliente);
	t_interrupcion_cpu* interrupcion = alocar_interrupcion();

	memcpy(interrupcion->codigo, buffer + *offset, tamanio_int);
	*offset += tamanio_int;

	deserializar_parametros_interrupcion(buffer, offset, interrupcion);

	int tamanio_pcb;
	memcpy(&tamanio_pcb, buffer + *offset, tamanio_int);
	*offset += tamanio_int;

	interrupcion->pcb = deserializar_pcb(-1, buffer, offset, tamanio_pcb);

	free(buffer);
	free(size);
	free(offset);
	return interrupcion;
}


void enviar_proceso(int socket_fd, t_pcb* pcb, op_code modulo_origen) {
	t_paquete *paquete = crear_paquete(modulo_origen, PAQUETE);

	uint32_t tamanio_pcb = calcular_tamanio_pcb(pcb);
	paquete->buffer->size = tamanio_pcb;

	void* stream = serializar_pcb(tamanio_pcb, pcb);
	paquete->buffer->stream = stream;

	enviar_paquete(paquete, socket_fd);
	eliminar_paquete(paquete);
    liberar_pcb(pcb);
}

void enviar_interrupcion(int socket_fd, t_interrupcion_cpu* interrupcion, op_code modulo_origen) {
	t_paquete *paquete = crear_paquete(modulo_origen, PAQUETE);

	uint32_t tamanio_interrupcion = calcular_tamanio_interrupcion(interrupcion);
	paquete->buffer->size = tamanio_interrupcion;
	void* stream = serializar_interrupcion(tamanio_interrupcion, interrupcion);
	paquete->buffer->stream = stream;

	enviar_paquete(paquete, socket_fd);
	eliminar_paquete(paquete);
    liberar_interrupcion(interrupcion, true);
}
