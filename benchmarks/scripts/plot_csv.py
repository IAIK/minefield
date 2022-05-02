#!/usr/bin/env python3
import matplotlib.pyplot as plt
import pandas as pd
import sys

if len(sys.argv) < 2:
    print(f"usage: {sys.argv[0]} results.csv [inv]")
    exit(-1)

df = pd.read_csv(sys.argv[1], sep=';', index_col=0, header=0)
df_sd = df


print(df)

for name, data in df.iteritems():
    if name.startswith("N "):
        df = df.drop(name, axis=1)
        df_sd = df_sd.drop(name, axis=1)
    else:

        if name.startswith("STD "):
            df = df.drop(name, axis=1)
        else:
            df_sd = df_sd.drop(name, axis=1)

found = False
for index, row in df.iterrows():
    if index == 'base':
        ref = row
        found = True
        print("using base as reference")

if not found:
    ref = df.loc[0]
    print("using 0 as reference")


if len(sys.argv) == 3 and sys.argv[2]=="inv": 
    inv = 1
else:
    inv = 0


dropped = False
for index, row in df.iterrows():
    if index == 'base':
        df = df.drop("base")
        dropped = True

if not dropped:
    df = df.drop(0)

for index, row in df.iterrows():
    if inv:
        nom = row
        den = ref
    else:
        nom = ref
        den = row

    try:
        test = nom / den
    except:
        # it is a time in string format convert to seconds
        nom = pd.to_timedelta(nom).dt.total_seconds()
        den = pd.to_timedelta(den).dt.total_seconds()

    # always calculate the overhead in percent
    df.loc[index] = ((nom > den) ) * (nom / den - 1.0) * 100.0 - ((nom <= den) ) * (den / nom - 1.0) * 100.0

df.index = df.index.map(float)

df = df.sort_values("NAME",axis="rows")

print(df)
print(df_sd)

df.to_csv("results_norm.csv")

# plot
df.plot(kind="bar",title="Relative performance overhead in percent")
plt.xticks(rotation=0)
#plt.yscale("log")
plt.grid()

df.transpose().plot(kind="bar",title="Relative performance overhead in percent")
#plt.yscale("log")
plt.xticks(rotation=0)
plt.grid()

plt.show()
