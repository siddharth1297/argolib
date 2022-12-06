#!/bin/bash

echo "Put paths inside parallel/export.env"

CWD=$PWD

cd "${CWD}/sequential/"
./benchmark_run.sh


cd "${CWD}/parallel/"
./benchmark_run.sh

cd $CWD

cp sequential/*_seq_latency.csv .
cp parallel/*_par_latency.csv .
cp parallel/*_task.csv .

for i in fib qsort iterative array_sum
do
        python3 latency_graph.py "${i}_seq_latency.csv" "${i}_par_latency.csv" "${i}_lat.jpeg"
done


for i in fib qsort iterative array_sum
do
	python3 task_plot.py "${i}_task.csv" "${i}_task.jpeg"
done
