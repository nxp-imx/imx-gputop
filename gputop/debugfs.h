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

#ifndef __GPUTOP_DEBUGFS_H
#define __GPUTOP_DEBUGFS_H

/**
 * debugfs_client:
 *
 * Useful structure to keep debugfs client(s). Acts as linked-list with
 * head always pointing to head of the list. If only one element
 * next would be null and head would point to itself.
 */
struct debugfs_client {
	uint32_t pid;
	char *name;

	/** pointer to an array of contexts, used with debugfs_get_current_ctx() */
	uint32_t *ctx;
	uint32_t ctx_no;

	/** next client */
	struct debugfs_client *next;

	/** ist head */
	struct debugfs_client *head;
};

struct debugfs_vid_mem_client {
	uint32_t index;
	uint32_t vertex;
	uint32_t texture;
	uint32_t render_target;
	uint32_t depth;
	uint32_t bitmap;
	uint32_t tile_status;
	uint32_t image;
	uint32_t mask;
	uint32_t scissor;
	uint32_t hz;
	uint32_t i_cache;
	uint32_t tx_desc;
	uint32_t fence;
	uint32_t tfbheader;
};

struct debugfs_clock {
	uint32_t gpu_core_0;
	uint32_t shader_core_0;

	/* for QM */
	uint32_t gpu_core_1;
	uint32_t shader_core_1;
};

enum governor {
	UNDERDRIVE = 1,
	NOMINAL,
	OVERDRIVE,
};

struct debugfs_govern {
	enum governor governor;

	uint32_t gpu_core_freq;
	uint32_t shader_core_freq;
};

/**
 * \brief: helper macro for iterating over clients list
 */
#define list_for_each(client, head) 	\
	for (client = head; client != NULL; client = client->next)


/**
 * gpuperf_debugfs_get_current_clients:
 *
 * Method to retrieve all the clients current associated with vivante GPU
 */
int
debugfs_get_current_clients(struct debugfs_client *clients, const char *path);

/**
 * gpuperf_debugfs_free_clients:
 *
 * Free all memory allocated by vivante_get_current_clients().
 */
void
debugfs_free_clients(struct debugfs_client *clients);

/**
 *
 */
int
debugfs_get_current_ctx(struct debugfs_client *client, const char *path);


/**
 * \brief: get all contexts at once from database.
 */
int
debugfs_get_contexts(struct debugfs_client *clients, const char *path);


/**
 *
 */
void
debugfs_print_contexts(struct debugfs_client *clients);

int
debugfs_get_vid_mem(struct debugfs_vid_mem_client *client, pid_t pid);

/**
 *
 */
int
debugfs_get_gpu_clocks(struct debugfs_clock *clocks, const char *path);

/**
 *
 */
int
debugfs_get_current_gpu_governor(struct debugfs_govern *governor);

#endif
