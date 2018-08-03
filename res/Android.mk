LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS  := optional
LOCAL_PACKAGE_NAME := animation
LOCAL_CERTIFICATE  := platform
include $(BUILD_PACKAGE)
