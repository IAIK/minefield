#!/bin/bash
set -e
git clone --branch llvmorg-11.0.0 --depth 1 https://github.com/llvm/llvm-project.git llvm
cd llvm
mkdir build 
patch -p1 < ../minefield.patch 
cd build
cmake ../llvm -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS=clang -DENABLE_EXPERIMENTAL_NEW_PASS_MANAGER=true
ninja clang -j`nproc`
