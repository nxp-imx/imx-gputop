/*
 * Copyright NXP 2017
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(__linux__)
#include <getopt.h>
#include <linux/limits.h>
#endif
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>

#include <termios.h>

#include "debugfs.h"

#include <gpuperfcnt/gpuperfcnt.h>
#include <gpuperfcnt/gpuperfcnt_vivante.h>
#include <gpuperfcnt/gpuperfcnt_log.h>

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
#include <ddrperfcnt/ddr-perf.h>
#endif

#include "states.c"
#include "top.h"

/* current flags */
static uint32_t flags = 0x0;

static const char *git_version = XSTR(GIT_SHA);
static const char *version = "1.4";

/* if a SIGINT/SIGTERM has been received */
static int volatile sig_recv = 0;
/* if a resize signal has been received */
static int volatile resized = 0;

/* for/not reading counters */
static bool paused = false;

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
static int perf_ddr_enabled = 0;
#endif

#ifdef __linux__
static struct profiler_state profiler_state = {
	.enabled = false,
	.state = 0
};
#else
static struct profiler_state profiler_state = {
	.enabled = true,
	.state = 0
};
#endif

static struct gtop_hw_drv_info gtop_info;
static struct perf_version perf_version;
static const char *governor_names[] = { "underdrive", "nominal", "overdrive" };

/* the  # of samples to take in a period of time  */
static int samples = 100;

/* current mode */
enum display_mode mode = MODE_PERF_SHOW_CLIENTS;
/* current display mode for counters */
enum display_samples samples_mode = SAMPLES_TIME;
/* curr page we're at */
uint8_t curr_page = PAGE_SHOW_CLIENTS;

/* current selected context */
static uint32_t selected_ctx = 0;

/* associated client we're tracking */
static struct debugfs_client *selected_client = NULL;

/* our prg name */
static const char *prg_name = "gputop";

/* current termios state */
struct termios tty_old;

/* this will clear the entire screen, much faster than printing new lines, see
 * console_codes(4) */
static const char clear_screen[] = { 0x1b, '[', 'H', 0x1b, '[', 'J', 0x0 };

/* this will write over the data already present */
// static const char *clear_data_screen = "\033c";

/* other modifiers */
static const char *bold_color = "\033[1m";
static const char *underlined_color = "\033[4m";
static const char *regular_color = "\033[0m";

const char *display_samples_names[] = {
	"TIME", "AVERAGE", "MIN", "MAX",
};

static struct p_page program_pages[] = {
	[PAGE_SHOW_CLIENTS]	= { PAGE_SHOW_CLIENTS, "Clients attached to GPU" },
	[PAGE_COUNTER_PART1]	= { PAGE_COUNTER_PART1, "HW Counters (context 1)" },
	[PAGE_COUNTER_PART2]	= { PAGE_COUNTER_PART2, "HW Counters (context 2)" },
	[PAGE_DMA]		= { PAGE_DMA, "DMA engines" },
	[PAGE_OCCUPANCY]	= { PAGE_OCCUPANCY, "Occupancy" },
	[PAGE_VID_MEM_USAGE]	= { PAGE_VID_MEM_USAGE, "VidMem" },
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	[PAGE_DDR_PERF]		= { PAGE_DDR_PERF, "DDR" },
#endif
};

struct dma_table dma_tables[] = {
	{ CMD_STATE, "Command state", NUM_VIV_CMD_STATE_NAMES, viv_cmd_state_names, NULL },
	{ CMD_DMA_STATE, "Command DMA state", NUM_VIV_CMD_DMA_STATE_NAMES, viv_cmd_dma_state_names, NULL },
	{ CMD_FETCH_STATE, "Command fetch state", NUM_VIV_CMD_FETCH_STATE_NAMES, viv_cmd_fetch_state_names, NULL },
	{ CMD_DMA_REQ_STATE, "DMA request state", NUM_VIV_REQ_DMA_STATE_NAMES, viv_req_dma_state_names, NULL },
	{ CMD_CAL_STATE, "Cal state", NUM_VIV_CAL_STATE_NAMES, viv_cal_state_names, NULL },
	{ CMD_VE_REQ_STATE, "VE req state", NUM_VIV_VE_REQ_STATE_NAMES, viv_ve_req_state_names, NULL }
};

#define NUM_DMA_TABLES (sizeof(dma_tables) / sizeof(dma_tables[0]))

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
/* what DDR pmus we want to read, if you want to add more you also need
 * to modify PERF_DDR_PMUS_COUNT  */
static struct perf_pmu_ddr perf_pmu_ddrs[] = {
	{ "imx8_ddr0", { { -1, "read-cycles" }, { -1, "write-cycles" } } },
	{ "imx8_ddr1", { { -1, "read-cycles" }, { -1, "write-cycles" } } },
};
static struct perf_pmu_ddr perf_pmu_axid_ddrs[] = {
	{ "imx8_ddr0", { { -1, "axid-read" }, { -1, "axid-write" } } },
	{ "imx8_ddr1", { { -1, "axid-read" }, { -1, "axid-write" } } },
  };

#endif
static int gtop_enable_profiling(struct perf_device *dev);

static uint64_t
get_ns_time(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		dprintf("clock_gettime()");
		exit(EXIT_FAILURE);
	}

	return (ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec;
}

/*
 * format uint64_t
 */
static void
format_number(char *num, size_t num_len, uint64_t sample)
{
	char tmp[100];
	size_t len;

	int groups, group_size;
	size_t in_ptr, out_ptr;
	int i, j;

	if (num_len == 0)
		return;

	snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long) sample);
	len = strlen(tmp);

	groups = (len + 2) / 3;
	group_size = len - (groups - 1) * 3;
	in_ptr = out_ptr = 0;

	num_len -= 1;

	for (i = 0; i < groups && out_ptr < num_len; i++) {
		for (j = 0; j < group_size && out_ptr < num_len; j++) {
			num[out_ptr++] = tmp[in_ptr++];
		}
		if (i != (groups - 1) && out_ptr < num_len) {
			num[out_ptr++] = ',';
		}

		group_size = 3;
	}

	num[out_ptr] = '\0';
}

static void
tty_init(struct termios *tty_o)
{
	struct termios new_tty;

	if (tcgetattr(STDIN_FILENO, tty_o) < 0 &&
	    !FLAG_IS_SET(flags, FLAG_SHOW_BATCH_PERF)) {
		fprintf(stderr, "Failed to get tty: %s\n", strerror(errno));
		fprintf(stderr, "Please use -f option when running in batch mode\n");
		exit(EXIT_FAILURE);
	}

	new_tty = *tty_o;

	/* 
	 * adjust termios to handle 
	 * read input without buffering 
	 */
	new_tty.c_lflag &= ~(ECHO | ICANON);
	new_tty.c_cc[VMIN]  = 1;
	new_tty.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_tty) < 0 &&
	    !FLAG_IS_SET(flags, FLAG_SHOW_BATCH_PERF)) {
		fprintf(stderr, "Failed to set tty: %s\n", strerror(errno));
		fprintf(stderr, "Please use -f option when running in batch mode\n");
		exit(EXIT_FAILURE);
	}
}

static void
tty_reset(struct termios *tty_o)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, tty_o);
}

static int
get_input_char(void)
{
	fd_set fds;
	int rc;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

#if defined __QNXTO__ || defined __QNX__
	struct timeval tval = {};
	tval.tv_sec = DELAY_SECS;
	tval.tv_usec = DELAY_NSECS / 1000;
	rc = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tval);
#else
	struct timespec ts = {};
	/* one second time out */
	ts.tv_sec = DELAY_SECS;
	ts.tv_nsec = DELAY_NSECS;
	rc = pselect(STDIN_FILENO + 1, &fds, NULL, NULL, &ts, NULL);
#endif

	return (rc < 0) ? 0 : rc;
}

static void
delay(void)
{
	nanosleep(&(struct timespec) { .tv_sec = DELAY_SECS, .tv_nsec = DELAY_NSECS }, NULL);
}

/*
 * Older version of the driver might not have these, or the board doesn't
 * support them so we don't fail here. When printing we verify if we have
 * something valid. Also QNX might not support all of these.
 */
static void
gtop_get_clocks_governor(struct gtop_clocks_governor *d)
{
	debugfs_get_gpu_clocks(&d->clock, NULL);
	debugfs_get_current_gpu_governor(&d->governor);
}

static bool
gtop_is_chip_model(uint32_t model, struct perf_device *dev)
{
	struct perf_hw_info *hw_info_iter;
	struct perf_hw_info hw_info = {};

	perf_get_hw_info(&hw_info, dev);
	list_for_each(hw_info_iter, hw_info.head) {
		if (hw_info_iter->model == model) {
			perf_free_hw_info(&hw_info, dev);
			return true;
		}
	}

	perf_free_hw_info(&hw_info, dev);
	return false;
}

