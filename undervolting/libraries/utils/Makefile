

all:
	g++ utils.cpp -c -o utils.o -std=c++2a -march=native 
	ar rcs libutils.a utils.o
	rm utils.o

clean:
	-rm libutils.a utils.o 