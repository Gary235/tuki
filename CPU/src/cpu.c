#include "cpu.h"

t_log *logger;
t_config *config;

int server_fd = 0, conexion_memoria = 0;

int main() {
	signal(SIGINT, signal_catch);
	signal(SIGTERM, signal_catch);

	logger = iniciar_logger("cpu.log", 1);

	char* config_path = get_config_path(CPU, "cpu.config");
	config = iniciar_config(config_path);
	free(config_path);

	if (config == NULL) {
		log_error(logger, "Config es NULL");
		terminar_programa();
		return EXIT_FAILURE;
	}

	conexion_memoria = conectar_con_modulo(
		config_get_string_value(config, "IP_MEMORIA"),
		config_get_string_value(config, "PUERTO_MEMORIA"),
		CPU,
		logger,
		NULL,
		1
	);

    if (conexion_memoria == -1) {
    	log_error(logger, "No se pudo establecer conexion con memoria");
		terminar_programa();
		return EXIT_FAILURE;
    }

	char *puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	server_fd = iniciar_servidor(logger, NULL, puerto_escucha);
	log_info(logger, "CPU listo para recibir al kernel");

	recibir_kernel(puerto_escucha);

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
