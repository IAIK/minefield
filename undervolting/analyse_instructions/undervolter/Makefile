INCLUDE_DIRS=-I../../libraries/undervolting -I../../libraries/logging -I../../libraries/fault-hardening -I../../libraries/utils
LIBRARY_DIRS=-L../../libraries/undervolting -L../../libraries/logging -L../../libraries/fault-hardening -L../../libraries/utils

CFLAG= $(INCLUDE_DIRS)  -Wpedantic -Wall -g3 -std=c++17 

LDFLAGS= $(LIBRARY_DIRS) -pthread -lundervolting -llogging -lfault-hardening -lutils -ldl -Wl,-rpath=.

all: undervolter

undervolter: main.cpp
	g++ $(CFLAG) main.cpp $(LDFLAGS) -o undervolter

clean:
	-rm undervolter