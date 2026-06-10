/*
 * Copyright NXP 2024
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef __MALI_H
#define __MALI_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
enum page_mali {
	PAGE_SHOW_GPU=0,
	PAGE_SHOW_GPU_INFO,
	PAGE_KERNEL_MEM_USAGE,
	PAGE_PID_MEM_USAGE,
	PAGE_DVFS_UTILIZATION,
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	PAGE_DDR_PERF,
#endif
	PAGE_NO,
};

enum flags_type {
      FLAG_NONE = 0,
      FLAG_SHOW_CONTEXTS = 1,
      FLAG_IGNORE_START_ERRORS,
};



EXTERNC void gtop_display_mali_gpu_info(void);
void gtop_display_mali_debugfs_info(void);
void gtop_display_mali_debugfs_ktx_info(void);
void gtop_display_mali_debugfs_pid_mem_info(void);
void gtop_display_mali_debugfs_dvfs_utilization_info(void);
void gtop_set_display_flags(enum flags_type flag);

int gtop_enable_gpu_profile(const char *path);
int gtop_disable_gpu_profile(const char *path);

#endif /* end _MALI_H */
