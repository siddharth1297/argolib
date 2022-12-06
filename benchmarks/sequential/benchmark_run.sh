#!/bin/bash

rm *.op *.csv > /dev/null 2>&1

echo $PWD

for i in qsort
do
	./run.sh $i
done

echo "Benchmark Done"
