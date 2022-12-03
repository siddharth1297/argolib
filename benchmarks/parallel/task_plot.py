import sys
import pandas as pd
import matplotlib.pyplot as plt

COLORS = ['b', 'r', 'g', 'y', 'k']
ip_file = sys.argv[1]
op_file = sys.argv[2]
df = pd.read_csv(ip_file)
#print(df.columns)
print(df.head())

fig = plt.figure(figsize =(10, 7))

# Horizontal Bar Plot
plt.bar(df['threads'], df['ratio'])


plt.savefig(op_file + '.jpeg')
