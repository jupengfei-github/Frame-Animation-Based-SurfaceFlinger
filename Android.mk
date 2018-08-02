LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := frame_main.cpp FrameParser.cpp \
		FrameStream.cpp
LOCAL_CPP_FEATURES += exceptions
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libandroidfw
LOCAL_MODULE  := frame_animation 
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
