# ndk makefile, see https://developer.android.com/ndk/guides/android_mk

LOCAL_PATH := $(call my-dir)

# memdump
include $(CLEAR_VARS)

LOCAL_MODULE := memdump
LOCAL_SRC_FILES := memdump.c mem.c
# LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.

# cmp
include $(CLEAR_VARS)

LOCAL_MODULE := cmp
LOCAL_SRC_FILES := cmp.c mem.c simi.c rbtree.c
# LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.

# testnapp, used for testing memdump
include $(CLEAR_VARS)

LOCAL_MODULE := testnapp
LOCAL_SRC_FILES := testnapp.c
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.
