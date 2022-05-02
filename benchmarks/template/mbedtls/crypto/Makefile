
INCLUDE_DIRS=-I../crypto/mbedtls/include

CFLAGS=$(INCLUDE_DIRS) -Wpedantic -Wall -O3 -march=native -std=c++17 -fPIE
LDFLAGS=

DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

MBEDTLS_CFLAGS= -O2 -fPIE
MBEDTLS_LDFLAGS= 

all:
	$(TR_CXX) $(CFLAGS) crypto.cpp $(LDFLAGS) -c -o crypto.o
	ar -x mbedtls/library/libmbedcrypto.a
	ar rcs libcrypto.a *.o
	rm *.o

clean:
	-make -C mbedtls/library clean
	-rm crypto.a

compile_time:
	for number in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19; do \
		echo here is $$number; \
		make -C mbedtls CC=$(TR_CC) CXX=$(TR_CXX) CFLAGS="$(MBEDTLS_CFLAGS)" LDFLAGS="$(MBEDTLS_LDFLAGS)" lib > /dev/null 2>&1 ; \
		make -C mbedtls/library clean > /dev/null; \
	done

.PHONY: mbedtls
mbedtls: 
	-make -C mbedtls/library clean
	make -C mbedtls CC=$(TR_CC) CXX=$(TR_CXX) CFLAGS="$(MBEDTLS_CFLAGS)" LDFLAGS="$(MBEDTLS_LDFLAGS)" lib
