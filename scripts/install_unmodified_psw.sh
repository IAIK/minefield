#!/bin/bash

# directories
script_directory=`pwd`
sgx_directory=$script_directory/../linux-sgx

set -e

cd $sgx_directory

#./download_prebuilt.sh

source /opt/intel/sgxsdk/environment
make deb_local_repo

# install new psw, this part is manual

echo "add the following to /etc/apt/sources.list"
echo "deb [trusted=yes arch=amd64] file:$sgx_directory/linux/installer/deb/sgx_debian_local_repo bionic main"
echo ""
echo "change bionic to match your ubuntu release:"
echo "ubuntu 20.04: focal"
echo "ubuntu 18.04: bionic"
echo ""
echo "then run:"
echo "sudo apt update"
echo "sudo apt install sgx-aesm-service '^libsgx_*'"


