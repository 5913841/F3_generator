# invalid
THUGEN_PATH = "~/F3_generator/"

core_num = 4

core_list = {
    1: [0],
    2: [0, 1],
    3: [0, 1, 2],
    4: [0, 1, 2, 3]
}

speed_base = 1.37
speed_bias = 0.1
speed_accuracy = 0.001

def bin_search(lcores, speed_base, speed_bias, speed_accuracy):
    speed_middle = speed_base
    speed_max = speed_base + speed_bias
    speed_min = speed_base - speed_bias
    while abs(speed_max - speed_min) > speed_accuracy:
        speed_middle = (speed_max + speed_min) / 2
        if check_speed(lcores, speed_middle):
            speed_min = speed_middle
        else:
            speed_max = speed_middle
    return speed_middle


def check_speed(lcores, speed):
    core_arg = ','.join(core_list[lcores])
    speed_arg = str(speed)+"M"
    cmd = THUGEN_PATH + "build/trail/client_cps_core {} {}".format(core_arg, speed_arg)
    


for i in range(core_num):
    test_core(i)