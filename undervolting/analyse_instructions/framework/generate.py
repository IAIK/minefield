
# strongly modified version of xmlToAssembler.py from uops.info

from posixpath import split
import xml.etree.ElementTree as ET
import os
import subprocess

from framework.extract import extract

script_path = os.path.dirname(os.path.realpath(__file__)) 


def get_witdh_from_name(name):
   name = name.upper()

   if "MM" in name:
      if name.startswith("MM"):
         return 64
      if name[0] == 'X':
         return 128
      if name[0] == 'Y':
         return 256
      if name[0] == 'Z':
         return 512
      return None

   if len(name) == 2:
      if name[1] == 'X':
         return 16
      if name[1] == 'L':
         return 8
      if name[1] == 'H':
         return 8
      if name[1] == 'S':
         return 16
      return None

   if len(name) == 3:
      if name[0] == 'E':
         return 32
      if name[0] == 'R':
         return 64
      return None

   if len(name) == 4:
      if name[3] == 'B':
         return 8
      if name[3] == 'W':
         return 16
      if name[3] == 'D':
         return 32
      return None

   return None

def parse_asm_string(instrNode):
   asm = instrNode.attrib['asm']
   first = True
   for operandNode in instrNode.iter('operand'):
      operandIdx = int(operandNode.attrib['idx'])

      if operandNode.attrib.get('suppressed', '0') == '1':
         continue

      if not first and not operandNode.attrib.get('opmask', '') == '1':
         asm += ', '
      else:
         asm += ' '
         first = False

      operand_asm = ""

      if operandNode.attrib['type'] == 'reg':
         registers = operandNode.text.split(',')
         register = registers[min(operandIdx, len(registers)-1)]
         if not operandNode.attrib.get('opmask', '') == '1':
            operand_asm += register
         else:
            operand_asm += '{' + register + '}'
            if instrNode.attrib.get('zeroing', '') == '1':
               operand_asm += '{z}'

         asm += operand_asm

      elif operandNode.attrib['type'] == 'mem':
         memoryPrefix = operandNode.attrib.get('memory-prefix', '')
         if memoryPrefix:
            operand_asm += memoryPrefix + ' '

         if operandNode.attrib.get('VSIB', '0') != '0':
            operand_asm += '[' + operandNode.attrib.get('VSIB') + '0]'
         else:
            operand_asm += '[R11]'

         memorySuffix = operandNode.attrib.get('memory-suffix', '')
         if memorySuffix:
            operand_asm += ' ' + memorySuffix

         asm += operand_asm

      elif operandNode.attrib['type'] == 'agen':
         agen = instrNode.attrib['agen']
         address = []

         if 'R' in agen: address.append('RCX')
         if 'B' in agen: address.append('RCX')
         if 'I' in agen: address.append('2*RDX')
         if 'D' in agen: address.append('8')

         operand_asm += ' [' + '+'.join(address) + ']'

         asm += operand_asm

      elif operandNode.attrib['type'] == 'imm':
         if instrNode.attrib.get('roundc', '') == '1':
            operand_asm += '{rn-sae}, '
         elif instrNode.attrib.get('sae', '') == '1':
            operand_asm += '{sae}, '
         width = int(operandNode.attrib['width'])
         if operandNode.attrib.get('implicit', '') == '1':
            imm = operandNode.text
         else:
            imm = (1 << (width-8)) + 1
         asm += operand_asm + str(imm)

      elif operandNode.attrib['type'] == 'relbr':
         asm = '1: ' + asm + '1b'

   if not 'sae' in asm:
      if instrNode.attrib.get('roundc', '') == '1':
         asm += ', {rn-sae}'
      elif instrNode.attrib.get('sae', '') == '1':
         asm += ', {sae}'

   return asm

