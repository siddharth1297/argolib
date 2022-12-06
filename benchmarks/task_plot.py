import sys
import pandas as pd
import matplotlib.pyplot as plt

#COLORS = ['b', 'r', 'g', 'y', 'k']
COLORS = ['b', 'r']
ip_file = sys.argv[1]
op_file = sys.argv[2]

print(ip_file, " ", op_file)

df = pd.read_csv(ip_file, dtype={'threads': str, 'ratio': float})
#print(df)

fig = plt.figure(figsize =(10, 7))

# Horizontal Bar Plot
plt.bar(df['threads'], df['ratio'], color=COLORS, width = 0.4)
plt.ylabel('latency(ms)')
plt.title("Latency");
plt.savefig(op_file, dpi=300)
