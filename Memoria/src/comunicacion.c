#include "comunicacion.h"

void aceptar_modulos() {
	bool modulos_sin_aceptar = true;
	while (modulos_sin_aceptar) {
		log_info(logger, "\n");

		int* cliente_fd = esperar_cliente(server_fd, logger);

		int modulo = recibir_operacion(*cliente_fd);
		log_info(logger, "Se conecto %s", get_nombre_por_codigo(modulo));

		recibir_operacion(*cliente_fd); // recibe cod_op
		recibir_mensaje(*cliente_fd, logger, 1);

		if (modulo == KERNEL) conexion_kernel = *cliente_fd;
		if (modulo == CPU) conexion_cpu = *cliente_fd;
		if (modulo == FILE_SYSTEM) conexion_fs = *cliente_fd;

		if (modulo == KERNEL) {
			enviar_mensaje(config_get_string_value(config, "CANT_SEGMENTOS"), *cliente_fd, MEMORIA, MENSAJE);
		} else {
			enviar_mensaje(OK, *cliente_fd, MEMORIA, MENSAJE);
		}

		modulos_sin_aceptar = conexion_kernel * conexion_fs * conexion_cpu == 0;
		free(cliente_fd);
	}
}

void escuchar_modulos() {
	int CANTIDAD_CONEXIONES = 3;

	struct pollfd sockets_fds[CANTIDAD_CONEXIONES];
	sockets_fds[0].fd = conexion_kernel;
	sockets_fds[0].events = POLLIN;

	sockets_fds[1].fd = conexion_cpu;
	sockets_fds[1].events = POLLIN;

	sockets_fds[2].fd = conexion_fs;
	sockets_fds[2].events = POLLIN;

	log_info(logger, "Escuchando Modulos");

	while(true) {
		int respuesta = poll(sockets_fds, CANTIDAD_CONEXIONES, -1);

		if (respuesta == -1) {
			log_error(logger, "No se pudo escuchar a los modulos");
			break;
		}

		if (sockets_fds[0].revents & POLLIN) handle_comunicacion(sockets_fds[0].fd);
		if (sockets_fds[1].revents & POLLIN) handle_comunicacion(sockets_fds[1].fd);
		if (sockets_fds[2].revents & POLLIN) handle_comunicacion(sockets_fds[2].fd);
	}

}

void handle_comunicacion(int cliente_fd) {
	int modulo_origen = recibir_operacion(cliente_fd);
	int operacion = recibir_operacion(cliente_fd);

	loggear_codigo(logger, operacion);
	switch(modulo_origen) {
		case KERNEL:
			handle_kernel(operacion);
			break;
		case CPU:
			handle_cpu(operacion);
			break;
		case FILE_SYSTEM:
			handle_fs(operacion);
			break;
		default:
			break;
	}
}
