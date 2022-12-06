import sys
import pandas as pd
import matplotlib.pyplot as plt

COLORS = ['b', 'r']

seq_file = sys.argv[1]
par_file = sys.argv[2]
op_file = sys.argv[3]

print(seq_file, " ", par_file, " ", op_file)

df_seq = pd.read_csv(seq_file)
df_par = pd.read_csv(par_file)

print(df_seq)
print(df_par)

avg_seq = df_seq['latency'].mean();
avg_par = df_par['latency'].mean();
print(avg_seq)
print(avg_par)

fig = plt.figure(figsize =(10, 7))
# Horizontal Bar Plot
plt.bar(['seq', 'par'], [avg_seq, avg_par], color=COLORS, width = 0.4)
plt.ylabel('latency(ms)')
plt.title("Latency");
plt.savefig(op_file, dpi=300)