static void
gtop_display_interactive_counters(const struct gtop_data *gtop,
				  uint32_t id, bool display_nl,
				  struct perf_device *dev)
{
	char num[100];
	struct perf_counter_info *info;

	switch (samples_mode) {
	case SAMPLES_TIME:
		format_number(num, sizeof(num), gtop->events_per_sample[id]);
		break;
	case SAMPLES_MIN:
		format_number(num, sizeof(num), gtop->events_per_sample_min[id]);
		break;
	case SAMPLES_AVERAGE:
		format_number(num, sizeof(num), gtop->events_per_sample_average[id]);
		break;
	case SAMPLES_MAX:
		format_number(num, sizeof(num), gtop->events_per_sample_max[id]);
		break;
	default:
		abort();
	}

	info = perf_get_counter_info(gtop->type, id, dev);
	if (!info)
		return;

	if (!display_nl)
		fprintf(stdout, "%15.15s %-50.50s ", num, info->desc);
	else
		fprintf(stdout, "%15.15s %-50.50s\n", num, info->desc);
}

static void
gtop_display_interactive_mode_perf(const struct gtop_data *gtop,
				   struct perf_device *dev)
{
	/*
	 * display the counter(s) over two columns so that we can
	 * print all of them. Have to see what to do in case the columns
	 * do not fit.
	 *
	 * num_value	descriptions	num_value	description
	 *
	 */
	uint32_t c;
	for (c = 0; c < gtop->num_perf_counters; c += 2) {
		uint32_t k = c + 1;
		gtop_display_interactive_counters(gtop, c, false, dev);

		/* we would reach over in case we just print a new line */
		if (k >= gtop->num_perf_counters) {
			fprintf(stdout, "\n");
		} else {
			gtop_display_interactive_counters(gtop, k, true, dev);
		}
	}
}

static void
gtop_display_interactive_mode_occupancy(const struct vivante_gpu_state *st)
{
	double percent;
	size_t i;

	for (i = 0; i < NUM_VIV_IDLE_MODULES; i++) {
		percent = 
			100.0f * (double) st->viv_idle_states[i] /
			(double) samples;

		/* if it inverse subtract */
		if (vivante_idle_module_names[i].inv)
			percent = 100.0f - percent;

		fprintf(stdout, " %s %.2f%%\n", vivante_idle_module_names[i].name,
				percent);
	}


	double cycles_idle_percent_core0;

	cycles_idle_percent_core0 = 100.0f * (double) st->total_idle_cycles_core0 / 
		(double) samples;


	fprintf(stdout, " IDLE0%28s %.2f%%\n", "", cycles_idle_percent_core0);
	fprintf(stdout, " USAGE%28s %.2f%%\n", "", 100.0f - cycles_idle_percent_core0);

	if (gtop_info.cores[0] > 1) {
		double cycles_idle_percent_core1;

		cycles_idle_percent_core1 = 100.0f * (double) st->total_idle_cycles_core1 / 
			(double) samples;

		fprintf(stdout, " IDLE1%28s %.2f%%\n", "", cycles_idle_percent_core1);
		fprintf(stdout, " USAGE%28s %.2f%%\n", "", 100.0f - cycles_idle_percent_core1);
	}
}


static void
attach_gpu_state_to_dma_table(struct dma_table *table, struct vivante_gpu_state *st)
{
	switch (table->type) {
	case CMD_STATE:
		table->data = st->viv_cmd_state;
		break;
	case CMD_DMA_STATE:
		table->data = st->viv_cmd_dma_state;
		break;
	case CMD_FETCH_STATE:
		table->data = st->viv_cmd_fetch_state;
		break;
	case CMD_DMA_REQ_STATE:
		table->data = st->viv_req_dma_state;
		break;
	case CMD_CAL_STATE:
		table->data = st->viv_cal_state;
		break;
	case CMD_VE_REQ_STATE:
		table->data = st->viv_ve_req_state;
		break;
	}
}

static void
gtop_display_interactive_mode_dma(const struct vivante_gpu_state *st)
{
	size_t t = 0;
	int i;

	/* first display the commands */
	struct dma_table *table = &dma_tables[t];
	attach_gpu_state_to_dma_table(table, (struct vivante_gpu_state *) st);

	fprintf(stdout, "%s\n", table->title);

	for (i = 0; i < table->data_size; i++) {
		double percent;
		percent = 100.0f * ((double) table->data[i] / (double) samples);
		fprintf(stdout, "%10.10s %.2f %%\n", table->data_names[i], percent);
	}

	fprintf(stdout, "\n");

	/* then all the other tables */
	for (t = 1; t < NUM_DMA_TABLES; t++) {

		struct dma_table *table = &dma_tables[t];
		attach_gpu_state_to_dma_table(table, (struct vivante_gpu_state *) st);

		fprintf(stdout, "%s\n", table->title);

		for (i = 0; i < table->data_size; i += 2) {

			int k = i + 1;
			double percent;

			percent = 100.0f * ((double) table->data[i] / (double) samples);
			fprintf(stdout, "%10.10s %.2f %% ", table->data_names[i], percent);

			if (k < table->data_size) {
				double percent;
				percent = 100.0f * ((double) table->data[k] / (double) samples);
				fprintf(stdout, "%10.10s %.2f %% ", table->data_names[k], percent);
			}
		}

		fprintf(stdout, "\n");
	}
}

static void
gtop_get_gtop_info(struct perf_device *dev, struct gtop_hw_drv_info *ginfo)
{
	struct perf_hw_info *hw_info_iter = NULL;

	perf_get_driver_info(&ginfo->drv_info, dev);
	perf_get_hw_info(&ginfo->hw_info, dev);

	list_for_each(hw_info_iter, ginfo->hw_info.head) {
		enum perf_core_type c_type = 
			perf_get_core_type(hw_info_iter->id, dev);
		switch (c_type) {
		case PERF_CORE_3D:
			ginfo->cores[0]++;
			break;
		case PERF_CORE_2D:
			ginfo->cores[1]++;
			break;
		case PERF_CORE_VG:
			ginfo->cores[2]++;
			break;
		default:
			break;
		}
	}

	ginfo->found = true;
}

static void
gtop_free_gtop_info(struct perf_device *dev, struct gtop_hw_drv_info *ginfo)
{
	perf_free_hw_info(&ginfo->hw_info, dev);
}

static void
gtop_display_drv_info(struct perf_device *dev, struct gtop_hw_drv_info *ginfo, struct gtop_clocks_governor governor)
{
	struct perf_hw_info *hw_info_iter = NULL;
	int core_id = 0;

	/* print info about driver */
	fprintf(stdout, "Galcore version:%d.%d.%d.%d, ",
			gtop_info.drv_info.major, gtop_info.drv_info.minor,
			gtop_info.drv_info.patch, gtop_info.drv_info.build);
	fprintf(stdout, "gpuperfcnt:%s, %s\n",
			perf_version.git_version, perf_version.version);

	if (governor.governor.governor)
		fprintf(stdout, "Governor: %s\n", governor_names[governor.governor.governor - 1]);

	list_for_each(hw_info_iter, ginfo->hw_info.head) {
		enum perf_core_type c_type = 
			perf_get_core_type(hw_info_iter->id, dev);
		switch (c_type) {
		case PERF_CORE_3D:
			fprintf(stdout, "3D:");
			break;
		case PERF_CORE_2D:
			fprintf(stdout, "2D:");
			break;
		case PERF_CORE_VG:
			fprintf(stdout, "VG:");
			break;
		default:
			fprintf(stdout, "UNKNOWN:");
			break;
		}
		fprintf(stdout, "GC%x,Rev:%x ",
				hw_info_iter->model,
				hw_info_iter->revision);

		if (governor.clock.gpu_core_0 && core_id == 0)
			fprintf(stdout, "Core: %u MHz, Shader: %u MHz ",
					governor.clock.gpu_core_0 / (1000 * 1000),
					governor.clock.shader_core_0 / (1000 * 1000));

		if (governor.clock.gpu_core_0  && governor.clock.gpu_core_1 && core_id == 0)
			fprintf(stdout, "\n");


		if (governor.clock.gpu_core_1 && core_id == 1)
			fprintf(stdout, "Core: %u MHz, Shader: %u MHz ",
					governor.clock.gpu_core_1 / (1000 * 1000),
					governor.clock.shader_core_1 / (1000 * 1000));

		core_id++;
	}

	fprintf(stdout, "\n");
	fprintf(stdout, "3D Cores:%d,2D Cores:%d,VG Cores:%d\n",
			ginfo->cores[0], ginfo->cores[1], ginfo->cores[2]);

}

