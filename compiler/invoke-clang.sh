#!/bin/bash
script_directory=`dirname "$0"`
$script_directory/llvm/build/bin/$1 -mllvm -fh-enable=$PLACEMENT_PROB -mllvm -fh-seed=0xdeadbeaf -mllvm -fh-handeling=abort -mllvm -fh-factor=0x2bbb871 -DFACTOR=0x2bbb871 -DINIT=0x998dbe9 ${@:2}
