#!/bin/bash

# directories
invoke_directory=`pwd`
script_directory=$invoke_directory/`dirname "$0"`

if [ $# -eq 0 ]; then
    benchmarks=`cat $script_directory/config.txt`
else
    benchmarks=$@
fi


benchmark_root_directory=$script_directory/../
sdk_directory=$script_directory/../../linux-sgx/
compiler_directory=$script_directory/../../linux-sgx-patched/

# build benchmarks
for bench in $benchmarks; do

    export PLACEMENT_PROB=$bench

    cd $invoke_directory

    benchmark_directory=$benchmark_root_directory/$bench
    sudo rm -r $benchmark_directory/benchmark

    # exit if any command failes ... otherwise we could nuke our own it repo
    set -e

    # copy template
    cp -R $benchmark_root_directory/template $benchmark_directory/benchmark

    # build sgx-nbench
    cd $benchmark_directory/benchmark/sgx-nbench

    make SGX_PRERELEASE=1 SGX_DEBUG=0 SGX_SDK=$benchmark_directory/sgxsdk TR_CC=$compiler_directory/compiler TR_CXX=$compiler_directory/compiler++ clean all

    # build sgxbench
    cd $benchmark_directory/benchmark/sgxbench

    autoconf
    ./configure CFLAGS='-O3' CXXFLAGS='-O3' --with-sgxsdk=$benchmark_directory/sgxsdk --with-sgx-build=prerelease
    make SGX_PRERELEASE=1 SGX_DEBUG=0 SGX_SDK=$benchmark_directory/sgxsdk TR_CC=$compiler_directory/compiler TR_CXX=$compiler_directory/compiler++ clean all

    # build mbedtls
    cd $benchmark_directory/benchmark/mbedtls/sgx
    
    make SGX_PRERELEASE=1 SGX_DEBUG=0 SGX_SDK=$benchmark_directory/sgxsdk TR_CC=$compiler_directory/compiler TR_CXX=$compiler_directory/compiler++ clean all

    set +e

done