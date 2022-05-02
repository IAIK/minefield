#include "poc.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
volatile const uint64_t __fault_mul_factor = FACTOR;
volatile const uint64_t __fault_mul_init   = INIT;
volatile uint64_t       __fault_pending    = 0;
}

static uint64_t target_faulted          = 0;
static uint64_t target_and_trap_faulted = 0;
static uint64_t trap_faulted            = 0;

static uint64_t effective_faults = 0;
static uint64_t overall_faults   = 0;

static void report_printf(char const *fmt, ...) {
    char    message_buffer[1000];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(message_buffer, 999, fmt, argptr);
    va_end(argptr);
    message_buffer[999] = 0;
    ocall_puts(message_buffer);
}

static void __attribute__((noinline)) analyse(int64_t undervolt_result, int64_t normal_result) {

    bool b_result_faulted = undervolt_result != normal_result;
    bool b_trap_faulted   = __fault_pending != 0;

    // used for recall/fscore
    target_faulted += b_result_faulted;
    target_and_trap_faulted += b_result_faulted && b_trap_faulted;
    trap_faulted += b_trap_faulted;

    // used for mitigation rate
    effective_faults += b_result_faulted && !b_trap_faulted;
    overall_faults += b_result_faulted || b_trap_faulted;
}

int64_t __attribute__((noinline)) experiment(uint64_t iterations, int64_t factor, int64_t init) {
    int64_t reg = init;

    // unroll the loop as otherwise the probabilistic placement density would require recompilations
#pragma clang loop unroll_count(256)
    for ( uint64_t i = 0; i < iterations; ++i ) {
        reg *= factor;
    }

    return reg;
}

// we need to split this in two functions as otherwise clang figures out what we are doing and removes the two calls...
// which is amazing btw
int64_t __attribute__((noinline)) check(uint64_t iterations, int64_t factor, int64_t init) {
    int64_t reg = init;
    for ( uint64_t i = 0; i < iterations; ++i ) {
        reg *= factor;
    }
    return reg;
}

static void __attribute__((noinline)) reset_pending_flag() {
    // put this inside a function as at the begining of the function the traps are checked.
    // reset the fault pending flag
    __fault_pending = 0;
}

extern "C" void ecall_experiment(uint64_t iterations, int64_t random_factor, int64_t random_init) {

    // do this manually setup minefield here. as we want to only build the poc with minefield and not the complete
    // binary
    asm volatile("mov __fault_mul_init(%rip), %r12");
    asm volatile("mov __fault_mul_init(%rip), %r13");
    reset_pending_flag();

    // undervolted execution
    ocall_undervolt_begin();

    int64_t undervolt_result = experiment(iterations, random_factor, random_init);

    ocall_undervolt_end();

    // normal execution
    int64_t normal_result = check(iterations, random_factor, random_init);

    analyse(undervolt_result, normal_result);
}

extern "C" void ecall_print_results(uint64_t current_iteration) {
    double fscore          = target_and_trap_faulted / (double)target_faulted;
    double mitigation_rate = 1.0 - (effective_faults / (double)overall_faults);

    report_printf("%5lu: fscore: %5.6f = %8llu / %8llu (%8llu) mitigation_rate: %5.6f = 1.0 - (%8llu / %8llu) \r",
                  current_iteration, fscore, target_and_trap_faulted, target_faulted, trap_faulted, mitigation_rate,
                  effective_faults, overall_faults);
}
