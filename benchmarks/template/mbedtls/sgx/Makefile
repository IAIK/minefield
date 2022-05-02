INCLUDE_DIRS=-I$(SGX_SDK)/include -Ienclave
LIBRARY_DIRS=-Lenclave

CFLAGS= $(INCLUDE_DIRS) -Wpedantic -Wall -O3 -march=native -std=c++17 

LDFLAGS= $(LIBRARY_DIRS) -lenclave_proxy -lsgx_urts -lm -lsgx_uae_service -pthread -L$(SGX_SDK)/lib64/ 

all: crypto
	make -C enclave CC=$(TR_CC) all
	g++ $(CFLAGS) main.cpp enclave/enclave_u.c $(LDFLAGS) -o main 

crypto:
	make -C ../crypto TR_CC=$(TR_CC) TR_CXX=$(TR_CXX) clean mbedtls all

clean:
	make -C ../crypto clean
	-rm main
