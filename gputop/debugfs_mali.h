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

#ifndef __GPUTOP_DEBUGFS_MALI_H
#define __GPUTOP_DEBUGFS_MALI_H
typedef enum
{
	CCTX_MEMORY_CLASS_UNNAMED = 0,
	CCTX_MEMORY_CLASS_DEFAULT_HEAP,
	CCTX_MEMORY_CLASS_BASE_INTERNAL,
	CCTX_MEMORY_CLASS_FRAMEPOOL,
	CCTX_MEMORY_CLASS_FRAME_INTERNAL,
	CCTX_MEMORY_CLASS_TILER,
	CCTX_MEMORY_CLASS_PROGRAM,
	CCTX_MEMORY_CLASS_EGL_COLOR_PLANE,
	CCTX_MEMORY_CLASS_EGL_COLOR_PLANE_IMPORTED,
	CCTX_MEMORY_CLASS_VAO,
	CCTX_MEMORY_CLASS_IMG,
	CCTX_MEMORY_CLASS_SURFACE,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_COHERENT,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_PROTECTED,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_PROTECTED_COHERENT,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_READ_PIXELS,
	CCTX_MEMORY_CLASS_TEXTURE,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_TEXTURE_COHERENT,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_EGL_DS_OR_MS,
	CCTX_MEMORY_CLASS_INTERNAL_SURFACE_EGL_DS_OR_MS_PROTECTED,
	CCTX_MEMORY_CLASS_BUF,
	CCTX_MEMORY_CLASS_BUF_INNER_SHAREABLE,
	CCTX_MEMORY_CLASS_BUF_GENERIC_TARGET,
	CCTX_MEMORY_CLASS_BUF_GENERIC_TARGET_MAP_PERSISTENT,
	CCTX_MEMORY_CLASS_BUF_PIXEL_UNPACK,
	CCTX_MEMORY_CLASS_BUF_PIXEL_UNPACK_MAP_PERSISTENT,
	CCTX_MEMORY_CLASS_BUF_SHADER_STORAGE,
	CCTX_MEMORY_CLASS_BUF_SHADER_STORAGE_MAP_PERSISTENT,
	CCTX_MEMORY_CLASS_CRC,
	CCTX_MEMORY_CLASS_CPOM_HMEM,
	CCTX_MEMORY_CLASS_CPOM_RSD,
	CCTX_MEMORY_CLASS_CPOM_MBS2,
	CCTX_MEMORY_CLASS_CPOM_STATIC,
	CCTX_MEMORY_CLASS_CFRAME_HMEM,
	CCTX_MEMORY_CLASS_CFRAME_SAMPLE_POS,
	CCTX_MEMORY_CLASS_CFRAME_DISCARD,
	CCTX_MEMORY_CLASS_CFRAME_TESS_GEOM,
	CCTX_MEMORY_CLASS_COBJ_HMEM,
	CCTX_MEMORY_CLASS_CMAR_HMEM,
	CCTX_MEMORY_CLASS_CMAR_PROF_DUMP,
	CCTX_MEMORY_CLASS_CBLEND_HMEM,
	CCTX_MEMORY_CLASS_GLES_HMEM,
	CCTX_MEMORY_CLASS_GLES_QUERY,
	CCTX_MEMORY_CLASS_GLES_QUERY_COHERENT,
	CCTX_MEMORY_CLASS_GLES_QUERY_HOST,
	CCTX_MEMORY_CLASS_GLES_MULTIVIEW,
	CCTX_MEMORY_CLASS_CDEPS_HMEM,
	CCTX_MEMORY_CLASS_CMEM_SUBALLOC,
	CCTX_MEMORY_CLASS_CMEM_HOARD,
	CCTX_MEMORY_CLASS_CMEM_REGISTRY,
	CCTX_MEMORY_CLASS_CL_PAYLOAD,
	CCTX_MEMORY_CLASS_CL_WLM_TLS,
	CCTX_MEMORY_CLASS_CL_HMEM,
	CCTX_MEMORY_CLASS_CL_SVM,
	CCTX_MEMORY_CLASS_CL_PROTECTED,
	CCTX_MEMORY_CLASS_CINSTR,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_UNCACHED,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_UNCACHED_FIXABLE,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_UNCACHED_FIXED,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_UNCACHED_SAME_VA,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_CACHED,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_CACHED_COHERENT,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_CACHED_COHERENT_FIXABLE,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_CPU_CACHED_COHERENT_FIXED,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_TRANSIENT,
	CCTX_MEMORY_CLASS_GFX_DEVICE_MEMORY_PROTECTED,
	CCTX_MEMORY_CLASS_GFX_DEVICE_EXTERNAL,
	CCTX_MEMORY_CLASS_GFX_DEVICE_EXTERNAL_SWAPCHAIN,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL_PLANE_SURFACE,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL_CSF_SYNC_EVENT,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL_TLS,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL_HOST,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL_PROTECTED,
	CCTX_MEMORY_CLASS_GFX_DEVICE_INTERNAL_TRANSIENT,
	CCTX_MEMORY_CLASS_GFX_DESCRIPTOR_POOL,
	CCTX_MEMORY_CLASS_GFX_DESCRIPTOR_POOL_CACHED,
	CCTX_MEMORY_CLASS_GFX_COMMAND_ALLOCATOR,
	CCTX_MEMORY_CLASS_GFX_COMMAND_ALLOCATOR_CACHED_GROWABLE,
	CCTX_MEMORY_CLASS_GFX_COMMAND_ALLOCATOR_CACHED,
	CCTX_MEMORY_CLASS_GFX_COMMAND_ALLOCATOR_HOST,
	CCTX_MEMORY_CLASS_GFX_COMMAND_ALLOCATOR_PROTECTED,
	CCTX_MEMORY_CLASS_GFX_COMMAND_ALLOCATOR_JIT,
	CCTX_MEMORY_CLASS_GFX_GRAPH_INTERNAL_HOST,
	CCTX_MEMORY_CLASS_GFX_GRAPH_INTERNAL_DEVICE,
	CCTX_MEMORY_CLASS_GFX_GRAPH_INTERNAL_DEVICE_TRANSIENT,
	CCTX_MEMORY_CLASS_VK_BOUND_BUFFER,
	CCTX_MEMORY_CLASS_VK_BOUND_IMAGE,
	CCTX_MEMORY_CLASS_CMAR_SIGNAL,
	CCTX_MEMORY_CLASS_CMAR_FLUSH_CHAIN,
	CCTX_MEMORY_CLASS_CMAR_METADATA_LIST,
//	CCTX_MEMORY_CLASS_HISTOGRAM_COUNT, 
//	CCTX_MEMORY_CLASS_JIT = CCTX_MEMORY_CLASS_HISTOGRAM_COUNT, 
//	CCTX_MEMORY_CLASS_MMU,
	CCTX_MEMORY_CLASS_COUNT 
} cctx_memory_class;


