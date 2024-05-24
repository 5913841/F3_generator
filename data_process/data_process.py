import random
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import array
import scipy
from scipy.stats import norm
import math


matplotlib.use('Agg') 
plt.rcParams['font.sans-serif']=['SimHei']
plt.rcParams['axes.unicode_minus'] = False

plt.rcParams['figure.figsize'] = (30.0, 20.0)
fig,ax1=plt.subplots()
ax2=fig.add_subplot(frame_on=False)

arrx1 = [1.59, 1.27, 0.95, 0.64, 0.32, 0.16]
arry1 = [99, 96, 77, 53, 27, 14]

colors1 = '#00CED1' #点的颜色
area = 20  # 点面积 

lns1 = ax1.plot(arrx1, arry1, linewidth = '4', linestyle='--', color=colors1, label = "flow pattern 1", marker='o', markersize=area, markerfacecolor="None", markeredgecolor=colors1, markeredgewidth=4)

for i in range(len(arrx1)):
    ax1.text(arrx1[i]+0.01, arry1[i]-1, f"({arrx1[i]}Mcps, {arry1[i]}%)", fontsize=40)

ax1.set_xlim(0, 1.8)
ax1.set_ylim(0, 100)
ax1.set_xticks([x/10 for x in range(0, 20, 2)],[x/10 for x in range(0, 20, 2)],fontsize=40)
ax1.set_yticks(list(range(10, 110, 10)),list(range(10, 110, 10)),rotation=-90,fontsize=40)
ax1.set_xlabel('pattern 1 traffic rate Mcps', fontsize=60)
ax1.set_ylabel('cpu usage %',fontsize=60)

arrx2 = np.array([0.39, 0.31, 0.23, 0.16, 0.08, 0.04])

arry2 = [99, 95, 67, 46, 23, 11]

colors2 = '#00B2F1' #点的颜色
area = 20  # 点面积

lns2 = ax2.plot(arrx2, arry2, linewidth = '4', linestyle='--', color=colors2, label = "flow pattern 2", marker='s', markersize=area, markerfacecolor="None", markeredgecolor=colors2, markeredgewidth=4)

for i in range(len(arrx2)):
    if(i < 2):
        ax2.text(arrx2[i]+0.04/4, arry2[i]-4, f"({arrx2[i]}Mcps, {arry2[i]}%)", fontsize=40)
    else:
        ax2.text(arrx2[i]+0.01/4, arry2[i]-3, f"({arrx2[i]}Mcps, {arry2[i]}%)", fontsize=40)

ax2.set_xlim(0, 0.4)
ax2.set_ylim(0, 100)
ax2.set_xticks([x/10/4 for x in range(0, 20, 2)],[x/10/4 for x in range(0, 20, 2)],fontsize=40)
ax2.set_yticks(list(range(10, 110, 10)),list(range(10, 110, 10)),rotation=-90,fontsize=40)
ax2.set_xlabel('pattern 2 traffic rate Mcps', fontsize=60)
ax2.xaxis.tick_top()
ax2.xaxis.set_label_position('top')

ax1.legend(lns1+lns2, [lns1[0].get_label(), lns2[0].get_label()], fontsize=64, shadow=True)
fig.tight_layout()
# plt.grid(True)
plt.savefig('./pattern_compare.png')

plt.cla()
plt.close()

matplotlib.use('Agg') 
plt.rcParams['font.sans-serif']=['SimHei']
plt.rcParams['axes.unicode_minus'] = False

plt.rcParams['figure.figsize'] = (30.0, 23.0)


arry3_add = np.array(arry1[1:])[::-1] + np.array(arry2[1:])
arry3_mix = np.array([98, 93, 98, 95, 98])


colors1 = '#00CED1' #点的颜色
area = 20  # 点面积 

lns1 = plt.plot(list(range(1,6)), arry3_add, linewidth = '4', linestyle='--', color=colors1, label = "flow pattern add 1 and 2", marker='o', markersize=area, markerfacecolor="None", markeredgecolor=colors1, markeredgewidth=4)

for i in range(5):
    plt.text(i+1+0.01, arry3_add[i], f"({arrx1[5-i]}+{arrx2[i+1]}Mcps, {arry3_add[i]}%)", fontsize=40)

colors2 = '#00B2F1' #点的颜色
area = 20  # 点面积

lns2 = plt.plot(list(range(1,6)), arry3_mix, linewidth = '4', linestyle='--', color=colors2, label = "flow pattern mix 1 and 2", marker='s', markersize=area, markerfacecolor="None", markeredgecolor=colors2, markeredgewidth=4)

for i in range(5):
    plt.text(i+1+0.01, arry3_mix[i]-0.2, f"({arrx1[5-i]}+{arrx2[i+1]}Mcps, {arry3_mix[i]}%)", fontsize=40)

plt.xlim(0, 7)
plt.ylim(90, 110)
plt.xticks(list(range(1,6)),[f"{arrx1[5-i-1]}+{arrx2[i]}" for i in range(5)],fontsize=40)
plt.yticks(list(range(90, 112, 2)),list(range(90, 112, 2)),rotation=-90,fontsize=40)
plt.xlabel('pattern 1 and 2 traffic rate Mcps', fontsize=60)
plt.ylabel('cpu usage %',fontsize=60)

