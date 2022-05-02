#!/bin/bash

# directories
invoke_directory=`pwd`
script_directory=$invoke_directory/`dirname "$0"`


if [ $# -eq 0 ]; then
    echo "specify result directory" 
    exit -1
fi

result_dir=$@
result_csv=$result_dir/results.csv

cd $result_dir
result_files=`find . -name "results_*.csv" | tr '\n' ' '`
cd ..

echo "NAME;TIME" > $result_csv

for result_file in $result_files; do
    bench_name=`echo $result_file | sed 's/.\/results_\(.*\).csv/\1/'`
    echo $bench_name

    printf "$bench_name;0:" >> $result_csv
    cat $result_dir/$result_file >> $result_csv
    #printf "\n" >> $result_csv
done
