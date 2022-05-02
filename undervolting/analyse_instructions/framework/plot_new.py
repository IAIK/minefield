import os
from matplotlib.pyplot import subplots
import numpy as np
import pandas as pd
#import pdb
#import seaborn as snb
from pandas.core.dtypes import dtypes

def parse_array(s):
  return s.split("[")[1].split("]")[0].split(",")

def parse_dict(s):
  data = {}
  for p in s.split("{")[1].split("}")[0].split(","):
    pp = p.split("=")
    if pp[0] == '':
      continue
    data[pp[0].strip()] = "=".join(pp[1:])
  return data


def file_get_faults(file):
  df = None 
  i = 0
  with open(file, "r") as f:
    print(file)
    for l in f.readlines():
      if "X faulted" in l or "OK" in l:
        timeout = False
        pass
      #elif "T timed out" in l:
      #  timeout = True
      #  pass
      else:
        continue

      data = {}

      parts = l.split(";")

      if len(parts) < 8:
        continue

      data['time'] = parts[0].strip()
      data['state'] = parts[1].strip()
      data['frequency_set'] = int(parts[2])
      data['voltage_offset_set'] = int(parts[3])
      data['core'] = int(parts[4])
      data['rounds'] = int(parts[5])
      data['iterations'] = int(parts[6])
      data['instruction'] = parts[7].strip()
      
      for p in parts[8:]:
        if "=" in p:
          pp = p.split("=")
          data[pp[0].strip()] = "=".join(pp[1:]).strip()

      if not 'v' in data and not 'f' in data:
        continue

      voltages = parse_array(data['v'])
      
    
      data['v_min'] = voltages[0]
      data['v_avg'] = voltages[1]
      data['v_max'] = voltages[2]
      
      frequencies = parse_array(data['f'])

      data['f_min'] = frequencies[0]
      data['f_avg'] = frequencies[1]
      data['f_max'] = frequencies[2]
      
      if 'hist' in data:
        data['hist'] = parse_dict(data['hist'])

      if 'mask' in data:
        data['mask'] = int(data['mask'].replace('_', ''), 16)

      if 'ticks' in data:
        data['faults_per_tick'] = float(data['faults']) / float(data['ticks']) * 1e6

      data['to_0'] = parse_dict(data['to_0'])
      data['to_1'] = parse_dict(data['to_1'])

      if data['instruction'].endswith("_AES"):
        data['instruction'] = data['instruction'].split("_AES")[0]
        data['trap_type'] = 'aes'
      if data['instruction'].endswith("_OR"):
        data['instruction'] = data['instruction'].split("_OR")[0]
        data['trap_type'] = 'or'
      else:
        data['trap_type'] = 'imul'


      # cleanup
      data.pop('v')
      data.pop('f')
      #data.pop('to_1')
      #data.pop('to_0')
      #data.pop('mask')
      data.pop('time')
      data.pop('bc')
      #data.pop('rounds')
      #data.pop('iterations')

      if df is None:
        df = pd.DataFrame(columns=data.keys())

      df.loc[i] = data
      i += 1

  return df

def agg_voltages(group):
  return group.agg({'v_min' : 'max', 'v_avg' :'max', 'v_max' :'max', 'voltage_offset_set' : 'max'}).reset_index()

def reduce_volatages(x):
    d = {}

    # all the samples in x are faulted

    # get the minimum undervolt
    vo_max_fault = x['voltage_offset_set'].max()

    #d['v_avg']             = x['v_avg'].max()
    d['voltage_offset_set'] = vo_max_fault

    #d['detected_meas'] = x['detected'].iloc[np.argmax(x['v_avg'])] > 0
    
    # check for multiple entries with the same undervolt
    max_vos_indicies = np.flatnonzero(x['voltage_offset_set'] == vo_max_fault)

    # get the detection values for these undervolts
    detected_at_vo = x['detected'].iloc[max_vos_indicies]

    # find the entry where we detected something ... if any
    detected_i = max_vos_indicies[np.argmax(detected_at_vo)]

    # distrbitue that detected index
    d['detected_set'] = x['detected'].iloc[detected_i]
    d['faults'] = x['faults'].iloc[detected_i]
    d['trap_type'] = x['trap_type'].iloc[detected_i]

    d['max_faults'] = x['faults'].iloc[detected_i]
    d['max_fault_rate'] = x['faults'].iloc[detected_i] / (x['iterations'].iloc[detected_i]*x['rounds'].iloc[detected_i])

    if x['ticks'].iloc[detected_i] != 0:
      d['max_fault_rate_time'] = -1#x['faults'].iloc[detected_i] / int(x['ticks'].iloc[detected_i])
    else:
      d['max_fault_rate_time'] = -1

    d['iterations'] = x['iterations'].iloc[detected_i]*x['rounds'].iloc[detected_i]
    d['ticks'] = x['ticks'].iloc[detected_i]

    
    #d['detected_set'] = x['detected'].iloc[np.argmax(x['voltage_offset_set'])] > 0
    #if len(np.argmax(x['voltage_offset_set'])) > 1:
    #  print(np.argmax(x['voltage_offset_set']))
    #  exit(0)


    #d['faults'] = x['faults'].iloc[np.argmax(x['voltage_offset_set'])]


    #d['trap_type'] = x['trap_type'].iloc[np.argmax(x['voltage_offset_set'])]

    return pd.Series(d, index=['v_avg', 'voltage_offset_set', 'detected_set', 'trap_type', 'max_faults', 'max_fault_rate', 'iterations'])

