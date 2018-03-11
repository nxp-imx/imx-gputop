/*
 * Copyright (C) NXP 2017
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
#ifndef __TOP_H
#define __TOP_H

#define NSEC_PER_SEC	(1000000000ULL)
#define USEC_PER_SEC	(1000000ULL)
#define MSEC_PER_SEC 	(1000ULL)

/* termios keys */
#define KB_SEEK		0x0000000A
#define KB_ESCAPE	0x0000001B
#define KEY_Q		0x00000071
#define KB_SPACE	0x00000020
#define KB_BACK		0x0000007F
#define KB_HOME		0x00485b1b
#define KB_END		0x00465b1b
#define KB_UP		0x00415B1B
#define KB_DOWN		0x00425B1B
#define KB_LEFT		0x00445B1B
#define KB_RIGHT	0x00435B1B
#define KB_KEY_P	0x00000070

#define KEY_0		0x00000030
#define KEY_1		0x00000031
#define KEY_2		0x00000032
#define KEY_3		0x00000033
#define KEY_4		0x00000034
#define KEY_5		0x00000035
#define KEY_6		0x00000036
#define KEY_7		0x00000037
#define KEY_8		0x00000038

#define KEY_D		0x00000064
#define KEY_R		0x00000072
#define KEY_H		0x00000068
#define KEY_QUESTION_MARK 	0x0000003f

#define KEY_X		0x00000078
#define KEY_S		0x00000073

#define KB_PGUP		0x00355B1B
#define KB_PGDN		0x00365B1B

#define KB_DEL		0x00335B1B

#define SET_BIT(x)		(1 << x)
#define SET_FLAG(mask, bit)	(mask |= SET_BIT(bit))
#define REMOVE_FLAG(mask, bit)	(mask &= ~SET_BIT(bit))
#define FLAG_IS_SET(mask, bit)	(mask & SET_BIT(bit))

#define STR(x)	#x
#define XSTR(x)	STR(x)

#define DELAY_SECS	1
#define DELAY_NSECS	0

/* do note these are encoded for VSI */
enum err_code {
        ERR_NO_ERROR = 0,
        ERR_NO_ACCESS = -1,
        ERR_NO_GAL_DEVICE = -2,
        ERR_KERNEL_MISMATCH = -3,
        ERR_GET_BASE_ADDRESS = -4,
        ERR_GET_CHIP_IDENTITY = -5,
        ERR_GET_CHIP_INFO = -6,
        ERR_GET_MEMORY_INFO = -7,
        ERR_GET_DEBUGFS_INFO = -8,
};

#define ARRAY_SIZE(a)		(sizeof(a)/sizeof(a[0]))

enum page {
	PAGE_SHOW_CLIENTS,
	PAGE_VID_MEM_USAGE,
	PAGE_COUNTER_PART1,	/* counters part 1 */
	PAGE_COUNTER_PART2,	/* counters part 2 */
	PAGE_DMA,		/* dma */
	PAGE_OCCUPANCY,		/* occupancy */
	PAGE_DDR_PERF,		/* DDR PMUs */
	
	PAGE_NO,
};

enum display_mode {
	MODE_PERF_SHOW_CLIENTS,
	MODE_PERF_VID_MEM_USAGE,
	MODE_PERF_COUNTER_PART1,
	MODE_PERF_COUNTER_PART2,
	MODE_PERF_DMA,
	MODE_PERF_OCCUPANCY,
	MODE_PERF_DDR,

	MODE_PERF_NO,
};

enum display_samples {
	SAMPLES_TIME = 0,
	SAMPLES_AVERAGE,
	SAMPLES_MIN,
	SAMPLES_MAX,
};

enum flags_type {
	FLAG_NONE = 0,
	FLAG_MODE = 1,
	FLAG_SAMPLES_PER_SEC = 2,
	FLAG_CONTEXT = 3,
	FLAG_REFRESH = 4,
	FLAG_DELAY = 5,
	FLAG_SHOW_CONTEXTS = 6,
	FLAG_SHOW_BATCH_CONTEXTS = 7,
	FLAG_SHOW_BATCH_PERF = 8,
	FLAG_IGNORE_START_ERRORS,
};

/* 
 * Used to determine if the profiler is enabled
 *
 * Note: currently this is supported only on Linux, under QNX
 * this is always true.
 */
struct profiler_state {
	bool enabled;
	int state;
};

struct p_page {
	uint8_t page_no;
	const char *page_desc;
};

struct gtop_data {
	enum vivante_profiler_type_counter type;

	uint32_t num_perf_counters;
	uint32_t num_perf_derived_counters;
	uint64_t total_num_perf_counters;

	uint32_t *counter_data;
	uint32_t *counter_data_last;

	uint64_t *events_per_sample;

	uint64_t *events_per_sample_max;
	uint64_t *events_per_sample_min;
	uint64_t *events_per_sample_average;

	bool *reset_after_read;
};

struct gtop {
	struct vivante_gpu_state st;
	struct gtop_data **perf_data;
};

enum dma_table_type {
	CMD_STATE,
	CMD_DMA_STATE,
	CMD_FETCH_STATE,
	CMD_DMA_REQ_STATE,
	CMD_CAL_STATE,
	CMD_VE_REQ_STATE
};

struct dma_table {
	enum dma_table_type type;
	const char *title;
	int data_size;
	const char **data_names;
	uint32_t *data;
};


struct gtop_hw_drv_info {
	struct perf_driver_info drv_info;
	struct perf_hw_info hw_info;

	/* encode the # of cores 0 - > 3D, 1 -> 2D, 2 -> VG */
	uint8_t cores[3];

	/* used to determine if we got the data and not to retrieve it every time */
	bool found;
};


#define PERF_DDR_PMUS_COUNT	2

struct perf_pmu_event_type {
	int fd;
	const char *name;
};

struct perf_pmu_ddr {
	const char *name;
	struct perf_pmu_event_type events[PERF_DDR_PMUS_COUNT];
};


#define PMU_GET_FD(pmu, i, j)		\
	pmu[i].events[j].fd

#define PMU_GET_EVENT_NAME(pmu, i, j)	\
	pmu[i].events[j].name

#define PMU_GET_TYPE_NAME(pmu, i)	\
	pmu[i].name

#define for_each_pmu(pmus, i)		\
	for (i = 0; i < ARRAY_SIZE(pmus); i++)

#define for_all_pmus(pmus, i, j)	\
	for_each_pmu(pmus, i)		\
		for_each_pmu(pmus[i].events, j)


#endif /* end __TOP_H */
