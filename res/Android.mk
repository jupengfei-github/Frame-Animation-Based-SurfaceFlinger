LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

PRODUCT_COPY_FILES += $(LOCAL_PATH)/animation.zip:data/frameAnimation/animation.zip
PRODUCT_COPY_FILES += $(LOCAL_PATH)/animation:data/frameAnimation/animation

LOCAL_MODULE_TAGS  := optional
LOCAL_PACKAGE_NAME := animation
LOCAL_CERTIFICATE  := platform
include $(BUILD_PACKAGE)
