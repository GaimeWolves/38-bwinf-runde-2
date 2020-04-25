#!/usr/bin/env bash

mkdir -p ./Visualisierungen

for file in ./Beispiele/*.txt
do
    fileName=${file%.txt}
    picName=${fileName##*/}.png
    python ./Prototyp/img.py ${file} ./Visualisierungen/${picName}
done