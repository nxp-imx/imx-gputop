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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "gpuperfcnt/gpuperfcnt_debugfs.h"
#include "debugfs.h"

int
debugfs_get_contexts(struct debugfs_client *clients, const char *path)
{
	FILE *file = NULL;
	char buf[1024];
	bool found_process = false;
	struct debugfs_client *client = NULL;

	if (!path) {
		file = debugfs_fopen("database", "r");
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return -1;

	memset(buf, 0, sizeof(char) * 1024);

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* check if this is the process we're interested in */
		if (strncmp(line, "Process:", 8) == 0) {

			uint32_t pid, err;


			char *name = calloc(512, sizeof(char));
			err = sscanf(line, "Process: %d   %[a-zA-Z0-9-]s\n", &pid, name);

			if (name && err == 2) {

				list_for_each(client, clients->head) {
					if (client->pid == pid &&
					    !strncmp(client->name, name, strlen(name))) {
						/* 
						 * This automatically stops at
						 * the right client, but we
						 * have to reset the no of
						 * contexts.
						 */
						client->ctx_no = 0;
						found_process = true;
						break;
					} else {
						/* 
						 * It could be that other
						 * process follow this one so
						 * we might get invalid
						 * contexts
						 */
						found_process = false;
					}
				}

			}
			free(name);
		}

		/* go to next line if we didn't found our process */
		if (!found_process)
			continue;

		if (!strncmp(line, "Context", 7)) {
			char *str = line;
			uint32_t no = 0;

			/* skip context */
			str += 7;

			/* skip white-space */
			while (str && (*str == ' ' || *str == '\t'))
					str++;

			/* we might need to get this value 0 -> GPU0, 1 -> GPU1 */
			if (*str == '0' || *str == '1')
				str++;

			/* skip white-space */
			while (str && (*str == ' ' || *str == '\t'))
					str++;

			sscanf(str, "%x", &no);

			/* save it if we found a valid ctx */
			if (no) {
				assert(client != NULL);

				if (client->ctx == NULL) {
					client->ctx = calloc(512, sizeof(uint32_t));
				}

				client->ctx[client->ctx_no++] = no;
			}
		}
	}

	fclose(file);
	return 0;
}

int
debugfs_get_current_ctx(struct debugfs_client *client, const char *path)
{
	FILE *file;
	char buf[1024];
	bool found_process = false;
	int __nr = 0;

	if (!path) {
		file = debugfs_fopen("database", "r");
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return -1;

	memset(buf, 0, sizeof(char) * 1024);

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* check if this is the process we're interested in */
		if (strncmp(line, "Process:", 8) == 0) {
			uint32_t pid, err;

			char *name = calloc(512, sizeof(char));
			err = sscanf(line, "Process: %d   %[a-zA-Z0-9-]s\n", &pid, name);

			if (name && err == 2) {
				if (client->pid == pid &&
				    !strncmp(client->name, name, strlen(name))) {
					client->ctx_no = 0;
					found_process = true;
				} else {
					/* it could be that other process
					 * follow this one so we might get
					 * invalid contexts */
					found_process = false;
				}
			}
			free(name);
		}

		/* go to next line if we didn't found our process */
		if (!found_process)
			continue;

		if (!strncmp(line, "Context", 7)) {
			char *str = line;
			uint32_t no = 0;

			/* skip context */
			str += 7;

			/* skip white-space */
			while (str && (*str == ' ' || *str == '\t'))
					str++;

			/* we might need to get this value 0 -> GPU0, 1 -> GPU1 */
			if (*str == '0' || *str == '1')
				str++;

			/* skip white-space */
			while (str && (*str == ' ' || *str == '\t'))
					str++;

			sscanf(str, "%x", &no);

			/* save it if we found a valid ctx */
			if (no) {
				if (client->ctx == NULL) {
					client->ctx = calloc(10, sizeof(uint32_t));
				}

				assert(__nr < 10);

				client->ctx[__nr] = no;
				__nr++;
			}
		}
	}

	client->ctx_no = __nr;

	fclose(file);
	return 0;
}

