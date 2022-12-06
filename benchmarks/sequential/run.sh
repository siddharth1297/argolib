#!/bin/bash

USAGE="./run {executable}"

if [ "$#" -ne 1 ]; then
    echo "Error: Expecting 1 argument, but given $#"
    echo "Usage: $USAGE"
    exit 1
fi

#rm *.csv *.op > /dev/null 2>&1

APP=$1

echo "App: " $APP
THREADS=4

for i in {1..5}
do
	FILE="${APP}_seq_latency_${i}.op"
	ARGOLIB_WORKERS=$THREADS ./$APP > $FILE 2>&1
	echo $FILE
done

LAT_FILE="${APP}_seq_latency.csv"

touch $LAT_FILE

echo "latency" > $LAT_FILE

for i in {1..5}
do
	FILE="${APP}_seq_latency_${i}.op"
	grep "Time" $FILE | awk '{print $(NF-1)}' >> $LAT_FILE
        echo "Collected from $FILE"
done

#python3 task_plot.py $APP_task.csv $APP