static void
gtop_display_vid_mem_usage(struct perf_device *dev, struct gtop_hw_drv_info *ginfo)
{
	struct debugfs_client clients;
	struct debugfs_client *curr_client;
	struct debugfs_vid_mem_client vid_mem_client;
	struct gtop_clocks_governor governor = {};
	uint32_t scale_factor = 1024;

	int nr_clients = 0;

	gtop_get_clocks_governor(&governor);

	gtop_display_drv_info(dev, ginfo, governor);

	nr_clients = debugfs_get_current_clients(&clients, NULL);

	/* if not clients are attached bail out */
	if (!nr_clients) {
		return;
	}

	fprintf(stdout, "%s", underlined_color);
	fprintf(stdout, "%6s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s", 
			"PID", "IN", "VE", "TE", "RT", "DE",
			"BM", "TS", "IM", "MA", "SC",
			"HZ", "IC", "TD", "FE", "TFB");
	fprintf(stdout, "\n");

	fprintf(stdout, "%s", regular_color);
	list_for_each(curr_client, clients.head) {
		/* skip our program from attached programs */
		if (!strncmp(curr_client->name, prg_name, strlen(prg_name)))
			continue;

		int ret = debugfs_get_vid_mem(&vid_mem_client, curr_client->pid);
		if (ret == -1)
			goto out_exit;

		/* scale them when their are too bigger */
		if (vid_mem_client.index > scale_factor)
			vid_mem_client.index /= scale_factor;
		if (vid_mem_client.vertex > scale_factor)
			vid_mem_client.vertex /= scale_factor;
		if (vid_mem_client.texture > scale_factor)
			vid_mem_client.texture /= scale_factor;
		if (vid_mem_client.render_target > scale_factor)
			vid_mem_client.render_target /= scale_factor;
		if (vid_mem_client.depth > scale_factor)
			vid_mem_client.depth /= scale_factor;
		if (vid_mem_client.bitmap > scale_factor)
			vid_mem_client.bitmap /= scale_factor;
		if (vid_mem_client.tile_status > scale_factor)
			vid_mem_client.tile_status /= scale_factor;
		if (vid_mem_client.image > scale_factor)
			vid_mem_client.image /= scale_factor;
		if (vid_mem_client.mask > scale_factor)
			vid_mem_client.mask /= scale_factor;
		if (vid_mem_client.scissor > scale_factor)
			vid_mem_client.scissor /= scale_factor;
		if (vid_mem_client.hz > scale_factor)
			vid_mem_client.hz /= scale_factor;
		if (vid_mem_client.i_cache > scale_factor)
			vid_mem_client.i_cache /= scale_factor;
		if (vid_mem_client.tx_desc > scale_factor)
			vid_mem_client.tx_desc /= scale_factor;
		if (vid_mem_client.fence > scale_factor)
			vid_mem_client.fence /= scale_factor;
		if (vid_mem_client.tfbheader > scale_factor)
			vid_mem_client.tfbheader /= scale_factor;

		fprintf(stdout, "%6u ",
				curr_client->pid);
		/* display */
		fprintf(stdout, "%5u %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u %5u",
				vid_mem_client.index, vid_mem_client.vertex,
				vid_mem_client.texture, vid_mem_client.render_target,
				vid_mem_client.depth, vid_mem_client.bitmap,
				vid_mem_client.tile_status, vid_mem_client.image,
				vid_mem_client.mask, vid_mem_client.scissor,
				vid_mem_client.hz, vid_mem_client.i_cache,
				vid_mem_client.tx_desc, vid_mem_client.fence,
				vid_mem_client.tfbheader);
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\nN: If value is bigger than %u, assume kBytes, otherwise Bytes\n", scale_factor);
out_exit:
	/* free all resources */
	debugfs_free_clients(&clients);
}

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
static void
gtop_set_perf_pmus_ddr(void)
{
    FILE *file;
    char buf[1024];
    file = fopen("/sys/devices/soc0/soc_id", "r");
    if (file == NULL) return;
    if((fgets(buf, 1024, file)) != NULL)
    {
        if(!strncmp(buf, "i.MX8MP",7)){
           memset((char *)perf_pmu_ddrs, 0,sizeof(perf_pmu_ddrs));
           memcpy((char *)perf_pmu_ddrs, (char *)perf_pmu_axid_ddrs, sizeof(perf_pmu_axid_ddrs));
        }
    }
}

static void
gtop_configure_pmus(void)
{
	uint32_t config, type;
	unsigned int i, j;

  gtop_set_perf_pmus_ddr();
	for_all_pmus(perf_pmu_ddrs, i, j) {
		const char *type_name = PMU_GET_TYPE_NAME(perf_pmu_ddrs, i);
		const char *event_name = PMU_GET_EVENT_NAME(perf_pmu_ddrs, i, j);
		if ( ! perf_event_pmu_exist(type_name) )
			continue;

		type = perf_event_pmu_get_type(type_name);
		config = perf_event_pmu_get_event(type_name, event_name);

		/* we might end up running on boards which do not support DDR or
		 * those PMus so we just ignore them */
		if (type == 0 || config == 0)
			continue;

		/* assign the fd */
		PMU_GET_FD(perf_pmu_ddrs, i, j) =
			perf_event_pmu_ddr_config(type, config);

		assert(PMU_GET_FD(perf_pmu_ddrs, i, j) > 0);
	}
}

static void
gtop_enable_pmus(void)
{
	unsigned int i, j;

	for_all_pmus(perf_pmu_ddrs, i, j) {
		int fd = PMU_GET_FD(perf_pmu_ddrs, i, j);
		if (fd > 0) {
			perf_event_pmu_reset(fd);
			perf_event_pmu_enable(fd);
		}
	}
}

static void
gtop_disable_pmus(void)
{
	unsigned int i, j;

	for_all_pmus(perf_pmu_ddrs, i, j) {
		int fd = PMU_GET_FD(perf_pmu_ddrs, i, j);
		if (fd > 0)
			perf_event_pmu_disable(fd);
	}
}

static inline void
gtop_display_white_space(size_t amount)
{
	for (size_t i = 0; i < amount; i++)
		fprintf(stdout, " ");
}

static void
gtop_display_perf_pmus(void)
{
	unsigned int i, j;
	if (!perf_ddr_enabled) {
		gtop_configure_pmus();
		gtop_enable_pmus();
		perf_ddr_enabled = 1;
	}

	fprintf(stdout, "\n");
	fprintf(stdout, "%s%5s", underlined_color, "");

	for_all_pmus(perf_pmu_ddrs, i, j) {
		int fd = PMU_GET_FD(perf_pmu_ddrs, i, j);
		if (fd > 0) {
			const char *type_name = PMU_GET_TYPE_NAME(perf_pmu_ddrs, i);
			const char *event_name = PMU_GET_EVENT_NAME(perf_pmu_ddrs, j, j);

			fprintf(stdout, "%s/%s%3s",
				type_name, event_name, "");

		}
	}

	fprintf(stdout, "(MB)%s\n", regular_color);
	fprintf(stdout, "%5s", "");
	
	char buf[PATH_MAX];
	/* use a marker to known how much we need to shift on the right and
	 * print white spaces */
	int p = 0;

	for_all_pmus(perf_pmu_ddrs, i, j) {
		int fd = PMU_GET_FD(perf_pmu_ddrs, i, j);
		if (fd > 0) {
			uint64_t counter_val = perf_event_pmu_read(fd);
			const char *type_name = PMU_GET_TYPE_NAME(perf_pmu_ddrs, i);
			const char *event_name = PMU_GET_EVENT_NAME(perf_pmu_ddrs, j, j);

			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s/%s", type_name, event_name);

			size_t buf_len = strlen(buf);
			double display_value = (double) counter_val * 16 / (1024 * 1024);
			
			/* how much we need the remove from default value */
			size_t adjust_float = 0;

			if (display_value > 10.0f)
				adjust_float++;
			if (display_value > 100.0f)
				adjust_float++;
			if (display_value > 1000.0f)
				adjust_float++;

			if (p == 0)
				gtop_display_white_space(buf_len - 4 - adjust_float);
			else
				/* +3 is %s/%s from buf, but we need to add this
				 * only from the second hence p > 0
				 */
				gtop_display_white_space(buf_len - 4 + 3 - adjust_float);

			/* 0.123 -> 4 chars */
			fprintf(stdout, "%.2f", display_value);

			perf_event_pmu_reset(fd);

			p++;
		}
	}
	fprintf(stdout, "\n");
}

static void
gtop_display_perf_pmus_short(void)
{
	unsigned int i, j;
	if (!perf_ddr_enabled) {
		gtop_configure_pmus();
		gtop_enable_pmus();
		perf_ddr_enabled = 1;
	}

	fprintf(stdout, "\n");

	for_each_pmu(perf_pmu_ddrs, i) {
		char type_name_upper[1024];
		const char *type_name = PMU_GET_TYPE_NAME(perf_pmu_ddrs, i);

		memset(type_name_upper, 0, 1024);

		const char *start = type_name;
		int index = 0;

		while (start && *start) {
			assert(index < 1024);
			type_name_upper[index] = toupper(*start);
			start++;
			index++;
		}
		type_name_upper[index] = '\0';

		fprintf(stdout, "%s", bold_color);
		fprintf(stdout, "%s: ", type_name_upper);
		fprintf(stdout, "%s", regular_color);

		for_each_pmu(perf_pmu_ddrs[i].events, j) {
			int fd = PMU_GET_FD(perf_pmu_ddrs, i, j);
			if (fd > 0) {
				const char *event_name = PMU_GET_EVENT_NAME(perf_pmu_ddrs, i, j);
				uint64_t counter_val = perf_event_pmu_read(fd);
				double display_value;
				if(!strncmp(event_name, "axid",4))
						display_value = counter_val  / (1024.0*1024.0);
				else
						display_value = counter_val *16  / (1024.0*1024.0);

				fprintf(stdout, "%s:%.2f", event_name, display_value);
				if (j < (ARRAY_SIZE(perf_pmu_ddrs[i].events) - 1))
						fprintf(stdout, ",");
				perf_event_pmu_reset(fd);
			}
		}
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
}
#endif

static void
gtop_display_clients(struct perf_device *dev, struct gtop_hw_drv_info *ginfo)
{
	struct debugfs_client clients;
	struct debugfs_client *curr_client;
	struct perf_client_memory client_total = {};
	struct gtop_clocks_governor governor = {};

	int nr_clients = 0;

	/* get and display clocks */
	gtop_get_clocks_governor(&governor);
	gtop_display_drv_info(dev, ginfo, governor);

	nr_clients = debugfs_get_current_clients(&clients, NULL);

	/* if not clients are attached bail out */
	if (!nr_clients) {
		return;
	}

	/* get all the contexts once to speed up display */
	if (debugfs_get_contexts(&clients, NULL) < 0) {
		return;
	}

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	gtop_display_perf_pmus_short();
#endif

	/* draw with bold */
	fprintf(stdout, "%s", underlined_color);

	if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS)) {
		fprintf(stdout, " %7s %9s %10s %10s %12s %10s %16s %14s\n",
				"PID", "RES(kB)", "CONT(kB)",
				"VIRT(kB)", "Non-PGD(kB)", "Total(kB)",
				"CMD", "CTX");
	} else {
		fprintf(stdout, " %7s %9s %10s %10s %12s %10s %16s\n",
				"PID", "RES(kB)", "CONT(kB)",
				"VIRT(kB)", "Non-PGD(kB)", "Total(kB)",
				"CMD");
	}

	/* reset drawing */
	fprintf(stdout, "%s", regular_color);

	list_for_each(curr_client, clients.head) {

		/* skip our program from attached programs */
		if (!strncmp(curr_client->name, prg_name, strlen(prg_name)))
			continue;

		struct perf_client_memory cmem = {};

		/* get contexts for this client */

		/* 
		 * skip also programs that do not have CTXs.
		 * Is this indeed valid? For X11 apps it seems so.
		 */
#if !defined __QNXTO__ && !defined __QNX__
		if (curr_client->ctx_no == 0)
			continue;
#endif

		perf_get_client_memory(&cmem, curr_client->pid, dev);

		fprintf(stdout, "%1s%7u%1s", "",
				curr_client->pid, "");

		fprintf(stdout, "%1s%8"PRIu64"%3s%8"PRIu64"%3s%8"PRIu64"%5s%8"PRIu64"%3s%8"PRIu64,
				"", cmem.reserved / (1024), 
				"", cmem.contigous / (1024), 
				"", cmem._virtual / (1024), 
				"", cmem.non_paged / (1024), 
				"", cmem.total / (1024));

		/* compute total amount */
		client_total.total += cmem.total;
		client_total.reserved += cmem.reserved;
		client_total.contigous += cmem.contigous;
		client_total._virtual += cmem._virtual;
		client_total.non_paged += cmem.non_paged;

		fprintf(stdout, "   %14s", curr_client->name);

		if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS)) {
			for (size_t ctx = 0; ctx < curr_client->ctx_no; ctx++) {
				if (ctx == (curr_client->ctx_no - 1))
					fprintf(stdout, " %2d", curr_client->ctx[ctx]);
				else
					fprintf(stdout, " %2d,", curr_client->ctx[ctx]);
			}
		}
		
		fprintf(stdout, "\n");
	}


	fprintf(stdout, "\n%s", bold_color);
	fprintf(stdout, "TOT: ");
	fprintf(stdout, "%s", regular_color);
	fprintf(stdout, "%13"PRIu64" %10"PRIu64" %10"PRIu64" %12"PRIu64" %10"PRIu64, 
			client_total.reserved / (1024) ,
			client_total.contigous / (1024),
			client_total._virtual / (1024),
			client_total.non_paged / (1024),
			client_total.total / (1024));

