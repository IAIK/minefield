# Detection Rate Benchmarks

This directory contains scripts to evaluate Minefield's detection rate.

# 0 Preparations 
**Follow the Readmes in the parent directories and make sure that the preparations are also executed on the `lab-machine` ([Readme](../README.md)'s "0 Preparations" and [Readme](../../README.md)'s "0 Preparations")**

These Benchmarks require the compiler extension for building.

Checkout and patch `mbedtls`:

```
cd mbedtls/poc
./checkout.sh
```

# 1 General Functionality
The benchmarks use Minefield to harden two PoCs. First, the Plundervolt PoC, and second, the MbedTLS library. We provide both native and SGX versions of the benchmarks. We experienced substantially more system freezes with the SGX version than the native versions during our evaluation. The native version can also be executed without an installed SGX environment and only require the Minefield compiler extension to be built.

In these benchmarks, we do not build the SGX_SDK with Minefield. Only the PoC implementations are protected by Minefield as we are only interested in the effectiveness of the protected implementations without the surrounding environment.

# 2 Executing the Benchmarks

In the following scripts, `start_voltage_offset` is the start undervolt offset to execute the benchmarks. Each run, the voltage is further reduced by one mV. `core` specifies the core the benchmark should be executed. This should be an isolated core as described in the parent readme. `placement_density` determines the placement density (with scale *=100, e.g., 1.0=100) for which the benchmark should be built.

Both the `imul` and `mbedtls` folders contain the following scripts: 
- `run_native` runs the benchmark without SGX.
- `run_sgx` executes the same benchmark inside SGX.

## Imul Benchmark
To run the native version, execute:

```
cd imul
./run_native start_voltage_offset 100000 core placement_density
```

To run the SGX version, execute:

```
cd imul
./run_sgx start_voltage_offset 100000 core placement_density
```

## MbedTLS Benchmark
To run the native version, execute:

```
cd mbedtls
./run_native start_voltage_offset 2000 core placement_density
```

To run the SGX version, execute:

```
cd mbedtls
./run_sgx start_voltage_offset 2000 core placement_density
```

## Example:

This show an example output for running the mbedtls benchmark.
```
sudo taskset -c 0 ./main 100000 -259 0 
running with -259 mV
iteration: fscore: factor = faulted_and_detected / faulted (detected) mitigation_rate: factor = 1.0 - (effective / overall_faults) 
    8: fscore: 1.000000 =        1 /        1 (       2) mitigation_rate: 1.000000 = 1.0 - (       0 /        2) 
```