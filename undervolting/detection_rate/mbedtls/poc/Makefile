PLACEMENT_PROB?=100

INCLUDE_DIRS=-Imbedtls/include

CFLAGS= $(INCLUDE_DIRS) -g -Os -Wall -march=native -std=c++17 -fno-fast-math -fPIE -fPIC -DFACTOR=0xFbbb8712bbb871 -DINIT=0xFbbb8712bbb871
MFLAGS= -mllvm -fh-enable=$(PLACEMENT_PROB) -mllvm -fh-seed=0xdeadbeaf -mllvm -fh-handeling=sticky -mllvm -fh-factor=0xFbbb8712bbb871  -fPIE -fPIC 
MLDFLAGS=
DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
COMPILER=$(DIR)/../../../../compiler/llvm/build/bin/clang

all: mbedtls
	$(COMPILER)++ $(CFLAGS) poc.cpp $(MLDFLAGS) -c -o poc.o
	ar -x mbedtls/library/libmbedcrypto.a
	ar rcs libcrypto.a *.o
	rm *.o

clean:
	-make -C mbedtls/library clean
	-rm libcrypto.a

.PHONY: mbedtls
mbedtls: 
	-make -C mbedtls/library clean
	make -C mbedtls CC=$(COMPILER) CXX=$(COMPILER)++ CFLAGS="$(MFLAGS)" DEBUG=1 LDFLAGS="$(MLDFLAGS)" lib
