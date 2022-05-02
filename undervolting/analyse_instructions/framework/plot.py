import os
from matplotlib.pyplot import subplots
import numpy as np
import pandas as pd
import pdb

from pandas.core.dtypes import dtypes

def file_get_faults(fault_dict, file, used_cores):

  df = pd.DataFrame(columns=('Instruction', 'Core', 'Frequency', 'UV', 'UV_TO'))

  i = 0

  with open(file, "r") as f:
    for l in f.readlines():
      if "X faulted" in l:
        timeout = False
        pass
      elif "T timed out" in l:
        timeout = True
        pass
      else:
        continue

      print(l,end='')
      p = list(filter(lambda x: x != "", l.split(" ")))

      #if "faults=" in l:
      #  faults = int(l.split("faults=")[1].split("mask=")[0])
      #else:
      #  faults = 0
      if len(p) < 4:
        continue
      instruction = p[4] #' '.join(l.split("X faulted")[0].split(" ")[4:]) #p[4:6] #' '.join(l.split("X faulted")[0].split(" ")[5:])
      frequency = int(p[1])

      if True and "voltage=" in l:
        voltage_offset = l.split("voltage=")[1].split(",")[2].split("]")[0]
      else:
        voltage_offset = int(p[2])
        
      core = int(p[3])

      if timeout:
        df.loc[i] = [instruction, core, frequency, np.NaN, voltage_offset]
      else:
        df.loc[i] = [instruction, core, frequency, voltage_offset, np.NaN]

      used_cores[core] = 1
      i += 1

  return df, used_cores


def distance(a, b):
  return abs(a-b)

def plot_fault_spectrum(fault_dict, ncores):
  import matplotlib
  import matplotlib.pyplot as plt

  axis = []

  fig, axis = plt.subplots(2, ncols=(ncores+1)//2,sharex=True,sharey=True)
  axis = axis.flatten()

  
  for instruction, core_dict in fault_dict.items():
    global_frequencies = []
    global_offsets = []

    for core, freq_dict in core_dict.items():
      
      freqeuncies = []
      offsets = []


      for freq, vos in freq_dict.items():

        freqeuncies.append(freq)
        offsets.append(np.max(vos))
        if False:
          freqeuncies.extend([freq] * len(vos))      
          voltage_offsets.extend(vos)

          freqeuncies.append(freq)
          voltage_mean.append(np.mean(vos))
          voltage_range.append([distance(np.mean(vos), np.min(vos)), distance(np.mean(vos), np.max(vos)) ])

      global_frequencies.append(freqeuncies)
      global_offsets.append()

        #freqeuncies.append(freq)
        #voltage_offsets.append(np.max(np.array(vos)))

      axis[core].scatter(freqeuncies, offsets, label=instruction, marker='x' if instruction == "IMUL" else 'o')
      axis[core].set_xticks(freqeuncies)

    #print(voltage_range)

    #axis[0].errorbar(freqeuncies, voltage_mean, yerr=np.array(voltage_range).T, label=instruction,fmt='x' if instruction == "IMUL" else 'o')

  font = {
          'weight' : 'bold',
          'size'   : 15}

  matplotlib.rc('font', **font)

  for core, a in enumerate(axis):
    #plt.stem(freqeuncies, voltage_offsets,label=instruction)
    a.set_title(f"Core {core}")
    a.set_xlabel("frequency MHz")
    a.set_ylabel("undervolt offset in mV")
    a.grid()
    a.legend()

def plot_faults(files): 

  import matplotlib
  import matplotlib.pyplot as plt

  df = pd.DataFrame(columns=('Instruction', 'Core', 'Frequency', 'UV', 'UV_TO'))

  used_cores = {}

  fault_dict = {}
  for f in files:
    df_new, used_cores = file_get_faults(fault_dict, f, used_cores)
    df = df.append(df_new, ignore_index=True)

  df = df.astype({'UV': 'float', 'UV_TO' : 'float'})

  group = df.groupby(['Instruction','Frequency','Core'])
  df = group.agg({'UV':'max', 'UV_TO' :'max'}).reset_index()
  

  ncores = len(df.groupby(['Core']))


  fig, ax = plt.subplots(nrows=2, ncols=(ncores+1)//2,sharex=True,sharey=True)
  ax = ax.flatten()

  color = ['b', 'r', 'y', 'c']

  for i, (core, items) in enumerate(df.groupby(['Core'])):
    for j, (instructions, items) in enumerate(items.groupby(['Instruction'])):
      ax[i].plot(items['Frequency'], items['UV'], 'x-', label=instructions,)
      #ax[i].plot(items['Frequency'], items['UV_TO'], 'o-', label=instructions,)
    ax[i].grid(True)
    ax[i].legend()
      #items.plot(kind='scatter',x='Frequency',y='UV', grid=True, ax=ax[i],label=instructions,marker='x',color=color[j % len(color)])
      #items.plot(kind='scatter',x='Frequency',y='UV_TO', grid=True, ax=ax[i],label=f"Core{core} TO",color=color[i % len(color)])

  plt.legend()
  plt.show()
  return


  markers={'AESENC':'x', 'IMUL':'o'}
  colors={0:'r',1:'b',2:'y',3:'b',4:'b',5:'b'}


  for instruction, items in df.groupby(['Instruction']):
    for i, (core, items) in enumerate(items.groupby(['Core'])):

      offset = i * 50

      ax.scatter(x=items['Frequency'] + offset, y=items['UV'], c=colors[core], marker=markers[instruction], label=instruction if i == 0 else None)
      ax.set_xlabel('Frequency')
      ax.set_ylabel('UV')
      ax.grid(True)

    #g.plot(kind='scatter',x='Frequency',y='UV',c grid=True, ax=ax,label=instructions)

  plt.legend()
  plt.show()

  return

  df_pivot = pd.pivot_table(
    g,
    values="UV",
    index="Frequency",
    columns="Instruction",
    aggfunc=np.mean
  )



  df_pivot.plot.bar(rot=90)
  plt.show()
  return

  fig, ax = plt.subplots()

  for instructions, items in df.groupby('Instruction'):
    for Frequency, items in items.groupby('Frequency'):

      cores = items['Cores']



  ncores = len(used_cores.items())

  plot_fault_spectrum(fault_dict, ncores)



  plt.show()
  