LOCAL_PATH := $(call my-dir)

################################################################################
# firmwared
################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := firmwared
LOCAL_DESCRIPTION := Firmware instance manager daemon
LOCAL_CATEGORY_PATH := sphinx/firmwared

LOCAL_LIBRARIES := \
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
	utils/fdc.complete:etc/bash_completion.d/ \
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

include $(BUILD_CUSTOM)
