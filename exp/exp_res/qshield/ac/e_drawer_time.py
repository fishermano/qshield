import pylab
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from matplotlib.pyplot import MultipleLocator
import numpy as np
import math

font1 = {'family' : 'Times New Roman',
'weight' : 'bold',
'size' : 6.5,
}

font3 = {'family' : 'Times New Roman',
'weight' : 'normal',
'size' : 9.5,
}

font4 = {'family' : 'Times New Roman',
'weight' : 'bold',
'size' : 5.5,
}

def ms_data(x):
	y = float(x) * 1000
	return y

def throughput_data(x):
	y = 1/float(x)
	return y

def log_data(x):
	y = math.log(x, 10)
	return y

def log_data_half(x):
	y = math.log(x, 10)
	return 0.7*y

def load_data(file_name):
	data_file = open(file_name, 'r')

	file_num = []
	exe_time = []

	for line in data_file:
		tmp = line.split(',')
		file_num.append(tmp[0])
		exe_time.append(tmp[1])

	return (file_num, exe_time)

(e_size_h, e_exe_time_h) = load_data('./e_sgx_dec.txt')
e_exe_time_h_ms = list(map(ms_data, e_exe_time_h))
e_exe_time_h_ms_log = list(map(log_data, e_exe_time_h_ms))
e_exe_time_h_ms_log_half = list(map(log_data_half, e_exe_time_h_ms))
e_throught_put_h = list(map(throughput_data, e_exe_time_h))
e_throught_put_h_log = list(map(log_data, e_throught_put_h))

(sgx_size_h, sgx_exe_time_h) = load_data('./sgx_dec.txt')
sgx_exe_time_h_ms = list(map(ms_data, sgx_exe_time_h))
sgx_exe_time_h_ms_log = list(map(log_data, sgx_exe_time_h_ms))
sgx_exe_time_h_ms_log_half = list(map(log_data_half, sgx_exe_time_h_ms))
sgx_throught_put_h = list(map(throughput_data, sgx_exe_time_h))
sgx_throught_put_h_log = list(map(log_data, sgx_throught_put_h))

fig = plt.figure(figsize=(4,2))
gs = gridspec.GridSpec(nrows=1, ncols=1)
ax1 = fig.add_subplot(gs[0,0])
# ax2 = fig.add_subplot(gs[1,0])
width = 0.3

x_data = ['10', '10K', '100K','200K', '400K', '600K', '800K', '1M']
ax1.grid(linestyle='-.', axis='y', zorder=1, alpha=0.5, which='minor')
ax1.bar(x=x_data, height=e_exe_time_h_ms_log, width=width, label='QShield', color='goldenrod', edgecolor='goldenrod', linewidth=0.5, alpha=1, zorder=2)
ax1.bar(x=x_data, height=sgx_exe_time_h_ms_log, width=width, label='Baseline', color='seagreen', edgecolor='seagreen', linewidth=0.5, alpha=1, zorder=3)
for x, y in enumerate(e_exe_time_h_ms_log):
    ax1.text(x, y + 0.05, '%s' % float('%.3f' % e_exe_time_h_ms[x]), ha='center', va='bottom', fontdict=font4)
for x, y in enumerate(sgx_exe_time_h_ms_log):
    ax1.text(x, y - 0.05 if (y<0) else y * (-1), '%s' % float('%.3f' % sgx_exe_time_h_ms[x]), ha='center', va='top', fontdict=font4)
ax1.set_xlabel('Data Size (Bytes)', font3)
ax1.set_ylabel('Logarithmic Time (ms)', font3)
y_minor_locator = MultipleLocator(0.5)
y_major_locator = MultipleLocator(0.5)
ax1.yaxis.set_minor_locator(y_minor_locator)
ax1.yaxis.set_major_locator(y_major_locator)
ax1.set(ylim=[-1.6, 0.6])
ax1.tick_params(labelsize=8)
labels = ax1.get_xticklabels() + ax1.get_yticklabels()
[label.set_fontname('Times New Roman') for label in labels]
ax1.legend(loc='lower right', frameon=True, prop=font1)

ax1.plot(x_data, e_exe_time_h_ms_log, color="black", linewidth=0.5, marker='v', ms=2, zorder=5)

ax1.plot(x_data, sgx_exe_time_h_ms_log, color="black", linewidth=0.5, marker='^', ms=2, zorder=5)

plt.tight_layout()
plt.show()
