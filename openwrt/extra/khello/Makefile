OPENWRT=$(shell readlink -f ../../)
include $(OPENWRT)/extra/bcm4717-toolchain.mk

MODULE_NAME="khello.ko"

SRCS= $(wildcard src/*.c)
MODULE_SRCPATH=$(PWD)/src
MODULE_OUTPATH=$(OUTDIR)/lib/modules/3.14.27


.PHONY: all clean

all : $(MODULE_OUTPATH)/$(MODULE_NAME) cpfiles


$(MODULE_OUTPATH)/$(MODULE_NAME): $(SRCS)
	make -C $(KERNEL) ARCH="mips" CROSS_COMPILE="mipsel-openwrt-linux-uclibc-" CC=$(CC) LD=$(LD) M=$(MODULE_SRCPATH) modules
	@install -d -m0755 $(MODULE_OUTPATH)
	cp $(MODULE_SRCPATH)/out.ko $(MODULE_OUTPATH)/$(MODULE_NAME)


clean : uninstall
	make -C $(KERNEL) ARCH="mips" CROSS_COMPILE="mipsel-openwrt-linux-uclibc-" CC=$(CC) LD=$(LD) M=$(MODULE_SRCPATH) clean
	@rm -rf $(OUTDIR)

include $(OPENWRT)/extra/bcm4717-rules.mk
