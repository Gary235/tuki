#include "consola.h"

t_log *logger;
t_config *config;

int conexion_kernel = -1;

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_catch);
	signal(SIGTERM, signal_catch);

	logger = iniciar_logger("consola.log", 1);

	if (argc != 3) {
		log_error(logger, "Cantidad de argumentos erroneo");
		log_destroy(logger);
		return EXIT_FAILURE;
	}

	char* config_path = argv[1];
	char* config_path_extendido = get_config_path(CONSOLA, config_path);
	config = iniciar_config(config_path_extendido);
	free(config_path_extendido);

	if (config == NULL) {
		log_error(logger, "Config es NULL");
		terminar_programa();
		return EXIT_FAILURE;
	}

	char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
	char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

	conexion_kernel = crear_conexion(ip_kernel, puerto_kernel);
	if (conexion_kernel == -1) {
		log_error(logger, "No se pudo conectar con el Kernel");
		terminar_programa();
		return EXIT_FAILURE;
	}

	char* instrucciones_path = argv[2];
	t_list *lista_instrucciones = leer_instrucciones(get_config_path(INSTRUCCIONES, instrucciones_path));
	if (lista_instrucciones == NULL) {
		log_error(logger, "No hay lista de instrucciones");
		terminar_programa();
		return EXIT_FAILURE;
	}

	enviar_instrucciones(conexion_kernel, lista_instrucciones);
	log_info(logger, "Instrucciones enviadas");

	bool should_continue = true;
	while (should_continue) {
		recibir_operacion(conexion_kernel); // recibe modulo
		int cod_op = recibir_operacion(conexion_kernel);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(conexion_kernel, logger, 1);
				break;
			case EXIT:
				handle_exit();
				should_continue = false;
				break;
			case -1:
				log_error(logger, "El cliente se desconecto. Terminando servidor");
				should_continue = false;
				break;
			default:
				log_warning(logger, "Operacion desconocida. Trato de leer lista");
				break;
		}
	}

	terminar_programa();
	return EXIT_SUCCESS;
}

t_list* leer_instrucciones(char *file_path) {
	FILE *fp;

	fp = fopen(file_path, "r");
	if (fp == NULL) {
		printf("Error opening file\n");
		return NULL;
	}

	char *linea = NULL;
	size_t linea_size = 0;

	t_list *lista_instrucciones = list_create();
	while (getline(&linea, &linea_size, fp) != -1) {
		list_add(lista_instrucciones, parsear_instruccion(linea));
	}

	fclose(fp);
	return lista_instrucciones;
}

void handle_exit() {
	char* respuesta = recibir_mensaje(conexion_kernel, logger, 0);

	if (strcmp(respuesta, "SUCCESS") == 0) {
		log_info(logger, "Termino Exitosamente");
	} else {
		log_error(logger, "Ocurrio un error, motivo: %s", respuesta);
	}

	free(respuesta);
}

void terminar_programa() {
	log_destroy(logger);
	config_destroy(config);
	close(conexion_kernel);
}

void signal_catch() {
	log_error(logger, "Terminado a la fuerza");
	terminar_programa();
	exit(EXIT_FAILURE);
}
