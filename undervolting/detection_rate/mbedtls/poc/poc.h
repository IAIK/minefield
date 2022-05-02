#pragma once
#include <stddef.h>
#include <stdint.h>

extern "C" {

void ecall_init();
void ecall_experiment();
void ecall_print_results(uint64_t current_iteration);

void ocall_undervolt_begin();
void ocall_undervolt_end();

void ocall_puts(char const *str);
}