#include "memoria.h"

t_log* logger;
t_config* config;

void* espacio_usuario;
t_list *huecos_libres, *tabla_segmentos;

int conexion_kernel = 0, conexion_cpu  = 0, conexion_fs = 0, server_fd = 0, id_hueco_actual = 0;

int main() {
	// Configura los manejadores de señales
	signal(SIGINT, signal_catch);
	signal(SIGTERM, signal_catch);

	logger = iniciar_logger("memoria.log", 1);

	char* config_path = get_config_path(MEMORIA, "memoria.config");
	config = iniciar_config(config_path);
	free(config_path);

	if (config == NULL) {
		log_error(logger, "Config es NULL");
		terminar_programa();
		return EXIT_FAILURE;
	}

	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	server_fd = iniciar_servidor(logger, NULL, puerto_escucha);
	log_info(logger, "Servidor listo para recibir clientes (fd: %i)", server_fd);

	aceptar_modulos(server_fd);

	iniciar_estructuras();

	escuchar_modulos();

    terminar_programa();
	return EXIT_SUCCESS;
}

void iniciar_estructuras() {
	huecos_libres = list_create();
	tabla_segmentos = list_create();

	int tamanio_segmento_0 = config_get_int_value(config, "TAM_SEGMENTO_0");
	int tamanio_memoria = config_get_int_value(config, "TAM_MEMORIA");
	espacio_usuario = malloc(tamanio_memoria); // Reserva un espacio de memoria de tamaño "TAM_MEMORIA"

    // Inicializa el segmento_0
	t_segmento* segmento_0 = alocar_segmento();
	*segmento_0->id = 0;
	*segmento_0->pid = -1;
	*segmento_0->tamanio = tamanio_segmento_0;
	*segmento_0->direccion_base = (intptr_t) espacio_usuario;

	// Inicializa el hueco inicial (resto de memoria despues de ocupar el Segmento 0)
	t_segmento* hueco_inicial = alocar_segmento();
	*hueco_inicial->id = id_hueco_actual;
	*hueco_inicial->tamanio = tamanio_memoria - tamanio_segmento_0;
	*hueco_inicial->direccion_base = ((intptr_t) espacio_usuario) + tamanio_segmento_0;

	log_info(logger, "segmento 0 - id: %i - tam: %i - dir: %ld", *segmento_0->id, *segmento_0->tamanio, *segmento_0->direccion_base);
	log_info(logger, "hueco inicial - id: %i - tam: %i - dir: %ld", *hueco_inicial->id, *hueco_inicial->tamanio, *hueco_inicial->direccion_base);

	id_hueco_actual++;

	list_add(tabla_segmentos, segmento_0);
	list_add(huecos_libres, hueco_inicial);

	log_info(logger, "Estructuras Iniciadas");
}

void terminar_programa() {
	// Destruye el logger y la configuración, y cierra el descriptor de archivo del servidor.
	log_destroy(logger);
	config_destroy(config);
	close(server_fd);
}

// Se invoca cuando se recibe una señal SIGINT o SIGTERM
void signal_catch() {
	log_error(logger, "Terminado a la fuerza");
	terminar_programa();
	exit(EXIT_FAILURE);
}
