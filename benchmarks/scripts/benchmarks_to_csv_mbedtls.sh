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
result_files=`find . -name "result_*.csv" | tr '\n' ' '`
cd ..

echo "NAME;TIME LONG;STD LONG;N LONG;TIME SHORT;STD SHORT;N SHORT" > $result_csv

for result_file in $result_files; do
    bench_name=`echo $result_file | sed 's/.\/result_\(.*\).csv/\1/'`
    echo $bench_name

    printf "$bench_name;" >> $result_csv
    cat $result_dir/$result_file >> $result_csv
    #printf "\n" >> $result_csv
done