def reduce_max_faults(x):
    d = {}
    i = np.argmax(x['faults'])
    d['max_faults'] = x['faults'].iloc[i] 
    d['max_fault_rate'] = x['faults'].iloc[i] / x['iterations'].iloc[i]

    return pd.Series(d, index=['max_faults', 'max_fault_rate'])



def reduce_mask(x):
    d = {}

    d['iterations'] = (x['iterations'] * x['rounds']).sum()
    d['faults'] = x['faults'].sum()

    d['mask'] = hex(np.bitwise_or.reduce(x['mask']))

    to_0 = 0
    to_1 = 0

    for index, r in x.iterrows():
      for (k, v) in r['to_0'].items():
        to_0 += int(v)

      for (k, v) in r['to_1'].items():
        to_1 += int(v)

    if (to_1 + to_0) == 0:
      d['p_to_0'] = -1
    else:
      d['p_to_0'] = to_0 / (to_1 + to_0)


    return pd.Series(d, index=['iterations', 'faults', 'mask', 'p_to_0'])


def calc_performance(df):

  undetected = 0
  detected = 0

  n = 'v_avg'
  #n = 'voltage_offset_set'

  for instruction, items1 in df.groupby(['instruction']):
    for freq, items2 in items1.groupby(['f_avg']):
      for core, items3 in items2.groupby(['core']):
          row = int(items3[items3[n] == items3[n].max()].iloc[0]["detected"])
          if row == 0 and not instruction.startswith("IMUL"):
            print(f"undetected {instruction} {freq} {core}")
            undetected += 1
          else:
            detected += 1

  if undetected+detected != 0:
    print(f"rate {detected/(undetected+detected)}")

  pass

def get_trap_symbol(instruction, data):
  if instruction.startswith("IMUL"):
    return "\\imultrap"

  if data['detected_set'] == 0:
    if instruction.startswith("VOR"):
      # we can always detect ors with the or trap ...
      return "\\ortrap"

    return "\\failed"
  if data['trap_type'] == 'imul':
    return "\\imultrap"
  if data['trap_type'] == 'or':
    return "\\ortrap"
  if data['trap_type'] == 'aes':
    return "\\aestrap"

  raise "error"

