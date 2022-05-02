#!/bin/bash

# directories
invoke_directory=`pwd`
script_directory=$invoke_directory/`dirname "$0"`

if [ $# -eq 0 ]; then
    benchmarks=`cat $script_directory/config.txt`
else
    benchmarks=$@
fi


benchmark_root_directory=$script_directory/..
compiler_directory=$script_directory/../../linux-sgx-patched/


date=`date +"%F_%H.%M:%S"`
result_dir=$benchmark_root_directory/result_ctime_$date

set -e
mkdir $result_dir

cat /proc/cpuinfo > $result_dir/meta_cpuinfo
cat /proc/cmdline > $result_dir/meta_cmdline
lsb_release -a > $result_dir/meta_lsbrelease
uname -r > $result_dir/meta_kernel

n_rus=1000

for bench in $benchmarks; do
    echo "Running: $bench"
    cd $benchmark_root_directory/$bench/benchmark/mbedtls/crypto
    
    benchmark_directory=$benchmark_root_directory/$bench

    export PLACEMENT_PROB=$bench

    #dry run
    make clean > /dev/null 2>&1
    make TR_CC=$compiler_directory/compiler TR_CXX=$compiler_directory/compiler compile_time 

    make clean > /dev/null 2>&1
    /usr/bin/time -f%E -o $result_dir/results_$bench.csv make TR_CC=$compiler_directory/compiler TR_CXX=$compiler_directory/compiler compile_time 
    
    cat $result_dir/results_$bench.csv
done
