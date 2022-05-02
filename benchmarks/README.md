# Performance Benchmarks

This directory contains scripts to evaluate Minefield's performance. The benchmarks are performed over the **placement density** parameter. As this is a compile-time parameter, we have to build the benchmarks and the SGX-SDK for all **placement densities** we want to evaluate.

# 0 Preparations
Checkout and patch the template benchmarks with:

```
./scripts/benchmarks_checkout.sh
```

# 1 Setting up SDKs and compiling benchmarks
This step will compile the Intel SDK and the benchmarks for each given **placement densities**. A placement density of **x** means **x** trap `imul` instructions after each regular instruction. 

The defaults are:

```
0, 0.25, 0.50, 0.75, 1, 1.25, 1.50, 1.75, and 2.00.
```

specified in [scripts/config.txt](scripts/config.txt).

The two important scripts are:
- [./scripts/benchmarks_build.sh](./scripts/benchmarks_build.sh) builds the SDK and benchmark for the given **placement densities**, passed in as integer argument list (scale=*100; eg 0.25=25). If none are specified, the defaults from above are used.

- [./scripts/benchmarks_update_template.sh](./scripts/benchmarks_update_template.sh) only builds the benchmarks, assuming the appropriate SDKs (with the same **placement density**) already exist.


Execute the following command to build the SDK and benchmarks, **from the benchmarks directory!**

```
./scripts/benchmarks_build.sh
```

# 2 Running Benchmarks

## Stable Environment
We strongly recommend adopting the grub command line to ensure stable conditions for the benchmark. 

Therefore edit `/etc/default/grub` with sudo or root privileges, e.g., when using `vi`:
```
sudo vi /etc/default/grub
```

Modify `GRUB_CMDLINE_LINUX_DEFAULT` to:

```
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=domain,managed_irq,1,5 intel_pstate=per_cpu_perf_limits"
```

This example assumes a **4** core CPU, and isolated one core (core **1**) and its SMT thread (core **5**) for the benchmarks, please adapt the number of cores your CPU has. If unsure, use:

```
lscpu -e
```

After the modification, use:

```
sudo update-grub
sudo reboot
```

to make the changes active and verify them after rebooting by executing:

```
cat /proc/cmdline
nproc
```

This should report the exact parameters you entered and fewer cores than your actual thread count, e.g., **6** with an **8** core CPU.

The run scripts have a variable `isolated_core` that you can adapt to one of your isolated cores, and the default is **1**,

## Fixed Frequency
All benchmarks should be run with a fixed sustainable CPU frequency on a quiet system to guarantee stable results. You can use the `cpupower` tool for this:
```
sudo cpupower -c all frequency-set -g performance -d 3G -u 3G 
```
To fix the frequency to 3GHz, for example.

Depending on your CPU and OS, this might be a bit different. You can verify the CPU frequency with:
```
sudo cpupower frequency-info
```

and

```
watch -n 0.1 "grep MHz /proc/cpuinfo"
```

**Please make sure that the frequency is fixed during the benchmarks**

Note: Isolating cores and fixing the frequency harms building the SDK and benchmark components and should be performed **after** the benchmarks were built.

## Scripts
The benchmark comprises **4** scripts, and each of the scripts takes the same list of **placement densities** as the build script to evaluate the benchmarks. If none are specified, the defaults are used.

The scripts are:
- [./scripts/benchmarks_run_nbench.sh](./scripts/benchmarks_run_nbench.sh) runs **sgx-nbench** benchmark (Figure 8)
- [./scripts/benchmarks_run_mbedtls.sh](./scripts/benchmarks_run_mbedtls.sh) runs the **mbedTLS** redundancy benchmark (Figure 9)
- [./scripts/benchmarks_run_size.sh](./scripts/benchmarks_run_size.sh) runs the **size** benchmark (Figure 10)
- [./scripts/benchmarks_run_ctime.sh](./scripts/benchmarks_run_ctime.sh) runs the **compile time** benchmark (Figure 11)

Each of the scripts (except the **size** benchmark, see below) stores the results in a generated `results_${BENCHMARK}_${DATE}` folder which must be further processed to obtain the results. This allows to split evaluation and analysis (see **3 Visualization**). If you're only interested in a quick check, you can shorten the **sgx-nbench** runtime (see **4 Customization**).

As the **size** benchmark is deterministic, making the split unnecessary. The **size results** are stored in `meta.csv` without an automatically generated results folder.

# 3 Visualization
To convert the results of the `results_${BENCHMARK}_${DATE}` folders into CSV format, we provide additional scripts:

- `./scripts/benchmarks_to_csv_nbench.sh result_nbench_${DATE}` generates `result_nbench_${DATE}/results.csv`
- `./scripts/benchmarks_to_csv_ctime.sh result_ctime_${DATE}` generates `result_ctime_${DATE}/results.csv`
- `./scripts/benchmarks_to_csv_mbedtls.sh result_mbedtls_${DATE}` generates `result_mbedtls_${DATE}/results.csv`
 
We also provide a conversion and plotting script to convert the absolute performance numbers of the `results.csv` files to relative performance numbers.

- `./scripts/plot_csv.py result_nbench_${DATE}/results.csv` visualizes the **sgx-nbench** results.
- `./scripts/plot_csv.py result_ctime_${DATE}/results.csv inv` visualizes the **compile time** results.
- `./scripts/plot_csv.py result_mbedtls_${DATE}/results.csv inv` visualizes the **mbedTLS** results.
- `./scripts/plot_csv.py meta.csv inv` visualizes the **size** results.

Note: `inv` stands for inverted, i.e., whether **lower** is better or visa versa.

# 4 Customization

The run scripts have a variable `isolated_core` to determine which core the benchmark should be run on.  
If you have no isolated core, you can set this to any valid number or remove the taskset command altogether.

The **sgx-nbench** benchmark can be sped up by reducing the number of iterations. Just change the `#define N_RUNS 25` in [template/sgx-nbench/nbenchPortal/nbench0.c:795](./template/sgx-nbench/nbenchPortal/nbench0.c#L795) to something else and recompile with `./scripts/benchmarks_update_template.sh`.

