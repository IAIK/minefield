import click
import os
import sys
import datetime

from framework.generate import generator_generate
from framework.undervolt import undervolt_run, undervolt_filter
from framework.remote import remote_run
from framework.plot import plot_faults
from framework.plot_new import plot_faults_new
from framework.extract import extract

import framework.cli as cli

script_path = os.path.dirname(os.path.realpath(__file__))

class Logger(object):
  def __init__(self, log_dir):
    log_name = datetime.datetime.now().strftime("results_%Y.%m.%d_%H:%M:%S")
    log_file = open(f"{log_dir}/{log_name}", "w")

    if log_file.closed:
      print("error creating log!")
      exit(-1)
      
    self.terminal = sys.stdout
    self.log = log_file

  def write(self, message):
    self.terminal.write(message)
    self.terminal.flush()
    self.log.write(message)
    self.log.flush()

  def flush(self):
    if not self.terminal.closed:
      self.terminal.flush()
    if not self.log.closed:
      self.log.flush()  

  def restore(self):
    sys.stdout = self.terminal

def resolve_files(path, recursively=False):
  if os.path.isfile(path):
    return [path]
  else:
    if recursively:
      result = []
      for root, dirs, files in os.walk(path):
        result.extend([os.path.join(root, f) for f in files])
      return result
    else:
      return [path + "/" + file for file in sorted(os.listdir(path)) if os.path.isfile(path + "/" + file)]
  
# Groups

@click.group()
def group_cli():
  pass

# Commands:

######################################
# generator generate
######################################

@group_cli.command("generate", help="generate instructions from XML file")
@click.option('-i', '--input', 'input_xml',
  help="XML instruction file",
  type=click.Path(exists=True,file_okay=True,dir_okay=False),
  default=f"{script_path}/data/instructions.xml"
)
@click.option('-o', '--output', 'output_directory',
  help="output directory for the generated instructions",
  type=click.Path(file_okay=False,dir_okay=True),
  default=f"{script_path}/instructions/all/"
)
@click.option('-e', '--extensions', 'extensions',
  help="the extensions to generate",
  multiple=True,
  type=click.Choice(['BASE', 'SSE', 'SSE2', 'AVX', 'AVX2', 'AVX512', 'AES', 'FMA', 'RANDOM'], case_sensitive=False),
  default=['BASE', 'SSE', 'SSE2', 'AVX', 'AVX2', 'AES', 'FMA', 'RANDOM'], #AVX512 is missing
  show_default=True
)
def command_generate(input_xml, output_directory, extensions):
  """"""
  generator_generate(input_xml, output_directory, extensions)
  


######################################
# generator filter
######################################

@group_cli.command("filter")
@click.option('-i', '--input', 'input_directory',
  help="instruction input directory",
  type=click.Path(exists=True,file_okay=False,dir_okay=True),
  default=f"{script_path}/instructions/all/"
)
@click.option('-or', '--out-runnable', 'runnable_directory',
  help="output directory for runnable instructions",
  type=click.Path(file_okay=False,dir_okay=True),
  default=f"{script_path}/instructions/runnable/"
)
@click.option('-on', '--out-non-runnable', 'non_runnable_directory',
  help="output directory for non-runnable instructions",
  type=click.Path(file_okay=False,dir_okay=True),
  default=f"{script_path}/instructions/non-runnable/"
)
@click.option('-e', '--extensions', 'extensions',
  help="the extensions to generate",
  multiple=True,
  type=click.Choice(['BASE', 'SSE', 'SSE2', 'AVX', 'AVX2', 'AVX512', 'AES', 'FMA', 'RANDOM'], case_sensitive=False),
  default=['BASE', 'SSE', 'SSE2', 'AVX', 'AVX2', 'AES', 'FMA', 'RANDOM'], #AVX512 is missing
  show_default=True
)
def command_filter(input_directory, runnable_directory, non_runnable_directory, extensions):
  """filter instructions into runnable and non-runnable instructions"""
  undervolt_filter(input_directory, runnable_directory, non_runnable_directory, extensions)


