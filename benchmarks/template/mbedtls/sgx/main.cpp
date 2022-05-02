#include "enclave/enclave_u.h"

#include <sgx_urts.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static sgx_enclave_id_t eid = 0;

void ocall_print_string(const char *str) {
    puts(str);
}

extern "C" int main(int argc, char **argv) {
    if ( argc != 4 ) {
        printf("%s rounds do_check long_key\n", argv[0]);
        exit(-1);
    }

    int64_t rounds   = strtol(argv[1], NULL, 10);
    int64_t do_check = strtol(argv[2], NULL, 10);
    int64_t long_key = strtol(argv[3], NULL, 10);

    printf("running for %lu rounds\n", rounds);

    char buffer[500];
    snprintf(buffer, 500, "%s", argv[0]);
    buffer[strlen(buffer) - 4] = 0;
    char buffer2[500];
    snprintf(buffer2, 500, "%s/enclave/enclave.so", buffer);

    int                updated = 0;
    sgx_launch_token_t token   = { 0 };
    if ( sgx_create_enclave(buffer2, /*debug=*/1, &token, &updated, &eid, NULL) != SGX_SUCCESS ) {
        puts("cannot open enclave");
        exit(-1);
    }

    int key_index = long_key == 1 ? 27 : 28;

    if ( crypto_init(eid, key_index) != SGX_SUCCESS ) {
        puts("cannot init enclave");
        exit(-1);
    }

    FILE *results = fopen("results.csv", "w");

    for ( int64_t i = 0; i < rounds; ++i ) {
        if ( crypto_clear(eid) != SGX_SUCCESS ) {
            puts("cannot clear enclave");
            exit(-1);
        }

        clock_t start = clock();

        if ( do_check == 1 ) {
            if ( enclave_crypto_run(eid) != SGX_SUCCESS ) {
                puts("cannot run enclave");
                exit(-1);
            }
        }
        else {
            if ( enclave_crypto_run_no_check(eid) != SGX_SUCCESS ) {
                puts("cannot run enclave");
                exit(-1);
            }
        }

        clock_t end  = clock();
        double  secs = (double)(end - start) / CLOCKS_PER_SEC;

        fprintf(results, "%f\n", secs);
        fflush(results);
    }

    sgx_destroy_enclave(eid);
    fclose(results);

    puts("done");
    return 0;
}
