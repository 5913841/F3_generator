
APP=THUGEN
SRCS-y := examples/http_flood/main.cpp src/timer/*.cpp src/common/*.cpp src/dpdk/*.cpp src/multi_thread/*.cpp src/protocols/*.cpp src/socket/*.cpp src/panel/*.cpp \
 			# src/socket/socket_table/*.cpp src/socket/socket_vector/*.cpp src/socket/socket_tree/*.cpp

#dpdk 17.11, 18.11, 19.11
ifdef RTE_SDK

RTE_TARGET ?= x86_64-native-linuxapp-gcc
include $(RTE_SDK)/mk/rte.vars.mk
CFLAGS += -DHTTP_PARSE
CFLAGS += $(WERROR_FLAGS) -Wno-address-of-packed-member -Wformat-truncation=0

ifdef DPERF_DEBUG
CFLAGS += -DDPERF_DEBUG
endif

LDLIBS += -lrte_pmd_bond

include $(RTE_SDK)/mk/rte.extapp.mk

#dpdk 20.11
else

PKGCONF = pkg-config

ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

ifdef DPERF_DEBUG
CFLAGS += -DDPERF_DEBUG
endif

CFLAGS += -O3 -g -I./src  -I./src/socket/socket_table/parallel-hashmap
CFLAGS += -DHTTP_PARSE
CFLAGS += -Wno-address-of-packed-member -Wformat-truncation=0
CFLAGS += $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk) -lpthread -lrte_net_bond -lrte_bus_pci -lrte_bus_vdev -lrte_pcapng -lrte_pdump -lrte_efd -lpcap -lstdc++

build/$(APP): $(SRCS-y)
	mkdir -p build
	g++ $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS)

build/dumpcap:dumpcap/main.c
	gcc $(CFLAGS) dumpcap/main.c -o build/dumpcap $(LDFLAGS)

clean:
	rm -rf build/

endif