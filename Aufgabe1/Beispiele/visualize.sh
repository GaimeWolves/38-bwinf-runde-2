#!/bin/bash

rm -rf ./generated
mkdir ./generated

diffs=(5 10 15 20 30 40 50 75 100)

for ((s=5; s<=20; s+=3))
do
    for d in ${diffs[@]}
    do
        printf -v r "%03d" $d
        filename="./generated/${r}_${s}x${s}.txt"
        
        let min=d-1
        let max=d+1
        
        ../Quelltext/stromralley generate ${filename} ${s} ${min} ${max}
    done
done

rm -rf ./visual
mkdir ./visual

for file in ./generated/*.txt
do
	fileName=${file%.txt}
	picName=${fileName##*/}.png
	python ../Prototyp/img.py ${file} ./visual/${picName}
done
