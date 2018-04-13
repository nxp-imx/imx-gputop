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
#if defined __QNXTO__ || defined __QNX__
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

#if defined __QNXTO__ || defined __QNX__
	debugfs_reopen(file, "r");
#endif


	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* check if this is the process we're interested in */
		if (strncmp(line, "Process:", 8) == 0) {

			uint32_t pid, err;


#if defined __QNXTO__ || defined __QNX__
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
#if defined __QNXTO__ || defined __QNX__
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
#if defined __QNXTO__ || defined __QNX__
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

#if defined __QNXTO__ || defined __QNX__
	debugfs_reopen(file, "r");
#endif

	memset(buf, 0, sizeof(char) * 1024);

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		/* check if this is the process we're interested in */
		if (strncmp(line, "Process:", 8) == 0) {
			uint32_t pid, err;

#if defined __QNXTO__ || defined __QNX__
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
#if defined __QNXTO__ || defined __QNX__
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
#if defined __QNX__ || defined __QNXTO__
	file = debugfs_fopen("debugfs", "w");
	if (!file)
		return -1;
	debugfs_write("vidmem", strlen("vidmem"), file);
#else
	file = debugfs_fopen("vidmem", "w+");
#endif
	if (!file)
		return -1;

	memset(pid_str, 0, 128);
	snprintf(pid_str, sizeof(pid_str), "%d", pid);

	debugfs_write(pid_str, strlen(pid_str), file);

#if defined __QNXTO__ || defined __QNX__
	debugfs_reopen(file, "r");
#endif

	while ((fgets(buf, 1024, file)) != NULL) {
		char *line = buf;

		if (!strncmp(line, "All-Types", strlen("All-Types")))
			continue;

		/* QNX doesn't know about %ms so we need to allocate by
		 * ourselves */
#if defined __QNXTO__ || defined __QNX__
		char *name = calloc(512, sizeof(char));
#else
		char *name = NULL;
#endif

		if (!strncmp(line, "Index", strlen("Index"))) {
#if defined __QNXTO__ || defined __QNX__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->index);
#else
			sscanf(line, "%ms %u", &name, &client->index);
#endif
		}

		if (!strncmp(line, "Vertex", strlen("Vertex"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->vertex);
#else
			sscanf(line, "%ms %u", &name, &client->vertex);
#endif
		}

		if (!strncmp(line, "Texture", strlen("Texture"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->texture);
#else
			sscanf(line, "%ms %u", &name, &client->texture);
#endif
		}

		if (!strncmp(line, "RenderTarget", strlen("RenderTarget"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->render_target);
#else
			sscanf(line, "%ms %u", &name, &client->render_target);
#endif
		}

		if (!strncmp(line, "Depth", strlen("Depth"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->depth);
#else
			sscanf(line, "%ms %u", &name, &client->depth);
#endif
		}

		if (!strncmp(line, "Bitmap", strlen("Bitmap"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->bitmap);
#else
			sscanf(line, "%ms %u", &name, &client->bitmap);
#endif
		}

		if (!strncmp(line, "TileStatus", strlen("TileStatus"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->tile_status);
#else
			sscanf(line, "%ms %u", &name, &client->tile_status);
#endif
		}

		if (!strncmp(line, "Image", strlen("Image"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->image);
#else
			sscanf(line, "%ms %u", &name, &client->image);
#endif
		}

		if (!strncmp(line, "Mask", strlen("Mask"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->mask);
#else
			sscanf(line, "%ms %u", &name, &client->mask);
#endif
		}

		if (!strncmp(line, "Scissor", strlen("Scissor"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->scissor);
#else
			sscanf(line, "%ms %u", &name, &client->scissor);
#endif
		}

		if (!strncmp(line, "HZ", strlen("HZ"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->hz);
#else
			sscanf(line, "%ms %u", &name, &client->hz);
#endif
		}

		if (!strncmp(line, "ICache", strlen("ICache"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->i_cache);
#else
			sscanf(line, "%ms %u", &name, &client->i_cache);
#endif
		}

		if (!strncmp(line, "TxDesc", strlen("TxDesc"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->tx_desc);
#else
			sscanf(line, "%ms %u", &name, &client->tx_desc);
#endif
		}

		if (!strncmp(line, "Fence", strlen("Fence"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->fence);
#else
			sscanf(line, "%ms %u", &name, &client->fence);
#endif
		}

		if (!strncmp(line, "TFBHeader", strlen("TFBHeader"))) {
#if defined __QNXTO__ || defined __QNXTO__
			sscanf(line, "%[a-zA-Z0-9-] %u", name, &client->tfbheader);
#else
			sscanf(line, "%ms %u", &name, &client->tfbheader);
#endif
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
#if defined __QNXTO__ || defined __QNX__
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

#if defined __QNXTO__ || defined __QNX__
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

#if defined __QNXTO__ || defined __QNX__
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
