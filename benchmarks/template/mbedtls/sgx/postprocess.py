#!/usr/bin/env python3

import numpy as np

data = np.loadtxt(f"./results.csv")
m = np.mean(data)
s = np.std(data)
n = len(data)
print(f"{m};{s};{n}")
