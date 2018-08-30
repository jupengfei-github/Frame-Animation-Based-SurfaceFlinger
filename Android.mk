LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := frame_main.cpp FrameParser.cpp \
		FrameStream.cpp FrameInfo.cpp \
		FramePlayer.cpp \
		FrameDisplay.cpp

LOCAL_STRIP_MODULE := false
LOCAL_CPPFLAGS += -fexceptions -frtti
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libandroidfw \
		libskia libEGL libGLESv3 libOpenSLES \
		libui libgui \
		libandroid_runtime \
		libbinder \

LOCAL_MODULE  := frame_animation
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
