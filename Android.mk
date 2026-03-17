LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gpu-top

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/../libgpuperfcnt/include \
  $(LOCAL_PATH)/gputop

LOCAL_CFLAGS += \
  -Wall -Wextra -Werror \
  -std=c99 \
  -O2 \
  -UNDEBUG \
  -DHAVE_DDR_PERF

LOCAL_STATIC_LIBRARIES += libgpuperfcnt

# MALI_GPU: 1 for imx95, 0 for imx8
ifeq ($(BOARD_SOC_CLASS),IMX8)
MALI_GPU ?= 0
else ifeq ($(BOARD_SOC_CLASS),IMX9)
MALI_GPU ?= 1
endif

ifeq ($(MALI_GPU), 1)
LOCAL_CPPFLAGS += -std=c++14
LOCAL_CFLAGS += -DHAVE_GPU_MALI=1
LOCAL_SRC_FILES := \
  gputop/top_mali.c \
  gputop/gpuinfo_mali.cpp \
  gputop/debugfs_mali.c
else ifeq ($(MALI_GPU), 0)
LOCAL_SRC_FILES := \
  gputop/debugfs.c \
  gputop/top.c
endif

LOCAL_VENDOR_MODULE  := true
LOCAL_MODULE_TAGS    := optional
include $(BUILD_EXECUTABLE)
