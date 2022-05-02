#!/bin/bash

# directories
script_directory=`pwd`
sgx_directory=$script_directory/../linux-sgx

set -e

cd $sgx_directory

./download_prebuilt.sh

# build unmodified sdk and install it
make sdk_install_pkg_no_mitigation

printf 'no\n/opt/intel\n' | sudo linux/installer/bin/sgx_linux_x64_sdk_2.10.100.2.bin
