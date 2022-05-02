#!/usr/bin/env python3

import numpy as np
import sys
import os

data = []
nrs=[int(x,10) for x in sys.argv[1:]]

for nr in nrs:
    print(f"reading: bench{nr}")
    s = np.loadtxt(f"./results/bench{nr}")
    data.append(np.mean(s))
    data.append(np.std(s))
    data.append(len(s))


data = np.array(data)
data = data[np.newaxis,:]

np.savetxt("./results/results.csv", data,delimiter=";")
