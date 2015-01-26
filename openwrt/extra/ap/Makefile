OPENWRT=$(shell readlink -f ../../)
include $(OPENWRT)/extra/bcm4717-toolchain.mk

SRCS= $(wildcard src/*.c)
OBJS= $(SRCS:.c=.o)

.PHONY: clean all

all: cpfiles

clean: uninstall

include $(OPENWRT)/extra/bcm4717-rules.mk
