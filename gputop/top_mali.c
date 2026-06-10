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

#include <gpuperfcnt/gpuperfcnt.h>
#include <gpuperfcnt/gpuperfcnt_log.h>

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
#include <ddrperfcnt/ddr-perf.h>
#endif

#include "top.h"

#include "mali.h"

static const char *prg_name = "gputop";
/* current termios state */
struct termios tty_old;
uint8_t curr_page = PAGE_SHOW_GPU;

/* if a SIGINT/SIGTERM has been received */
static int volatile sig_recv = 0;
static int display_gpu_info_only = 0;
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
static int perf_ddr_enabled = 0;
#endif

/* other modifiers */
static const char clear_screen[] = { 0x1b, '[', 'H', 0x1b, '[', 'J', 0x0 };
static const char *bold_color = "\033[1m";
static const char *underlined_color = "\033[4m";
static const char *regular_color = "\033[0m";

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
/* what DDR pmus we want to read, if you want to add more you also need
 * to modify PERF_DDR_PMUS_COUNT  */
static struct perf_pmu_ddr perf_pmu_ddrs[] = {
{ "imx9_ddr0", { { -1, "eddrtq_pm_rd_beat_filt0" }, { -1, "eddrtq_pm_wr_beat_filt" } } },
 };
#endif

static struct p_page program_pages[] = {
   [PAGE_SHOW_GPU]  = { PAGE_SHOW_GPU, "Main page " },
   [PAGE_SHOW_GPU_INFO] = { PAGE_SHOW_GPU_INFO, "GPU Info" },
   [PAGE_KERNEL_MEM_USAGE] = { PAGE_KERNEL_MEM_USAGE, "Kernel Memory Usage" },
   [PAGE_PID_MEM_USAGE]     = { PAGE_PID_MEM_USAGE, "PID Based Process Memory Usage" },
   [PAGE_DVFS_UTILIZATION]  = { PAGE_DVFS_UTILIZATION, "DVFS Utilization" },
#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
   [PAGE_DDR_PERF]      = { PAGE_DDR_PERF, "DDR" },
#endif
};

static void
gtop_display_interactive_help(void)
{
	int dummy;

	fprintf(stdout, "%s\n", clear_screen);

	fprintf(stdout, " Arrows (<-|->) to navigate between pages         | Use 0-5 to switch directly\n");
	fprintf(stdout, "\n Type any key to resume...");
	fflush(NULL);

	ssize_t nr = read(STDIN_FILENO, &dummy, sizeof(int));
	(void) nr;

}


