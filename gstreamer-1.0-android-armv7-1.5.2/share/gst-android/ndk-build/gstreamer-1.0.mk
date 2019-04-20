# cerbero - a multi-platform build system for Open Source software
# Copyright (C) 2012 Andoni Morales Alastruey <ylatuya@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

$(call assert-defined, GSTREAMER_ROOT)
$(if $(wildcard $(GSTREAMER_ROOT)),,\
  $(error "The directory GSTREAMER_ROOT=$(GSTREAMER_ROOT) does not exists")\
)


#####################
#  Setup variables  #
#####################

ifndef GSTREAMER_PLUGINS
  $(info "The list of GSTREAMER_PLUGINS is empty")
endif

# Expand home directory (~/)
GSTREAMER_ROOT := $(wildcard $(GSTREAMER_ROOT))

# Path for GStreamer static plugins
ifndef GSTREAMER_STATIC_PLUGINS_PATH
GSTREAMER_STATIC_PLUGINS_PATH := lib/gstreamer-1.0
endif
GSTREAMER_STATIC_PLUGINS_PATH := $(GSTREAMER_ROOT)/lib/gstreamer-1.0/static

# Path for the NDK integration makefiles
ifndef GSTREAMER_NDK_BUILD_PATH
GSTREAMER_NDK_BUILD_PATH := $(GSTREAMER_ROOT)/share/gst-android/ndk-build
endif

ifndef GSTREAMER_INCLUDE_FONTS
GSTREAMER_INCLUDE_FONTS := yes
endif

ifndef GSTREAMER_INCLUDE_CA_CERTIFICATES
GSTREAMER_INCLUDE_CA_CERTIFICATES := yes
endif

ifndef GSTREAMER_JAVA_SRC_DIR
GSTREAMER_JAVA_SRC_DIR := src
endif

# Include tools
include $(GSTREAMER_NDK_BUILD_PATH)/tools.mk

# Path for the static GIO modules
G_IO_MODULES_PATH := $(GSTREAMER_ROOT)/lib/gio/modules/static

# Host tools
ifeq ($(HOST_OS),windows)
    HOST_SED := $(GSTREAMER_NDK_BUILD_PATH)/tools/windows/sed
    GSTREAMER_LD :=
else
endif

ifndef GSTREAMER_ANDROID_MODULE_NAME
GSTREAMER_ANDROID_MODULE_NAME := gstreamer_android
endif
GSTREAMER_BUILD_DIR           := gst-build-$(TARGET_ARCH_ABI)
GSTREAMER_ANDROID_O           := $(GSTREAMER_BUILD_DIR)/$(GSTREAMER_ANDROID_MODULE_NAME).o
GSTREAMER_ANDROID_SO          := $(GSTREAMER_BUILD_DIR)/lib$(GSTREAMER_ANDROID_MODULE_NAME).so
GSTREAMER_ANDROID_C           := $(GSTREAMER_BUILD_DIR)/$(GSTREAMER_ANDROID_MODULE_NAME).c
GSTREAMER_ANDROID_C_IN        := $(GSTREAMER_NDK_BUILD_PATH)/gstreamer_android-1.0.c.in
GSTREAMER_DEPS                := $(GSTREAMER_EXTRA_DEPS) gstreamer-1.0
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
	GSTREAMER_LD                  := -fuse-ld=mcld
else
	GSTREAMER_LD                  := -fuse-ld=gold
endif

################################
#  NDK Build Prebuilt library  #
################################

# Declare a prebuilt library module, a shared library including
# gstreamer, its dependencies and all its plugins.
# Since the shared library is not really prebuilt, but will be built
# using the defined rules in this file, we can't use the
# PREBUILT_SHARED_LIBRARY makefiles like explained in the docs,
# as it checks for the existance of the shared library. We therefore
# use a custom gstreamer_prebuilt.mk, which skips this step

