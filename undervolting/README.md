# Undervolting Benchmarks

This directory contains scripts to evaluate undervolting on the system. We evaluate the transition time between undervolt requests, search for susceptible instruction, and validate the detection rate of Minefield.


# 0 Preparations

## Stable Environment
We strongly recommend adopting the grub command line to ensure stable conditions and minimize the risk for system freezes during undervolting. 

Therefore edit `/etc/default/grub` with sudo or root privileges, e.g., when using `vi`:
```
sudo vi /etc/default/grub
```

Modify `GRUB_CMDLINE_LINUX_DEFAULT` to:

```
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash mitigations=off nmi_watchdog=0 mce=off idle=poll isolcpus=domain,managed_irq,0,1,2,4,5,6 nohz_full=0,1,2,4,5,6 intel_pstate=per_cpu_perf_limits"
```
Note: `nohz_full` might not be supported but is no must-have.

This example assumes a **4** core CPU and isolated all but two cores (core **3**) and its SMT thread (core **7**) for the benchmarks, please adapt the number of cores your CPU has. If unsure, use:

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

This should report the exact parameters you entered and fewer cores than your actual thread count, e.g., **2** with an **8** core CPU.

## Fixed Frequency
The benchmarks require the `cpupower` tool to fix the frequency combined with `msr-tools` to fix the `voltage`. Please make sure that both these commands are available:

```
rdmsr
wrmsr
cpupower
```

**Please verify that the CPU frequency can be fixed by executing the following and confirming that the output shows 3000 MHz in this example.**
```
sudo cpupower -c all frequency-set -g performance -d 3G -u 3G 
watch -n 0.1 "grep MHz /proc/cpuinfo"
```

**Otherwise, these benchmarks will NOT work and lead to system freezes**

## Build Libraries
The libraries folder contains common extracted source code. Execute the following to build the libraries:

```
cd libraries
make
```

# 1 Undervolting Timing
This benchmark measures the timing between voltage transitions (Figure 3).

To run the benchmark, execute:

```
cd transition_timing
make record UV=-100 
```

To plot the results, run:
```
cd transition_timing
make plot
```

Note: The `UV` variable specifies the undervolting offset in units of millivolt. We recommend `-100mV` in the example as this is usually a save undervolting offset where neither faults nor crashes occur. If you experience system freezes, change this value.

# 2 Instructions Susceptible to Undervolting (difficult)
To build and run the instruction finding framework to analyze the x86 instruction set, follow the [Readme](./analyse_instructions/README.md)

# 3 Detection Rate (difficult)
To build and run the detection rate benchmark, follow the [Readme](./detection_rate/README.md)
