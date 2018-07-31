# Copyright (C) 2018-2024 The Surface Frame-Animation Project
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
