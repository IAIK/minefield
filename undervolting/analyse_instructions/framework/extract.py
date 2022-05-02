import os
import numpy as np
import re 

# never change the format of your log files during a project!
fault_0 = re.compile("..:..:.. \d+ -*\d+ \s*\d+ (.*?)steady:")
fault_1 = re.compile("..:..:.. \d{4} -*\d+\s+\d+\s(.*?)X faulted")
fault_2 = re.compile("..:..:..; X faulted; \d+; -*\d+; \s*\d+;\s\s*\d+;\s\d+;\s(.*?); faults=")

# never change the format of your log files during a project!
no_fault_0 = re.compile("..:..:.. \d+ -*\d+ \s*\d+ (.*?)steady:")
no_fault_1 = re.compile("..:..:.. \d{4} -*\d+\s+\d+\s(.*?)OK")
no_fault_2 = re.compile("..:..:..;        OK; \d+; .*\d+; \s*\d+;\s\s*\d+;\s*\d+;\s(.*?); faults=")


def get_faulted_instructions(file):
  faulted_instructions = []
  non_fualted_instructions = []
  with open(file, "r") as f:
    for l in f.readlines():
      l = l.replace("_aes", "")
      l = l.replace("_aes", "")
      if "OK" in l:
        if m := re.match(no_fault_0, l):
          non_fualted_instructions.append(m.group(1).strip())
          continue

        if m := re.match(no_fault_1, l):
          non_fualted_instructions.append(m.group(1).strip())
          continue

        if m:= re.match(no_fault_2, l):
          non_fualted_instructions.append(m.group(1).strip())
          continue
          
        print(f"could not match the following line:\n'{l}'")
        exit(0)

      if "X faulted" in l:

        if m := re.match(fault_0, l):
          faulted_instructions.append(m.group(1).strip())
          continue

        if m := re.match(fault_1, l):
          faulted_instructions.append(m.group(1).strip())
          continue

        if m:= re.match(fault_2, l):
          faulted_instructions.append(m.group(1).strip())
          continue

        print(f"could not match the following line:\n'{l}'")
        exit(0)

  return faulted_instructions, non_fualted_instructions

def extract(log_files, instruction_directory, output_directory): 
  faulted_instructions = []
  non_faulted_instructions = []
  for f in log_files:
    f, nf = get_faulted_instructions(f)
    faulted_instructions.extend(f)
    non_faulted_instructions.extend(nf)

  faulted_instructions = list(dict.fromkeys(faulted_instructions))
  non_faulted_instructions = list(dict.fromkeys(non_faulted_instructions))

  for i in non_faulted_instructions:
    print(i)

  print(len(non_faulted_instructions))
  exit(0)
  #for i in faulted_instructions:
  #  print(i)

  #exit(0)
  os.system(f"mkdir -p '{output_directory}' ")
 
  for f in faulted_instructions:

    for e in ['BASE', 'SSE', 'SSE2', 'AVX', 'AVX2', 'AVX512', 'FMA', 'AES']:
      if os.path.isfile(f"{instruction_directory}/{e}/{f}"):
        os.system(f"cp '{instruction_directory}/{e}/{f}' '{output_directory}' ")
        break
    else:
      print(f"could not find instruction '{f}'")