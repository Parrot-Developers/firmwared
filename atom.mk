LOCAL_PATH := $(call my-dir)

################################################################################
# firmwared
################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := firmwared
LOCAL_DESCRIPTION := Firmware instance manager daemon
LOCAL_CATEGORY_PATH := simulator/

LOCAL_SRC_FILES += \
	$(call all-c-files-in,src)

include $(BUILD_EXECUTABLE)
