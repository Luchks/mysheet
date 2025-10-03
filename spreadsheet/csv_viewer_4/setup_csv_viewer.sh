#!/bin/bash
mkdir -p bin
make clean
make
echo "Proyecto listo. Ejecuta: ./bin/csv_viewer ejemplo.csv"
