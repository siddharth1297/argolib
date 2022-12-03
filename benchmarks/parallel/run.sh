#!/bin/bash

USAGE="./run {executable}"

if [ "$#" -ne 1 ]; then
    echo "Error: Expecting 1 argument, but given $#"
    echo "Usage: $USAGE"
    exit 1
fi

rm *.csv
rm *.op

APP=$1

echo "App: " $APP
THREADS=4

#for i in {1..5}
#do
#	ARGOLIB_WORKERS=$THREADS ./$APP > $APP_op_$i.op
#done

for i in {1..5}
do
	FILE_NAME=$APP_op_$i.op
	echo "Op file: " $FILE_NAME
	ARGOLIB_WORKERS=$THREADS ./$APP > $FILE_NAME
done


OP_FILE=$APP_task.csv

echo "TaskCnt dumping in " $OP_FILE
touch $OP_FILE
echo "threads,ratio" > $OP_FILE

for i in {1..5}
do
	grep ARGOLIB_TOTPOOLCNT $_op_$i.op | awk '{print $3","$9}' >> $OP_FILE
done

python3 task_plot.py $APP_task.csv $APP
