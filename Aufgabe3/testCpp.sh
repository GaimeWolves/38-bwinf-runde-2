#!/usr/bin/env bash

for file in ./Beispiele/*.txt
do
	time ./Quelltext/app ${file} 1.10 >> /dev/null
	time ./Quelltext/app ${file} 1.15 >> /dev/null
	time ./Quelltext/app ${file} 1.20 >> /dev/null
	time ./Quelltext/app ${file} 1.30 >> /dev/null
	time ./Quelltext/app ${file} 1000 >> /dev/null # "Unendlich Prozent"
done
