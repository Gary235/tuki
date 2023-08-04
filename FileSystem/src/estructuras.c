#include "estructuras.h"

char *path_superbloque, *path_bitmap, *path_bloques;

void iniciar_estructuras() {
	fcbs = list_create();

	path_superbloque = expand_tilde(config_get_string_value(config, "PATH_SUPERBLOQUE"));
	path_bitmap = expand_tilde(config_get_string_value(config, "PATH_BITMAP"));
	path_bloques = expand_tilde(config_get_string_value(config, "PATH_BLOQUES"));

	if (!existe_fs()) {
		log_info(logger, "No existe un FileSystem previo, iniciando creacion...");

		tamanio_bloque = 64;
		cantidad_bloques = 1024;

		setear_punto_montaje(1);
		crear_superbloque();
		crear_archivo_bloques();
		crear_bitmap_bloques();

	} else {
		setear_punto_montaje(0);
		superbloque = iniciar_config(path_superbloque);

		tamanio_bloque = config_get_int_value(superbloque, "BLOCK_SIZE");
		cantidad_bloques = config_get_int_value(superbloque, "BLOCK_COUNT");
	}

	tamanio_file_system = tamanio_bloque * cantidad_bloques;

	log_info(logger, "Iniciando mapeo del FileSystem...");
	mapear_bloques();
	mapear_bitmap();
	mapear_fcbs();
}

