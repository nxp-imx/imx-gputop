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
#ifdef __QNXTO__
		file = debugfs_fopen("debugfs", "w");
		if (!file)
			return -1;

		debugfs_write("database", strlen("database"), file);
#else
		file = debugfs_fopen("database", "r");
#endif
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return -1;

	memset(buf, 0, sizeof(char) * 1024);

#ifdef __QNXTO__
	debugfs_reopen(file, "r");
#endif


	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* check if this is the process we're interested in */
		if (strncmp(line, "Process:", 8) == 0) {

			uint32_t pid, err;


#ifdef __QNXTO__
			char *name = malloc(sizeof(char) * 512);
			memset(name, 0, sizeof(char) * 512);
			err = sscanf(line, "Process: %d   %[a-zA-Z0-9-]s\n", &pid, name);
#else
			char *name = NULL;
			err = sscanf(line, "Process: %u     %ms", &pid, &name);
#endif

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
#ifdef __QNXTO__
			free(name);
#endif
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
					client->ctx = malloc(sizeof(uint32_t) * 10);
					memset(client->ctx, 0, sizeof(uint32_t) * 10);
				}

				assert(client->ctx_no < 10);

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
#ifdef __QNXTO__
		file = debugfs_fopen("debugfs", "w");
		if (!file)
			return -1;
		debugfs_write("database", strlen("database"), file);
#else
		file = debugfs_fopen("database", "r");
#endif
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return -1;

#ifdef __QNXTO__
	debugfs_reopen(file, "r");
#endif

	memset(buf, 0, sizeof(char) * 1024);

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* check if this is the process we're interested in */
		if (strncmp(line, "Process:", 8) == 0) {
			uint32_t pid, err;

#ifdef __QNXTO__
			char *name = malloc(sizeof(char) * 512);
			memset(name, 0, sizeof(char) * 512);
			err = sscanf(line, "Process: %d   %[a-zA-Z0-9-]s\n", &pid, name);
#else
			char *name = NULL;
			err = sscanf(line, "Process: %u     %ms", &pid, &name);
#endif

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
#ifdef __QNXTO__
			free(name);
#endif
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
					client->ctx = malloc(sizeof(uint32_t) * 10);
					memset(client->ctx, 0, sizeof(uint32_t) * 10);
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

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		if (!strncmp(line, "All-Types", strlen("All-Types")))
			continue;

		char *name = NULL;

		if (!strncmp(line, "Index", strlen("Index"))) {
			sscanf(line, "%ms %u", &name, &client->index);
		}

		if (!strncmp(line, "Vertex", strlen("Vertex"))) {
			sscanf(line, "%ms %u", &name, &client->vertex);
		}

		if (!strncmp(line, "Texture", strlen("Texture"))) {
			sscanf(line, "%ms %u", &name, &client->texture);
		}

		if (!strncmp(line, "RenderTarget", strlen("RenderTarget"))) {
			sscanf(line, "%ms %u", &name, &client->render_target);
		}

		if (!strncmp(line, "Depth", strlen("Depth"))) {
			sscanf(line, "%ms %u", &name, &client->depth);
		}

		if (!strncmp(line, "Bitmap", strlen("Bitmap"))) {
			sscanf(line, "%ms %u", &name, &client->bitmap);
		}

		if (!strncmp(line, "TileStatus", strlen("TileStatus"))) {
			sscanf(line, "%ms %u", &name, &client->tile_status);
		}

		if (!strncmp(line, "Image", strlen("Image"))) {
			sscanf(line, "%ms %u", &name, &client->image);
		}

		if (!strncmp(line, "Mask", strlen("Mask"))) {
			sscanf(line, "%ms %u", &name, &client->mask);
		}

		if (!strncmp(line, "Scissor", strlen("Scissor"))) {
			sscanf(line, "%ms %u", &name, &client->scissor);
		}

		if (!strncmp(line, "HZ", strlen("HZ"))) {
			sscanf(line, "%ms %u", &name, &client->hz);
		}

		if (!strncmp(line, "ICache", strlen("ICache"))) {
			sscanf(line, "%ms %u", &name, &client->i_cache);
		}

		if (!strncmp(line, "TxDesc", strlen("TxDesc"))) {
			sscanf(line, "%ms %u", &name, &client->tx_desc);
		}

		if (!strncmp(line, "Fence", strlen("Fence"))) {
			sscanf(line, "%ms %u", &name, &client->fence);
		}

		if (!strncmp(line, "TFBHeader", strlen("TFBHeader"))) {
			sscanf(line, "%ms %u", &name, &client->tfbheader);
		}

		if (name)
			free(name);
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
#ifdef __QNXTO__
		/* 
		 * On QNX we open the named pipe and then we write
		 * what we want to retrieve 
		 */

		file = debugfs_fopen("debugfs", "w");
		if (!file) {
			fprintf(stderr, "Failed to open debugfs\n");
			return 0;
		}

		debugfs_write("clients", strlen("clients"), file);
#else
		file = debugfs_fopen("clients", "r");
#endif
	} else {
		file = fopen(path, "r");
	}

	if (!file)
		return 0;

	memset(buf, 0, sizeof(char) * 1024);

#ifdef __QNXTO__
	debugfs_reopen(file, "r");
#endif

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* skip PID and -- */
		if (*line == 'P' || *line == '-') {
			continue;
		}

		struct debugfs_client *client;
		int err;

		client = malloc(sizeof(*client));
		memset(client, 0, sizeof(*client));

#ifdef __QNXTO__
		client->name = malloc(sizeof(char) * 512);
		err = sscanf(line, "%d  %[a-zA-Z0-9-]s\n", &client->pid, client->name);
#else
		err = sscanf(line, "%u     %ms", &client->pid, &client->name);
#endif

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
			free(it);
		} else {
			/* save next so we can remove the current node */
			struct debugfs_client *it_next = it->next;

			/* free all storage */
			if (it->ctx)
				free(it->ctx);
			free(it);

			it = it_next;
		}
		it = it->next;
	}

	memset(clients, 0, sizeof(*clients));
}