######################################
# undervolt
######################################
@group_cli.command("undervolt", help="undervolt instructions")
@cli.input_instructions
@cli.operating_points
@cli.undervolting_config
@cli.frequency_config
@cli.core
@cli.rounds
@cli.iterations
@cli.skip_until
@cli.find_faulting_points
@cli.verbose
@cli.remote_config
@cli.trap_factor
@cli.trap_init
@click.pass_context
def command_undervolt(ctx, input_instructions, operating_points, vo_start, vo_end, vo_step, freq_start, freq_end, freq_step, core, rounds, iterations, skip_until, find_faulting_points, verbose, pdu, remote, log_dir, timeout, retry_count, trap_factor, trap_init):

  if (vo_start is not None and freq_start is not None) and len(operating_points) == 0:
    pass
  elif (vo_start is None and freq_start is None) and len(operating_points) != 0:
    pass
  else:
    raise click.UsageError("set either (start voltage and start frequency) or the (operating points)", ctx=ctx)
  
  if len(operating_points) == 0:
    frequencies = list(range(freq_start, (freq_start if freq_end is None else freq_end) + freq_step, freq_step))
    vo_starts   = [vo_start] * len(frequencies)

    operating_points = list(zip(frequencies, vo_starts))
  else:
    operating_points = list(operating_points)
    
  prev_connection = None

  files = resolve_files(input_instructions)

  if log_dir is not None:
    sys.stdout = Logger(log_dir)



  for freq, vo_start in operating_points:

    files_to_run = files.copy()

    voltage_range = list(range(vo_start, (vo_start if vo_end is None else vo_end) + vo_step, vo_step))
    current_vo_step = vo_step
    
    binary_searching = False

    while len(voltage_range) != 0:

      current_voltage = voltage_range.pop(0)

      if remote is None:
        undervolt_run(files_to_run, current_voltage, freq, core, rounds, iterations, skip_until, verbose, trap_factor, trap_init)
      else:
        prev_connection, faulted_files, skipped_files = remote_run(files_to_run, current_voltage, freq, core, rounds, iterations, skip_until, verbose, remote, pdu, timeout, retry_count, prev_connection, trap_factor, trap_init)
        
      
        if find_faulting_points:
          if abs(current_vo_step) == 1:
            for f in faulted_files:
              #print(f"file {f} removed from runnables since it faulted")
              files_to_run.remove(f)

          for f in skipped_files:
            #print(f"file {f} removed from runnables since it timed out")
            files_to_run.remove(f)


          if len(files) == 1:
            faulted_or_timedout = files[0] in faulted_files or files[0] in skipped_files

          # if faulted or timed out reduce step and role back
            if (faulted_or_timedout or binary_searching) and abs(current_vo_step) != 1:

              if faulted_or_timedout:
                current_voltage -= current_vo_step//2
              else:
                current_voltage += current_vo_step//2

              voltage_range = list(range(current_voltage, current_voltage + vo_step, current_vo_step//2))
              #print(f"> changed voltage step from {current_vo_step} to {current_vo_step//2}")
              current_vo_step = current_vo_step//2
              binary_searching = True
            
            # we are done binary searching 
            elif binary_searching and abs(current_vo_step) == 1 and files[0] in files_to_run:
              files_to_run.remove(files[0])


        if len(files_to_run) == 0:
          break

  if log_dir is not None:
    sys.stdout.restore()

  if prev_connection is not None:
    prev_connection[0].close()

######################################
# plot
######################################
@group_cli.command("plot", help="plot fault results")
@cli.input_log
@cli.latex_file
def command_show(input_log, latex_file):
  files = []
  for i in input_log:
    files.extend(resolve_files(i))
  plot_faults_new(files, latex_file)

######################################
# extract
######################################
@group_cli.command("extract", help="extract faulted instructions")
@cli.input_log
@click.option('--instruction_directory', 'instruction_directory',
  help="output directory for faulted instructions",
  type=click.Path(file_okay=False,dir_okay=True),
  default=f"{script_path}/instructions/all/"
)
@click.option('--output_directory', 'output_directory',
  help="output directory for faulted instructions",
  type=click.Path(file_okay=False,dir_okay=True),
  default=f"{script_path}/instructions/faulted/"
)
def command_show(input_log, instruction_directory, output_directory):
  log_files = []
  for i in input_log:
    log_files.extend(resolve_files(i, True))
  extract(log_files, instruction_directory, output_directory)

if __name__ == '__main__':
  group_cli(obj={})
