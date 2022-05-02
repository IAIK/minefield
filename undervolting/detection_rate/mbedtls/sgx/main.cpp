#include "enclave_u.h"
#include "sgx_urts.h"
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

void enclave_puts(char const *str) {
    puts("%s");
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

    sgx_enclave_id_t   eid     = 0;
    sgx_launch_token_t token   = { 0 };
    sgx_status_t       ret     = SGX_ERROR_UNEXPECTED;
    int                updated = 0;

    // Create enclave
    if ( sgx_create_enclave("enclave.signed.so", SGX_DEBUG_FLAG, &token, &updated, &eid, NULL) != SGX_SUCCESS ) {
        printf("Failed to start enclave!\n");
        return -1;
    }

    int64_t iterations = 1024ull * 30;
    int64_t rounds     = strtol(argv[1], NULL, 10);
    int64_t offset     = strtol(argv[2], NULL, 10);
    uint8_t core       = strtol(argv[3], NULL, 10);

    undervolting::open(core);
    undervolting::set_undervolting_target(offset);

    ecall_init(eid);

    printf("running with %3ld mV\n", offset);
    printf("iteration: fscore: factor = faulted_and_detected / faulted (detected) mitigation_rate: factor = 1.0 - "
           "(effective / overall_faults) \n");

    for ( uint64_t i = 0; i < rounds; ++i ) {
        ecall_experiment(eid);

        ecall_print_results(eid, i);
    }
    printf("\n");

    undervolting::close();

    sgx_destroy_enclave(eid);

    return 0;
}
