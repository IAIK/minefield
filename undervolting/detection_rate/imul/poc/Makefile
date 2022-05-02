PLACEMENT_PROB?=100

CFLAGS= $(INCLUDE_DIRS) -g -Os -Wall -march=native -std=c++17 -fno-fast-math -fPIE -fPIC
MFLAGS= -mllvm -fh-enable=$(PLACEMENT_PROB) -mllvm -fh-seed=0xdeadbeaf -mllvm -fh-handeling=sticky -mllvm -fh-factor=0xFbbb8712bbb871 -DFACTOR=0xFbbb8712bbb871 -DINIT=0xFbbb8712bbb871

COMPILER=../../../../compiler/llvm/build/bin/clang++

all: poc.o

poc.o: poc.cpp poc.h
	$(COMPILER) -c $(CFLAGS) $(MFLAGS) poc.cpp -o poc.o

clean:
	-rm poc.o