#include "../poc/poc.h"
#include "undervolting.h"

#include <immintrin.h>
#include <iterator>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void ocall_undervolt_begin() {
    undervolting::begin();
}
void ocall_undervolt_end() {
    undervolting::end();
    usleep(200); // wait for stable state after end
}

void ocall_puts(char const *str) {
    printf("%s", str); // no /n so /r also works
    fflush(stdout);
}

int main(int argc, char **argv) {

    srand(time(0));

    if ( argc != 4 ) {
        printf("%s rounds offset core\n", argv[0]);
        exit(-1);
    }

    int64_t iterations = 1024ull * 30;
    int64_t rounds     = strtol(argv[1], NULL, 10);
    int64_t offset     = strtol(argv[2], NULL, 10);
    uint8_t core       = strtol(argv[3], NULL, 10);

    undervolting::open(core);
    undervolting::set_undervolting_target(offset);

    printf("running with %3ld mV\n", offset);

    printf("iteration: fscore: factor = faulted_and_detected / faulted (detected) mitigation_rate: factor = 1.0 - "
           "(effective / overall_faults) \n");

    for ( uint64_t i = 0; i < rounds; ++i ) {
        int64_t factor, init;
        while ( _rdrand64_step((unsigned long long *)&factor) == 0 )
            ;
        while ( _rdrand64_step((unsigned long long *)&init) == 0 )
            ;

        ecall_experiment(iterations, factor, init);

        ecall_print_results(i);
    }
    printf("\n");

    undervolting::close();

    return 0;
}
