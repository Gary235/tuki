#include "comunicacion.h"


void recibir_proceso(int cliente_fd) {
	log_info(logger, "\n");
	log_info(logger, "Me llego un pcb");

	t_pcb* pcb = deserializar_pcb(cliente_fd, NULL, NULL, 0);
	t_interrupcion_cpu* interrupcion = ejecutar_instrucciones(pcb);

	log_info(logger, "Sale con interrupcion: %s", get_nombre_por_codigo(*interrupcion->codigo));

	enviar_interrupcion(cliente_fd, interrupcion, CPU);
}

void recibir_kernel(char *puerto) {
	int* cliente_fd = esperar_cliente(server_fd, logger);

	recibir_operacion(*cliente_fd); // recibe modulo
	recibir_operacion(*cliente_fd); // recibe cod_op
	recibir_mensaje(*cliente_fd, logger, 1);
	enviar_mensaje(OK, *cliente_fd, CPU, MENSAJE);

	bool should_continue = true;
	while (should_continue) {
		recibir_operacion(*cliente_fd); // recibe modulo
		int cod_op = recibir_operacion(*cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(*cliente_fd, logger, 1);
				break;
			case PAQUETE:
				recibir_proceso(*cliente_fd);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				should_continue = false;
				break;
			default:
				log_warning(logger, "Operacion desconocida. Trato de leer lista");
				break;
		}
	}

	free(cliente_fd);
}
