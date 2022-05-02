# Minefield

This is the PoC implementation for the USENIX 2022 paper **Minefield: A Software-only Protection for SGX Enclaves against DVFS Attacks** by Andreas Kogler, Daniel Gruss, and Michael Schwarz.

This Readme walks you through compiling a modified **clang compiler**, the **SGX driver**, the modified **SGX SDK**, and finally, the benchmarks.
The modified compiler and SDK comprise Minefield, which allows you to harden SGX enclaves against undervolting.
The total install time should be around 1h-2h.

As we need to use software-based undervolting access to **sudo** or a **root** user is required.

# 0 Preparations

## Ubuntu Packages
Install the following packages and make sure `msr-tools` are available.
```
sudo apt install msr-tools cmake ninja-build build-essential git python3 python3-pip
sudo apt install linux-tools-$(uname -r)
sudo modprobe msr
```
**Please install all prerequisites for your system according to the [linux SGX SDK](https://github.com/intel/linux-sgx/blob/master/README.md) .**

For Ubuntu 18.04 and 20.04, these are:  
```
sudo apt-get install libssl-dev libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper cmake reprepro unzip
```

additionally, for Ubuntu 18.04:  
```
sudo apt-get install build-essential ocaml ocamlbuild automake autoconf libtool wget python libssl-dev git cmake perl
```

additionally, for Ubuntu 20.04:  
```
sudo apt-get install build-essential ocaml ocamlbuild automake autoconf libtool wget python-is-python3 libssl-dev git cmake perl
```

Please see the Intel page for other operating systems.

## Python packages
Please install the required python3 packages required via:

```
python3 -m pip install -r requirements.txt
```

# 1 Compiler
To build the compiler for Minefield, install the prerequisites for LLVM (e.g. `cmake` and `ninja-build`) and run `./build.sh` inside the `compiler` folder:

```
cd compiler
./build.sh
```

If the build fails or hangs (e.g. not enough ram), you can try to restart with `ninja clang` in the `compiler/llvm/build` directory.

```
cd compiler/llvm/build
ninja clang
```

# 2 SGX Driver
Install the SGX driver according to the [linux SGX driver](https://github.com/intel/linux-sgx-driver) repository.

After inserting the Linux SGX driver module, verify that the following command returns **0**:

```
dmesg | grep "is missing SGX" -c
```

# 3 SGX SDK/PSW

In this step, we will check out the SGX-SDK/PSW twice.
- First, we install an unmodified version of the PSW, i.e., the runtime environment needed to start and execute SGX enclaves. (creates the `linux-sgx` folder)
- Second, we patch the modified SGX SDK used for Minefield.
Note that we **do not build and install** the modified SGX SDK in this step but later in the benchmark step, as the SDK build is dependent on the **placement density**. (creates the `linux-sgx-patched` folder)

## 3.1 Semi-Automatic Installation

If you have no need to preserve an old PSW or SDK and defaults for everything are acceptable (and you're feeling lucky), you can skip all steps in **3.2** by running `setup.sh` from within the `scripts` folder.

```
cd scripts
./setup.sh
```

**You still have to manually update your source.list file and install the packages. The script prints these instructions at the end.**


## 3.2 Manual Installation
We detail the installation steps from the semi-automatic approach if the `setup.sh` script should fail in the following.

We advise completely removing all SDK and PSW installations from the system before starting.
If you have previously installed a version of SGX that uses local packages, you may uninstall SDK and PSW with 

```
cd scripts
./uninstall_psw_sdk.sh
```

This step installs an unpatched SDK/PSW to install all the services needed to run enclaves.

First, check out the source twice and patch one version:

```
cd scripts
./checkout_and_patch.sh
```

Second, install the unmodified SDK:

```
cd scripts
./install_unmodified_sdk.sh
```

Finally, install the unmodified PSW with:

```
cd scripts
./install_unmodified_psw.sh
```

and follow the adaptation of the source.list as prompted and install the build PSW with the prompted apt commands.

# 4 Performance Benchmarks
Scripts to install and run the benchmarks are provided in the `benchmarks` folder, continue in the [Readme](benchmarks/README.md) there.

# 5 Undervolting Benchmarks (difficult)
Scripts to install and run the undervolting related benchmarks are provided in the `undervolting` folder, continue in the [Readme](undervolting/README.md) there.

# Warnings
**Warning #1**: We are providing this code as-is. You are responsible for protecting yourself, your property and data, and others from any risks caused by this code. This code may cause unexpected and undesirable behavior to occur on your machine.

**Warning #2**: This code is only for testing purposes. Do not run it on any production systems. Do not run it on any system that might be used by another person or entity.
