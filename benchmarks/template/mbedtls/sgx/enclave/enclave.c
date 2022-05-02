#include "crypto.h"
#include "enclave_t.h"

#include <stdint.h>
extern "C" {
uint64_t volatile __fault_count      = 0;
uint64_t volatile __fault_pending    = 0;
uint64_t volatile __fault_mul_factor = 0;
}
extern "C" void __fault_abort(void) {
    ocall_print_string("faulted!");
    asm volatile("ud2");
}

void enclave_puts(char const *p) {
    ocall_print_string(p);
}

int mitigation_is_faulted(void) {
    return __fault_count;
}

void test_ocall(void) {
    ocall_print_string("ocall is working!");
}

void enclave_crypto_run() {
    crypto_run();
}

void enclave_crypto_run_no_check() {
    crypto_run_no_check();
}