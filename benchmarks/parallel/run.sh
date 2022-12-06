#!/bin/bash

ulimit -c unlimited

USAGE="./run {executable}"

if [ "$#" -ne 1 ]; then
    echo "Error: Expecting 1 argument, but given $#"
    echo "Usage: $USAGE"
    exit 1
fi

#rm *.csv *.op > /dev/null 2>&1

APP=$1

echo "App: " $APP
THREADS=20

for i in {1..5}
do
	FILE="${APP}_par_latency_${i}.op"
	touch $FILE
	ARGOLIB_WORKERS=$THREADS ./$APP > $FILE 2>&1
	echo $FILE
	sleep 3
done

LAT_FILE="${APP}_par_latency.csv"

touch $LAT_FILE

echo "latency" > $LAT_FILE

for i in {1..5}
do
	FILE="${APP}_par_latency_${i}.op"
        grep ARGOLIB_ELAPSEDTIME $FILE | awk '{print $2}' >> $LAT_FILE
	echo "Collected from $FILE"
done

## Task Count

cnt=1
for i in 2 4 8 16 20
#for i in 1 2 3 4
do
	FILE="${APP}_task_${cnt}.op"
	touch $FILE
	ARGOLIB_WORKERS=$i ./$APP >> $FILE
	cnt=$((cnt+1))
	sleep 3
done


OP_FILE="${APP}_task.csv"

echo "TaskCnt dumping in " $OP_FILE
touch $OP_FILE
echo "threads,ratio" > $OP_FILE

for i in {1..5}
do
	FILE="${APP}_task_${i}.op"
	grep ARGOLIB_TOTPOOLCNT $FILE | awk '{print $3","$9}' >> $OP_FILE
	echo "Collected from $FILE"
done