static void
tty_init(struct termios *tty_o)
{
	struct termios new_tty;

	if (tcgetattr(STDIN_FILENO, tty_o) < 0) {
		fprintf(stderr, "Failed to get tty: %s\n", strerror(errno));
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

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_tty) < 0 ) {
		fprintf(stderr, "Failed to set tty: %s\n", strerror(errno));
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

/*static uint64_t
get_ns_time(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		dprintf("clock_gettime()");
		exit(EXIT_FAILURE);
	}

	return (ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec;
}*/

static int
gtop_check_keyboard(void)
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
		 * Break out if a termination signal was received to avoid spinning
		 * forever when read() returns EINTR.
		 */
		do {
			nread = read(STDIN_FILENO, &buf, sizeof(buf));
		} while ((nread == -1 || nread == 2 || nread == 0) && !sig_recv);

		if (sig_recv)
			return -1;
	
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
		case KB_ESCAPE:
		case KEY_Q:
			return -1;
			break;
		case KB_UP:
		case KB_RIGHT:
		case KB_RIGHT_SERIAL:
		case KB_UP_SERIAL:
			curr_page++;
			break;
		case KB_DOWN:
		case KB_LEFT:
		case KB_LEFT_SERIAL:
		case KB_DOWN_SERIAL:
			/* verify if we indeed still have a valid context, but
			 * we are displaying old info */
			curr_page--;
			break;
		case KEY_X:
			gtop_set_display_flags(FLAG_SHOW_CONTEXTS);
			break;

		case KEY_0:
			curr_page = PAGE_SHOW_GPU;
			break;
		case KEY_1:
			curr_page = PAGE_SHOW_GPU_INFO;
			break;
		case KEY_2:
			curr_page = PAGE_KERNEL_MEM_USAGE;
			break;
		case KEY_3:
			curr_page = PAGE_PID_MEM_USAGE;
			break;
		case KEY_4:
			curr_page = PAGE_DVFS_UTILIZATION;
			break;
		case KEY_5:
			curr_page = PAGE_DDR_PERF;
			break;
		}
	
		/* in case we reach end or maxium of pages */
		if (curr_page == PAGE_NO || curr_page == 0xff)
			curr_page = 0;
		return 0;
	}

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
static void
gtop_configure_pmus(void)
{
	uint32_t config, type;
	unsigned int i, j;

	for_all_pmus(perf_pmu_ddrs, i, j) {
		const char *type_name = PMU_GET_TYPE_NAME(perf_pmu_ddrs, i);
		const char *event_name = PMU_GET_EVENT_NAME(perf_pmu_ddrs, i, j);
		//fprintf(stdout, " i , j  %d  %d %s    %s\n", i, j, type_name, event_name);
		if ( ! perf_event_pmu_exist(type_name) )
			continue;

		type = perf_event_pmu_get_type(type_name);
		config = perf_event_pmu_get_event(type_name, event_name);

		/* we might end up running on boards which do not support DDR or
		 * those PMus so we just ignore them */
		if (type == 0 || config == 0)
			continue;
		//fprintf(stdout, " i , j  %d  %d  %x    %x\n", i, j, type, config);
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
				if(!strncmp(event_name, "eddrtq",6))
						display_value = counter_val*32 / (1024.0*1024);
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
			const char *event_name = PMU_GET_EVENT_NAME(perf_pmu_ddrs, i, j);

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
			double display_value = (double) counter_val * 32 / (1024 * 1024);

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

#endif
static
void help(void)
{
	dprintf("Usage:\n");
	dprintf("  %s   [-i ]  [-h ] \n", prg_name);
	dprintf("\n");
	dprintf(" 4 pages, 0: Main page 1:performance statistics,kernel,drive and arch info  2: kernel mem info  3: PID based process mem info\n");
	dprintf("  4: gpu utilization  5: Perf DDR memory bandwidth\n");
	dprintf("press key 0-5 to display different page or arrow key to rotate between pages\n");
	dprintf("press q or Esc key to quit\n");
	dprintf("\n");
	dprintf("  -i          display gpu info only\n");
	dprintf("  -x         display with context\n");
	dprintf("  -h         Show this help message\n");
	exit(EXIT_FAILURE);
}

static void
parse_args(int argc, char **argv)
{
   int i =1;
   // Walk all command line arguments.
   for (i = 1; i < argc; ++i)
   {
      if (argv[i][0] == '-')
      {
         switch (argv[i][1])
         {
         case 'i':
            display_gpu_info_only= 1;
            break;
         case 'h':
            help();
            break;
         default:
            break;
         }
      }
      else
      {
         break;
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
	//uint64_t  t1, t2;
	parse_args(argc, argv);
	install_sighandler();

	tty_init(&tty_old);
	gtop_enable_gpu_profile(NULL);

	gtop_display_mali_gpu_info();
	uint8_t	last_page =curr_page;

	do {
		//struct timespec interval = {};
		//interval.tv_sec = 0;
		//interval.tv_nsec = (NSEC_PER_SEC);
		if (sig_recv)
			break;
		if(display_gpu_info_only)
			break;
		//t1 = get_ns_time();
		if((curr_page != PAGE_SHOW_GPU_INFO) ||(last_page != curr_page)){
			fflush(stdout);
			fprintf(stdout, "%s", clear_screen);
			fprintf(stdout, "%s | %u / %u \n ", program_pages[curr_page].page_desc, curr_page, (PAGE_NO - 1));
		}
		switch (curr_page) {
		case PAGE_SHOW_GPU:
			gtop_display_perf_pmus_short();
			gtop_display_mali_debugfs_info();
			break;
		case PAGE_SHOW_GPU_INFO:
			if(last_page == curr_page)
				break;
			gtop_display_mali_gpu_info();
			break;
		case PAGE_KERNEL_MEM_USAGE:
			gtop_display_mali_debugfs_ktx_info();
			break;
		case PAGE_PID_MEM_USAGE:
			gtop_display_mali_debugfs_pid_mem_info();
			break;
		case PAGE_DVFS_UTILIZATION:
			gtop_display_mali_debugfs_dvfs_utilization_info();
			break;
		case PAGE_DDR_PERF:
			gtop_display_perf_pmus();
			break;

		case PAGE_NO:
		default:
			break;
		}
		last_page =curr_page;
		//t2 = get_ns_time();
		//interval.tv_nsec = (NSEC_PER_SEC -(t2-t1));
		//nanosleep(&interval, NULL);
	}while(gtop_check_keyboard() >=0);
	

#if defined HAVE_DDR_PERF && (defined __linux__ || defined __ANDROID__ || defined ANDROID)
	if(perf_ddr_enabled)
  {
       gtop_disable_pmus();
       perf_ddr_enabled=0;
  }
#endif

	gtop_disable_gpu_profile(NULL);
	tty_reset(&tty_old);
	return 0;
}
