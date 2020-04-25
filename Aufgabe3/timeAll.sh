#!/usr/bin/env bash

for file in ./Beispiele/*.txt
do
    echo ${file} 1.10
    python ./Prototyp/py2.py ${file} 1.10 >> /dev/null
    echo ${file} 1.15
    python ./Prototyp/py2.py ${file} 1.15 >> /dev/null
    echo ${file} 1.20
    python ./Prototyp/py2.py ${file} 1.20 >> /dev/null
    echo ${file} 1.30
    python ./Prototyp/py2.py ${file} 1.30 >> /dev/null
done
