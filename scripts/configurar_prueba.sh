PRUEBA=$1

for folder in archivos_config/*
do
  if [[
    $folder != "archivos_config/consola" &&
    $folder != "archivos_config/instrucciones"
  ]]; then

    cat "$folder/${folder#*/}.$PRUEBA.config" > "$folder/${folder##*/}.config"

    echo "prueba seteada para: $folder"
  fi
done
