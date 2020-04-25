#!/usr/bin/env bash

echo '"Datei","max. Faktor","Kurven","Dist.","min. Dist.","Faktor","Pfad"' > ./Visualisierungen/bsp.csv
#echo '"Datei","max. Faktor","Kurven","Dist.","min. Dist.","Faktor"' > ./Visualisierungen/bsp.csv

for file in ./Beispiele/*.txt
do
    echo '"","","","","","",""' >> ./Visualisierungen/bsp.csv
    #echo '"","","","","",""' >> ./Visualisierungen/bsp.csv
    python ./Prototyp/py2.py ${file} 1.10 >> ./Visualisierungen/bsp.csv
    python ./Prototyp/py2.py ${file} 1.15 >> ./Visualisierungen/bsp.csv
    python ./Prototyp/py2.py ${file} 1.20 >> ./Visualisierungen/bsp.csv
    python ./Prototyp/py2.py ${file} 1.30 >> ./Visualisierungen/bsp.csv
done

python ./Prototyp/printer.py ./Visualisierungen/bsp.csv > ./Visualisierungen/table.txt
