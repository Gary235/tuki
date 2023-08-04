#include "file_system.h"

t_list* fcbs;
t_bitarray* bitmap_bloques;
void* bloques_en_memoria, *bitmap_en_memoria;
char* base_archivo;

t_log *logger;
t_config *config, *superbloque;

int conexion_memoria = 0, server_fd = 0;
int tamanio_bloque, cantidad_bloques, tamanio_file_system;

int main() {
	signal(SIGINT, signal_catch);
	signal(SIGTERM, signal_catch);

	logger = iniciar_logger("file_system.log", 1);

	char* config_path = get_config_path(FILE_SYSTEM, "file_system.config");
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
		FILE_SYSTEM,
		logger,
		NULL,
		1
	);

	if (conexion_memoria == -1) {
    	log_error(logger, "No se pudo establecer conexion con memoria");
		terminar_programa();
		return EXIT_FAILURE;
    }

	iniciar_estructuras();

	char *puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	server_fd = iniciar_servidor(logger, NULL, puerto_escucha);
	log_info(logger, "File System listo para recibir al kernel");

	recibir_kernel(puerto_escucha);

	terminar_programa();
	return EXIT_SUCCESS;
}

void terminar_programa() {
	munmap(bitmap_en_memoria, cantidad_bloques);
	munmap(bloques_en_memoria, tamanio_file_system);

	close(server_fd);
	log_destroy(logger);
	config_destroy(config);
}

void signal_catch() {
	log_error(logger, "Terminado a la fuerza");
	terminar_programa();
	exit(EXIT_FAILURE);
}
