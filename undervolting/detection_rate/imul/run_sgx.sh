#!/bin/bash
if [ $# -ne 4 ] ; then
    echo "Incorrect number of arguments" >&2
    echo "Usage: $0 <starting_number e.g. 100> <rounds e.g. 20> <core id from 0> <placement_density>" >&2
    exit 1
fi

set -e
make PLACEMENT_PROB=$4 clean all

sudo modprobe msr
sudo cpupower -c all frequency-set -d 1.0GHz -u 1.0GHz > /dev/null

cd sgx

for (( i=$1;i<500;i++ ))
do
  echo "Testing -${i}mV for $2 rounds..."
  sudo taskset -c $3 ./main $2 -$i $3
  R=$?
  if [ $R -eq 255 ]; then
    echo "aborted"
    exit 1
  fi
	sleep 1
done

sudo cpupower -c all frequency-set -d 1.0GHz -u 4.0GHz
