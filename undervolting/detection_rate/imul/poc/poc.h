#pragma once

#include <cstdint>

extern "C" {

void ecall_experiment(uint64_t iterations, int64_t random_value, int64_t random_init);
void ecall_print_results(uint64_t current_iteration);

void ocall_undervolt_begin();
void ocall_undervolt_end();

void ocall_puts(char const *str);
}