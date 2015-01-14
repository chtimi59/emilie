include $(TOPDIR)/rules.mk

PKG_NAME:=hello
PKG_VERSION:=0
PKG_RELEASE:=1


include $(INCLUDE_DIR)/package.mk

define Package/hello
  CATEGORY:=Emilie
  TITLE:=Hello world sample
endef

define Package/hello/description
My first Hello world package
endef

define Build/Prepare
	# PREPARE (copying to build_dir/target-<arch>/hello-<version> folder)
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(INSTALL_DATA) ./src/hello.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	# COMPILE
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall \
		-o $(PKG_BUILD_DIR)/hello $(PKG_BUILD_DIR)/hello.c
endef

define Package/hello/install
	# INSTALL (copying binary to build_dir/target-<arch>/hello-<version>/ipk<> folder)
	$(CP) ./files/* $(1)/
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hello $(1)/bin/
endef

$(eval $(call BuildPackage,hello))
