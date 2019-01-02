LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := memdump
LOCAL_SRC_FILES := main.c
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
TARGET_ARCH_ABI := arm64-v8a
TARGET_PLATFORM := android-26
APP_PLATFORM := android-26


include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.

