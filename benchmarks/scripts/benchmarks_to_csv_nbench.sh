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
result_files=`find . -name "result_*.log" | tr '\n' ' '`
cd ..
#FP EMULATION;FP EMULATION SD;
echo "NAME;NUMERIC SORT;STD NUMERIC SORT;STRING SORT;STD STRING SORT;BITFIELD;STD BITFIELD;FOURIER;STD FOURIER;ASSIGNMENT;STD ASSIGNMENT;IDEA;STD IDEA;HUFFMAN;STD HUFFMAN;NEURAL NET;STD NEURAL NET;LU DECOMPOSITION;STD LU DECOMPOSITION" > $result_csv

for result_file in $result_files; do
    bench_name=`echo $result_file | sed 's/.\/result_\(.*\).log/\1/'`

    printf "$bench_name; " >> $result_csv
    cat $result_dir/$result_file | tail -n +4 | head -n -1 | tr -s ' ' | cut -d":" -f2-3 | tr ':' ';' | paste -sd ";" | tr -d '\n'  >> $result_csv
    echo "" >> $result_csv
done