#if !defined __QNXNTO__ && !defined __QNX__
	char cmdline[512];
	uint32_t contigousSize;
	FILE *file;

	file = fopen("/sys/module/galcore/parameters/contiguousSize", "rb");
	if (file == NULL)
		goto skip;

	memset(cmdline, 0, sizeof(cmdline));
	size_t nread = fread(cmdline, 1, sizeof(cmdline) - 1, file);
	fclose(file);

	(void) nread;
	sscanf(cmdline, "%"PRIu32, &contigousSize);


	fprintf(stdout, "\n");
	fprintf(stdout, "%s", bold_color);
	fprintf(stdout, "TOT_CON:");
	fprintf(stdout, "%s", regular_color);
	fprintf(stdout, "%9s- %9s- %9s- %11s-%11" PRIu64 " ", 
			"", "", "", "", (contigousSize - client_total.reserved) / (1024));
skip:
#endif
	/* free all resources */
	debugfs_free_clients(&clients);
}

static void
gtop_check_profiler_state(void)
{
	/* verify that we have profiling enabled */
	if (!profiler_state.enabled) {
		switch (profiler_state.state) {
		case -1:
			fprintf(stdout, "Err: gpuProfiler not enabled in kernel!...\n");
			break;
		case -2:
			fprintf(stdout, "Err: powerManagement not disabled in kernel!...\n");
			break;
		case 0:
		default:
			break;
		}
	}
}

static void
gtop_display_interactive(struct perf_device *dev, const struct gtop gtop)
{

	fflush(stdout);
	fprintf(stdout, "%s", clear_screen);

	/* check any errors related to module *not* being properly loaded */
	gtop_check_profiler_state();

	if (FLAG_IS_SET(flags, FLAG_MODE))
		fprintf(stdout, "%s | %u / %u ", program_pages[mode].page_desc, mode, (MODE_PERF_NO - 1));
	else
		fprintf(stdout, "%s | %u / %u ", program_pages[curr_page].page_desc, curr_page, (PAGE_NO - 1));

	fprintf(stdout, " (sample_mode: %s", display_samples_names[samples_mode]);
	if (samples_mode == SAMPLES_TIME) {
		fprintf(stdout, " - %d.%d secs)", DELAY_SECS, DELAY_NSECS);
	} else {
		fprintf(stdout, ")");
	}

	if (selected_client && selected_client->name) {
		fprintf(stdout, "(PID: %u, Program: %s, CTX = %u)\n",
				selected_client->pid, selected_client->name, selected_ctx);
	} else {
		fprintf(stdout, "\n");
	}

	if (FLAG_IS_SET(flags, FLAG_MODE)) {
		switch (mode) {
		case MODE_PERF_SHOW_CLIENTS:
			gtop_display_clients(dev, &gtop_info);
			break;
		case MODE_PERF_VID_MEM_USAGE:
			gtop_display_vid_mem_usage(dev, &gtop_info);
			break;
		case MODE_PERF_COUNTER_PART1:
			gtop_display_interactive_mode_perf(gtop.perf_data[VIV_PROF_COUNTER_PART1], dev);
			break;
		case MODE_PERF_COUNTER_PART2:
			gtop_display_interactive_mode_perf(gtop.perf_data[VIV_PROF_COUNTER_PART2], dev);
			break;
		case MODE_PERF_DMA:
			gtop_display_interactive_mode_dma(&gtop.st);
			break;
		case MODE_PERF_OCCUPANCY:
			gtop_display_interactive_mode_occupancy(&gtop.st);
			break;
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
		case MODE_PERF_DDR:
			gtop_display_perf_pmus();
			break;
#endif
		default:
			dprintf("No valid page specified in interactive mode\n");
			exit(EXIT_FAILURE);
			break;
		}
	} else {
		switch (curr_page) {
		case PAGE_SHOW_CLIENTS:
			gtop_display_clients(dev, &gtop_info);
			break;
		case PAGE_VID_MEM_USAGE:
			gtop_display_vid_mem_usage(dev, &gtop_info);
			break;
		case PAGE_COUNTER_PART1:
			gtop_display_interactive_mode_perf(gtop.perf_data[VIV_PROF_COUNTER_PART1], dev);
			break;
		case PAGE_COUNTER_PART2:
			gtop_display_interactive_mode_perf(gtop.perf_data[VIV_PROF_COUNTER_PART2], dev);
			break;
		case PAGE_DMA:
			gtop_display_interactive_mode_dma(&gtop.st);
			break;
		case PAGE_OCCUPANCY:
			gtop_display_interactive_mode_occupancy(&gtop.st);
			break;
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
		case PAGE_DDR_PERF:
			gtop_display_perf_pmus();
			break;
#endif
		default:
			dprintf("No valid mode specified in interactive mode\n");
			exit(EXIT_FAILURE);
			break;
		}
	}

	fprintf(stdout, "\n");

}