def to_latex(df, latex_file):

  with open(latex_file, "w") as latex:

    def w(x, c):
      return enumerate(x.groupby([c]))

    instructions = [
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

    for fi, (freq, items) in w(df, 'f_avg'):
      if fi != 0:
        latex.write("\\freqend\n")
        
      line = f" & {freq:4.0f} &"

      for ci, (core, items) in w(items, 'core'):
        if ci != 0:
          line = "&      &"
        line += f" {core:2d} "

        instr_dict = {}

        for ii, (instruction, items) in w(items, 'instruction'):
          if len(list(items.iterrows())) != 1:
            print(items)
            print("error")
            exit(0)

          value_row = ""

          for _,row in items.iterrows():
          
            detected =  get_trap_symbol(instruction, row)
            v_set = f"{-row['voltage_offset_set']:.0f}"
            value_row = f"& {detected} & \\SIx{{{v_set}}}"# & \\SIx{{{v_meas}}} "

          instr_dict[instruction] = value_row

        any_instruction = False
        for instruction in instructions:
          if instruction in instr_dict:
            line += instr_dict[instruction]
            any_instruction = True
          else:
            line += "&         &           "

        line += "\\\\"

        #if any_instruction:
        latex.write(line)



def plot_faults_new(files, latex_file): 

  import matplotlib
  import matplotlib.pyplot as plt

  df = None 

  for f in files:
    df_new = file_get_faults(f)
    if df is None:
      df = df_new
    else:
      df = df.append(df_new, ignore_index=True)

  if df is None:
    print("no samples")
    return

  df = df.astype({
    'voltage_offset_set' : 'float', 
    'v_min': 'float', 
    'v_avg' : 'float', 
    'v_max' : 'float', 
    'f_min': 'float', 
    'f_avg' : 'float', 
    'f_max' : 'float', 
    'frequency_set' : 'float', 
    'detected' : 'int',
    'faults' : 'int'
  })

  if 'v_base' in df:
    df = df.astype({'v_base': 'float'})
    
  for freq, items in df.groupby('f_avg'):
    if 'v_base' in df:
      v = np.mean(items['v_base'])
    else:
      v = np.mean(items['v_avg'] - items['voltage_offset_set']/1000)
    
    df.loc[df['f_avg'] == freq, 'v_avg'] = (df[df['f_avg'] == freq]['v_avg'] - v) * 1000


  df_faulted  = df[df['state'] == 'X faulted']
  df_detected = df.loc[df['detected'] > 0]




  calc_performance(df_faulted)

  detection_rate = (df_faulted['detected'] > 0).sum() / len(df_faulted.index) * 100

  df_faulted2  = df_faulted.groupby(['instruction']).apply(reduce_volatages)

  df_faulted  = df_faulted.groupby(['instruction','f_avg','core']).apply(reduce_volatages)
  df_detected = df_detected.groupby(['f_avg','core']).apply(reduce_volatages)

  ncores = len(df.groupby(['core']))

  #
  #  print("Faulted:")
  #  print(df_faulted)


  if latex_file is not None:
    to_latex(df_faulted, latex_file)
  with pd.option_context('display.max_rows', None, 'display.max_columns', None):  # more options can be specified also
    pd.set_option('display.expand_frame_repr', False)
    print(df_faulted)
    #print(df_faulted[df_faulted['detected_set'] != df_faulted['detected_meas']])

    df_max_faults = df.groupby(['instruction']).apply(reduce_max_faults)
    df_mask = df.groupby(['instruction']).apply(reduce_mask)
    #print(df_mask)
    for instruction, items in df_mask.groupby(['instruction']):
      for i, r in items.iterrows():
        if r['p_to_0'] < 0:
          continue
        m = int(r['mask'],16)
        print(f"{instruction:<50} & \\texttt{{0x{m:032x}}} & \SI{{{(1-r['p_to_0'])*100:.1f}}}{{\\percent}}")

    print(df_faulted2)

    #for instruction, items in df_max_faults.groupby(['instruction']):
    #  for i, r in items.iterrows():
    #    print(f"{instruction} {r['max_faults']} {r['max_fault_rate']}")

    #if 'faults_per_tick' in df:
    #  for i, r in df.iterrows():
    #    print(f" {r['instruction']} -> {r['faults']} | {r['faults_per_tick']} | {r['voltage_offset_set']} v {r['v_avg']}")
  exit(0)

  

  for vkey in ['v_avg', 'voltage_offset_set']:

    fig, ax = plt.subplots(nrows=1 if ncores == 1 else 2, ncols=(ncores+1)//2,sharex=True,sharey=True)
    try:
      ax = ax.flatten()
    except:
      ax = [ax]
      pass

    colors = {}

    for i, (core, items) in enumerate(df_faulted.groupby(['core'])):
      for j, (instruction, items) in enumerate(items.groupby(['instruction'])):
        if instruction not in colors:
          colors[instruction] = None
        
        p = ax[i].plot(items['f_avg'], items[vkey], 'x-', label=instruction, color=colors[instruction])

        if colors[instruction] is None:
          colors[instruction] = p[-1].get_color()

      ax[i].grid(True)
      #ax[i].legend()
    
    for i, (core, items) in enumerate(df_detected.groupby(['core'])):
      #for j, (instruction, items) in enumerate(items.groupby(['instruction'])):
      ax[i].plot(items['f_avg'], items[vkey], 'o-', label=f'minefield {instruction}')
      ax[i].grid(True)
      plt.legend(loc='upper left')
      #ax[i].legend()

    ax[0].set_title(f"results with {vkey}")
    plt.legend(loc='upper left')
    #lines_labels = [ax.get_legend_handles_labels() for ax in fig.axes]
    #lines, labels = [sum(lol, []) for lol in zip(*lines_labels)]
    #fig.legend(lines, labels, )


  print(f"detection rate {detection_rate}%")

  if "hist" in df:
    N = np.max(df['iterations'])

    plt.figure()
    #fig, axis = plt.subplots(nrows=2,ncols=1)
    for i, (instruction, items) in enumerate(df.groupby(['instruction'])):

      histogram = np.zeros((N,))
      
      for hist in items['hist']:
        bins = np.array(list(hist.keys())).astype(np.int)
        counts = np.array(list(hist.values())).astype(np.int)
        histogram[bins] += counts

      ticks = np.median(np.array(items['ticks']).astype(np.int))
      print(f"{instruction} = {ticks}")

      hist_raw = []

      

      for i, h in enumerate(histogram):
        if h != 0:
          hist_raw.extend([i]*int(h))

      hist_raw = np.array(hist_raw)/N * ticks
      snb.distplot(hist_raw,label=instruction)
      continue


      N2 = 50 #N//1000

      histogram = np.reshape(histogram, (N2,-1))

      histogram = np.sum(histogram, axis=1)
      histogram = histogram / np.sum(histogram)

      time = np.arange(N2)/N2 * ticks

      plt.plot(time, histogram, label=instruction)

    plt.grid()
    plt.legend()
    plt.xlabel("time")
    plt.ylabel("density")
    print(df['ticks'])

  plt.show()