#!/bin/sh
cd ../
git clone https://github.com/intel/linux-sgx.git linux-sgx
git clone https://github.com/intel/linux-sgx.git linux-sgx-patched

# unpatched
cd linux-sgx
git checkout 60d36e0de7055e8edd2fe68693b3c39f3f10fd3c
cd ../

# patched
cd linux-sgx-patched
git checkout 60d36e0de7055e8edd2fe68693b3c39f3f10fd3c
ln -s ../compiler/clang compiler
ln -s ../compiler/clang++ compiler++

patch -p1 < ../scripts/sgx-sdk.patch