static struct gtop_data *
gtop_data_create(enum vivante_profiler_type_counter type,
		 uint32_t num_perf_counters,
		 uint32_t num_perf_derived_counters)
{
	uint64_t total_num_perf_counters =
		num_perf_counters + num_perf_derived_counters;

	struct gtop_data *gtop = malloc(sizeof(*gtop));
	if (!gtop) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}
	memset(gtop, 0, sizeof(*gtop));

	gtop->type = type;
	gtop->num_perf_counters = num_perf_counters;
	gtop->num_perf_derived_counters = num_perf_derived_counters;
	gtop->total_num_perf_counters = total_num_perf_counters;

	gtop->counter_data = calloc(total_num_perf_counters, sizeof(uint32_t));
	if (!gtop->counter_data) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}
	gtop->counter_data_last = calloc(total_num_perf_counters, sizeof(uint32_t));
	if (!gtop->counter_data_last) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}

	gtop->events_per_sample = calloc(total_num_perf_counters, sizeof(uint64_t));
	if (!gtop->events_per_sample) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}

	gtop->events_per_sample_max = calloc(total_num_perf_counters, sizeof(uint64_t));
	if (!gtop->events_per_sample_max) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}

	gtop->events_per_sample_min = calloc(total_num_perf_counters, sizeof(uint64_t));
	if (!gtop->events_per_sample_min) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}

	gtop->events_per_sample_average = calloc(total_num_perf_counters, sizeof(uint64_t));
	if (!gtop->events_per_sample_average) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}

	gtop->reset_after_read = calloc(total_num_perf_counters, sizeof(bool));
	if (!gtop->reset_after_read) {
		dprintf("malloc?\n");
		exit(EXIT_FAILURE);
	}

	return gtop;
}

static void
gtop_data_destroy(struct gtop_data *gtop)
{
	if (gtop) {

		if (gtop->counter_data)
			free(gtop->counter_data);

		if (gtop->counter_data_last)
			free(gtop->counter_data_last);

		if (gtop->events_per_sample)
			free(gtop->events_per_sample);

		if (gtop->events_per_sample_max)
			free(gtop->events_per_sample_max);

		if (gtop->events_per_sample_min)
			free(gtop->events_per_sample_min);

		if (gtop->events_per_sample_average)
			free(gtop->events_per_sample_average);

		if (gtop->reset_after_read)
			free(gtop->reset_after_read);

		free(gtop);
		gtop = NULL;
	}
}

static void
gtop_data_clear_samples(struct gtop_data *gtop)
{
	if (gtop) {
		if (gtop->events_per_sample)
			memset(gtop->events_per_sample, 0, sizeof(uint64_t) *
					gtop->total_num_perf_counters);
	}
}

static int
gtop_compute_mode_dma(struct perf_device *dev, struct vivante_gpu_state *st)
{
	uint32_t data = 0;
	uint32_t cmd_state_idx;
	int err;

	if (!profiler_state.enabled) {

		if (perf_check_profiler(&profiler_state.state, dev) < 0)
			return -1;

		if (gtop_enable_profiling(dev) < 0)
			return -1;

		if (perf_profiler_start(dev) < 0)
			return -1;

		profiler_state.enabled = true;

	}

	err = perf_read_register(PERF_MGPU_3D_CORE_0, VIVS_FE_DMA_DEBUG_STATE, &data, dev);
	if (err < 0) {
		dprintf("Failed perf_read_register()\n");
		return err;
	}

	cmd_state_idx = data & 0x1f;
	if (cmd_state_idx >= (NUM_VIV_CMD_STATE_NAMES - 1)) {
		cmd_state_idx = NUM_VIV_CMD_STATE_NAMES - 1;
	}


	st->viv_cmd_state[cmd_state_idx]++;
	st->viv_cmd_dma_state[(data >> 8) & 3]++;
	st->viv_cmd_fetch_state[(data >> 10) & 3]++;

	st->viv_req_dma_state[(data >> 12)  & 3]++;
	st->viv_cal_state[(data >> 14) & 3]++;
	st->viv_ve_req_state[(data >> 16) & 3]++;


	return 0;
}

static int
gtop_compute_mode_occupancy(struct perf_device *dev, struct vivante_gpu_state *st)
{
	uint32_t data = 0;
	uint32_t mid;
	int err;
	uint32_t idle_reg_addr = GC_TOTAL_IDLE_CYCLES;

	if (gtop_is_chip_model(0x880, dev)) {
		idle_reg_addr = GC_TOTAL_CYCLES;
	} else if (gtop_is_chip_model(0x2000, dev)) {
		idle_reg_addr = GC_2000_TOTAL_IDLE_CYCLES;
	}


	if (!profiler_state.enabled) {

		if (perf_check_profiler(&profiler_state.state, dev) < 0)
			return -1;

		if (gtop_enable_profiling(dev) < 0)
			return -1;

		if (perf_profiler_start(dev) < 0)
			return -1;

		profiler_state.enabled = true;

	}

	err = perf_read_register(PERF_MGPU_3D_CORE_0, VIVS_HI_IDLE_STATE, &data, dev);
	if (err < 0) {
		dprintf("Failed to read 0x%x\n", VIVS_HI_IDLE_STATE);
		return err;
	}

	for (mid = 0; mid < NUM_VIV_IDLE_MODULES; mid++) {
		if (data & vivante_idle_module_names[mid].bit) {
			st->viv_idle_states[mid]++;
		}
	}

	/*
	 * used to be read then reset, turns out reset then read works better.
	 */
	err = perf_write_register(PERF_MGPU_3D_CORE_0, idle_reg_addr, 0x0, dev);
	if (err < 0) {
		dprintf("Failed to reset 0x%x for CORE0\n", idle_reg_addr);
		return err;
	}

	if (gtop_info.cores[0] > 1) {
		err = perf_write_register(PERF_MGPU_3D_CORE_1, idle_reg_addr, 0x0, dev);
		if (err < 0) {
			dprintf("Failed to reset 0x%x for CORE1\n", idle_reg_addr);
			return err;
		}
	}
	err = perf_read_register(PERF_MGPU_3D_CORE_0, idle_reg_addr, &st->idle_cycles_core0, dev);
	if (err < 0) {
		dprintf("Failed to read 0x%x for CORE0\n", idle_reg_addr);
		return err;
	}

	if (st->idle_cycles_core0)
		st->total_idle_cycles_core0++;

	if (gtop_info.cores[0] > 1) {
		err = perf_read_register(PERF_MGPU_3D_CORE_1, idle_reg_addr, &st->idle_cycles_core1, dev);
		if (err < 0) {
			dprintf("Failed to read 0x%x for CORE1\n", idle_reg_addr);
			return err;
		}
		if (st->idle_cycles_core1)
			st->total_idle_cycles_core1++;
	}

	return 0;
}

static void
gtop_scale_counters_by(struct gtop_data *gtop, uint64_t diff)
{
	uint32_t c;

	/* scale counters by elapsed time */
	for (c = 0; c < gtop->num_perf_counters; c++) {

		gtop->events_per_sample[c] =
			(gtop->events_per_sample[c] * USEC_PER_SEC * 10) / diff;

		gtop->events_per_sample_average[c] /= samples;

	}
}

static int
gtop_enable_profiling(struct perf_device *dev)
{
	int err = 0;

	if (FLAG_IS_SET(flags, FLAG_CONTEXT)) {
		err = perf_profiler_enable_with_ctx(selected_ctx, dev);
	} else {
		err = perf_profiler_enable(dev);
	}

	if (err < 0) {
		dprintf("Failed to enable profiler\n");
		return err;
	}

	return err;
}

