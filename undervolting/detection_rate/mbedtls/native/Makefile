LIB_DIR=../../../libraries

INCLUDE_DIRS=-I$(LIB_DIR)/undervolting 
LIBRARY_DIRS=-L$(LIB_DIR)/undervolting -L../poc/mbedtls/library -L../poc

CFLAGS= $(INCLUDE_DIRS) -g -Os -Wall -march=native -std=c++17  -fno-fast-math 
LDFLAGS= $(LIBRARY_DIRS) -lundervolting -lmbedcrypto -lcrypto

COMPILER=../../../../compiler/llvm/build/bin/clang++

all: main

main: libraries main.cpp
	make -C ../poc
	$(COMPILER) -c $(CFLAGS) main.cpp -o main.o
	$(COMPILER) main.o $(LDFLAGS) -o main

libraries:
	make -C $(LIB_DIR)

clean:
	-rm main main.o

deploy:
	ssh mobile-lab07 "sudo cpupower frequency-set -d 3.0GHz -u 3.0Ghz"
	scp main mobile-lab07:~