int
debugfs_get_vid_mem(struct debugfs_vid_mem_client *client, pid_t pid)
{
	FILE *file = NULL;
	char buf[1024];
	char pid_str[128];

	memset(client, 0, sizeof(*client));
	file = debugfs_fopen("vidmem", "w+");
	
	if (!file)
		return -1;

	memset(pid_str, 0, 128);
	snprintf(pid_str, sizeof(pid_str), "%d", pid);

	debugfs_write(pid_str, strlen(pid_str), file);

#if defined __QNX__ || defined __QNXTO__
	/* we still needs this for QNX... */
	debugfs_reopen(file, "r");
#endif

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		if (!strncmp(line, "All-Types", strlen("All-Types")))
			continue;

		/* QNX doesn't know about %ms so we need to allocate by
		 * ourselves */
		char *name = calloc(512, sizeof(char));

		if (!strncmp(line, "Index", strlen("Index"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->index);
		}

		if (!strncmp(line, "Vertex", strlen("Vertex"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->vertex);
		}

		if (!strncmp(line, "Texture", strlen("Texture"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->texture);
		}

		if (!strncmp(line, "RenderTarget", strlen("RenderTarget"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->render_target);
		}

		if (!strncmp(line, "Depth", strlen("Depth"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->depth);
		}

		if (!strncmp(line, "Bitmap", strlen("Bitmap"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->bitmap);
		}

		if (!strncmp(line, "TileStatus", strlen("TileStatus"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->tile_status);
		}

		if (!strncmp(line, "Image", strlen("Image"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->image);
		}

		if (!strncmp(line, "Mask", strlen("Mask"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->mask);
		}

		if (!strncmp(line, "Scissor", strlen("Scissor"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->scissor);
		}

		if (!strncmp(line, "HZ", strlen("HZ"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->hz);
		}

		if (!strncmp(line, "ICache", strlen("ICache"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->i_cache);
		}

		if (!strncmp(line, "TxDesc", strlen("TxDesc"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->tx_desc);
		}

		if (!strncmp(line, "Fence", strlen("Fence"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->fence);
		}

		if (!strncmp(line, "TFBHeader", strlen("TFBHeader"))) {
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->tfbheader);
		}

		if (name) {
			free(name);
		}
	}

	fclose(file);
	return 0;
}

void
debugfs_print_contexts(struct debugfs_client *clients)
{
	size_t i;

	if (clients->ctx_no) {
		fprintf(stdout, "Showing contexts %"PRIu32" for PID %"PRIu32": ",
				clients->ctx_no, clients->pid);

		for (i = 0; i < clients->ctx_no; i++) {
			if (i == (clients->ctx_no - 1))
				fprintf(stdout, "%u", clients->ctx[i]);
			else
				fprintf(stdout, "%u,", clients->ctx[i]);
		}

		fprintf(stdout, "\n");
	}
}

/*
 * struct debugfs_client clients;
 * vivante_get_current_clients(&clients);
 *
 * struct debugfs_client *curr = clients.head;
 * for (; curr != NULL; curr = curr->next) {
 *
 * }
 *
 */
int
debugfs_get_current_clients(struct debugfs_client *clients, const char *path)
{
	FILE *file = NULL;
	char buf[1024];
	int i = 0;

	memset(clients, 0, sizeof(*clients));

	if (!path) {
		file = debugfs_fopen("clients", "r");
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return 0;

	memset(buf, 0, sizeof(char) * 1024);

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* skip PID and -- */
		if (*line == 'P' || *line == '-') {
			continue;
		}

		struct debugfs_client *client;
		int err;

		client = calloc(1, sizeof(*client));

		client->name = calloc(512, sizeof(char));
		err = sscanf(line, "%d  %[a-zA-Z0-9-]s\n", &client->pid, client->name);

		/* it could be we have garbage in clients */
		if (err != 2) {
			free(client);
			continue;
		}

		client->next = clients->head;
		clients->head = client;


		i++;
	}

	fclose(file);
	return i;
}

void
debugfs_free_clients(struct debugfs_client *clients)
{
	struct debugfs_client *it = clients->head;

	while (it) {
		if (it->next == NULL) {
			/* this is the last one */
			if (it->ctx)
				free(it->ctx);
			if (it->name)
				free(it->name);
			free(it);
			it = NULL;
		} else {
			/* save next so we can remove the current node */
			struct debugfs_client *it_next = it->next;

			/* free all storage */
			if (it->ctx)
				free(it->ctx);
			if (it->name)
				free(it->name);
			free(it);

			it = it_next;
		}
	}

	memset(clients, 0, sizeof(*clients));
}

/*
 * returns current clock
 */
int
debugfs_get_gpu_clocks(struct debugfs_clock *clocks, const char *path)
{
	FILE *file = NULL;
	char buf[1024];

	if (!path) {
		file = debugfs_fopen("clk", "r");
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return -1;

	memset(buf, 0, sizeof(buf));
	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		if (!strncmp(line, "gpu", 3)) {
			int core = -1;
			unsigned int clock_freq = 0;

			int err = sscanf(line, "gpu%d mc clock: %u HZ.", &core, &clock_freq);
			if (err == 2) {
				if (core == 0) {
					if (clock_freq) {
						clocks->gpu_core_0 = clock_freq;
						/* no need to test for shader */
						continue;
					}
				} else if (core == 1) {
					if (clock_freq) {
						clocks->gpu_core_1 = clock_freq;
						/* no need to test for shader */
						continue;
					}
				}
			}

			core = -1;
			clock_freq = 0;
			err = sscanf(line, "gpu%d sh clock: %u HZ.", &core, &clock_freq);
			if (err == 2) {
				if (core == 0) {
					if (clock_freq) {
						clocks->shader_core_0 = clock_freq;
					}
				} else if (core == 1) {
					if (clock_freq) {
						clocks->shader_core_1 = clock_freq;
					}
				}

			}


		}

	}

	/* verify that we got data */
	if (clocks->gpu_core_0 == 0 || clocks->shader_core_0 == 0)
		return -1;

	fclose(file);

	return 0;
}

/*
 * retrieves current gpu governor.
 */
int
debugfs_get_current_gpu_governor(struct debugfs_govern *governor)
{
	FILE *file = NULL;
	char buf[1024];
	/* no need to allocate each time */
	static struct debugfs_govern *__governor = NULL;
	unsigned int __governor_index = 0;

	const char path[] = "/sys/bus/platform/drivers/galcore/gpu_mode";
	/* newer version 6.2.4.p2 */
	const char path2[] = "/sys/bus/platform/drivers/galcore/gpu_govern";
	file = fopen(path, "r");

	if (!file) {
		file = fopen(path2, "r");
		if (!file)
			return -1;
	}

	memset(buf, 0, sizeof(buf));
	unsigned int modes = 0;

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;


		if (!strncmp(line, "GPU support", 11)) {
			int err = sscanf(line, "GPU support %d modes", &modes);
			assert(err == 1);
			assert(modes != 0);

			if (__governor == NULL)
				__governor = calloc(modes, sizeof(struct debugfs_govern));
			else
				memset(__governor, 0, sizeof(*__governor));
			continue;
		}

		assert(modes != 0);

		/* overdrive:      core_clk frequency: 800000000   shader_clk frequency: 1000000000	 */
		char *naming_mode = calloc(1024, sizeof(char));
		int err = sscanf(line, "%[a-zA-Z0-9-]s", naming_mode);
		assert(err == 1 || naming_mode != NULL);


		if (!strncmp(naming_mode, "overdrive", 9) ||
		    !strncmp(naming_mode, "nominal", 7) ||
		    !strncmp(naming_mode, "underdrive", 10)) {

			char *n_line = line + strlen(naming_mode);

			/* eat whitespace */
			while (n_line && *n_line) {
				if (*n_line == ' ' || *n_line == '\t')
					n_line++;
				else
					break;
			}

			unsigned long int core_clock_freq = 0;
			unsigned long int shader_clock_freq = 0;

			int rv = sscanf(n_line, "%*[^c]core_clk frequency: %lu%*[^s]shader_clk frequency: %lu", &core_clock_freq, &shader_clock_freq);
			if (rv != 2) {
				fprintf(stderr, "reading core-freq(%lu) and shader-freq(%lu) failed: %s\n", core_clock_freq, shader_clock_freq, n_line);
				assert(!"exit");
				return -1;
			}

			__governor[__governor_index].gpu_core_freq = core_clock_freq;
			__governor[__governor_index].shader_core_freq = shader_clock_freq;

			if (!strncmp(naming_mode, "overdrive", 9))
			       __governor[__governor_index].governor = OVERDRIVE;
			else if (!strncmp(naming_mode, "nominal", 7))
				__governor[__governor_index].governor = NOMINAL;
			else if (!strncmp(naming_mode, "underdrive", 10))
				__governor[__governor_index].governor = UNDERDRIVE;


			/* fail if we coudn't find a proper type */
			assert(__governor->governor != 0);
			/* fail if more have been added */
			assert(__governor_index <= modes);

			__governor_index++;

			if (naming_mode)
				free(naming_mode);

			/* go to next line */
			continue;
		}

		/* now we need to find out which one of those we are currently running */
		if (!strncmp(line, "Currently", 9)) {

			unsigned int current_mode_enum = 0;
			/* check if indeed we found it, we might have failed parsing? */
			int found = 0;

			char *current_mode = calloc(1024, sizeof(char));
			int err = sscanf(line, "Currently GPU runs on mode %[a-zA-Z0-9-]s", current_mode);
			assert(err == 1);

			if (!strncmp(current_mode, "overdrive", 9))
				current_mode_enum = OVERDRIVE;
			else if (!strncmp(current_mode, "nominal", 7))
				current_mode_enum = NOMINAL;
			else if (!strncmp(current_mode, "underdrive", 10))
				current_mode_enum = UNDERDRIVE;


			/* go over what we've got and determine which one to use */
			for (size_t i = 0; i < modes; i++) {
				if (current_mode_enum == __governor[i].governor) {
					governor->governor = current_mode_enum;
					governor->gpu_core_freq = __governor[i].gpu_core_freq;
					governor->shader_core_freq = __governor[i].shader_core_freq;
					found = 1;
					break;
				}
			}

			if (current_mode)
				free(current_mode);

			/* just in we case we didn't find something relevant */
			assert(found == 1);
		}

	}

	fclose(file);

	return 0;
}

