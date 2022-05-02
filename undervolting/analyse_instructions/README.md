# Susceptible Instruction Finding Framework

This framework was used to verify that the `imul` instruction is a good candidate for the trap instruction.

# General Functionality

To perform undervolting safely on a remote system, the framework uses a remote power switch, in our case a self-built pi-pdu to restart the remote system after a crash was detected. The framework assumes that the same files are present on the remote as on the host system to execute the remote correctly. All the commands are invoked on the host system and the framework mirrors the commands to the remote system and tracks the progress there to safely continue after a system crash was detected without losing progress.

We provide a [deploy](./scripts/deploy.sh) script that syncronizes the files from the **host** to the **remote**. **Not in both directions!**. The script takes the remote as parameter, for this readme we assume the remote is labled `lab-machine` in the ssh config file `~/.ssh/config`. The following command will sync the files to the remote.

```
./scripts/deploy.sh lab-machine
```

This will copy the needed files from the current directory to the remote. The remote path is `user@lab-machine:~/analyse-instructions/*`.

To show all available commands of the framework, use:
```
python3 analyse.py --help
```

The framework consists of the following parts:

## Instruction Generator
Uses the x86 instruction xml file and an adapted form of the xml parser from [uops-info](https://uops.info/xml.html), to generate shared libraries from each of the instructions to analyze. The shared library contains code to store each of the outputs of the instruction and execute the instruction in a loop multiple times. The generated shared libraries are named after the instruction used in the library and are stored in the [instructions/all](instructions/all) directory. During generation, a filter can be applied to only generate instructions from a given x86 instructions set. (see help)

To show all available generation commands use:
```
python3 analyse.py generate --help
```

## Instruction Filtering
The generated instruction shared libraries are executed without undervolting, to filter out instruction where the framework did not automatically generate the correct code, or the instruction cannot be used in the framework, e.g., `rdrand` and `rdtsc` cannot be checked for differences, as the output will differ each invocation. The shared libraries that are executed without error are stored in the [instructions/runnable](instructions/runnable) directory, the ones with errors in the [instructions/non-runnable](instructions/non-runnable) directory.

To show all available filter commands, use:
```
python3 analyse.py filter --help
```

## Undervolting
This is the central part of the framework. We use the generated shared libraries in combination with undervolting to detect faults. To recover from system crashes during undervolting, the framework relies on the connected pi-pdu, which safely triggers the reset pin of the remote system. The shared libraries code is executed during undervolting, and each of the generated outputs is analyzed for faults and compared to a reference execution without undervolting. The framework tracks the number of faults and additional information. The framework allows specifying different commands and searching a complete range of frequency and voltage operating points.

To show all available undervolt commands, use:
```
python3 analyse.py undervolt --help
```

## Plotting (experimental)
This part of the framework scans the log folder, extracts instruction names and faulting points, and visualizes them in a plot.

To show all available plot commands, use:
```
python3 analyse.py plot --help
```

## Extract (experimental)
Searches the log folder for faulted instructions.

To show all available extract commands, use:
```
python3 analyse.py extract --help
```

## Remote Restarts
For the remote restarts to work we need to specify the connected reset pins and the remotes name [here](./framework/remote.py#L32) and [here](./pdu.py#L68). The framework automatically restarts the remote system after the specified timeout is reached. We provide [pdu.py](./pdu.py) to manually restart the remote:

```
python3 pdu.py lab-machine
```

# 0 Preparations 
**Follow the Readmes in the parent directories and make sure that the preparations are also executed on the `lab-machine` ([Readme](../README.md)'s "0 Preparations" and [Readme](../../README.md)'s "0 Preparations")**

## Build the Undervolter
Build the program that loads the shared libraries and executes the undervolt with:

```
cd undervolter
make
```

Checkout the `instruction.xml` file from uops.info:

```
./get_instruction_xml.sh
```



# 1 Reproduce faulting results (Table 1 and Table 2):
We provide the faulting instructions found during our analysis in the [instructions/faulted](instructions/faulted) directory. These can be used as a subset to speed up the reproducibility of the results (see last step here). To generate all instructions follow these steps.

First, generate the instructions with:

```
python3 analyse.py generate -i data/instructions.xml -o instructions/all -e BASE -E SSE -e SSE2 -e FMA -e AVX -e AVX2
```

Second, filter the generated instructions to remove non-runnable instructions. Note that if this step is performed on the host machine, the host needs also to support the instruction-set of the remote machine.

```
python3 analyse.py filter -i instructions/all -or instructions/runnable -on instructions/non-runnable -e BASE -E SSE -e SSE2 -e FMA -e AVX -e AVX2
```

Third, after the filtering is finished execute the [deploy](./scripts/deploy.sh) script to synchronize the generated instructions to the `lab-machine`.

```
./scripts/deploy.sh lab-machine
```

Fourth, after synchronizing the filtered instructions, the faulting instructions can be searched at a fixed frequency and with variable undervolts. For example: To search over all the runnable instructions at a frequency of **3.5 GHz** starting at an undervolt from **-100** to **-290 mV** in **-5 mV** steps, on the `lab-machine` system, use the following command, where extension should be replaces with the generated folder, e.g., AVX2.  

```
mkdir -p logs/lab-machine
python3 analyse.py undervolt \
  --input "instructions/runnable/EXTENSION" \
  --op=3500 -100 \
  --vo_end=-290 \
  --vo_step=-5 \
  --core=0 \
  --rounds=5 \
  --iterations=500000 \
  --remote="lab-machine" \
  --pdu="pi-pdu" \
  --log_dir="logs/lab-machine" \
  --timeout=40   \
  --retry_count=2
```

If the search should be performed on the **HOST** instead of the **REMOTE** use:

```
python3 analyse.py undervolt \
  --input "instructions/runnable/EXTENSION" \
  --op=3500 -100 \
  --vo_end=-290 \
  --vo_step=-5 \
  --core=0 \
  --rounds=5 \
  --iterations=500000
```

The framework will report the faulted instructions.

Finally, after the set of faultable instructions is found, the instructions can be further analyzed to find the exact faulting points. We provide our found faulted instructions in [./instructions/faulted](./instructions/faulted).

```
mkdir -p logs/lab-machine

for c in {0,1,2,3}
do
python3 analyse.py undervolt \
  --input "instructions/faulted" \
  --op=1500 -120 \
  --op=2000 -120 \
  --op=2500 -120 \
  --op=3000 -120 \
  --op=3500 -120 \
  --op=4000 -120 \
  --vo_end=-290 \
  --vo_step=-5 \
  --core=$c \
  --rounds=5 \
  --iterations=500000 \
  --remote="lab-machine" \
  --pdu="pi-pdu" \
  --log_dir="logs/lab-machine" \
  --find_faulting_points \
  --timeout=40   \
  --retry_count=2
done

```

Note: adapt the for loop to match the maximum number of cores without SMT threads.

# 2 Analyze the results
During the previous step the log director is filled with the output of the runs. Execute the following to search through all the files of that directory:

```
python3 analyse.py plot --input logs/lab-machine
```

or specify a log file directly:

```
python3 analyse.py plot --input file
```
