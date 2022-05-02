#!/bin/bash

# directories
isolated_core=1
invoke_directory=`pwd`
script_directory=$invoke_directory/`dirname "$0"`

if [ $# -eq 0 ]; then
    benchmarks=`cat $script_directory/config.txt`
else
    benchmarks=$@
fi


benchmark_root_directory=$script_directory/..

date=`date +"%F_%H.%M:%S"`
result_dir=$benchmark_root_directory/result_mbedtls_$date

set -e
mkdir $result_dir

cat /proc/cpuinfo > $result_dir/meta_cpuinfo
cat /proc/cmdline > $result_dir/meta_cmdline
lsb_release -a > $result_dir/meta_lsbrelease
uname -r > $result_dir/meta_kernel

n_rus=1000

echo "Running: baseline"
cd $benchmark_root_directory/0/benchmark/mbedtls/sgx
set +e
rm results.csv
set -e


taskset -c $isolated_core ./main $n_rus 1 1 2> /dev/null
./postprocess.py | tr '\n' ' ' > $result_dir/result_base.csv

printf ";" >> $result_dir/result_base.csv

taskset -c $isolated_core ./main $n_rus 1 0 2> /dev/null
./postprocess.py >> $result_dir/result_base.csv

for bench in $benchmarks; do
    echo "Running: $bench"
    cd $benchmark_root_directory/$bench/benchmark/mbedtls/sgx
    
    set +e
    rm results.csv
    set -e
    

    taskset -c $isolated_core ./main $n_rus 0 1 2> /dev/null
    ./postprocess.py | tr '\n' ' ' > $result_dir/result_$bench.csv

    printf ";" >> $result_dir/result_$bench.csv

    taskset -c $isolated_core ./main $n_rus 0 0 2> /dev/null
    ./postprocess.py >> $result_dir/result_$bench.csv
done
