MODULO=$1
INSTRUCCIONES=$2

echo "Levantando: $MODULO"
echo $HOME

PATH_AL_EJECUTABLE="$HOME/tp-2023-1c-Los-Magios/$MODULO/Debug/$MODULO"

if [[ $MODULO == "Consola" ]]; then
    $PATH_AL_EJECUTABLE consola.config $INSTRUCCIONES.txt
fi


if [[ $MODULO != "Consola" ]]; then
    $PATH_AL_EJECUTABLE
fi
