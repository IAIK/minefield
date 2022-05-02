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

# build benchmarks
for bench in $benchmarks; do
    sudo rm -r $benchmark_root_directory/$bench
done