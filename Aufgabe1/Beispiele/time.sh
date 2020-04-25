#!/bin/bash

for file in ./generated/*.txt
do
    echo ${file}
	time timeout 5m ../Quelltext/stromralley solve ${file} >> /dev/null
done
