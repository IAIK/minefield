all:
	g++ -fPIE fault-hardening.cpp -c -o fault-hardening.o
	ar rcs libfault-hardening.a fault-hardening.o
	objdump fault-hardening.o -dC
	rm fault-hardening.o
	

clean:
	-rm libfault-hardening.a fault-hardening.o