static int
gtop_disable_profiling(struct perf_device *dev)
{
	return perf_profiler_disable(dev);
}

static int
gtop_compute_perf(struct perf_device *dev, struct gtop_data *gtop_d)
{
	uint32_t c;
	int err;

	if (!profiler_state.enabled) {

		if (perf_check_profiler(&profiler_state.state, dev) < 0)
			return -1;

		if (gtop_enable_profiling(dev) < 0)
			return -1;

		if (perf_profiler_start(dev) < 0)
			return -1;

		profiler_state.enabled = true;

	}

	err = perf_read_counters_3d(gtop_d->type, gtop_d->counter_data, dev);
	if (err < 0) {
		dprintf("reading counters failed!\n");
		exit(EXIT_FAILURE);
	}

	for (c = 0; c < gtop_d->num_perf_counters; c++) {
		if (!gtop_d->reset_after_read[c]) {
			if (gtop_d->counter_data_last[c] > gtop_d->counter_data[c]) {
				gtop_d->events_per_sample[c] += gtop_d->counter_data[c];
			} else {
				gtop_d->events_per_sample[c] +=
					(uint64_t)(gtop_d->counter_data[c] -
							   gtop_d->counter_data_last[c]);
			}
		} else {
			gtop_d->events_per_sample[c] += gtop_d->counter_data[c];
		}

		gtop_d->events_per_sample_average[c] += gtop_d->counter_data[c];


		/* compute maximum */
		if (gtop_d->counter_data[c] > gtop_d->counter_data_last[c])
			gtop_d->events_per_sample_max[c] = gtop_d->counter_data[c];

		/* compute minium */
		if (gtop_d->counter_data[c] < gtop_d->counter_data_last[c])
			gtop_d->events_per_sample_min[c] = gtop_d->counter_data[c];
	}

	memcpy(gtop_d->counter_data_last, gtop_d->counter_data,
			gtop_d->num_perf_counters * sizeof(uint32_t));

	return 0;
}

static void
gtop_wait_for_keyboard(const char *str, bool clean_screen)
{
	int dummy;

	if (clean_screen)
		fprintf(stdout, "%s\n", clear_screen);
	if (str)
		fprintf(stdout, "%s", str);

	fprintf(stdout, "\n Type any key to resume...\n");

	fflush(NULL);
	ssize_t nr = read(STDIN_FILENO, &dummy, sizeof(int));
	(void) nr;
}

static bool
gtop_check_ctx_is_valid(uint32_t c)
{
	struct debugfs_client clients;
	struct debugfs_client *curr_client;
	int nr_clients = 0;
	bool ctx_found = false;

	nr_clients = debugfs_get_current_clients(&clients, NULL);

	/* if no clients are attached bail out */
	if (!nr_clients)
		return ctx_found;


	list_for_each(curr_client, clients.head) {

		/* skip our program from attached programs */
		if (!strncmp(curr_client->name, prg_name, strlen(prg_name)))
			continue;

		/* get contexts for this client */
		debugfs_get_current_ctx(curr_client, NULL);

		for (size_t ctx = 0; ctx < curr_client->ctx_no; ctx++) {

			if (curr_client->ctx[ctx] == c) {
				size_t name_len;

				ctx_found = true;

				name_len = strlen(curr_client->name);

try_again:
				/*
				 * if this is the first time, or we're modifying it
				 * allocate some space for it
				 */
				if (selected_client == NULL) {

					selected_client = malloc(sizeof(*selected_client));
					memset(selected_client, 0, sizeof(*selected_client));

					selected_client->name = malloc(name_len + 1);
					memset(selected_client->name, 0, name_len + 1);

				} else {

					/* if it is not NULL, free it first and try again */
					if (selected_client->name)
						free(selected_client->name);

					memset(selected_client, 0, sizeof(*selected_client));
					free(selected_client);
					selected_client = NULL;
					goto try_again;
				}

				/* copy the info over */
				selected_client->pid = curr_client->pid;
				memcpy(selected_client->name, curr_client->name, name_len);
			}
		}

	}

	/* free all resources */
	debugfs_free_clients(&clients);

	return ctx_found;
}

static uint32_t
gtop_get_ctx_from_keyboard(struct perf_device *dev)
{
	uint32_t c_ctx;
	int m;

	/* restore back tty so we can get a number */
	tty_reset(&tty_old);

	fprintf(stdout, "Type context to attach to (0 to quit): ");
	m = scanf("%u", &c_ctx);
	(void) m;

	/* if we don't support this board */
	if (gtop_is_chip_model(0x7000, dev) && (gtop_info.drv_info.build < 150331)) {
		tty_init(&tty_old);
		gtop_wait_for_keyboard("GC7000 not supported at the moment!\n", true);
		tty_reset(&tty_old);
		goto out;
	}

	while (!gtop_check_ctx_is_valid(c_ctx) && c_ctx != 0) {
		fprintf(stdout, "Type context to attach to (0 to quit): ");
		m = scanf("%u", &c_ctx);
	}

out:
	/* go back into canonical mode */
	tty_init(&tty_old);


	return c_ctx;
}

static void
gtop_get_no_samples_from_keyboard(void)
{
	int m;

	/* restore back tty so we can get a number */
	tty_reset(&tty_old);

	fprintf(stdout, "# Samples: ");
	m = scanf("%u", &samples);
	(void) m;

	/* go back into canonical mode */
	tty_init(&tty_old);
}

static int
gtop_compute(struct perf_device *dev, struct gtop *gtop)
{
	int s;
	struct timespec interval = {};
	int err = 0;

	interval.tv_sec = 0;
	interval.tv_nsec = (USEC_PER_SEC / samples);


	/* bail early in case we just display clients */
	if (mode == MODE_PERF_SHOW_CLIENTS &&
	    curr_page == MODE_PERF_SHOW_CLIENTS)
		return 0;

	/* clear every time gpu state so we get % values correctly */
	memset(&gtop->st, 0, sizeof(struct vivante_gpu_state));

	if (FLAG_IS_SET(flags, FLAG_SHOW_BATCH_CONTEXTS))
		samples = 1;

	/* in batch mode we just run it once */
	for (s = 0; s < samples; s++) {

		if (FLAG_IS_SET(flags, FLAG_MODE)) {
			switch (mode) {
			case MODE_PERF_COUNTER_PART1:
				err = gtop_compute_perf(dev, gtop->perf_data[VIV_PROF_COUNTER_PART1]);
				break;
			case MODE_PERF_COUNTER_PART2:
				err = gtop_compute_perf(dev, gtop->perf_data[VIV_PROF_COUNTER_PART2]);
				break;
			case MODE_PERF_DMA:
				err = gtop_compute_mode_dma(dev, &gtop->st);
				break;
			case MODE_PERF_OCCUPANCY:
				err = gtop_compute_mode_occupancy(dev, &gtop->st);
				break;
			case MODE_PERF_VID_MEM_USAGE:
			case MODE_PERF_SHOW_CLIENTS:
#if defined HAVE_DDR_PERF && (defined __linux__  || defined __ANDROID__ || defined ANDROID)
			case MODE_PERF_DDR:
				break;
#else
				break;
#endif

			default:
				dprintf("Invalid mode specified\n");
				exit(EXIT_FAILURE);
			}
		} else {
			switch (curr_page) {
			case PAGE_COUNTER_PART1:
				err = gtop_compute_perf(dev, gtop->perf_data[VIV_PROF_COUNTER_PART1]);
				break;
			case PAGE_COUNTER_PART2:
				err = gtop_compute_perf(dev, gtop->perf_data[VIV_PROF_COUNTER_PART2]);
				break;
			case PAGE_DMA:
				err = gtop_compute_mode_dma(dev, &gtop->st);
				break;
			case PAGE_OCCUPANCY:
				err = gtop_compute_mode_occupancy(dev, &gtop->st);
				break;
			case MODE_PERF_VID_MEM_USAGE:
			case MODE_PERF_SHOW_CLIENTS:
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
			case MODE_PERF_DDR:
				break;
#else
				break;
#endif
			default:
				dprintf("Invalid page view specified!\n");
				exit(EXIT_FAILURE);
			}
		}

		if (err < 0) {
			return err;
		}

		if (FLAG_IS_SET(flags, FLAG_SHOW_BATCH_CONTEXTS))
			return 0;

		nanosleep(&interval, NULL);
	}
	
	return 0;
}