def parse_sources_and_sinks(instrNode):

   sources = []
   sinks = []

   for operandNode in instrNode.iter('operand'):
      operandIdx = int(operandNode.attrib['idx'])
      width = operandNode.attrib.get('width', 'none')
      operand_asm = ""
      
      register_exclude = ["MSRS", "GDTR", "TSC", "IDTR", "LDTR", "TR", "MXCSR", "X87CONTROL"]

      if operandNode.attrib['type'] == 'reg':

         registers = operandNode.text.split(',')
         register = registers[min(operandIdx, len(registers)-1)]
         if not operandNode.attrib.get('opmask', '') == '1':
            operand_asm += register
         else:
            operand_asm += '{' + register + '}'
            if instrNode.attrib.get('zeroing', '') == '1':
               operand_asm += '{z}'

         if register in register_exclude or register.startswith("CR"):
            return (sources, sinks)

         register_width = get_witdh_from_name(register)
         if register_width == None and width != 'none':
            register_width = width
         elif register_width == None and width == 'none':
            print(operand_asm)
            raise "unknown register size"

         width = str(register_width) 
      
         if operandNode.attrib.get('w', '0') == '1':
            sinks.append(("reg", int(width), operand_asm))
            
         if operandNode.attrib.get('r', '0') == '1':
            sources.append(("reg", int(width), operand_asm))


      elif operandNode.attrib['type'] == 'mem':
         memoryPrefix = operandNode.attrib.get('memory-prefix', '')
         if memoryPrefix:
            operand_asm += memoryPrefix + ' '

         if operandNode.attrib.get('VSIB', '0') != '0':
            operand_asm += '[' + operandNode.attrib.get('VSIB') + '0]'
         else:
            operand_asm += '[R11]'

         memorySuffix = operandNode.attrib.get('memory-suffix', '')
         if memorySuffix:
            operand_asm += ' ' + memorySuffix


         if operandNode.attrib.get('w', '0') == '1':
            sinks.append(("mem", int(width), operand_asm))
         
         if operandNode.attrib.get('r', '0') == '1':
            sources.append(("mem", int(width), operand_asm))


      elif operandNode.attrib['type'] == 'agen':
         agen = instrNode.attrib['agen']
         address = []

         if 'R' in agen: address.append('RCX')
         if 'B' in agen: address.append('RCX')
         if 'I' in agen: address.append('2*RDX')
         if 'D' in agen: address.append('8')

         operand_asm += ' [' + '+'.join(address) + ']'

         if width == "none":
            width = "64"

         if operandNode.attrib.get('w', '0') == '1':
            sinks.append(("agen", int(width), operand_asm))
         
         if operandNode.attrib.get('r', '0') == '1':
            sources.append(("agen",int(width), operand_asm))

      elif operandNode.attrib['type'] == 'flags':
         if operandNode.attrib.get('w', '0') == '1':
            sinks.append(("flags", 64, ""))
         
         if operandNode.attrib.get('r', '0') == '1':
            sources.append(("flags", 64, ""))

   return (sources, sinks)



def compile_instruction(asm, sources, sinks, output_directory, trap_type, random_instructions=[]):

   # generate template
   with open(f"{script_path}/../data/template.S", "r") as template:
      with open(f"{script_path}/../tmp.S", "w") as out:
         (template_asm, template_protected, template_loaded, template_stored) = build_template(asm, sources, sinks, trap_type, random_instructions)
         out.write(template.read()
            .replace("%%template_asm%%", template_asm)
            .replace("%%template_stored%%", str(template_stored))
            .replace("%%template_loaded%%", str(template_loaded))
            .replace("%%template_protected%%", str(template_protected))
         ) 
   asm_no_newline = asm.replace("\n", "|")
   trap_type_name = "" if trap_type == "" else ("_" + trap_type)

   r = subprocess.run(["g++", "-shared", f"{script_path}/../tmp.S", "-o", f"{output_directory}/{asm_no_newline}{trap_type_name}"])
   if r.returncode != 0:
      print("error during compilation!")
      print(f"asm: {asm}")
      print(f"sources: {sources}")
      print(f"sinks: {sinks}")
      c = input("continue y/n? ")
      #c = 'y'
      if c == 'n':
         exit(r.returncode)

   r = subprocess.run(["rm", f"{script_path}/../tmp.S"])
   if r.returncode != 0:
      print("error during cleanup!")
      exit(r.returncode)


