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
