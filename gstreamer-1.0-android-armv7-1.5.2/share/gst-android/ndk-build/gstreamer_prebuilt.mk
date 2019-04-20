# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# this file is included from prebuilt-shared-library.mk or
# prebuilt-static-library.mk to declare prebuilt library binaries.
#

$(call assert-defined, LOCAL_BUILD_SCRIPT LOCAL_MAKEFILE LOCAL_PREBUILT_PREFIX LOCAL_PREBUILT_SUFFIX)

$(call check-defined-LOCAL_MODULE,$(LOCAL_BUILD_SCRIPT))
$(call check-LOCAL_MODULE,$(LOCAL_MAKEFILE))
$(call check-LOCAL_MODULE_FILENAME)

# Check that LOCAL_SRC_FILES contains only paths to shared libraries
ifneq ($(words $(LOCAL_SRC_FILES)),1)
$(call __ndk_info,ERROR:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): The LOCAL_SRC_FILES for a prebuilt static library should only contain one item))
$(call __ndk_error,Aborting)
endif

bad_prebuilts := $(filter-out %$(LOCAL_PREBUILT_SUFFIX),$(LOCAL_SRC_FILES))
ifdef bad_prebuilts
$(call __ndk_info,ERROR:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_SRC_FILES should point to a file ending with "$(LOCAL_PREBUILT_SUFFIX)")
$(call __ndk_info,The following file is unsupported: $(bad_prebuilts))
$(call __ndk_error,Aborting)
endif


# HACK: This part was modified from the original file as
# the shared library will be built with the gstreamer.mk rules
prebuilt := $(GSTREAMER_ANDROID_SO)

# If LOCAL_MODULE_FILENAME is defined, it will be used to name the file
# in the TARGET_OUT directory, and then the installation one. Note that
# if shouldn't have an .a or .so extension nor contain directory separators.
#
# If the variable is not defined, we determine its value from LOCAL_SRC_FILES
#
LOCAL_MODULE_FILENAME := $(strip $(LOCAL_MODULE_FILENAME))
ifndef LOCAL_MODULE_FILENAME
    LOCAL_MODULE_FILENAME := $(notdir $(LOCAL_SRC_FILES))
    LOCAL_MODULE_FILENAME := $(LOCAL_MODULE_FILENAME:%$(LOCAL_PREBUILT_SUFFIX)=%)
endif
$(eval $(call ev-check-module-filename))

LOCAL_BUILT_MODULE := $(TARGET_OUT)/$(LOCAL_MODULE_FILENAME)$(LOCAL_PREBUILT_SUFFIX)
LOCAL_OBJS_DIR     := $(TARGET_OBJS)/$(LOCAL_MODULE)
LOCAL_OBJECTS      := $(prebuilt)
LOCAL_SRC_FILES    :=

$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)

include $(BUILD_SYSTEM)/build-module.mk
