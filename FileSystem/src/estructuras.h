
#ifndef FS_ESTRUCTURAS_H_
#define FS_ESTRUCTURAS_H_

#include <shared_tuki.h>

#include "file_system.h"

void iniciar_estructuras();

void setear_punto_montaje(int crear);

bool existe_archivo(char* path);
bool existe_fs();

char* expand_tilde(char* path);
void crear_superbloque();
void crear_archivo_bloques();
void crear_bitmap_bloques();

void mapear_bloques();
void mapear_bitmap();
void mapear_fcbs();

t_fcb* alocar_fcb(int len_nombre);
void crear_fcb(char* nombre_archivo, char* path);

void set_proximo_puntero_fcb(t_fcb* fcb);
uint32_t get_bloque_libre();
void ocupar_bloque(uint32_t bloque, t_fcb* fcb, int usar_puntero_directo, int offset);
void encontrar_y_ocupar_bloque(t_fcb* fcb, int usar_puntero_directo, int offset);
void asignar_bloques(int cantidad_bloques, t_fcb* fcb);

void liberar_ultimo_puntero_fcb(t_fcb* fcb);
void liberar_bloque(t_fcb* fcb, uint32_t bloque, int liberar_puntero_indirecto, int offset);
void desasignar_bloques(int cantidad_bloques, t_fcb* fcb);

uint32_t get_bloque_por_indice(t_fcb* fcb, int indice);
void get_info_de_bloque(t_fcb* fcb, int indice, int offset_bloque, int tamanio, char* buffer);
void set_info_en_bloque(t_fcb* fcb, int indice, int offset_bloque, int tamanio, char* buffer);

extern char *path_superbloque, *path_bitmap, *path_bloques;

#endif /* FS_ESTRUCTURAS_H_ */
