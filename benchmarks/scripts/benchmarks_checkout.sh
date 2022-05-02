#!/bin/bash

invoke_directory=`pwd`
script_directory=$invoke_directory/`dirname "$0"`

benchmark_template_directory=$script_directory/../template

set -e

#download mbedtls
cd $benchmark_template_directory/mbedtls/crypto
git clone --branch mbedtls-2.24.0 --depth 1 https://github.com/Mbed-TLS/mbedtls.git
cd mbedtls
patch -p1 < ../mbedtls.patch

#download and patch sgxbench
cd $benchmark_template_directory
wget https://github.com/sgxbench/sgxbench/releases/download/v1.0/sgxbench.tar.gz 
tar xvfz sgxbench.tar.gz
rm sgxbench.tar.gz
cd sgxbench
patch -s -p1 < ../sgxbench.patch
cp ../postprocess.py .
chmod +x postprocess.py

#download and patch sgx-nbench
cd $benchmark_template_directory
git clone https://github.com/utds3lab/sgx-nbench.git
cd sgx-nbench
git checkout 799f0fcd32d0f0392a3d3cd5b51455c48f121488
patch -s -p1 < ../sgx-nbench.patch

