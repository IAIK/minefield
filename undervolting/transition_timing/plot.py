#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt

def plot_data(name, range=None):
    data = np.loadtxt(name, delimiter=',')
    data = data[:,1]
    data = data[data != 0]

    hist, bins = np.histogram(data, bins=1000,range=range,normed=True)

    m = np.min(data)

    print(f"minimum: {m}")

    plt.plot(bins[1:], hist,label=name)
    plt.xlabel("us")

    return hist, bins

h1, b1 = plot_data("log.txt")
h2, b2 = plot_data("log_3ghz.txt",range=(b1[0],b1[-1]))

result = np.stack((b1[:-1],h1,h2),axis=1)

#np.savetxt("results.csv", result, delimiter=',', header='us,oneghz,threeghz')

#print(result.shape)

plt.legend()
plt.grid()
plt.show()
