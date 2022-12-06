#!/bin/bash

rm *.op *.csv > /dev/null 2>&1

echo $PWD

for i in fib qsort array_sum iterative  
do
	./run.sh $i
done

echo "Benchmark Done"
