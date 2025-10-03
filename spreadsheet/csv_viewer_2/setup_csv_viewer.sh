#!/bin/bash
mkdir -p csv_viewer/src
mkdir -p csv_viewer/bin
cd csv_viewer || exit

# Si quieres, aquí puedes crear los archivos automáticamente
# o simplemente copiar los archivos .c, .h y Makefile a la carpeta src/

make clean
make

echo "Proyecto listo. Ejecuta: ./bin/csv_viewer ejemplo.csv"
