import random
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from matplotlib.gridspec import GridSpec
import array
import scipy
from scipy.stats import norm
import math

matplotlib.use('Agg') 
plt.rcParams['font.sans-serif']=['SimHei']
plt.rcParams['axes.unicode_minus'] = False

plt.rcParams['figure.figsize'] = (35.0, 20.0)
fig = plt.figure()
gs = GridSpec(4, 1, figure=fig)
ax2 = fig.add_subplot(gs[3,0])
ax1 = fig.add_subplot(gs[0:3,0], sharex = ax2, frame_on=True)

ax1.set_xlim(20.8, 27)
ax2.set_xlim(20.8, 27)
ax1.set_ylim(0.05, 2.6)
ax2.set_ylim(0, 0.011)
ax1.spines['bottom'].set_visible(False)
ax2.spines['top'].set_visible(False)
ax1.xaxis.tick_top()
ax1.tick_params(labeltop='off')
ax2.xaxis.tick_bottom()

THUGEN_socket_size = 104

THUGEN_max_cc = np.array([41, 40, 30, 35, 35])
THUGEN_memuse = (THUGEN_max_cc / 0.4375 * (8 + 1)) + (THUGEN_max_cc / 0.4375 * (8 + 1) * 0.5) + THUGEN_max_cc * THUGEN_socket_size # Bytes
dperf_memuse = np.array([0.16, 0.31, 0.63, 1.25, 2.50]) # GB

fivetuples_log2 = np.array([21, 22, 23, 24, 25])

lns1 = ax1.plot(fivetuples_log2, dperf_memuse, 'o', label='D-Perf', markersize=40, color='red', linewidth = '4', linestyle='--')
lns2 = ax2.plot(fivetuples_log2, THUGEN_memuse / 1024 / 1024, 'o', label='THUGEN', markersize=40, color='blue', linewidth = '4', linestyle='--')
for i in range(len(fivetuples_log2)):
    ax1.text(fivetuples_log2[i]+0.01, dperf_memuse[i]-0.07, f"(2^{fivetuples_log2[i]}fivetuples, {dperf_memuse[i]}GB)", fontsize=40)
    if i % 2 == 0:
        ax2.text(fivetuples_log2[i]+0.01, (THUGEN_memuse[i] / 1024 / 1024)+0.001, f"(2^{fivetuples_log2[i]}fivetuples, {int(THUGEN_memuse[i])}Bytes)", fontsize=40)
    else:
        ax2.text(fivetuples_log2[i]+0.01, (THUGEN_memuse[i] / 1024 / 1024)-0.003, f"(2^{fivetuples_log2[i]}fivetuples, {int(THUGEN_memuse[i])}Bytes)", fontsize=40)
        

ax2.set_xticks(fivetuples_log2, fivetuples_log2, fontsize=40)
ax1.set_xticks(fivetuples_log2, fivetuples_log2, fontsize=40, color = 'white')
ax1.set_yticks(np.arange(0.1, 3.1, 0.5), np.arange(0.1, 3.1, 0.5), fontsize=40)
ax2.set_yticks(np.arange(0, 0.015, 0.005), np.arange(0, 0.015, 0.005), fontsize=40)
ax2.set_xlabel('log2(Number of Tuples)', fontsize=60)
ax1.set_ylabel('Memory Usage (GB)', loc = 'bottom', fontsize=60)

d = 0.01
kwargs = dict(transform=ax1.transAxes, color='k', clip_on=False)
ax1.plot((-d,+d),(-d,+d), **kwargs)
ax1.plot((1-d,1+d),(-d,+d), **kwargs)

kwargs.update(transform=ax2.transAxes)  # switch to the bottom axes
ax2.plot((-d,+d),(1-3*d,1+3*d), **kwargs)
ax2.plot((1-d,1+d),(1-3*d,1+3*d), **kwargs)


ax1.legend(lns1+lns2, [lns1[0].get_label(), lns2[0].get_label()], fontsize=64, shadow=True, loc=2)

plt.savefig('memuse.png')