plt.legend(fontsize=52, shadow=True, bbox_to_anchor=(0.5, 1.01), loc = 3, borderaxespad=0.)
plt.savefig('./pattern_mix.png')

plt.cla()
plt.close()

data = {
    125000 : [120907, 125000, 125000, 125004, 120950, 125000, 133090, 125000, 125020, 125000, 125000, 125004, 125000, 125000, 125000, 125000, 125000, 125000, 125004, 125000, 125000, 125000, 125000, 125000, 125000, 125000, 125004, 125000, 125000, 125000],
    250000 : [250000, 250000, 250004, 250000, 250004, 250000, 250000, 250000, 250004, 250004, 250000, 250000, 250004, 250000, 250000, 250004, 250000, 250004, 250000, 250000, 250004, 250000, 250004, 250000, 250000, 250004, 250000, 250004, 250000, 250000],
    375000 : [375000, 375004, 375000, 375004, 375004, 375000, 375004, 375000, 375004, 375000, 375004, 375004, 375000, 375004, 375000, 375004, 375000, 375004, 375000, 375004, 375004, 375000, 375004, 375000, 375004, 375004, 375000, 375004, 375000, 375004],
    500000 : [500020, 500024, 500020, 500024, 500020, 500024, 500020, 500024, 500020, 500020, 500024, 500020, 500024, 500020, 500024, 500020, 500024, 500020, 500020, 500024, 500020, 500024, 500020, 500024, 500020, 500020, 500024, 500020, 500024, 500020],
    625000 : [625044, 625044, 625044, 625048, 625044, 625044, 625044, 625044, 625044, 625044, 625048, 625044, 625044, 625044, 625044, 625048, 625044, 625044, 625044, 625044, 625044, 625048, 625044, 625044, 625044, 625044, 625048, 625044, 625044, 625044],
    750000 : [750048, 750052, 750052, 750052, 750052, 750052, 750049, 750051, 750052, 750052, 750052, 750052, 750048, 750052, 750052, 750052, 750052, 750048, 750056, 750048, 750052, 750052, 750052, 750052, 750052, 750048, 750052, 750052, 750048, 750056],
    875000 : [875036, 875036, 875032, 875036, 875036, 875032, 875036, 875036, 875036, 875032, 875036, 875036, 875036, 875036, 875032, 875036, 875036, 875032, 875036, 875036, 875036, 875033, 875035, 875036, 875036, 875032, 875036, 875036, 875036, 875032],
    1000000 : [1000084, 1000088, 1000084, 1000084, 1000088, 1000084, 1000084, 1000084, 1000088, 1000084, 1000084, 1000088, 1000084, 1000084, 1000076, 1000096, 1000076, 1000092, 1000084, 1000088, 1000084, 1000088, 1000084, 1000084, 1000084, 1000088, 1000084, 1000084, 1000084, 1000088],
    1125000 : [1125056, 1125040, 1125044, 1125044, 1125040, 1125044, 1125044, 1125044, 1125040, 1125044, 1125044, 1125044, 1125040, 1125044, 1125044, 1125040, 1125044, 1125044, 1125040, 1125044, 1125044, 1125044, 1125040, 1125044, 1125044, 1125040, 1125044, 1125044, 1125044, 1125041],
    1250000 : [1250080, 1250080, 1250084, 1250080, 1250080, 1250080, 1250084, 1250080, 1250084, 1250080, 1250080, 1250080, 1250084, 1250080, 1250084, 1250080, 1250080, 1250080, 1250080, 1250084, 1250080, 1250080, 1250084, 1250080, 1250080, 1250084, 1250080, 1250080, 1250084, 1250080],
    1375000 : [1375040, 1375036, 1375040, 1375040, 1375036, 1375040, 1375036, 1375044, 1375036, 1375040, 1375037, 1375039, 1375040, 1375040, 1375032, 1375044, 1375040, 1375036, 1375036, 1375044, 1375036, 1375040, 1375040, 1375036, 1375040, 1375040, 1375036, 1375040, 1375036, 1375040],
    1500000 : [1500020, 1500016, 1500016, 1500020, 1500008, 1500028, 1500016, 1500008, 1500028, 1500020, 1500024, 1500008, 1500024, 1500024, 1500016, 1500020, 1500012, 1500024, 1500020, 1500020, 1500016, 1500020, 1500016, 1500020, 1500016, 1500020, 1500020, 1500016, 1500020, 1500016],
    1625000 : [1625190, 1625190, 1625192, 1625192, 1625190, 1625190, 1625192, 1625192, 1625185, 1625071, 1625119, 1625182, 1625172, 1625182, 1625172, 1625172, 1625178, 1625192, 1625180, 1625204, 1625188, 1625192, 1625192, 1625192, 1623608, 1624100, 1625295, 1625026, 1625099, 1623541],
}

arr = []

for key in data:
    arr.append(data[key])

arr = np.array(arr)
keys = np.array(list(data.keys()))

