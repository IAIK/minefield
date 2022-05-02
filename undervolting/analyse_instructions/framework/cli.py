import click
from click_option_group import AllOptionGroup, RequiredMutuallyExclusiveOptionGroup, OptionGroup

def pack(*param_decls, **attrs):
  return (param_decls, attrs)

def combine(group, *decs):
  options = []
  for (kwargs, attrs) in decs:
    options.append(group.option(*kwargs, **attrs))

  def combined_decorator(f):
    for dec in reversed(options):
      f = dec(f)
    return f
  return combined_decorator


input_instructions = click.option('--input', 'input_instructions',
  help="input instruction or directory of instructions",
  type=click.Path(exists=True, file_okay=True, dir_okay=True),
  required=True
)


input_log = click.option('--input', 'input_log',
  help="input log file or directory",
  type=click.Path(exists=True, file_okay=True, dir_okay=True),
  required=True,
  multiple=True
)

latex_file = click.option('--latex', 'latex_file',
  help="",
  type=click.Path(file_okay=True, dir_okay=False),
  required=False
)


file_list = click.option('--select', 'select',
  help="selected file names to run",
  type=click.Path(exists=True, file_okay=True, dir_okay=False)
)

operating_points = click.option(
  '--op', 'operating_points',
  help="operating point (frequency,voltage_offset)",
  type=(int,int),
  multiple=True
)

undervolting_config = combine(OptionGroup('undervolting configuration', help=""), 
  pack(
    '--vo_start', 'vo_start',
    help="start undervolting offset in mV",
    type=int
  ),
  pack(
    '--vo_end', 'vo_end',
    help="end undervolting offset in mV",
    type=int,
  ),
  pack(
    '--vo_step', 'vo_step',
    help="undervolting step in mV",
    type=int,
    default=-1
  )
)

frequency_config = combine(OptionGroup('frequency configuration', help=""), 
  pack(
    '--freq_start', 'freq_start',
    help="start frequency in MHz",
    type=int
  ),
  pack(
    '--freq_end', 'freq_end',
    help="end frequency in MHz",
    type=int,
  ),
  pack(
    '--freq_step', 'freq_step',
    help="frequency step in MHz",
    type=int,
    default=100
  ),
)



freq_start = click.option(
  '--freq_start', 'freq_start',
  help="start frequency in MHz",
  type=int,
  required=True
)

freq_end = click.option(
  '--freq_end', 'freq_end',
  help="end frequency in MHz",
  type=int,
)

freq_step = click.option(
  '--freq_step', 'freq_step',
  help="frequency step in MHz",
  type=int,
  default=100
)

core = click.option('--core', 'core',
  help="core",
  type=int,
  default=0
)

rounds = click.option('--rounds', 'rounds',
  help="number of rounds",
  type=int,
  default=5
)

iterations = click.option('--iterations', 'iterations',
  help="number of iterations per round",
  type=int,
  default=900000
)

skip_until = click.option('--skip_until', 'skip_until',
  help="skip until specified instruction",
  type=str,
)

find_faulting_points = click.option('--find_faulting_points', 'find_faulting_points',
  help="continue with next undervolting offset after fault was found",
  type=bool,
  default=False,
  is_flag=True
)

verbose = click.option('--verbose', 'verbose',
  help="print additional information",
  type=bool,
  default=False,
  is_flag=True
)

class BasedIntParamType(click.ParamType):
  name = "integer"

  def convert(self, value, param, ctx):
    if isinstance(value, int):
      return value

    try:
      if value[:2].lower() == "0x":
        return int(value[2:], 16)
      elif value[:1] == "0":
        return int(value, 8)
      return int(value, 10)
    except ValueError:
      self.fail(f"{value!r} is not a valid integer", param, ctx)

BASED_INT = BasedIntParamType()

trap_factor = click.option('--trap_factor', 'trap_factor',
  help="trap factor",
  type=BASED_INT,
  default=0x2bbb871
)

trap_init = click.option('--trap_init', 'trap_init',
  help="trap init",
  type=BASED_INT,
  default=0x55555555555
)

remote_config = combine(AllOptionGroup('remote configuration', help=""), 
  pack(
    '--pdu', 'pdu',
    help="ip address of the raspberry pdu",
    type=str,
  ),
  pack(
    '--remote', 'remote',
    help="ip address of the remote",
    type=str
  ),
  pack(
    '--log_dir', 'log_dir',
    help="directory to store log files",
    type=click.Path(exists=True,file_okay=False,dir_okay=True)
  ),
  pack(
    '--timeout', 'timeout',
    help="timeout of the remote ssh connections in seconds",
    type=int
  ),
  pack(
    '--retry_count', 'retry_count',
    help="number of retries after timeouts",
    type=int
  ),
)