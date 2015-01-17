OPENWRT=$(shell readlink -f ../../)
export STAGING_DIR=$(OPENWRT)/staging_dir

KERNEL = $(OPENWRT)/build_dir/toolchain-mipsel_74kc+dsp2_gcc-4.8-linaro_uClibc-0.9.33.2/linux
LD=$(OPENWRT)/staging_dir/toolchain-mipsel_74kc+dsp2_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-ld
CROSS_COMPILE="mipsel-openwrt-linux-"
EXTRA_LDSFLAGS="-I${OPENWRT}/build_dir/toolchain-mipsel_74kc+dsp2_gcc-4.8-linaro_uClibc-0.9.33.2/linux -include symtab.h"
KBUILD_HAVE_NLS=no
CONFIG_SHELL="/bin/bash" 

obj-m += hello.o

.PHONY: all clean tt

tt:
	$(info fd;lksd)
	touch tt

all:
	make -C $(KERNEL) ARCH="mips" CC="$(OPENWRT)/staging_dir/toolchain-mipsel_74kc+dsp2_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc"  M=$(PWD) modules
	touch rr

clean:
        #make -C $(KERNEL) CC="${OPENWRT}/staging_dir/toolchain-mipsel_74kc+dsp2_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-gcc"  M=$(PWD) clean

