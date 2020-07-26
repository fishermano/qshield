import pylab
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from matplotlib.pyplot import MultipleLocator
import numpy as np
import math

legend_font = {'family' : 'Times New Roman',
'weight' : 'bold',
'size' : 5,
}

label_font = {'family' : 'Times New Roman',
'weight' : 'normal',
'size' : 6.5,
}

text_font = {'family' : 'Times New Roman',
'weight' : 'bold',
'size' : 5,
}

def load_data(file_name):
	data_file = open(file_name, 'r')
	exe_time = []
	for line in data_file:
		exe_time.append(float(line))
	return exe_time

dag_updating = load_data('./exp_res/dag/res/dag_updating_mean')
dag_matching = load_data('./exp_res/dag/res/dag_matching_mean')

fig = plt.figure(figsize=(4,1.5))
gs = gridspec.GridSpec(nrows=1, ncols=1)
ax1 = fig.add_subplot(gs[0,0])

bar_width = 0.4
x_l = ['Q1\n(m=5)', 'Q2\n(m=9)', 'Q3\n(m=19)']

ax1.grid(linestyle='-.', axis='x', zorder=1, linewidth=0.25)
ax1.barh(y=range(len(x_l)), width=dag_updating, height=bar_width, label='UDAG', color='goldenrod', edgecolor='white', linewidth=0.25, alpha=1, zorder=2)
ax1.barh(y=np.arange(len(x_l)) + bar_width, width=dag_matching, height=bar_width, label='MDAG', color='seagreen', edgecolor='white', linewidth=0.25, alpha=1, zorder=2)
for y, x in enumerate(dag_updating):
    ax1.text(x + 0.015, y - 0.3*bar_width, '%s' % float('%.3f' % dag_updating[y]), ha='center', va='bottom', fontdict=text_font)
for y, x in enumerate(dag_matching):
    ax1.text(x + 0.015, y + 0.7*bar_width, '%s' % float('%.3f' % dag_matching[y]), ha='center', va='bottom', fontdict=text_font)
ax1.set_yticks(np.arange(len(x_l))+0.5*bar_width)
ax1.set_yticklabels(x_l, rotation=20)
x_major_locator = MultipleLocator(0.1)
ax1.xaxis.set_major_locator(x_major_locator)
ax1.set(xlim=[0, 0.35])
ax1.tick_params(labelsize=6)
labels = ax1.get_xticklabels() + ax1.get_yticklabels()
[label.set_fontname('Times New Roman') for label in labels]
ax1.legend(loc='lower right', frameon=True, prop=legend_font)
ax1.set_ylabel('Benchmark Query Type', label_font)
ax1.set_xlabel('Execution Time (ms)', label_font)

plt.tight_layout()
plt.show()
