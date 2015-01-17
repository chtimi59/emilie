OPENWRT=$(shell readlink -f ../../)
include $(OPENWRT)/extra/bcm4717-toolchain.mk

SRCS= $(wildcard src/*.c)
OBJS= $(SRCS:.c=.o)

.PHONY: clean

all: $(OUTDIR)/bin/hello2 cpfiles


$(OUTDIR)/bin/hello2: $(OBJS)
	@mkdir -p $(OUTDIR)
	install -d -m0755 $(OUTDIR)/bin/
	$(CC) $(LDFLAGS) -o $(OUTDIR)/bin/hello2 $(OBJS)

clean: 
	@rm -f  $(OBJS)
	@rm -rf $(OUTDIR)

include $(OPENWRT)/extra/bcm4717-rules.mk
