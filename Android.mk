LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := framemain.cpp FrameParser.cpp \
        FrameStream.cpp FrameInfo.cpp \
        FramePlayer.cpp \
        FrameDisplay.cpp

LOCAL_STRIP_MODULE := false
LOCAL_CPPFLAGS += -fexceptions -fno-rtti
LOCAL_CFLAGS   += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libandroidfw \
        libEGL libGLESv3 libOpenSLES \
        libui libgui libhwui \
        libandroid_runtime \
        libbinder libbase

LOCAL_MODULE  := frameanimation
LOCAL_INIT_RC := frameanim.rc

include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