include $(CLEAR_VARS)
LOCAL_MODULE            := $(GSTREAMER_ANDROID_MODULE_NAME)
LOCAL_SRC_FILES         := $(GSTREAMER_ANDROID_SO)
LOCAL_BUILD_SCRIPT      := PREBUILT_SHARED_LIBRARY
LOCAL_MODULE_CLASS      := PREBUILT_SHARED_LIBRARY
LOCAL_MAKEFILE          := $(local-makefile)
LOCAL_PREBUILT_PREFIX   := lib
LOCAL_PREBUILT_SUFFIX   := .so
LOCAL_EXPORT_C_INCLUDES := $(subst -I$1, $1, $(call pkg-config-get-includes,$(GSTREAMER_DEPS)))
LOCAL_EXPORT_C_INCLUDES += $(GSTREAMER_ROOT)/include


##################################################################
#   Our custom rules to create a shared libray with gstreamer    #
#   and the requested plugins in GSTREAMER_PLUGINS starts here   #
##################################################################

include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer_prebuilt.mk

# Some plugins use a different name for the module name, like the playback
# plugin, which uses playbin for the module name: libgstplaybin.so
fix-plugin-name = \
	$(subst gst$1 ,gst$2 ,$(GSTREAMER_PLUGINS_LIBS))

fix-deps = \
	$(subst $1,$1 $2,$(GSTREAMER_ANDROID_LIBS))


# Generate list of plugin links (eg: -lcoreelements -lvideoscale)
GSTREAMER_PLUGINS_LIBS       := $(foreach plugin, $(GSTREAMER_PLUGINS), -lgst$(plugin) )
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,camerabin,camerabin2)
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,encoding,encodebin)
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,soup,souphttpsrc)
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,gstsiren,siren)
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,sdp,sdpelem)
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,scaletempo,scaletempoplugin)
GSTREAMER_PLUGINS_LIBS       := $(call fix-plugin-name,realmedia,rmdemux)

