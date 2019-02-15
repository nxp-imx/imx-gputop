LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gpu-top

LOCAL_C_INCLUDES += \
  $(TOP)/external/libgpuperfcnt/include/ \
  $(TOP)/external/gputop/

LOCAL_CFLAGS += \
  -Wall -Wextra -Werror \
  -Wstrict-prototypes \
  -Wmissing-prototypes \
  -std=c99 \
  -O2 \
  -UNDEBUG \
  -DHAVE_DDR_PERF

LOCAL_SHARED_LIBRARIES += libgpuperfcnt

LOCAL_SRC_FILES := \
  gputop/debugfs.c \
  gputop/top.c

include $(BUILD_EXECUTABLE)
