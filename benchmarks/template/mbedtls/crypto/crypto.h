#pragma once

extern "C" {
void crypto_init(int key);
void crypto_clear();
void crypto_run();
void crypto_run_no_check();
int  crypto_is_faulted();
}