#----------------------------------------
#
# COMMON RULES DEFINITIONS
#
#-------------------------------------

.PHONY: install uninstall cpfiles clobber root.squashfs

install:
	$(info Copying files to $(ROOTFSDIR))
	@cp -r $(OUTDIR)/* $(ROOTFSDIR)

uninstall:
	$(info Remove files in $(ROOTFSDIR))
	@if [ -e $(OUTDIR_ESC) ]; then find $(OUTDIR_ESC) -type f | sed 's/$(OUTDIR_ESC)/$(ROOTFSDIR_ESC)/' | xargs rm -f; fi

cpfiles:
	@if [ -e ./files/ ]; then mkdir -p $(OUTDIR) ; cp -rf ./files/* $(OUTDIR); fi

root.squashfs:
	$(MKSQUASHFS) $(ROOTFSDIR) root.squashfs -nopad -noappend -root-owned -comp xz -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2 -b 256k \
             -p '/dev d 755 0 0' -p '/dev/console c 600 0 0 5 1' -processors 1

image: e2000.bin
e2000.bin: root.squashfs $(LINUX_BUILD_DIR)/loader.gz $(LINUX_BUILD_DIR)/fs_mark
	$(MKTRX) -o e2000.trx \
             -f $(LINUX_BUILD_DIR)/loader.gz \
             -f $(LINUX_BUILD_DIR)/vmlinux.lzma -a 1024 \
             -f root.squashfs  -a 0x10000 \
             -A $(LINUX_BUILD_DIR)/fs_mark
	$(ADDPATTERN) -4 -p 32XN -v v1.0.4 -i e2000.trx -o e2000.bin 
	@rm -f e2000.trx

clobber: clean uninstall
	@rm -f e2000.bin
	@rm -f root.squashfs

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