static void
gtop_scale_counters(struct gtop *gtop, uint64_t diff)
{
	if (FLAG_IS_SET(flags, FLAG_MODE)) {
		if (mode == MODE_PERF_COUNTER_PART1)
			gtop_scale_counters_by(gtop->perf_data[VIV_PROF_COUNTER_PART1], diff);
		if (mode == MODE_PERF_COUNTER_PART2)
			gtop_scale_counters_by(gtop->perf_data[VIV_PROF_COUNTER_PART2], diff);
	} else {
		if (curr_page == PAGE_COUNTER_PART1)
			gtop_scale_counters_by(gtop->perf_data[VIV_PROF_COUNTER_PART1], diff);
		if (curr_page == PAGE_COUNTER_PART2)
			gtop_scale_counters_by(gtop->perf_data[VIV_PROF_COUNTER_PART2], diff);
	}
}


static void
gtop_display_interactive_help(void)
{
	int dummy;

	fprintf(stdout, "%s\n", clear_screen);

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	fprintf(stdout, " Arrows (<-|->) to navigate between pages         | Use 0-6 to switch directly\n");
#else
	fprintf(stdout, " Arrows (<-|->) to navigate between pages         | Use 0-5 to switch directly\n");
#endif
	fprintf(stdout, " Use SPACE to specify a context (for PART1|PART2) | Use p to pause display\n");
	fprintf(stdout, " Use x to show application's GPU id contexts      | Use q<ESC> to quit\n");
	fprintf(stdout, " Use r to change between TIME/MIN/AVERAGE/MAX values of counters\n");

	fprintf(stdout, "\n Type any key to resume...");
	fflush(NULL);

	ssize_t nr = read(STDIN_FILENO, &dummy, sizeof(int));
	(void) nr;

}

static int
gtop_check_keyboard(struct perf_device *dev)
{
	int rc;
	long long buf;

	rc = get_input_char();
	if (rc == 0) {
		return 0;
	}

	memset(&buf, 0, sizeof(buf));
	ssize_t nread = -1;

	/*
	 * Over serial reading input chars is problematic as we need 3-bytes.
	 */
	do {
		nread = read(STDIN_FILENO, &buf, sizeof(buf));
	} while (nread == -1 || nread == 2 || nread == 0);

	/* mask the other bytes as buf will be overwritten when the third byte
	 * is read, see top.h as for serial we've encoded the arrow keys with
	 * just one byte */
	if (buf >> 8 && nread != 3)
		buf &= 0x000000ff;

	/* please compiler */
	(void) nread;

	switch (buf) {
	case KEY_H:
	case KEY_QUESTION_MARK:
		gtop_display_interactive_help();
		break;
	case KB_KEY_P:
		if (paused)
			paused = false;
		else
			paused = true;
		break;
	case KB_ESCAPE:
	case KEY_Q:
		return -1;
		break;
	case KB_SPACE:
		/* select ctx */
		selected_ctx = gtop_get_ctx_from_keyboard(dev);
		/* change the context so we can retrieve counters */
		if (selected_ctx) {
			perf_context_set(selected_ctx, dev);
		} else {
			/* if context not OK wipe out */
			if (selected_client && selected_client->name) {
				free(selected_client->name);
				free(selected_client);

				memset(selected_client, 0, sizeof(*selected_client));
				selected_client = NULL;
			}
		}
		break;
	case KB_UP:
	case KB_RIGHT:
	case KB_RIGHT_SERIAL:
	case KB_UP_SERIAL:
		curr_page++;

		/* verify if we indeed still have a valid context, but
		 * we are displaying old info */
		if (curr_page == PAGE_COUNTER_PART1 ||
		    curr_page == PAGE_COUNTER_PART2) {

			if (selected_client && selected_client->name) {
				if (!gtop_check_ctx_is_valid(selected_ctx)) {

					free(selected_client->name);
					free(selected_client);

					memset(selected_client, 0, sizeof(*selected_client));
					selected_client = NULL;

					selected_ctx = 0;
					/* use our own context in this case */
					perf_context_set(selected_ctx, dev);
				}
			} else {
				gtop_wait_for_keyboard("* Context not selected or feature not available, set context first before viewing context related pages or switch to other view mode!\n", true);
				if (curr_page == PAGE_COUNTER_PART1)
					curr_page += 2;
				else
					curr_page++;
				break;
			}
		}
		break;
	case KB_DOWN:
	case KB_LEFT:
	case KB_LEFT_SERIAL:
	case KB_DOWN_SERIAL:
		/* verify if we indeed still have a valid context, but
		 * we are displaying old info */
		curr_page--;

		if (curr_page == PAGE_COUNTER_PART1 ||
		    curr_page == PAGE_COUNTER_PART2) {
			if (selected_client && selected_client->name) {
				if (!gtop_check_ctx_is_valid(selected_ctx)) {

					free(selected_client->name);
					free(selected_client);

					memset(selected_client, 0, sizeof(*selected_client));
					selected_client = NULL;

					selected_ctx = 0;
					/* use our own context in this case */
					perf_context_set(selected_ctx, dev);
				}
			} else {
				gtop_wait_for_keyboard("** Context not selected or feature not available, set context first before viewing context related pages or switch to other view mode!\n", true);
				if (curr_page == PAGE_COUNTER_PART2)
					curr_page -= 2;
				else
					curr_page--;
				break;
			}
		}
		break;
	case KEY_0:
		curr_page = PAGE_SHOW_CLIENTS;
		break;
	case KEY_1:
		curr_page = PAGE_VID_MEM_USAGE;
		break;
	case KEY_2:
		if (selected_client && selected_client->name) {
			if (!gtop_check_ctx_is_valid(selected_ctx)) {

				free(selected_client->name);
				free(selected_client);

				memset(selected_client, 0, sizeof(*selected_client));
				selected_client = NULL;

				selected_ctx = 0;
				/* use our own context in this case */
				perf_context_set(selected_ctx, dev);
			}
		} else {
			gtop_wait_for_keyboard("! Context not selected or feature not available, set context first before viewing context related pages or switch to other view mode!\n", true);
			break;
		}

		curr_page = PAGE_COUNTER_PART1;
		break;
	case KEY_3:
		if (selected_client && selected_client->name) {
			if (!gtop_check_ctx_is_valid(selected_ctx)) {

				free(selected_client->name);
				free(selected_client);

				memset(selected_client, 0, sizeof(*selected_client));
				selected_client = NULL;

				selected_ctx = 0;
				/* use our own context in this case */
				perf_context_set(selected_ctx, dev);
			}
		} else {
			gtop_wait_for_keyboard("!! Context not selected or feature not available, set context first before viewing context related pages or switch to other view mode!\n", true);
			break;
		}
		curr_page = PAGE_COUNTER_PART2;
		break;
	case KEY_4:
		curr_page = PAGE_DMA;
		break;
	case KEY_5:
		curr_page = PAGE_OCCUPANCY;
		break;
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	case KEY_6:
		curr_page = PAGE_DDR_PERF;
		break;
#endif
	case KEY_X:
		if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS))
			REMOVE_FLAG(flags, FLAG_SHOW_CONTEXTS);
		else
			SET_FLAG(flags, FLAG_SHOW_CONTEXTS);
		break;
	case KEY_S:
		gtop_get_no_samples_from_keyboard();
		break;
	case KEY_R:
		samples_mode++;
		break;
	default:
		break;
	}

	/* in case we reach end or maxium of pages */
	if (curr_page == PAGE_NO || curr_page == 0xff)
		curr_page = 0;

	if (samples_mode > SAMPLES_MAX)
		samples_mode = 0;

	/* disable profiler when not in counter page */
	if ((curr_page == PAGE_SHOW_CLIENTS ||
	     curr_page == PAGE_VID_MEM_USAGE) && profiler_state.enabled) {
		gtop_disable_profiling(dev);
		perf_profiler_stop(dev);
		profiler_state.enabled = false;
	}

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	/* disable reading DDR perf PMUs */
	if ((curr_page != PAGE_SHOW_CLIENTS ||
	     curr_page != PAGE_DDR_PERF) && perf_ddr_enabled) {
		gtop_disable_pmus();
		perf_ddr_enabled = 0;
	}
#endif
	return 0;
}


/*
 * retrieve PART1 and PART2
 */
