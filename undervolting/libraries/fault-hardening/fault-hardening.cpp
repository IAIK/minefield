#include "fault-hardening.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// these variables will be used by the compiler extension
extern "C" {
volatile uint64_t __fault_mul_store;

//#include "fault-hardening-values.h"

volatile uint64_t __fault_count = 0;
volatile uint64_t __fault_mask  = 0;
volatile uint64_t __fault_pending = 0;

volatile uint64_t __attribute__((weak)) __fault_mul_factor = 0x11;
volatile uint64_t __attribute__((weak)) __fault_mul_inverse;
volatile uint64_t __attribute__((weak)) __fault_mul_init = 0x555555555;
volatile uint64_t __fault_mul_mask;
volatile uint64_t __attribute__((aligned(256))) __fault_mul_checks[2048 * 2];

void __attribute__((weak, noreturn)) __fault_abort_callback() {
    puts("detected fault!");
    exit(-1);
}

void __fault_abort() {
    asm volatile("mov __fault_mul_checks(%rip), %r12");
    asm volatile("lea __fault_mul_checks(%rip), %r13");
    asm volatile("mov __fault_mul_factor(%rip), %r14");
    __fault_abort_callback();
}
}

__uint128_t extended_gcd(__uint128_t a, __uint128_t b, __uint128_t &x, __uint128_t &y) {
    if ( a == 0 ) {
        x = 0;
        y = 1;
        return b;
    }
    __uint128_t x1, y1;
    __uint128_t gcd = extended_gcd(b % a, a, x1, y1);
    x               = y1 - (b / a) * x1;
    y               = x1;

    return gcd;
}

__uint128_t find_inverse(__uint128_t factor, __uint128_t modulo) {
    __uint128_t x, y;
    __uint128_t gcd = extended_gcd(factor, modulo, x, y);
    if ( gcd != 1 ) {
        printf("cannot find inverse!\n");
        exit(-1);
    }
    //printf("found inverse: 0x%llx%0llx\n", (uint64_t)(x >> 64), (uint64_t)x);
    return x;
}

extern "C" int _main(int argc, char *argv[]);

int main(int argc, char *argv[]) {

    __fault_mul_inverse = find_inverse(__fault_mul_factor, ((__uint128_t)1 << 64));

    uint64_t state  = __fault_mul_init;
    uint64_t factor = __fault_mul_factor;
    for (int i = 0; i < 2048; ++i) {
        __fault_mul_checks[i] = state;
        if ( i < 10 ) {
            // printf("state[%d]: 0x%llx\n", i, state);
        }
        state = state * factor;
    }
    asm volatile("mov __fault_mul_checks(%rip), %r12");
    asm volatile("lea __fault_mul_checks(%rip), %r13");
    asm volatile("mov __fault_mul_factor(%rip), %r14");
    asm volatile("mov __fault_mul_checks(%rip), %r13");
    int ret = _main(argc, argv);
    return ret;
}

namespace fault_hardening {

uint64_t get_fault_count() {
    uint64_t tmp  = __fault_count;
    __fault_count = 0;
    return tmp;
}

uint64_t get_fault_mask() {
    uint64_t tmp = __fault_mask;
    __fault_mask = 0;
    return tmp;
}

} // namespace fault_hardening
