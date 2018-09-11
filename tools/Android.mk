LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gputop

LOCAL_C_INCLUDES += ../gputop ${GPUPERFCNT_INCLUDE_PATH}
LOCAL_CFLAGS += -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes \
		-std=c99 -O2 -UNDEBUG 

LOCAL_SHARED_LIBRARIES += gpuperfcnt
LOCAL_LDLIBS += -lgpuperfcnt
LOCAL_LDFLAGS += -L${GPUPERFCNT_LIB_PATH}

LOCAL_SRC_FILES := ../gputop/debugfs.c	\
		   ../gputop/top.c

include $(BUILD_EXECUTABLE)