static void
gtop_retrieve_perf_counters(struct perf_device *dev, bool batch)
{
	struct gtop gtop = {};

	uint32_t num_perf_counters_part1;
	uint32_t num_perf_counters_part2;

	uint64_t begin_time, end_time, diff;

	num_perf_counters_part1 = perf_get_num_counters(VIV_PROF_COUNTER_PART1, dev);
	num_perf_counters_part2 = perf_get_num_counters(VIV_PROF_COUNTER_PART2, dev);

	gtop.perf_data = calloc(2, sizeof(struct gtop_data));

	gtop.perf_data[VIV_PROF_COUNTER_PART1] = 
		gtop_data_create(VIV_PROF_COUNTER_PART1, num_perf_counters_part1, 0);
	gtop.perf_data[VIV_PROF_COUNTER_PART2] =
		gtop_data_create(VIV_PROF_COUNTER_PART2, num_perf_counters_part2, 0);


	fprintf(stdout, "%s", clear_screen);

	begin_time = get_ns_time();
	while (1) {
		if (sig_recv)
			goto out;

		if (paused) {
			goto show_hw_counters;
		}

		/* clear the samples before sampling */
		for (uint8_t i = 1; i < 3; i++)
			gtop_data_clear_samples(gtop.perf_data[i]);

		/* retrieve the counters, or read registers */
		gtop_compute(dev, &gtop);

show_hw_counters:
		end_time = get_ns_time();
		diff = end_time - begin_time;

		if (!paused) {
			gtop_scale_counters(&gtop, diff);
		}

		/* figure out if we got anything from keyboard, or if we're
		 * running batched */
		if (batch) {
			delay();
		} else {
			if (gtop_check_keyboard(dev) < 0)
				goto out;
		}


		gtop_display_interactive(dev, gtop);

		if (FLAG_IS_SET(flags, FLAG_SHOW_BATCH_CONTEXTS))
			goto out;

		begin_time = end_time;

	}

out:
	gtop_data_destroy(gtop.perf_data[VIV_PROF_COUNTER_PART1]);
	gtop_data_destroy(gtop.perf_data[VIV_PROF_COUNTER_PART2]);

	free(gtop.perf_data);
}

static
void help(void)
{
	dprintf("Usage:\n");
	dprintf("  %s (GIT: %s, V: %s) [-m mode] [-c <ctx>] [-x]\n", prg_name, git_version, version);
	dprintf("\n");
	dprintf("  -m <mode>\n");
	dprintf("                mem         Show memory usage of clients attached to GPU\n");
	dprintf("                counter_1   Show counters part 1\n");
	dprintf("                counter_2   Show counters part 2\n");
	dprintf("                occupancy   Show occupancy (non-idle) states of modules\n");
	dprintf("                dma         DMA engine states\n");
	dprintf("                vidmem	    Additional video memory information\n");
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	dprintf("                ddr	    Show Kernel PMUs related to memory bandwidth\n");
#endif
	dprintf("  -c <ctx>      Specify context to track\n");
	dprintf("  -b            Show batch (instantaneous of requested mode)\n");
	dprintf("  -f            Read counters in batch mode\n");
	dprintf("  -x            Display contexts in memory viewing page\n");
	dprintf("  -i		Ignore errors when opening a connection with the driver\n");
	dprintf("  -v            Show version\n");
	dprintf("  -h            Show this help message\n");

	exit(EXIT_FAILURE);
}

static void
show_version(void)
{
	fprintf(stdout, "gputop GIT: %s, Version: %s\n", git_version, version);
	fprintf(stdout, "libgpuperfcnt GIT: %s, Version: %s\n",
			perf_version.git_version, perf_version.version);
	exit(EXIT_SUCCESS);
}

static void
parse_args(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "m:hc:xbvfi")) != -1) {
		switch (c) {
		case 'm':
			SET_FLAG(flags, FLAG_MODE);

			if (!strncmp(optarg, "mem", strlen("mem"))) {
				mode = MODE_PERF_SHOW_CLIENTS;
			} else if (!strncmp(optarg, "counter_1", strlen("counter_1"))) {
				mode = MODE_PERF_COUNTER_PART1;
			} else if (!strncmp(optarg, "counter_2", strlen("counter_2"))) {
				mode = MODE_PERF_COUNTER_PART2;
			} else if (!strncmp(optarg, "occupancy", strlen("occupancy"))) {
				mode = MODE_PERF_OCCUPANCY;
			} else if (!strncmp(optarg, "dma", strlen("dma"))) {
				mode = MODE_PERF_DMA;
			} else if (!strncmp(optarg, "vidmem", strlen("vidmem"))) {
				mode = MODE_PERF_VID_MEM_USAGE;
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
			} else if (!strncmp(optarg, "ddr", strlen("ddr"))) {
				mode = MODE_PERF_DDR;
#endif
			} else {
				dprintf("Unknown mode %s\n", optarg);
				help();
			}
			break;
		case 'x':
			SET_FLAG(flags, FLAG_SHOW_CONTEXTS);
			break;
		case 'c':
			SET_FLAG(flags, FLAG_CONTEXT);
			selected_ctx = atoi(optarg);
			break;
		case 'b':
			SET_FLAG(flags, FLAG_SHOW_BATCH_CONTEXTS);
			break;
		case 'f':
			SET_FLAG(flags, FLAG_SHOW_BATCH_PERF);
			break;
		case 'v':
			show_version();
			break;
		case 'i':
			SET_FLAG(flags, FLAG_IGNORE_START_ERRORS);
			break;
		case 'h':
		default:
			help();
		}
	}
}

static void
sigint_handler(int sig, siginfo_t *si, void *unused)
{
	(void) sig;
	(void) si;
	(void) unused;

	sig_recv  = 1;
}

static void
sigresize_handler(int sig, siginfo_t *si, void *unused)
{
	(void) sig;
	(void) si;
	(void) unused;

	resized ^= 1;
}

static void
install_sighandler(void)
{
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	sa.sa_sigaction = sigint_handler;

	if (sigaction(SIGTERM, &sa, NULL) == -1)
		exit(EXIT_FAILURE);
	if (sigaction(SIGINT, &sa, NULL) == -1)
		exit(EXIT_FAILURE);

	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	/* install resize signal handler */
	sa.sa_sigaction = sigresize_handler;
	if (sigaction(SIGWINCH, &sa, NULL) == -1)
		exit(EXIT_FAILURE);

}

int main(int argc, char *argv[])
{
	struct perf_device *dev = NULL;
	int err;
	bool batch = false;


	memset(&gtop_info, 0, sizeof(struct gtop_hw_drv_info));
	perf_version = perf_get_library_version();

	parse_args(argc, argv);
	install_sighandler();

	tty_init(&tty_old);

	dev = perf_init(&vivante_ops);
	if (!dev) {
		fprintf(stderr, "perf_init()! failed\n");
		tty_reset(&tty_old);
		exit(EXIT_FAILURE);
	}

	err = perf_open(VIV_HW_3D, dev);
	if (err < 0 && err != ERR_KERNEL_MISMATCH) {
		fprintf(stderr, "Failed to open driver connection: %s\n",
				perf_get_last_error(dev));
		tty_reset(&tty_old);
		exit(EXIT_FAILURE);
	}

	/* get driver, hw info */
	gtop_get_gtop_info(dev, &gtop_info);

	if (err < 0) {
		if (err == ERR_KERNEL_MISMATCH) {
		    if (!FLAG_IS_SET(flags, FLAG_IGNORE_START_ERRORS)) {
			fprintf(stdout, "Warning: Kernel mismatch, ");
			fprintf(stdout, "Driver version %d.%d\n",
				gtop_info.drv_info.major,
				gtop_info.drv_info.minor
				);
			fprintf(stdout, "Library version: GIT: %s, VERSION: %s\n",
				perf_version.git_version,
				perf_version.version);

			gtop_wait_for_keyboard(NULL, false);
		    }
		} else {
			fprintf(stderr, "Failed to open driver connection: %s\n",
					perf_get_last_error(dev));
			tty_reset(&tty_old);
			exit(EXIT_FAILURE);
		}
	}

	if (FLAG_IS_SET(flags, FLAG_CONTEXT)) {
		/* 
		 * Before 6.2.4p1 gc7000 does not support reading counters using
		 * available methods.
		 */
		if (gtop_is_chip_model(0x7000, dev) && gtop_info.drv_info.build < 150331) {
			fprintf(stderr, "Reading counters for GC7000 not supported at the moment!\n");
			tty_reset(&tty_old);
			perf_exit(dev);
			exit(EXIT_FAILURE);
		}

		if (!gtop_check_ctx_is_valid(selected_ctx)) {
			fprintf(stderr, "Either application not running or feature not available\n");
			tty_reset(&tty_old);
			perf_exit(dev);
			exit(EXIT_FAILURE);
		}
	}

	if (FLAG_IS_SET(flags, FLAG_SHOW_BATCH_PERF))
		batch = true;

	gtop_retrieve_perf_counters(dev, batch);

	gtop_free_gtop_info(dev, &gtop_info);

   if (profiler_state.enabled)
   {
      perf_profiler_stop(dev);
      perf_profiler_disable(dev);
   }
	perf_exit(dev);
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	if(perf_ddr_enabled)
  {
       gtop_disable_pmus();
       perf_ddr_enabled=0;
  }
#endif

	tty_reset(&tty_old);
	return 0;
}