/**
 * debugfs_ctx_client:
 *
 * Useful structure to keep debugfs client(s). Acts as linked-list with
 * head always pointing to head of the list. If only one element
 * next would be null and head would point to itself.
 */
 struct debugfs_mali_info {
	uint64_t busy_time;
	uint64_t idle_time;
	uint64_t last_busy_time;
	uint64_t last_idle_time;
	uint64_t busy_delta_time;
	uint64_t idle_delta_time;
	uint64_t protm_time;
       uint64_t last_protm_time;
	uint64_t protm_delta_time;
	uint64_t compute_time;
	uint64_t frag_time;
	uint64_t tiler_time;
	uint64_t last_frag_time;
	uint64_t last_compute_time;
	uint64_t last_tiler_time;
	uint64_t frag_delta_time;
	uint64_t compute_delta_time;
	uint64_t tiler_delta_time;
	uint32_t total_mem_used;
	uint32_t last_render_freq;
	bool no_shader_usage;

	char  name[128];
	char  version[128];
	uint32_t version_major;
};

 struct debugfs_kctx_mem_client {
  char kctx[128];
  uint32_t  mem_used;
 };

 struct debugfs_kctx_client {
	uint32_t kctx_no;
	struct debugfs_kctx_mem_client mem;
	char kctx_name[128]; 
	/** next client */
	struct debugfs_kctx_client *next;

	/**list head */
	struct debugfs_kctx_client *head;
};

struct debugfs_ctx_mem_client {
	uint32_t class_mem_total;
	uint32_t class_mem[CCTX_MEMORY_CLASS_COUNT ];
};

struct debugfs_ctx_client {
	uint32_t ctx_no;
	char ctx_dir_name[128];
	char process_name[128];
	char *process_full_path_name;
	uint32_t PID;
	uint32_t CTX;
       uint32_t ctx_mem_total;
	uint32_t gpu_mem_total;
	uint32_t ctx_gpu_mem_total;
	struct debugfs_ctx_mem_client mem;
	uint32_t explict_va_mem;
	uint32_t explict_commit_mem;
	uint32_t explict_uncommit_mem;

	uint32_t implict_va_mem;
	uint32_t implict_commit_mem;
	uint32_t implict_uncommit_mem;

	uint32_t imported_va_mem;
	uint32_t imported_commit_mem;
	uint32_t imported_uncommit_mem;

	/** next client */
	struct debugfs_ctx_client *next;

	/**list head */
	struct debugfs_ctx_client *head;
};

#define list_for_each(client, head) 	\
	for (client = head; client != NULL; client = client->next)

int debugfs_get_gpu_kctx(struct debugfs_kctx_client *client, const char *path );
int debugfs_get_gpu_ctx(struct debugfs_ctx_client *client, const char *path );
int debugfs_get_gpu_simple(struct debugfs_ctx_client *client, const char *path );
int debugfs_get_gpu_usage(struct debugfs_mali_info *info, const char *path);
int debugfs_get_gpu_meminfo(struct debugfs_kctx_client *kctx_client, struct debugfs_ctx_client *ctx_client);

void debugfs_free_kctx_clients(struct debugfs_kctx_client *kctx_clients);
void debugfs_free_ctx_clients(struct debugfs_ctx_client *ctx_clients);

#endif
