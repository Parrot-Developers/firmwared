LOCAL_PATH := $(call my-dir)

################################################################################
# firmwared
################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := firmwared
LOCAL_DESCRIPTION := Firmware instance manager daemon
LOCAL_CATEGORY_PATH := simulator/

LOCAL_LIBRARIES := \
	libulog \
	libpomp \
	librs \
	libutils \
	libcrypto \
	libpidwatch \
	libuv

LOCAL_SRC_FILES := \
	$(call all-c-files-under,src)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/folders

LOCAL_COPY_FILES := \
	resources/adjectives:usr/share/$(LOCAL_MODULE)/ \
	resources/names:usr/share/$(LOCAL_MODULE)/ \
	hooks/mount.hook:usr/libexec/$(LOCAL_MODULE)/mount.hook

LOCAL_CFLAGS := \
	-DPOMP_ENABLE_ADVANCED_API \
	-fopenmp

LOCAL_LDFLAGS := -fopenmp

include $(BUILD_EXECUTABLE)
