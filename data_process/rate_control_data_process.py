import numpy as np
import re

file = open('rate_control_data.txt', 'r')
data = file.readlines()
file.close()

# not_process, process
state = "not_process1"
second = 0

datas = []

for line in data:
    if line.find("seconds") != -1:
        if(second >= 30):
            break
        if state == "not_process1":
            state = "process1"
        if state == "not_process2":
            state = "process2"
    if (state == "process1" or state == "process2") and line.find("skOpen") != -1:
        match = re.search(r'skOpen\s+([\d,]+)', line)
        skopen = match.group(1)
        skopen = skopen.replace(",", "")
        skopen = int(skopen)
        if (state == "process1") and (skopen != 0):
            second += 1
            if(second >= 4):
                state = "not_process2"
                second = 0
            else:
                state = "not_process1"
        elif (state == "process2") and (skopen != 0):
            datas.append(skopen)
            second += 1
            state = "not_process2"
        else:
            state = "not_process1"
    
print(datas)