# plt.ylim(0.99999, 1.00005)
plt.yscale('symlog')
plt.xlabel('Set Rate (Mbps)',fontsize=60)
plt.ylabel('Actual Rate - Set Rate' ,fontsize=60)
plt.yticks(fontsize=40)
plt.xticks(fontsize=40)
# plt.boxplot((arr.T / keys.T), labels=keys, sym="r+", showmeans=True)
plt.boxplot(x = arr.T-keys,     # 绘图数据 
            labels=keys/1000000,                  # 标签
            whis=3,                     # 设置IQR的范围，默认是1.5倍的箱线范围
            showfliers=True,              # 设置是否显示异常值，默认显示
            # notch = True,      #设置中位线处凹陷，（注意：下图看起来有点丑）
            patch_artist=True, # 设置用自定义颜色填充盒形图，默认白色填充 
            showmeans=True,    # 以点的形式显示均值 
            boxprops = {"color":"black","facecolor":"#F43D68"}, # 设置箱体属性，填充色and 边框色 
            flierprops = {"marker":"o","markerfacecolor":"#59EA3A","color":"#59EA3A"}, # 设置异常值属性，点的形状、填充色和边框色 
            meanprops = {"marker":"D","markerfacecolor":"white"}, # 设置均值点的属性，点的形状、填充色 
            medianprops = {"linestyle":"--","color":"#FBFE00"}         # 设置中位数线的属性，线的类型和颜色
            )
plt.savefig('./rate_control.png')

plt.cla()
plt.close()

F3_core_cps_data = {
    1 : [1578377, 1604704, 1575061, 1610122, 1601925, 1576045, 1607081, 1608374, 1587362, 1593357, 1607341, 1601691, 1605793, 1580533, 1596585, 1607686, 1607416, 1608165, 1609741, 1588831, 1589517, 1608803, 1610031, 1609198, 1609981, 1608515, 1611375, 1606173, 1573098, 1603313],
    2 : [3931178, 3947774, 3986924, 3960580, 4001146, 3992732, 3982933, 4000860, 3959874, 4003590, 4002527, 4003917, 3983744, 4004453, 3997711, 3981410, 3839425, 3944586, 3997649, 4034621, 3992721, 4031645, 3926570, 3915896, 3983180, 3967058, 3938302, 3979644, 3999038, 4004873],
    4 : [8532278, 8559273, 8504745, 8473920, 8363324, 8446492, 8509949, 8450755, 8500856, 8501767, 8351432, 8375765, 8434470, 8450292, 8402038, 8394119, 8407434, 8353870, 8420921, 8435108, 8371479, 8408358, 8512969, 8546356, 8550926, 8556917, 8550674, 8553938, 8557641, 8509256],
    6 : [12053086, 11162239, 10916796, 10991430, 10949717, 11076905, 11035653, 11067928, 11125097, 11103773, 11104923, 10464071, 10922704, 11183560, 11154945, 10910795, 10904039, 11185372, 11208190, 11254183, 11155895, 11072853, 11246287, 11254913, 11249051, 11229106, 11257787, 11267779, 11284608, 11283665],
    8 : [15831649, 15742762, 15713829, 15684202, 15710686, 15694085, 15687031, 15709446, 15574750, 15617258, 15508933, 15666194, 15592072, 15581823, 15619813, 15616580, 15422241, 15326151, 15379011, 15325864, 15353372, 15380478, 15358812, 15414133, 15407895, 15452644, 15434064, 15488892, 15530399, 15541257],
    10 : [20216422, 20164581, 20162401, 20190113, 20204588, 20196575, 20188062, 20162429, 20156593, 20179944, 20191883, 20176914, 20183650, 19096866, 20512269, 20499220, 20368209, 20028251, 19226053, 20552215, 20366888, 20241255, 20153204, 20203707, 20208749, 20210146, 20126097, 20118680, 20110087, 20110653],

}

arry1 = np.rint(np.array(list(F3_core_cps_data.values())).mean(axis=1))
arry2 = np.array([2830000, 4050000, 7500000, 10500000, 16000000, 21500000])
arrx = np.array(list(F3_core_cps_data.keys()))

plt.plot(arrx, arry1, 'o', label='F3', markersize=40, color='blue', linewidth = '4', linestyle='--')
plt.plot(arrx, arry2, 'o', label='D-perf', markersize=40, color='red', linewidth = '4', linestyle='--')


plt.xlim(-3, 11)
plt.xticks(list(range(1,10)),list(range(1,10)),fontsize=40)
plt.yticks(fontsize=40)
plt.xlabel('Number of used cores', fontsize=60)
plt.ylabel('Max throughput of MCPS',fontsize=60)

for i in range(len(arrx)):
        plt.text(arrx[i]-3.7, arry1[i], f"({arrx[i]}cores, {int(arry1[i])}cps)", fontsize=40)
        plt.text(arrx[i]+0.1, arry2[i], f"({arrx[i]}cores, {int(arry2[i])}cps)", fontsize=40)
plt.legend(fontsize=52, shadow=True)
plt.savefig('./cps_core.png')

plt.cla()
plt.close()