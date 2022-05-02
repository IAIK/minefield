import os
import datetime
import subprocess

script_path = os.path.dirname(os.path.realpath(__file__))

def undervolt(path, voltage_offset, frequency, core, rounds, iterations, verbose, trap_factor, trap_init):
  # file voltage_offset target_core core_count rounds iterations

  now = datetime.datetime.now().strftime("%H:%M:%S")
  file = os.path.basename(path)

  print(f"{now}; ", end="", flush=True)

  try:
    r = subprocess.run(["sudo", "cpupower", "frequency-set", "-d", f"{frequency}MHz", "-u", f"{frequency}MHz"], stdout=subprocess.DEVNULL)
    if r.returncode != 0:
      print("F failed init")
      exit(0)

    r = subprocess.run(["lscpu", "-e"], capture_output=True)
    if r.returncode != 0:
      print("F failed init")
      exit(0)
    ncores = len([x for x in r.stdout.decode("utf-8").split("\n") if "yes" in x])
    #ncores = 0
    #print("no cores")

    r = subprocess.run(["lscpu", "-p"], capture_output=True)
    if r.returncode != 0:
      print("F failed init")
      exit(0)

    sibling = [x.split(",")[0] for x in r.stdout.decode("utf-8").split("\n") if x != '' and x[0] != '#' and x.split(",")[1] == str(core) and x.split(",")[0] != str(core)]
    sibling = core if len(sibling) == 0 else sibling[0]

    args = ["sudo", f"{script_path}/../undervolter/undervolter", f"{path}", f"{frequency}", f"{voltage_offset}", f"{core}", f"{sibling}", f"{ncores}", f"{rounds}", f"{iterations}", f"{0}", f"{trap_factor}", f"{trap_init}"]

    #print(args)
    r = subprocess.run(args, capture_output=True)

    output = r.stdout.decode("utf-8")
    
    if output.endswith("\n"):
      output = output[:-1]
    
    if r.returncode == 0:
      print(f"       OK; {output}")
      return True
    elif r.returncode == 3:
      print(f"X faulted; {output}")
      return False
    else:
      print(f"C crashed; {output}")
      return False

  except KeyboardInterrupt:
    print("aborted")
    exit(0)


def undervolt_run(files, voltage_offset, frequency, core, rounds, iterations, skip_until, verbose, trap_factor, trap_init):
  for file in files:
    if skip_until is not None and skip_until in file:
      skip_until = None
    elif skip_until is not None:
      continue

    undervolt(file, voltage_offset, frequency, core, rounds, iterations, verbose, trap_factor, trap_init)


def undervolt_filter(input_directory, runnable_directory, non_runnable_directory, extensions):
  for e in extensions:
    os.system(f"mkdir -p '{runnable_directory}/{e}' ")
    os.system(f"mkdir -p '{non_runnable_directory}/{e}' ")

    for file in sorted(os.listdir(input_directory + "/" + e)):
      dir_file = input_directory + "/" + e + "/" + file
      
      if os.path.isfile(dir_file):
          runnable = undervolt(dir_file, 0, 3000, 0, 1, 10, False, 10, 10)
          dest = runnable_directory if runnable else non_runnable_directory
          os.system(f"cp '{dir_file}' '{dest}/{e}' ")
          