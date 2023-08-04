echo Comienza a Compilar Modulos
sleep 1

echo Compilando: Shared Tuki
cd shared_tuki/Debug
make clean
make all
cd ../..
sleep 2

echo Compilando: Memoria
cd Memoria/Debug
make clean
make all
cd ../..
sleep 2

echo Compilando: FileSystem
cd FileSystem/Debug
make clean
make all
cd ../..
sleep 2

echo Compilando: CPU
cd CPU/Debug
make clean
make all
cd ../..
sleep 2

echo Compilando: Kernel
cd Kernel/Debug
make clean
make all
cd ../..
sleep 2

echo Compilando: Consola
cd Consola/Debug
make clean
make all
cd ../..
sleep 2

echo Finalizó la compilación de los ḿodulos con compilar_modulos.sh
