APP := THUGEN
EXAMPLES := $(shell find examples/ -type f)

SRCS-y := src/timer/*.cpp src/common/*.cpp src/dpdk/*.cpp src/protocols/*.cpp src/socket/*.cpp src/panel/*.cpp src/socket/socket_table/*.cpp  src/socket/socket_vector/*.cpp src/dpdk/divert/*.cpp src/api/*.cpp \
 			# src/socket/socket_table/*.cpp src/socket/socket_vector/*.cpp src/socket/socket_tree/*.cpp
SRCS-h := src/timer/*.h src/common/*.h src/dpdk/*.h src/protocols/*.h src/socket/*.h src/panel/*.h src/socket/socket_table/*.h  src/socket/socket_vector/*.h src/dpdk/divert/*.h src/api/*.h \

EX_TO_NA = $(subst examples/,build/,$(subst .cpp,,$1))
NA_TO_EX = $(subst build/,examples/,$1).cpp
NAMES := $(foreach file,$(EXAMPLES),$(call EX_TO_NA,$(file)))


.PHONY: all clean 

#dpdk 17.11, 18.11, 19.11
ifdef RTE_SDK

RTE_TARGET ?= x86_64-native-linuxapp-gcc
include $(RTE_SDK)/mk/rte.vars.mk
CFLAGS += -DHTTP_PARSE
CFLAGS += $(WERROR_FLAGS) -Wno-address-of-packed-member -Wformat-truncation=0


LDLIBS += -lrte_pmd_bond

include $(RTE_SDK)/mk/rte.extapp.mk

#dpdk 20.11
else

PKGCONF = pkg-config

ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

CFLAGS += -O3 -g -I./src  -I./src/socket/socket_table/parallel-hashmap
CFLAGS += -DHTTP_PARSE
# CFLAGS += -DDEBUG_
# CFLAGS += -DUSE_CTL_THREAD
CFLAGS += -Wno-address-of-packed-member -Wformat-truncation=0
CFLAGS += $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk) -lpthread -lrte_net_bond -lrte_bus_pci -lrte_bus_vdev -lrte_pcapng -lrte_pdump -lrte_efd -lpcap -lstdc++


all: build $(NAMES) F3_generator

build::
	mkdir -p build

F3_generator: generator/F3_generator.cpp $(SRCS-y)
	g++ $(CFLAGS) generator/F3_generator.cpp $(SRCS-y) -o F3_generator $(LDFLAGS)

build/%: examples/%.cpp $(SRCS-y)
	mkdir -p $(dir $@)
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/dumpcap:dumpcap/main.c
	gcc $(CFLAGS) dumpcap/main.c -o build/dumpcap $(LDFLAGS)

test: test_hashmap.cpp $(SRCS-y)
	g++ $(CFLAGS) test_hashmap.cpp $(SRCS-y) -o test $(LDFLAGS)

clean:
	rm -rf build/

endif
