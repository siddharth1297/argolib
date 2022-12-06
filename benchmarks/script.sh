
CWD=$PWD

cd "${CWD}/sequential/"
./benchmark_run.sh


cd "${CWD}/parallel/"
./benchmark_run.sh

cd $CWD

for i in fib qsort iterative array_sum
do
	python3 latency_graph.py "sequential/${i}_seq_latency.csv" "parallel/${i}_par_latency.csv" "${i}_lat.jpeg"
done

#for i in fib qsort iterative array_sum
#do
#        python3 latency_graph.py "sequential/${i}_seq_latency.csv" "parallel/${i}_par_latency.csv" "${i}_lat.jpeg"
#done
