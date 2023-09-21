include $(TOPDIR)/rules.mk

PKG_NAME:=gateway
PKG_VERSION:=1.0
PKG_RELEASE:=0

SOURCE_DIR:=$(shell pwd)/src
IOT_INC_DIR = -I$(PKG_BUILD_DIR)/include -I$(PKG_BUILD_DIR)/include/exports -I$(PKG_BUILD_DIR)/internal_inc
TARGET_CFLAGS += -Wall -Wno-error=sign-compare -Wno-error=format -pthread -DFORCE_SSL_VERIFY
IOT_LDFLAGS = -Wl,--start-group $(PKG_BUILD_DIR)/lib/libiot_sdk.a $(PKG_BUILD_DIR)/lib/libmbedtls.a $(PKG_BUILD_DIR)/lib/libmbedx509.a $(PKG_BUILD_DIR)/lib/libmbedcrypto.a $(PKG_BUILD_DIR)/lib/libiot_platform.a -Wl,--end-group

include $(INCLUDE_DIR)/package.mk

define Package/gateway
	SECTION:=examples
	CATEGORY:=Examples
	TITLE:=A IoT gateway!
	DEPENDS:=+libuci
endef

define Package/gateway/description
	This is a IoT gateway.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(IOT_INC_DIR) -o $(PKG_BUILD_DIR)/gateway.o -c $(PKG_BUILD_DIR)/gateway.c
#	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/config.o -c $(PKG_BUILD_DIR)/config.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/comport.o -c $(PKG_BUILD_DIR)/comport.c
	$(TARGET_CC) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/$1 $(PKG_BUILD_DIR)/gateway.o -luci $(PKG_BUILD_DIR)/comport.o $(IOT_LDFLAGS)
endef

define Package/gateway/install
#	$(INSTALL_DIR) $(1)/etc/config
#	$(INSTALL_CONF) ./src/mytest.config $(1)/etc/config/mytest
	$(INSTALL_DIR) $(1)/usr/bin	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gateway $(1)/usr/bin
#	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
#	$(INSTALL_CONF) ./src/other.lua $(1)/usr/lib/lua/luci/controller/other.lua
#	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi
#	$(INSTALL_CONF) ./src/mytest.lua $(1)/usr/lib/lua/luci/model/cbi/mytest.lua
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./src/mytest $(1)/etc/init.d/mytest
	
endef
$(eval $(call BuildPackage,gateway))
