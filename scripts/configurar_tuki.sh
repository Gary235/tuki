echo Inicia a Configurar TUKI
sleep 1

cd ~/
echo Paso 1: Clonación de repo so-commons-library
sleep 2
git clone https://github.com/sisoputnfrba/so-commons-library/

echo Paso 2: Instalación de commons 
sleep 2
cd so-commons-library
sudo make install

echo Paso 3: Posicionarse en carpeta de nuestro TP 
cd /home/utnso/tp-2023-1c-Los-Magios/
sleep 1

echo Paso 4: Reconocimiento de shared_tuki
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utnso/tp-2023-1c-Los-Magios/shared_tuki/Debug
cd shared_tuki/Debug
sleep 1

echo Paso 5: Compilación de shared_tuki
make clean
make all
sleep 2

echo Paso 6: Copiar headers a usr
sudo cp -u libshared_tuki.so /usr/lib
cd ../src/
sudo cp --parents -u shared_tuki.h /usr/include
sleep 2

echo Paso 7: Volver a carpeta del TP
cd /home/utnso/tp-2023-1c-Los-Magios

echo Finaliza configurar_tuki.sh
