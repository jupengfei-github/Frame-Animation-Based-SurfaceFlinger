LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := frame_main.cpp FrameParser.cpp \
		FrameStream.cpp FrameInfo.cpp

LOCAL_CPPFLAGS += -fexceptions -frtti
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libandroidfw
LOCAL_MODULE  := frame_animation 
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
