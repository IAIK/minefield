LIB_DIR=../../../libraries

INCLUDE_DIRS=-I$(LIB_DIR)/undervolting 
LIBRARY_DIRS=-L$(LIB_DIR)/undervolting 

CFLAGS= $(INCLUDE_DIRS) -g -Os -Wall -march=native -std=c++17  -fno-fast-math 
LDFLAGS= $(LIBRARY_DIRS) -lundervolting

COMPILER=../../../../compiler/llvm/build/bin/clang++

all: main

../poc/poc.o: ../poc/poc.cpp ../poc/poc.h
	make -C poc

main: libraries main.cpp ../poc/poc.o
	make -C ../poc
	$(COMPILER) -c $(CFLAGS) main.cpp -o main.o
	$(COMPILER) main.o ../poc/poc.o $(LDFLAGS) -o main

libraries:
	make -C $(LIB_DIR)

clean:
	-rm main main.o

deploy:
	ssh mobile-lab07 "sudo cpupower frequency-set -d 3.0GHz -u 3.0Ghz"
	scp main mobile-lab07:~

