# TUKI

## Deployment Guide ##

1. If necessary, make the necessary configurations in PUTTY to connect the PC with the VM.

2. Open a terminal in `home/utnso/` (this is likely the base directory).


```bash
# 3. Clone the project from github
$ git clone https://github.com/sisoputnfrba/tp-2023-1c-Los-Magios.git

# 4. Access the directory of the cloned folder
$ cd tp-2023-1c-Los-Magios/

# 5. Run the script that configures the project
$ bash ./scripts/configurar_tuki.sh

# 6. Access the directory "/home/utnso/"
$ cd

# 7. Set the $LD_LIBRARY_PATH environment variable
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Los-Magios/shared_tuki/Debug

# 8. Display the variable to verify that it has been set correctly
$ echo $LD_LIBRARY_PATH

# 9. Go back to the directory of the cloned folder
$ cd tp-2023-1c-Los-Magios/

# 10. Run the script that compiles the modules
$ bash ./scripts/compilar_modulos.sh
```

11. Start the modules in the VMs in the same order they were turned on (Memory -> FileSystem -> CPU -> Kernel).


## Guide to Start Module ##

```bash
# Replace <MODULE> with the name of the folder of that module (Consola / Kernel / CPU / FileSystem / Memoria)
# If the module to start is Consola, the desired instructions should be passed as the second parameter (for example: error_1, error_2, fs_1, base_2...)
$ bash ./scripts/levantar_modulo.sh <MODULE> <INSTRUCTIONS?>
```

## Guide for Running Tests ##

```bash

# 1. Replace the IPs in the config files with the appropriate ones (default is 127.0.0.1)
$ bash ./scripts/setear_ips.sh <IP_MEMORY> <IP_FILESYSTEM> <IP_CPU>```


# 2. Configurar la prueba deseada (por ejemplo: prueba_base, prueba_deadlock, prueba_errores...)
$ bash ./scripts/configurar_prueba.sh <PRUEBA>

```
