export STAGING_DIR=$(OPENWRT)/staging_dir

TOOLCHAIN=$(STAGING_DIR)/toolchain-mipsel_74kc+dsp2_gcc-4.8-linaro_uClibc-0.9.33.2
TARGET=$(STAGING_DIR)/target-mipsel_74kc+dsp2_uClibc-0.9.33.2/
SCRIPTS=$(OPENWRT)/scripts

CROSS=$(TOOLCHAIN)/bin/mipsel-openwrt-linux-uclibc-
export AR=$(CROSS)gcc-ar
export AS=$(CROSS)gcc -c -Os -pipe -mno-branch-likely -mips32r2 -mtune=74kc -mdspr2 -fno-caller-saves -fhonour-copts -Wno-error=unused-but-set-variable -msoft-float
export LD=$(CROSS)ld
export NM=$(CROSS)gcc-nm
export CC=$(CROSS)gcc
export GCC=$(CROSS)gcc
export CXX=$(CROSS)g++
export RANLIB=$(CROSS)gcc-ranlib
export STRIP=$(CROSS)strip
export OBJCOPY=$(CROSS)objcopy
export OBJDUMP=$(CROSS)objdump
export SIZE=$(CROSS)size

export CFLAGS=-Os -pipe -mno-branch-likely -mips32r2 -mtune=74kc -mdspr2 -fno-caller-saves -fhonour-copts -Wno-error=unused-but-set-variable -msoft-float -mips16 -minterlink-mips16 
export CXXFLAGS=-Os -pipe -mno-branch-likely -mips32r2 -mtune=74kc -mdspr2 -fno-caller-saves -fhonour-copts -Wno-error=unused-but-set-variable -msoft-float -mips16 -minterlink-mips16 
export CPPFLAGS=-I$(TARGET)/usr/include -I$(TARGET)/include -I$(TOOLCHAIN)/usr/include -I$(TOOLCHAIN)/include 
export LDFLAGS=-L$(TARGET)/usr/lib -L$(TARGET)/lib -L$(TOOLCHAIN)/usr/lib -L$(TOOLCHAIN)/lib 


#STRIP=$(STAGING_DIR)/host/bin/sstrip
STRIP_KMOD=$(SCRIPTS)/strip-kmod.sh
STRIP_R=$(SCRIPTS)/rstrip.sh

IPKG_BUILD=$(STAGING_DIR)/host/bin/ipkg-build -c -o 0 -g 0 

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

