#!/bin/bash

source export.env

ulimit -c unlimited
echo $PWD

rm *.op *.csv > /dev/null 2>&1
for i in fib qsort
do
	./run.sh $i
done

echo "Benchmark Done"
