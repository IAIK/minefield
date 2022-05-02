#!/bin/bash

result=`sudo lsmod | grep isgx -c`

if [ $result -eq 0 ]; then
echo "SGX Driver is not loaded! please load the driver and rerun"
exit -1
fi

sudo /opt/intel/sgx-aesm-service/startup.sh

echo "SGX is available"
