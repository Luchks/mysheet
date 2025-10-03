#!/bin/bash
mkdir -p csv_viewer/bin
cd csv_viewer || exit

make clean
make

echo "Proyecto listo. Ejecuta: ./bin/csv_viewer ejemplo.csv"
