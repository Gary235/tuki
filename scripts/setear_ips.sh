IP_MEMORIA=$1
IP_FILESYSTEM=$2
IP_CPU=$3

for folder in archivos_config/*
do
  for file in $folder/*
  do
    sed -i "s/IP_MEMORIA=127.0.0.1/IP_MEMORIA=$IP_MEMORIA/g" $file
    sed -i "s/IP_FILESYSTEM=127.0.0.1/IP_FILESYSTEM=$IP_FILESYSTEM/g" $file
    sed -i "s/IP_CPU=127.0.0.1/IP_CPU=$IP_CPU/g" $file
  done

  echo "IPS seteadas para: $folder"
done
