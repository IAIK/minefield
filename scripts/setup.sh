#!/bin/bash

# directories
script_directory=`pwd`
sgx_directory=$script_directory/../linux-sgx

#uninstall sdk/psw
$script_directory/uninstall_psw_sdk.sh

echo "DONE uninstalling default sdk/psw"

set -e

#checkout sdk/psw
$script_directory/checkout_and_patch.sh

echo "DONE checking out and patching sdk/psw"

#install unmodified sdk to /opt/intel
$script_directory/install_unmodified_sdk.sh

echo "DONE installing unmodified sdk"

#install modified PSW
$script_directory/install_unmodified_psw.sh
echo "DONE installing unmodified psw"

