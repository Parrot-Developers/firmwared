LOCAL_PATH := $(call my-dir)

################################################################################
# firmwared
################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := firmwared
LOCAL_DESCRIPTION := Firmware instance manager daemon
LOCAL_CATEGORY_PATH := sphinx/firmwared

LOCAL_LIBRARIES := \
	libfwd \
	libulog \
	libpomp \
	librs \
	libutils \
	libcrypto \
	libpidwatch \
	libptspair \
	liblua \
	libioutils

LOCAL_SRC_FILES := \
	$(call all-c-files-under,src)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/folders

LOCAL_COPY_FILES := \
	utils/fdc.complete:etc/bash_completion.d/fdc \
	man/firmwared.1:usr/share/man/man1/ \
	man/firmwared.conf.5:usr/share/man/man5/ \
	resources/firmwared.service:lib/systemd/system/ \
	resources/adjectives:usr/share/$(LOCAL_MODULE)/ \
	resources/names:usr/share/$(LOCAL_MODULE)/ \
	resources/firmwared.apparmor.profile:usr/share/$(LOCAL_MODULE)/ \
	hooks/curl.hook:usr/libexec/$(LOCAL_MODULE)/curl.hook \
	hooks/mount.hook:usr/libexec/$(LOCAL_MODULE)/mount.hook \
	hooks/net.hook:usr/libexec/$(LOCAL_MODULE)/net.hook

LOCAL_CFLAGS := \
	-fopenmp

LOCAL_LDFLAGS := -fopenmp

LOCAL_LDLIBS := -lapparmor

LOCAL_REQUIRED_MODULES := \
	ulogger

include $(BUILD_EXECUTABLE)

################################################################################
# libfwd
################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := libfwd
LOCAL_DESCRIPTION := Utilities for implementing firmwared clients
LOCAL_CATEGORY_PATH := sphinx/firmwared

LOCAL_SRC_FILES := \
	$(call all-c-files-under,lib)

LOCAL_LIBRARIES := \
	libblkid \
	libutils

LOCAL_CFLAGS := -DFWD_INTERPRETER=\"$(TARGET_LOADER)\"
LOCAL_LDFLAGS := -Wl,-e,$(LOCAL_MODULE)_main

LOCAL_EXPORT_C_INCLUDES  := $(LOCAL_PATH)/include

include $(BUILD_LIBRARY)

################################################################################
# fdc
################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := fdc
LOCAL_DESCRIPTION := Command line interface for firmwared
LOCAL_CATEGORY_PATH := sphinx/firmwared

LOCAL_REQUIRED_MODULES := firmwared \
	pomp-cli

LOCAL_COPY_FILES := \
	man/fdc.1:usr/share/man/man1/ \
	utils/fdc:usr/bin/fdc

LOCAL_LIBRARIES := \
	libfwd

include $(BUILD_CUSTOM)
