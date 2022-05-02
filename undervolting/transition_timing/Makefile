INCLUDE_DIRS=-I../libraries/undervolting -I../libraries/logging 
LIBRARY_DIRS=-L../libraries/undervolting -L../libraries/logging

CFLAGS= $(INCLUDE_DIRS) -Wpedantic -g -Os -Wall -march=native -std=c++17  -fno-fast-math 

FACTOR?=0x7
INIT?=0x555555555
ALIGN?=0

CFLAGS+= -DFACTOR=$(FACTOR) -DINIT=$(INIT) -DALIGNMENT=$(ALIGN)
LDFLAGS= $(LIBRARY_DIRS) -lundervolting -llogging -pthread

DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
COMPILER=$(DIR)/../compiler/clang

ISOLATED_CORE=1

all: libraries
	g++ $(CFLAGS) main.cpp $(LDFLAGS) -o main

libraries:
	make -C ../libraries

clean:
	-rm main

record: all
	sudo cpupower frequency-set -d 1GHz -u 1Ghz 
	sudo ./main 100000 $(UV) $(ISOLATED_CORE) 100000 2> log.txt
	sudo cpupower frequency-set -d 3GHz -u 3Ghz 
	sudo ./main 100000 $(UV) $(ISOLATED_CORE) 100000 2> log_3ghz.txt

plot:
	python3 plot.py