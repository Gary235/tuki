#include "kernel.h"

t_log* logger;
t_config* config;
int conexion_cpu = 0, conexion_fs = 0, conexion_memoria = 0, server_fd = 0, max_segmentos_por_proceso = 0;

int main() {
	signal(SIGINT, signal_catch);
	signal(SIGTERM, signal_catch);

	logger = iniciar_logger("kernel.log", 1);
	
	char* config_path = get_config_path(KERNEL, "kernel.config");
	config = iniciar_config(config_path);
	free(config_path);

	if (config == NULL) {
		log_error(logger, "Config es NULL");
		terminar_programa();
		return EXIT_FAILURE;
	}

	iniciar_planificacion();

	log_info(logger, "Conectando con Memoria...");
	char* buffer_memoria = malloc(3);
	conexion_memoria = conectar_con_modulo(
		config_get_string_value(config, "IP_MEMORIA"),
		config_get_string_value(config, "PUERTO_MEMORIA"),
		KERNEL,
		logger,
		buffer_memoria,
		0
	);
	log_info(logger, "Recibida Cant Segmentos por Proceso %s", buffer_memoria);
	max_segmentos_por_proceso = strtol(buffer_memoria, NULL, 10);
	free(buffer_memoria);

	log_info(logger, "Conectando con CPU...");
	conexion_cpu = conectar_con_modulo(
		config_get_string_value(config, "IP_CPU"),
		config_get_string_value(config, "PUERTO_CPU"),
		KERNEL,
		logger,
		NULL,
		1
	);

	log_info(logger, "Conectando con FileSystem...");
	conexion_fs = conectar_con_modulo(
		config_get_string_value(config, "IP_FILESYSTEM"),
		config_get_string_value(config, "PUERTO_FILESYSTEM"),
		KERNEL,
		logger,
		NULL,
		1
	);

    if (conexion_cpu == -1 || conexion_fs == -1 || conexion_memoria == -1) {
    	log_error(logger, "No se pudo establecer conexion con modulos");
		terminar_programa();
		return EXIT_FAILURE;
    }

	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	server_fd = iniciar_servidor(logger, NULL, puerto_escucha);
	log_info(logger, "Kernel listo para recibir Consolas");

	recibir_clientes(logger, server_fd, puerto_escucha, atender_consola);

    terminar_programa();
	return EXIT_SUCCESS;
}

void terminar_programa() {
	close(server_fd);
	log_destroy(logger);
	config_destroy(config);
}

void signal_catch() {
	log_error(logger, "Terminado a la fuerza");
	terminar_programa();
	exit(EXIT_FAILURE);
}
