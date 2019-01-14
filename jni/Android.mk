# ndk makefile, see https://developer.android.com/ndk/guides/android_mk

LOCAL_PATH := $(call my-dir)

#########3
# memdump
include $(CLEAR_VARS)
LOCAL_MODULE := memdump
LOCAL_SRC_FILES := memdump.c mem.c
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
LOCAL_CFLAGS :=  -Wall -W
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.

#############3
# cmp
include $(CLEAR_VARS)
LOCAL_MODULE := cmp
LOCAL_SRC_FILES := cmp.c mem.c simi.c rbtree.c
LOCAL_CFLAGS :=  -Wall -W
# LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.


#############3
# setcmp
include $(CLEAR_VARS)
LOCAL_MODULE := setcmp
LOCAL_SRC_FILES := setcmp.c mem.c simi.c rbtree.c
LOCAL_CFLAGS :=  -Wall -W
# LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.


# testnapp, used for testing memdump
include $(CLEAR_VARS)
LOCAL_MODULE := testnapp
LOCAL_SRC_FILES := testnapp.c
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
LOCAL_CFLAGS :=  -Wall -W
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.
