include $(TOPDIR)/rules.mk

PKG_NAME:=mytest
PKG_VERSION:=1.0
PKG_RELEASE:=3

SOURCE_DIR:=$(shell pwd)/src

include $(INCLUDE_DIR)/package.mk

define Package/mytest
	SECTION:=examples
	CATEGORY:=Examples
	TITLE:=A test project!
	DEPENDS:=+libuci
endef

define Package/mytest/description
	This is a mytest project.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/mytest.o -c $(PKG_BUILD_DIR)/mytest.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/config.o -c $(PKG_BUILD_DIR)/config.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/comport.o -c $(PKG_BUILD_DIR)/comport.c
	$(TARGET_CC) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/$1 $(PKG_BUILD_DIR)/mytest.o $(PKG_BUILD_DIR)/config.o -luci $(PKG_BUILD_DIR)/comport.o
endef

define Package/mytest/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./src/mytest.config $(1)/etc/config/mytest
	$(INSTALL_DIR) $(1)/usr/bin	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mytest $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_CONF) ./src/other.lua $(1)/usr/lib/lua/luci/controller/other.lua
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi
	$(INSTALL_CONF) ./src/mytest.lua $(1)/usr/lib/lua/luci/model/cbi/mytest.lua
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./src/mytest $(1)/etc/init.d/mytest
	
endef
$(eval $(call BuildPackage,mytest))

