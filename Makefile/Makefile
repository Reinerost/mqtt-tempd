include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt-tempd
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)


# Adjust this path to the local mqtt-tempd source directory.
#
SOURCE_DIR:=/path/to/mqtt-tempd

include $(INCLUDE_DIR)/package.mk


define Package/mqtt-tempd
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=MQTT temperature daemon
  DEPENDS:=+libuci +libmosquitto-nossl +librrd
endef


define Package/mqtt-tempd/description
 MQTT temperature daemon for OpenWrt.
 Receives temperature values via MQTT and stores them in RRD databases.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(SRC_DIR)/* $(PKG_BUILD_DIR)/
endef


define Build/Compile
	$(TARGET_CC) \
		$(TARGET_CPPFLAGS) \
		$(TARGET_CFLAGS) \
		-o $(PKG_BUILD_DIR)/mqtt-tempd \
		$(PKG_BUILD_DIR)/src/main.c \
		$(PKG_BUILD_DIR)/src/log.c \
		$(PKG_BUILD_DIR)/src/signals.c \
		$(PKG_BUILD_DIR)/src/config.c \
		$(PKG_BUILD_DIR)/src/mqtt.c \
		$(PKG_BUILD_DIR)/src/rrd.c \
		$(TARGET_LDFLAGS) \
		-L$(STAGING_DIR)/usr/lib \
		-lrrd \
		-luci \
		-lmosquitto
endef


define Package/mqtt-tempd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqtt-tempd \
		$(1)/usr/sbin/mqtt-tempd

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/mqtt-tempd.init \
		$(1)/etc/init.d/mqtt-tempd

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/mqtt-tempd.config \
		$(1)/etc/config/mqtt-tempd
	
	$(INSTALL_DIR) $(1)/www/cgi-bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/tempread \
		$(1)/www/cgi-bin/tempread

	$(INSTALL_DIR) $(1)/www
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/files/temperatur.html \
		$(1)/www/temperatur.html
endef


$(eval $(call BuildPackage,mqtt-tempd)) 
