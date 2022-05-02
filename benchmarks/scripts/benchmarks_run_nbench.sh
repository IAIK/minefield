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
result_dir=$benchmark_root_directory/result_nbench_$date

set -e
mkdir $result_dir

cat /proc/cpuinfo > $result_dir/meta_cpuinfo
cat /proc/cmdline > $result_dir/meta_cmdline
lsb_release -a > $result_dir/meta_lsbrelease
uname -r > $result_dir/meta_kernel

for bench in $benchmarks; do
    echo "Running: $bench"
    cd $benchmark_root_directory/$bench/benchmark/sgx-nbench/
    taskset -c $isolated_core ./main | tee $result_dir/result_$bench.log
done
