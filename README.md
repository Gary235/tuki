# tp-2023-1c-Los-Magios

Gary Ezequiel Berkman - 2034104 - K3153

Roman Brezman - 2034323 - K3053

Federico Ascorti - 2033823 - K3014

Tamir Feldenblum - 2036022 - K3053

## Guia para Deploy ##

1. Si es necesario, hacer las configuraciones necesarias en PUTTY para conectar la pc con la vm.

2. Abrir una terminal en `home/utnso/` (probablemente sea la direccion base).


```bash
# 3. Clonar el proyecto desde github
$ git clone https://github.com/sisoputnfrba/tp-2023-1c-Los-Magios.git

# 4. Acceder directorio de la carpeta clonada
$ cd tp-2023-1c-Los-Magios/

# 5. Correr script que configura el proyecto
$ bash ./scripts/configurar_tuki.sh

# 6. Acceder al directorio "/home/utnso/"
$ cd

# 7. Setear la variable de entorno $LD_LIBRARY_PATH
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Los-Magios/shared_tuki/Debug

# 8. Mostrar la variable para verificar que se haya seteado correctamente
$ echo $LD_LIBRARY_PATH

# 9. Volver al directorio de la carpeta clonada
$ cd tp-2023-1c-Los-Magios/

# 10. Correr script que compila los modulos
$ bash ./scripts/compilar_modulos.sh
```

11. Levantar los módulos en las VMs en el mismo orden que se prendieron las mismas (Memoria -> FileSystem -> CPU -> Kernel).


## Guia para Levantar Modulo ##

```bash
# Reemplazar <MODULO> con el nombre de la carpeta de tal modulo (Consola / Kernel / CPU / FileSystem / Memoria)
# Si el modulo a levantar es Consola, se debera mandar como segundo parametro las intrucciones deseadas (por ejemplo: error_1, error_2, fs_1, base_2...)
$ bash ./scripts/levantar_modulo.sh <MODULO> <INSTRUCCIONES?>
```

## Guia para correr las Pruebas ##

```bash

# 1. Reemplazar las ips de los archivos config con las que corresponda (por default es 127.0.0.1)
$ bash ./scripts/setear_ips.sh <IP_MEMORIA> <IP_FILESYSTEM> <IP_CPU>

# 2. Configurar la prueba deseada (por ejemplo: prueba_base, prueba_deadlock, prueba_errores...)
$ bash ./scripts/configurar_prueba.sh <PRUEBA>

```