def generate_instruction(file_name, output_directory_dict, extensions, faulted_instructions=None):
 
   category_exclude = ["STRINGOP", "SEMAPHORE", "SEGOP"]

   root = ET.parse(file_name)


   asm = "0x2bbb871"

   for i, instrNode in enumerate(root.iter('instruction')):

      extension = instrNode.attrib['extension']

      if extension not in extensions:
         if 'AVX512' in extensions and extension.startswith("AVX512"):
            extension = 'AVX512'
            pass
         else:
            continue
         
      if any(x in instrNode.attrib['isa-set'] for x in ['BF16_', 'VP2INTERSECT']):
         continue

      if instrNode.attrib['cpl'] != '3':
         continue

      if instrNode.attrib['category'] in category_exclude:
         continue

      asm = parse_asm_string(instrNode)


      # skip calls and jumps
      if asm == "" or "CALL" in asm or "1:" in asm or "JMP" in asm or "SAVE" in asm:
         continue

      sources, sinks = parse_sources_and_sinks(instrNode)

      if faulted_instructions is not None:
         if not asm in faulted_instructions:
            continue
         yield (asm, sources, sinks)
         continue

      compile_instruction(asm, sources, sinks, output_directory_dict[extension], "")
      compile_instruction(asm, sources, sinks, output_directory_dict[extension], "AES")
      compile_instruction(asm, sources, sinks, output_directory_dict[extension], "OR")

      yield (asm, sources, sinks)

width_sufix = { 8 : "b", 16 : "w", 32 : "d", 64 : ""}  
width_prefix = { 80: "X", 128 : "X", 256 : "Y", 512 : "Z" }

def handle_higher_part_registers(operand, width):
   # handle AH etc
   if width == 8 and len(operand) == 2 and operand[1].upper() == 'H':
      operand = f"R{operand[0]}X"
      width = 64
   return (operand, width)

def get_move_instruction(operand, width):
   if operand.startswith("K"):
      return "kmovq"

   if width == 512:
      return "VMOVDQU32"
   elif width > 64:
      return "vmovdqu"
   elif operand.startswith("MM"):
      return "movq"
   else:
      return "mov"

def convert_mask_register(operand):
   if "{" in operand or "}" in operand:
      if operand.endswith("{z}"):
         return operand[1:-4]
      return operand[1:-1]
   else:
      return operand

def load_address(loaded, width):
   byte = (width + 7)//8
   return f"[R8 + R9*8 -{(byte+loaded*8) }]"

