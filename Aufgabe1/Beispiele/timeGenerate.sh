#!/bin/bash

rm -rf ./generated
mkdir ./generated

diffs=(10 20 30 50 75 100)

for ((s=5; s<=20; s+=5))
do
    for d in ${diffs[@]}
    do
        printf -v r "%03d" $d
        filename="./generated/${r}_${s}x${s}.txt"
        
        let min=d-1
        let max=d+1
        
        echo ${filename}
        time ../Quelltext/stromralley generate ${filename} ${s} ${min} ${max} >> /dev/null
    done
done
