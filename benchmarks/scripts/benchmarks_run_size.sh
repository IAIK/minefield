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

echo "NAME;NBENCH IMULS;NBENCH SIZE;SGXBENCH IMULS;SGXBENCH SIZE;MBEDTLS IMULS;MBEDTLS SIZE;TRTS IMULS;TRTS SIZE;TSTDC IMULS;TSDTC SIZE;TCXX IMULS;TCXX SIZE" > $benchmark_root_directory/meta.csv

for bench in $benchmarks; do    
    echo "$bench:"
    cd $benchmark_root_directory/$bench/benchmark/sgx-nbench/
    n_imul=`objdump Enclave.so -dC | grep "imul" | wc -l` 
    size=`wc -c <  Enclave.so` 
    printf "$bench;$n_imul;$size;" >> $benchmark_root_directory/meta.csv

    cd $benchmark_root_directory/$bench/benchmark/sgxbench/Enclave
    n_imul=`objdump Enclave.so -dC | grep "imul" | wc -l` 
    size=`wc -c <  Enclave.so` 
    printf "$n_imul;$size;"  >> $benchmark_root_directory/meta.csv

    cd $benchmark_root_directory/$bench/benchmark/mbedtls/sgx/enclave
    n_imul=`objdump enclave.so -dC | grep "imul" | wc -l`
    size=`wc -c <  enclave.so` 
    printf "$n_imul;$size;" >> $benchmark_root_directory/meta.csv

    cd $benchmark_root_directory/$bench/sgxsdk/lib64/
    n_imul=`objdump libsgx_trts.a -dC | grep "imul" | wc -l` 
    size=`wc -c <  libsgx_trts.a` 
    printf "$n_imul;$size;" >> $benchmark_root_directory/meta.csv

    cd $benchmark_root_directory/$bench/sgxsdk/lib64/
    n_imul=`objdump libsgx_tstdc.a -dC | grep "imul" | wc -l` 
    size=`wc -c <  libsgx_tstdc.a` 
    printf "$n_imul;$size;" >> $benchmark_root_directory/meta.csv

    cd $benchmark_root_directory/$bench/sgxsdk/lib64/
    n_imul=`objdump libsgx_tcxx.a -dC | grep "imul" | wc -l` 
    size=`wc -c <  libsgx_tcxx.a` 
    printf "$n_imul;$size" >> $benchmark_root_directory/meta.csv

    echo "" >> $benchmark_root_directory/meta.csv

done