def load_register(code, operand, width, loaded):
   (operand, width) = handle_higher_part_registers(operand, width)

   return (code + f"{get_move_instruction(operand, width)} {operand}, {load_address(loaded, width)}\n", loaded + (width + 63)//64)

def load_memory(code, operand, width, loaded):
   if "[R11]" in operand:
      return (code, loaded + (width + 63)//64) # handled in template
   print(f"load memory unexpected: {operand}")
   return (code, loaded + (width + 63)//64)

def store_address(stored, width):
   byte = (width + 7)//8
   return f"[R12 + R13*8 -{(byte+stored*8) }]"

def store_register(code, operand, width, stored):
   # handle AH logic
   (operand, width) = handle_higher_part_registers(operand, width)

   if operand == "RIP":
      print("cannot store RIP, skipping")
      return (code, stored)

   width = max(width, 64)

   return (code + f"{get_move_instruction(operand, width)} {store_address(stored, width)}, {operand}\n", stored + (width + 63)//64)

def store_memory(code, operand, width, stored):
   instr = get_move_instruction(operand, width)

   tmp_register = f"{width_prefix[width]}MM14" if width > 64 else f"R14{width_sufix[width]}"

   code += f"{instr} {tmp_register}, {operand}\n"

   return store_register(code, tmp_register, width, stored)

def build_template(asm, sources, sinks, trap_type, random_instructions):

   # the number of uint64_ts stored
   loaded = 0
   stored = 0 

   # check if the instruction reads the flags register
   reads_flags = False
   for type, _, _ in sources:
      if type == "flags":
         reads_flags = True

   # check if the instruction writes the flags register
   writes_flags = False
   for type, _, _ in sinks:
      if type == "flags":
         writes_flags = True

   # if it reads the flags ensure that we always have the same flags for each iteration
   if reads_flags:
      ins = "CMP R13b, 64\n"
      asm = ins + asm

   # generate the pre init code
   pre = "".join(random_instructions)
   for type, width, operand in sources:
      operand = convert_mask_register(operand)

      if type == "flags":
         pass
      elif type == "reg":
         (pre, loaded) = load_register(pre, operand, width, loaded)
      elif type == "mem":
         (pre, loaded) = load_memory(pre, operand, width, loaded)
      elif type == "agen":
         pass # agens are never dereferenced therefore we can skip them
      else:
         print(type)
         raise "unimplemented type"
   
   # if the instruction updates the flags register store it
   post = ""
   if writes_flags:
      post += "PUSHF\n"
      post += "POP R14\n"
      (post, stored) = store_register(post, "R14", 64, stored)

   # store the sinks
   for type, width, operand in sinks:
      operand = convert_mask_register(operand)
      
      if type == "flags":
         continue
      elif type == "reg":
         (post, stored) = store_register(post, operand, width, stored)
      elif type == "mem":
         (post, stored) = store_memory(post, operand, width, stored)
      elif type == "agen":
         raise "agen as sink, how?"
      else:
         print(type)
         raise "unimplemented type"

   # add the loop code
   loop = ""
   loop += f"SUB R9, {loaded}\n"
   loop += f"SUB R13, {stored}\n"
   loop += "JNZ 2b\n"
   loop += "XOR RAX, RAX\n"

   # combineq
   asm_not_protected = f"2:{pre}{asm}\n{post}{loop}\n"

   trap = ""
   cmp_code = ""

   if trap_type == "AES":
      trap += f"AESENC xmm5, xmm5\n"

      cmp_code += "pextrq rax, xmm5, 0\n"
      cmp_code += "pextrq rdx, xmm5, 1\n"

   if trap_type == "OR":
      trap += f"VORPD xmm4, xmm4, xmm4\n"

      cmp_code += "pextrq rax, xmm4, 0\n"
      cmp_code += "pextrq rdx, xmm4, 1\n"

   elif trap_type == "":
      trap += f"imul RSI, R15\n"

      cmp_code += "mov rax, rsi\n"
      cmp_code += "mov rdx,   0\n"
   else:
      cmp_code += "xor rax, rax\n"
      cmp_code += "xor rdx, rdx\n"
   
   # combine
   asm_protected = f"2:{pre}{asm}\n{post}{trap}{loop}{cmp_code}"

   return (asm_not_protected, asm_protected, loaded, stored)

def generator_generate(input_xml, output_directory, extensions):

   output_directory_dict = {}
   for e in extensions:
      output_directory_dict[e] = f"{output_directory}/{e}"
      os.system(f"mkdir -p '{output_directory}/{e}' ")

   if 'RANDOM' in extensions:
      faulted_instructions = [
         "IMUL RCX, RDX",
         "AESENC XMM1, XMM2", 
         "VANDNPD XMM1, XMM2, XMM3", 
         "VANDPD XMM1, XMM2, XMM3", 
         "VORPD XMM1, XMM2, XMM3",
         "VXORPD XMM1, XMM2, XMM3",
         "VPCLMULQDQ XMM1, XMM2, XMM3, 2",
         "VPSRAD XMM1, XMM2, XMM3",
         "VPMAXSD XMM1, XMM2, XMM3",
         "VPCMPGTD XMM1, XMM2, XMM3",
         "VSQRTPD XMM1, XMM2",
         "VPADDQ XMM1, XMM2, XMM3",
      ]

      for i, (asm, sources, sinks) in enumerate(generate_instruction(input_xml, output_directory_dict, ['BASE', 'SSE', 'SSE2', 'AVX', 'AVX2', 'AVX512', 'AES', 'FMA'], faulted_instructions)):
         d = output_directory_dict['RANDOM'] + "/" + asm.split(" ")[0]

         os.system(f"mkdir -p '{d}' ")

         compile_instruction(asm,  sources, sinks, d, "reference", []) 
         compile_instruction(asm,  sources, sinks, d, "lfence",    ["lfence\n"]) 
         compile_instruction(asm,  sources, sinks, d, "avx",       ["VORPD YMM1, YMM2, YMM3\n"])
         #compile_instruction(asm,  sources, sinks, output_directory_dict['RANDOM'], "cpuid", ["cpuid\n"])

         for i, f in enumerate(faulted_instructions):
            compile_instruction(asm,  sources, sinks, d, f"f{i}", [f"{f}\n"])

      exit(0)

   for i, (asm,_,_) in enumerate(generate_instruction(input_xml, output_directory_dict, extensions)):
      print(f"{i: <6}{asm: <50}")