GSTREAMER_PLUGINS_CLASSES    := $(strip \
			$(subst $(GSTREAMER_NDK_BUILD_PATH),, \
			$(foreach plugin, $(GSTREAMER_PLUGINS), \
			$(wildcard $(GSTREAMER_NDK_BUILD_PATH)$(plugin)/*.java))))

GSTREAMER_PLUGINS_WITH_CLASSES := $(strip \
			$(subst $(GSTREAMER_NDK_BUILD_PATH),, \
			$(foreach plugin, $(GSTREAMER_PLUGINS), \
			$(wildcard $(GSTREAMER_NDK_BUILD_PATH)$(plugin)))))

# Generate the plugins' declaration strings
GSTREAMER_PLUGINS_DECLARE    := $(foreach plugin, $(GSTREAMER_PLUGINS), \
			GST_PLUGIN_STATIC_DECLARE($(plugin));\n)
# Generate the plugins' registration strings
GSTREAMER_PLUGINS_REGISTER   := $(foreach plugin, $(GSTREAMER_PLUGINS), \
			GST_PLUGIN_STATIC_REGISTER($(plugin));\n)

# Generate list of gio modules
G_IO_MODULES_PATH            := $(foreach path, $(G_IO_MODULES_PATH), -L$(path))
G_IO_MODULES_LIBS            := $(foreach module, $(G_IO_MODULES), -lgio$(module))
G_IO_MODULES_DECLARE         := $(foreach module, $(G_IO_MODULES), \
			GST_G_IO_MODULE_DECLARE($(module));\n)
G_IO_MODULES_LOAD            := $(foreach module, $(G_IO_MODULES), \
			GST_G_IO_MODULE_LOAD($(module));\n)

# Get the full list of libraries
# link at least to gstreamer-1.0 in case the plugins list is empty
GSTREAMER_ANDROID_LIBS       := $(call pkg-config-get-libs,$(GSTREAMER_DEPS))
GSTREAMER_ANDROID_LIBS       += $(GSTREAMER_PLUGINS_LIBS) $(G_IO_MODULES_LIBS) -llog -lz
GSTREAMER_ANDROID_WHOLE_AR   := $(call pkg-config-get-libs-no-deps,$(GSTREAMER_DEPS))
# Fix deps for giognutls
GSTREAMER_ANDROID_LIBS       := $(call fix-deps,-lgiognutls, -lhogweed)
GSTREAMER_ANDROID_CFLAGS     := $(call pkg-config-get-includes,$(GSTREAMER_DEPS)) -I$(GSTREAMER_ROOT)/include

# In newer NDK, SYSROOT is replaced by SYSROOT_INC and SYSROOT_LINK, which
# now points to the root directory. But this will probably change in the future from:
# https://android.googlesource.com/platform/ndk/+/fa8c1b4338c1bef2813ecee0ee298e9498a1aaa7
ifndef SYSROOT
    SYSROOT_GST := $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-$(TARGET_ARCH)
else
    SYSROOT_GST := $(SYSROOT)
endif

# Create the link command
GSTREAMER_ANDROID_CMD        := $(call libtool-link,$(TARGET_CC) $(TARGET_LDFLAGS) -shared --sysroot=$(SYSROOT_GST) \
	-o $(GSTREAMER_ANDROID_SO) $(GSTREAMER_ANDROID_O) \
	-L$(GSTREAMER_ROOT)/lib -L$(GSTREAMER_STATIC_PLUGINS_PATH) $(G_IO_MODULES_PATH) \
	$(GSTREAMER_ANDROID_LIBS), $(GSTREAMER_LD)) -Wl,-no-undefined $(GSTREAMER_LD)
GSTREAMER_ANDROID_CMD        := $(call libtool-whole-archive,$(GSTREAMER_ANDROID_CMD),$(GSTREAMER_ANDROID_WHOLE_AR))

# This triggers the build of our library using our custom rules
$(GSTREAMER_ANDROID_SO): buildsharedlibrary_$(TARGET_ARCH_ABI)
$(GSTREAMER_ANDROID_SO): copyjavasource_$(TARGET_ARCH_ABI)
ifeq ($(GSTREAMER_INCLUDE_FONTS),yes)
$(GSTREAMER_ANDROID_SO): copyfontsres_$(TARGET_ARCH_ABI)
endif
ifeq ($(GSTREAMER_INCLUDE_CA_CERTIFICATES),yes)
$(GSTREAMER_ANDROID_SO): copycacertificatesres_$(TARGET_ARCH_ABI)
endif

delsharedlib_$(TARGET_ARCH_ABI): PRIV_B_DIR := $(GSTREAMER_BUILD_DIR)
delsharedlib_$(TARGET_ARCH_ABI):
	@$(call host-rm,$(prebuilt))
	@$(foreach path,$(wildcard $(PRIV_B_DIR)/sed*), $(call host-rm,$(path)))
$(LOCAL_INSTALLED): delsharedlib_$(TARGET_ARCH_ABI)

# Generates a source file that declares and registers all the required plugins
genstatic_$(TARGET_ARCH_ABI): PRIV_C := $(GSTREAMER_ANDROID_C)
genstatic_$(TARGET_ARCH_ABI): PRIV_B_DIR := $(GSTREAMER_BUILD_DIR)
genstatic_$(TARGET_ARCH_ABI): PRIV_C_IN := $(GSTREAMER_ANDROID_C_IN)
genstatic_$(TARGET_ARCH_ABI): PRIV_P_D := $(GSTREAMER_PLUGINS_DECLARE)
genstatic_$(TARGET_ARCH_ABI): PRIV_P_R := $(GSTREAMER_PLUGINS_REGISTER)
genstatic_$(TARGET_ARCH_ABI): PRIV_G_L := $(G_IO_MODULES_LOAD)
genstatic_$(TARGET_ARCH_ABI): PRIV_G_R := $(G_IO_MODULES_DECLARE)
genstatic_$(TARGET_ARCH_ABI):
	@$(HOST_ECHO) "GStreamer      : [GEN] => $(PRIV_C)"
	@$(call host-mkdir,$(PRIV_B_DIR))
	@$(call host-cp,$(PRIV_C_IN),$(PRIV_C))
	@$(HOST_SED) -i "s/@PLUGINS_DECLARATION@/$(PRIV_P_D)/g" $(PRIV_C)
	@$(HOST_SED) -i "s/@PLUGINS_REGISTRATION@/$(PRIV_P_R)/g" $(PRIV_C)
	@$(HOST_SED) -i "s/@G_IO_MODULES_LOAD@/$(PRIV_G_L)/g" $(PRIV_C)
	@$(HOST_SED) -i "s/@G_IO_MODULES_DECLARE@/$(PRIV_G_R)/g" $(PRIV_C)

# Compile the source file
$(GSTREAMER_ANDROID_O): PRIV_C := $(GSTREAMER_ANDROID_C)
$(GSTREAMER_ANDROID_O): PRIV_CC_CMD := $(TARGET_CC) --sysroot=$(SYSROOT_GST) $(TARGET_CFLAGS) \
	-c $(GSTREAMER_ANDROID_C) -Wall -Werror -o $(GSTREAMER_ANDROID_O) $(GSTREAMER_ANDROID_CFLAGS)
$(GSTREAMER_ANDROID_O): PRIV_GST_CFLAGS := $(GSTREAMER_ANDROID_CFLAGS) $(TARGET_CFLAGS)
$(GSTREAMER_ANDROID_O): genstatic_$(TARGET_ARCH_ABI)
	@$(HOST_ECHO) "GStreamer      : [COMPILE] => $(PRIV_C)"
	@$(PRIV_CC_CMD)

# Creates a shared library including gstreamer, its plugins and all the dependencies
buildsharedlibrary_$(TARGET_ARCH_ABI): PRIV_CMD := $(GSTREAMER_ANDROID_CMD)
buildsharedlibrary_$(TARGET_ARCH_ABI): PRIV_SO := $(GSTREAMER_ANDROID_SO)
buildsharedlibrary_$(TARGET_ARCH_ABI): $(GSTREAMER_ANDROID_O)
	@$(HOST_ECHO) "GStreamer      : [LINK] => $(PRIV_SO)"
	@$(PRIV_CMD)

copyjavasource_$(TARGET_ARCH_ABI):
	@$(call host-mkdir,$(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer)
	@$(call host-cp,$(GSTREAMER_NDK_BUILD_PATH)/GStreamer.java,$(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer)
	@$(foreach plugin,$(GSTREAMER_PLUGINS_WITH_CLASSES), \
		$(call host-mkdir, $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/$(plugin)))
	@$(foreach file,$(GSTREAMER_PLUGINS_CLASSES), \
		$(call host-cp, \
		$(GSTREAMER_NDK_BUILD_PATH)$(file), \
		$(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/$(file)))
ifeq ($(GSTREAMER_INCLUDE_FONTS),yes)
	@$(HOST_SED) -i "s;@INCLUDE_FONTS@;;g" $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/GStreamer.java
else
	@$(HOST_SED) -i "s;@INCLUDE_FONTS@;//;g" $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/GStreamer.java
endif
ifeq ($(GSTREAMER_INCLUDE_CA_CERTIFICATES),yes)
	@$(HOST_SED) -i "s;@INCLUDE_CA_CERTIFICATES@;;g" $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/GStreamer.java
else
	@$(HOST_SED) -i "s;@INCLUDE_CA_CERTIFICATES@;//;g" $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/GStreamer.java
endif
ifneq (,$(findstring yes,$(GSTREAMER_INCLUDE_FONTS)$(GSTREAMER_INCLUDE_CA_CERTIFICATES)))
	@$(HOST_SED) -i "s;@INCLUDE_COPY_FILE@;;g" $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/GStreamer.java
else
	@$(HOST_SED) -i "s;@INCLUDE_COPY_FILE@;//;g" $(GSTREAMER_JAVA_SRC_DIR)/org/freedesktop/gstreamer/GStreamer.java
endif

copyfontsres_$(TARGET_ARCH_ABI):
	@$(call host-mkdir,assets/fontconfig)
	@$(call host-mkdir,assets/fontconfig/fonts/truetype/)
	@$(call host-cp,$(GSTREAMER_NDK_BUILD_PATH)/fontconfig/fonts.conf,assets/fontconfig)
	@$(call host-cp,$(GSTREAMER_NDK_BUILD_PATH)/fontconfig/fonts/Ubuntu-R.ttf,assets/fontconfig/fonts/truetype)
copycacertificatesres_$(TARGET_ARCH_ABI):
	@$(call host-mkdir,assets/ssl/certs)
	@$(call host-cp,$(GSTREAMER_ROOT)/etc/ssl/certs/ca-certificates.crt,assets/ssl/certs)

