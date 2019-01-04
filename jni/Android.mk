# ndk makefile, see https://developer.android.com/ndk/guides/android_mk

LOCAL_PATH := $(call my-dir)

# memdump
include $(CLEAR_VARS)

LOCAL_MODULE := memdump
LOCAL_SRC_FILES := main.c
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
APP_ABI := arm64-v8a
TARGET_ARCH := arm64-v8a
TARGET_ARCH_ABI := arm64-v8a
TARGET_ABI := android-26-arm64-v8a
TARGET_PLATFORM := android-26
APP_PLATFORM := android-26
# TAT, doesn't work, how to specify one target arch

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.


# testnapp, used for testing
include $(CLEAR_VARS)

LOCAL_MODULE := testnapp
LOCAL_SRC_FILES := testnapp.c
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
APP_ABI := arm64-v8a
TARGET_ARCH := arm64-v8a
TARGET_ARCH_ABI := arm64-v8a
TARGET_ABI := android-26-arm64-v8a
TARGET_PLATFORM := android-26
APP_PLATFORM := android-26

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.