void setear_punto_montaje(int crear) {
	log_info(logger, "Seteando Punto Montaje, se crea? %i", crear);

	char** directorios = string_split(path_superbloque, "/");

	int i = 1; // Arranca en 1, porque el primer elemento de `directorios es vacio`
	char* punto_de_montaje = string_new();
	while (directorios[i] != NULL) {
		if (strcmp(directorios[i], directorios[cantidad_elementos(directorios) - 1]) == 0) break;

		string_append(&punto_de_montaje, "/");
		string_append(&punto_de_montaje, directorios[i]);

		i++;
	}

	if (crear) mkdir(punto_de_montaje, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	string_append(&punto_de_montaje, "/fcb/");
	if (crear) mkdir(punto_de_montaje, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	base_archivo = malloc(string_length(punto_de_montaje) + 1);
	strcpy(base_archivo, punto_de_montaje);

	int j = 0;
	while (directorios[j] != NULL) {
		free(directorios[j]);
		j++;
	}

	free(directorios);
	free(punto_de_montaje);
}

bool existe_archivo(char* path) {
	FILE* archivo = fopen(path, "r");

	if (archivo == NULL) return false;

	fclose(archivo);
	return true;
}

bool existe_fs() {
	bool existe_SB = existe_archivo(path_superbloque);
	bool existe_BM = existe_archivo(path_bitmap);
	bool existe_BL = existe_archivo(path_bloques);

	return existe_SB && existe_BM && existe_BL;
}

char* expand_tilde(char* path) {
	if (path[0] != '~') return path;

	char* home = getenv("HOME");
	char* path_extendido = string_new();

	string_append(&path_extendido, home);
	string_append(&path_extendido, path + 1); // + 1 para que se el saltee "~"

	return path_extendido;
}

void crear_superbloque() {
	log_info(logger, "Creando Super Bloque");

	FILE* archivo_bloques = fopen(path_superbloque, "w");
	fclose(archivo_bloques);

	superbloque = iniciar_config(path_superbloque);

	char* parsed_tamanio_bloques = string_itoa(tamanio_bloque);
	char* parsed_cantidad_bloques = string_itoa(cantidad_bloques);
	config_set_value(superbloque, "BLOCK_SIZE", parsed_tamanio_bloques);
	config_set_value(superbloque, "BLOCK_COUNT", parsed_cantidad_bloques);

	config_save(superbloque);

	free(parsed_tamanio_bloques);
	free(parsed_cantidad_bloques);
}

void crear_archivo_bloques() {
	log_info(logger, "Creando Bloques");

	FILE* archivo_bloques = fopen(path_bloques, "w+");
    fseek(archivo_bloques, cantidad_bloques - 1, SEEK_SET);
    fwrite("", 1, 1, archivo_bloques);
	fclose(archivo_bloques);
}

void crear_bitmap_bloques() {
	log_info(logger, "Creando Bitmap");

	FILE* archivo_bitmap = fopen(path_bitmap, "w+");
    fseek(archivo_bitmap, cantidad_bloques - 1, SEEK_SET);
    fwrite("", 1, 1, archivo_bitmap);
	fclose(archivo_bitmap);
}

void mapear_bloques() {
	int bloques_fd = open(path_bloques, O_CREAT | O_RDWR, (mode_t) 0777);

	bloques_en_memoria = mmap(NULL, tamanio_file_system, PROT_WRITE | PROT_READ, MAP_SHARED, bloques_fd, 0);

	close(bloques_fd);
}

void mapear_bitmap() {
	int bitmap_fd = open(path_bitmap, O_CREAT | O_RDWR, (mode_t) 0777);

	bitmap_en_memoria = mmap(NULL, cantidad_bloques, PROT_WRITE | PROT_READ, MAP_SHARED, bitmap_fd, 0);

	int tamanio_bitmap = ceil(cantidad_bloques / 8); // 1 bit por bloque && 8 bits = 1 byte
	void* ptr_a_bitmap = malloc(tamanio_bitmap);

	memcpy(ptr_a_bitmap, bitmap_en_memoria, tamanio_bitmap);

	bitmap_bloques = bitarray_create_with_mode(ptr_a_bitmap, tamanio_bitmap, LSB_FIRST);

	close(bitmap_fd);
}

void mapear_fcbs() {
    DIR* directorio;
    struct dirent* entry;

    directorio = opendir(base_archivo);

    while((entry=readdir(directorio))) {
    	if (strcmp(entry->d_name, ".") == 0) continue;
    	if (strcmp(entry->d_name, "..") == 0) continue;

        char* path_al_archivo = string_new();
        string_append(&path_al_archivo, base_archivo);
        string_append(&path_al_archivo, entry->d_name);

        t_config* config_archivo = iniciar_config(path_al_archivo);

    	t_fcb* nuevo_fcb = alocar_fcb(string_length(entry->d_name));
    	strcpy(nuevo_fcb->nombre_archivo, config_get_string_value(config_archivo, "NOMBRE_ARCHIVO"));
    	*nuevo_fcb->puntero_directo = (uint32_t) config_get_int_value(config_archivo, "PUNTERO_DIRECTO");
    	*nuevo_fcb->puntero_indirecto = (uint32_t) config_get_int_value(config_archivo, "PUNTERO_INDIRECTO");
    	*nuevo_fcb->tamanio = config_get_int_value(config_archivo, "TAMANIO_ARCHIVO");

    	list_add(fcbs, nuevo_fcb);

    	config_destroy(config_archivo);
		free(path_al_archivo);
    }

    closedir(directorio);
}

t_fcb* alocar_fcb(int len_nombre) {
	t_fcb* fcb = malloc(sizeof(t_fcb));

	fcb->nombre_archivo = malloc(len_nombre + 1);
	fcb->puntero_directo = malloc(sizeof(uint32_t));
	fcb->puntero_indirecto = malloc(sizeof(uint32_t));
	fcb->tamanio = malloc(sizeof(int));

	return fcb;
}

void crear_fcb(char* nombre_archivo, char* path) {
	t_fcb* nuevo_fcb = alocar_fcb(string_length(nombre_archivo));

	strcpy(nuevo_fcb->nombre_archivo, nombre_archivo);
	*nuevo_fcb->puntero_directo = -1;
	*nuevo_fcb->puntero_indirecto = -1;
	*nuevo_fcb->tamanio = 0;

	list_add(fcbs, nuevo_fcb);

	FILE* archivo_fcb = fopen(path, "w");
	fclose(archivo_fcb);

	t_config* config_fcb = iniciar_config(path);

	char* parsed_tamanio = string_itoa(*nuevo_fcb->tamanio);
	char* parsed_pd = string_itoa((int) *nuevo_fcb->puntero_directo);
	char* parsed_pid = string_itoa((int) *nuevo_fcb->puntero_indirecto);

	config_set_value(config_fcb, "NOMBRE_ARCHIVO", nombre_archivo);
	config_set_value(config_fcb, "TAMANIO_ARCHIVO", parsed_tamanio);
	config_set_value(config_fcb, "PUNTERO_DIRECTO", parsed_pd);
	config_set_value(config_fcb, "PUNTERO_INDIRECTO", parsed_pid);

	config_save(config_fcb);
	config_destroy(config_fcb);

	free(parsed_tamanio);
	free(parsed_pd);
	free(parsed_pid);

	log_info(logger, "Crear Archivo: %s", nombre_archivo);
}

bool validar_bloque(uint32_t bloque) {
	return bloque > 0 && bloque <= cantidad_bloques - 1;
}

void set_proximo_puntero_fcb(t_fcb* fcb) {
	if (*fcb->puntero_directo == -1) {
		encontrar_y_ocupar_bloque(fcb, 1, -1);

		log_info(
			logger,
			"Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque File System: %i",
			fcb->nombre_archivo, 0, *fcb->puntero_directo
		);
		return;
	}

	if (*fcb->puntero_indirecto == -1) encontrar_y_ocupar_bloque(fcb, 0, -1);

	int offset = *fcb->puntero_indirecto * tamanio_bloque;
	int limite = (*fcb->puntero_indirecto + 1) * tamanio_bloque; // proximo bloque
	int num_bloque_archivo = 1; // arranca en 1, porque el 0 es el directo

	while(offset < limite) {
		uint32_t bloque_en_memoria;
		memcpy(&bloque_en_memoria, (void*) (bloques_en_memoria + offset), sizeof(uint32_t));

		if (validar_bloque(bloque_en_memoria)) {
			offset += sizeof(uint32_t);
			num_bloque_archivo++;
			continue;
		}

		log_info(
			logger,
			"Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque File System: %i",
			fcb->nombre_archivo, num_bloque_archivo, *fcb->puntero_indirecto
		);

		encontrar_y_ocupar_bloque(fcb, 0, offset);
		break;
	}
}

uint32_t get_bloque_libre() {
	uint32_t i = 0;
	while(i < cantidad_bloques - 1) {
		if (bitarray_test_bit(bitmap_bloques, i) == 0) return i;
		i++;
	}

	return -1;
}

void ocupar_bloque(uint32_t bloque, t_fcb* fcb, int usar_puntero_directo, int offset) {
	bitarray_set_bit(bitmap_bloques, bloque);
	memcpy((void*) (bitmap_en_memoria + bloque), "1", 1);
	msync(bitmap_en_memoria, cantidad_bloques, MS_SYNC);

	log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 1", bloque);

	if (offset == -1) {
		if (usar_puntero_directo) *fcb->puntero_directo = bloque;
		else *fcb->puntero_indirecto = bloque;

		char* path_al_archivo = string_new();
        string_append(&path_al_archivo, base_archivo);
        string_append(&path_al_archivo, fcb->nombre_archivo);

    	t_config* config_fcb = iniciar_config(path_al_archivo);

		char* parsed_puntero;
    	if (usar_puntero_directo) {
			parsed_puntero = string_itoa((int) *fcb->puntero_directo);
			config_set_value(config_fcb, "PUNTERO_DIRECTO", parsed_puntero);
		} else {
			parsed_puntero = string_itoa((int) *fcb->puntero_indirecto);
			config_set_value(config_fcb, "PUNTERO_INDIRECTO", parsed_puntero);
		}

    	config_save(config_fcb);
    	config_destroy(config_fcb);
		free(parsed_puntero);
		free(path_al_archivo);
	} else {
		memcpy((void*) (bloques_en_memoria + offset), &bloque, sizeof(uint32_t));
		msync(bloques_en_memoria, tamanio_file_system, MS_SYNC);
	}
}

void encontrar_y_ocupar_bloque(t_fcb* fcb, int usar_puntero_directo, int offset) {
	uint32_t bloque_libre = get_bloque_libre();
	ocupar_bloque(bloque_libre, fcb, usar_puntero_directo, offset);
}

void asignar_bloques(int cantidad_bloques, t_fcb* fcb) {
	int i = 0;
	while(i < cantidad_bloques) {
		set_proximo_puntero_fcb(fcb);
		sleep(config_get_int_value(config, "RETARDO_ACCESO_BLOQUE") / 1000);
		i++;
	}
}

void liberar_ultimo_puntero_fcb(t_fcb* fcb) {
	if (*fcb->puntero_indirecto != -1) {
		int inicio = *fcb->puntero_indirecto * tamanio_bloque;
		int offset = (*fcb->puntero_indirecto + 1) * tamanio_bloque - sizeof(uint32_t); // ultimos 4 bytes antes del fin del bloque

		while(offset >= inicio) {
			uint32_t bloque_en_memoria;
			memcpy(&bloque_en_memoria, (void*) (bloques_en_memoria + offset), sizeof(uint32_t));

			if (validar_bloque(bloque_en_memoria)) {
				offset -= sizeof(uint32_t);
				continue;
			}

			int num_bloque_archivo = (offset - inicio) / sizeof(uint32_t) + 1; // + 1 porque el arranca desde 1

			log_info(
				logger,
				"Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque File System: %i",
				fcb->nombre_archivo, num_bloque_archivo, *fcb->puntero_indirecto
			);

			liberar_bloque(fcb, bloque_en_memoria, offset == inicio, offset);
			break;
		}
	}

	if (*fcb->puntero_directo != -1) {
		log_info(
			logger,
			"Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque File System: %i",
			fcb->nombre_archivo, 0, *fcb->puntero_directo
		);

		liberar_bloque(fcb, *fcb->puntero_directo, 0, -1);
	}
}

void liberar_bloque(t_fcb* fcb, uint32_t bloque, int liberar_puntero_indirecto, int offset) {
	bitarray_clean_bit(bitmap_bloques, bloque);
	memcpy((void*) (bitmap_en_memoria + bloque), "0", 1);

	log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 0", bloque);

	msync(bitmap_en_memoria, cantidad_bloques, MS_SYNC);

	if (offset == -1) {
		*fcb->puntero_directo = -1;

		char* path_al_archivo = string_new();
        string_append(&path_al_archivo, base_archivo);
        string_append(&path_al_archivo, fcb->nombre_archivo);

    	t_config* config_fcb = iniciar_config(path_al_archivo);
		char* parsed_pd = string_itoa((int) *fcb->puntero_directo);
    	config_set_value(config_fcb, "PUNTERO_DIRECTO", parsed_pd);
    	config_save(config_fcb);
    	config_destroy(config_fcb);
		
		free(parsed_pd);
		free(path_al_archivo);
	} else {
		memset((void*) (bloques_en_memoria + offset), '\0', sizeof(uint32_t));

		msync(bloques_en_memoria, tamanio_file_system, MS_SYNC);

		if (liberar_puntero_indirecto) {
			bitarray_clean_bit(bitmap_bloques, *fcb->puntero_indirecto);
			memcpy((void*) (bitmap_en_memoria + *fcb->puntero_indirecto), "0", 1);

			log_info(logger, "Acceso a Bitmap - Bloque: %i - Estado: 0", *fcb->puntero_indirecto);

			msync(bitmap_en_memoria, cantidad_bloques, MS_SYNC);

			*fcb->puntero_indirecto = -1;

			char* path_al_archivo = string_new();
	        string_append(&path_al_archivo, base_archivo);
	        string_append(&path_al_archivo, fcb->nombre_archivo);

	    	t_config* config_fcb = iniciar_config(path_al_archivo);
			char* parsed_pid = string_itoa((int) *fcb->puntero_indirecto);
	    	config_set_value(config_fcb, "PUNTERO_INDIRECTO", parsed_pid);
	    	config_save(config_fcb);
	    	config_destroy(config_fcb);

			free(parsed_pid);
			free(path_al_archivo);
		}
	}
}

void desasignar_bloques(int cantidad_bloques, t_fcb* fcb) {
	int i = 0;
	while(i < cantidad_bloques) {
		liberar_ultimo_puntero_fcb(fcb);
		sleep(config_get_int_value(config, "RETARDO_ACCESO_BLOQUE") / 1000);
		i++;
	}
}

uint32_t get_bloque_por_indice(t_fcb* fcb, int indice) {
	if (indice == 0) return *fcb->puntero_directo;

	int offset = *fcb->puntero_indirecto * tamanio_bloque;
	int limite = (*fcb->puntero_indirecto + 1) * tamanio_bloque; // proximo bloque
	int num_bloque_archivo = 1; // arranca en 1, porque el 0 es el directo

	while(offset < limite) {
		uint32_t bloque_en_memoria;
		memcpy(&bloque_en_memoria, (void*) (bloques_en_memoria + offset), sizeof(uint32_t));

		if (validar_bloque(bloque_en_memoria) && num_bloque_archivo != indice) {
			offset += sizeof(uint32_t);
			num_bloque_archivo++;
			continue;
		}

		return bloque_en_memoria;
	}

	return -1;
}

void get_info_de_bloque(t_fcb* fcb, int indice, int offset_bloque, int tamanio, char* buffer) {
	uint32_t bloque = get_bloque_por_indice(fcb, indice);
	int puntero_a_info = bloque * tamanio_bloque + offset_bloque;

	log_info(
		logger,
		"Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque File System: %i",
		fcb->nombre_archivo, indice, bloque
	);

	memcpy(buffer, (void*) (bloques_en_memoria + puntero_a_info), tamanio);

	sleep(config_get_int_value(config, "RETARDO_ACCESO_BLOQUE") / 1000);
}

void set_info_en_bloque(t_fcb* fcb, int indice, int offset_bloque, int tamanio, char* buffer) {
	uint32_t bloque = get_bloque_por_indice(fcb, indice);
	int puntero_a_info = bloque * tamanio_bloque + offset_bloque;

	log_info(
		logger,
		"Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque File System: %i",
		fcb->nombre_archivo, indice, bloque
	);

	memcpy((void*) (bloques_en_memoria + puntero_a_info), buffer, tamanio);

	sleep(config_get_int_value(config, "RETARDO_ACCESO_BLOQUE") / 1000);
}
