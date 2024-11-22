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
#include <dirent.h>

#include "gpuperfcnt/gpuperfcnt_debugfs.h"
#include "debugfs_mali.h"
#include "mali.h"
#include "top.h"

#ifdef HAVE_GPU_MALI
static const char *underlined_color = "\033[4m";
static const char *regular_color = "\033[0m";
/* current flags */
static uint32_t flags = 0x0;

//static struct debugfs_mali_info info;
static char class_mem[CCTX_MEMORY_CLASS_COUNT ][128]= {
   "Unnamed (Total memory: ",
   "Default Heap (Total memory: ",
   "Base Internal (Total memory: ",
   "Framepool (Total memory: ",
   "Frame Internal (Total memory: ",
   "Tiler (Total memory: ",
   "GPU Program (Total memory: ",
   "EGL Color Plane (Total memory: ",
   "EGL Color Plane Imported (Total memory: ",
   "GLES Vertex Array Object (Total memory: ", 
   "Image Descriptor (Total memory: ", 
   "Surface (Total memory: ",
   "Internal Surface (Total memory: ",
   "Coherent Surface (Total memory: ",
   "Protected Surface (Total memory: ",
   "Protected Coherent Surface (Total memory: ",
   "CPU Cached Surface To Read Pixels (Total memory: ",
   "Texture (Total memory: ",
   "Coherent Texture (Total memory: ",
   "EGL Depth/stencil or Multisample Surface (Total memory: ",
   "EGL Protected Depth/stencil or Multisample Surface (Total memory: ",
   "Buffer (Total memory: ",
   "Inner-Shareable Buffer (Total memory: ",
   "Generic Buffer (Total memory: ",
   "Persistently Mapped Generic Buffer (Total memory: ",
   "Pixel Unpack Buffer (Total memory: ",
   "Persistently Mapped Pixel Unpack Buffer (Total memory: ",
   "Shader Storage Buffer (Total memory: ",
   "Persistently Mapped Shader Storage Buffer (Total memory: ",
   "CRC Buffer (Total memory: ", 
   "CPOM Host Memory (Total memory: ",
   "CPOM Render State (Total memory: ", 
   "CPOM Compute Shader (Total memory: ", 
   "CPOM Static Data (Total memory: ", 
   "CFRAME Host Memory (Total memory: ", 
   "CFRAME Sample Position (Total memory: ", 
   "CFRAME Discardable FBD (Total memory: ", 
   "CFRAME Tessellation/Geometry (Total memory: ", 
   "COBJ Host Memory (Total memory: ", 
   "CMAR Host Memory (Total memory: ",
   "CMAR Profiling/Dumping (Total memory: ",
   "CBLEND Host Memory (Total memory: ", 
   "GLES Host Memory (Total memory: ", 
   "GLES Query/XFB/Unroll (Total memory: ", 
   "GLES Query/XFB/Unroll Host (Total memory: ",
   "GLES Query/XFB/Unroll coherent (Total memory: ",
   "GLES Multiview (Total memory: ", 
   "CDEPS Host Memory (Total memory: ", 
   "CMEM Sub-allocators (Total memory: ",
   "CMEM Hoard (Total memory: ",
   "CMEM Registry (Total memory: ",
   "CL Command Payloads (Total memory: ",
   "CL Workgroup/Thread (Total memory: ",
   "CL Host Memory (Total memory: ",
   "CL Shared Virtual Memory (Total memory: ",
   "CL Protected Memory (Total memory: ",
   "CINSTR Memory (Total memory: ",
   "GFX Device Memory CPU Uncached (Total memory: ",
   "GFX Device Memory CPU Uncached Fixable (Total memory: ",
   "GFX Device Memory CPU Uncached Fixed (Total memory: ",
   "GFX Device Memory CPU Uncached Same VA (Total memory: ",
   "GFX Device Memory CPU Cached (Total memory: ",
   "GFX Device Memory CPU Cached Coherent (Total memory: ",
   "GFX Device Memory CPU Cached Coherent Fixable (Total memory: ",
   "GFX Device Memory CPU Cached Coherent Fixed (Total memory: ",
   "GFX Device Memory Transient (Total memory: ",
   "GFX Device Memory Protected (Total memory: ",
   "GFX Device External Memory (Total memory: ",
   "GFX Device External Swapchain Memory (Total memory: ",
   "GFX Device Internal Memory (Total memory: ",
   "GFX Device Internal Plane/Surface Descriptor Memory (Total memory: ",
   "GFX Device Internal CSF Sync Event Memory (Total memory: ",
   "GFX Device Internal TLS Memory (Total memory: ",
   "GFX Device Internal Host Memory (Total memory: ",
   "GFX Device Internal Protected Memory (Total memory: ",
   "GFX Device Internal Transient Memory (Total memory: ",
   "GFX Descriptor Pool Memory (Total memory: ",
   "GFX Descriptor Pool Memory Cached (Total memory: ",
   "GFX Command Allocator Memory (Total memory: ",
   "GFX Command Allocator Memory Cached Growable (Total memory: ",
   "GFX Command Allocator Memory Cached (Total memory: ",
   "GFX Command Allocator Host Memory (Total memory: ",
   "GFX Command Allocator Protected Memory (Total memory: ",
   "GFX Command Allocator JIT Memory (Total memory: ",
   "GFX Graph internal host (Total memory: ",
   "GFX Graph internal device (Total memory: ",
   "GFX Graph internal device transient (Total memory: ",
   "Vulkan Bound Buffer Memory (Total memory: ",
   "Vulkan Bound Image Memory (Total memory: ",
   "CMAR Signal Memory (Total memory: ",
   "CMAR Flush Chain Memory (Total memory: ",
   "CMAR Metadata List Memory (Total memory: ",
};

static char class_mem_name[CCTX_MEMORY_CLASS_COUNT ][128]= {
   "Unnamed",
   "Default Heap ",
   "Base Internal",
   "Framepool",
   "Frame Internal",
   "Tiler",
   "GPU Program",
   "EGL Color Plane",
   "EGL Color Plane Imported",
   "GLES Vertex Array Object", 
   "Image Descriptor", 
   "Surface",
   "Internal Surface ",
   "Coherent Surface ",
   "Protected Surface",
   "Protected Coherent Surface",
   "CPU Cached Surface To Read Pixels",
   "Texture",
   "Coherent Texture (Total memory: ",
   "EGL Depth/stencil or Multisample Surface ",
   "EGL Protected Depth/stencil or Multisample Surface",
   "Buffer",
   "Inner-Shareable Buffer",
   "Generic Buffer",
   "Persistently Mapped Generic Buffer",
   "Pixel Unpack Buffer (Total memory: ",
   "Persistently Mapped Pixel Unpack Buffer",
   "Shader Storage Buffer (Total memory: ",
   "Persistently Mapped Shader Storage Buffer",
   "CRC Buffer", 
   "CPOM Host Memory",
   "CPOM Render State", 
   "CPOM Compute Shader", 
   "CPOM Static Data", 
   "CFRAME Host Memory", 
   "CFRAME Sample Position", 
   "CFRAME Discardable FBD", 
   "CFRAME Tessellation/Geometry", 
   "COBJ Host Memory", 
   "CMAR Host Memory",
   "CMAR Profiling/Dumping",
   "CBLEND Host Memory", 
   "GLES Host Memory", 
   "GLES Query/XFB/Unroll ", 
   "GLES Query/XFB/Unroll Host",
   "GLES Query/XFB/Unroll coherent",
   "GLES Multiview", 
   "CDEPS Host Memory", 
   "CMEM Sub-allocators",
   "CMEM Hoard",
   "CMEM Registry",
   "CL Command Payloads",
   "CL Workgroup/Thread",
   "CL Host Memory ",
   "CL Shared Virtual Memory ",
   "CL Protected Memory ",
   "CINSTR Memory",
   "GFX Device Memory CPU Uncached",
   "GFX Device Memory CPU Uncached Fixable",
   "GFX Device Memory CPU Uncached Fixed",
   "GFX Device Memory CPU Uncached Same VA",
   "GFX Device Memory CPU Cached",
   "GFX Device Memory CPU Cached Coherent",
   "GFX Device Memory CPU Cached Coherent Fixable",
   "GFX Device Memory CPU Cached Coherent Fixed",
   "GFX Device Memory Transient",
   "GFX Device Memory Protected",
   "GFX Device External Memory",
   "GFX Device External Swapchain Memory",
   "GFX Device Internal Memory",
   "GFX Device Internal Plane/Surface Descriptor Memory",
   "GFX Device Internal CSF Sync Event Memory",
   "GFX Device Internal TLS Memory",
   "GFX Device Internal Host Memory",
   "GFX Device Internal Protected Memory",
   "GFX Device Internal Transient Memory",
   "GFX Descriptor Pool Memory",
   "GFX Descriptor Pool Memory Cached",
   "GFX Command Allocator Memory",
   "GFX Command Allocator Memory Cached Growable",
   "GFX Command Allocator Memory Cached",
   "GFX Command Allocator Host Memory",
   "GFX Command Allocator Protected Memory",
   "GFX Command Allocator JIT Memory",
   "GFX Graph internal host",
   "GFX Graph internal device",
   "GFX Graph internal device transient",
   "Vulkan Bound Buffer Memory",
   "Vulkan Bound Image Memory",
   "CMAR Signal Memory",
   "CMAR Flush Chain Memory",
   "CMAR Metadata List Memory",
};

static struct debugfs_mali_info info;
static  char* get_process_name_by_pid(char * pid)
{
    char* name = (char*)calloc(1024,sizeof(char));
    if(name){
        sprintf(name, "/proc/%s/cmdline",pid);
        FILE* f = fopen(name,"r");
        if(f){
            size_t size;
            size = fread(name, sizeof(char), 1024, f);
            if(size>0){
                if('\n'==name[size-1])
                    name[size-1]='\0';
            }
            fclose(f);
        }
    }
    return name;
}
static int get_gpu_ctx_dir(struct debugfs_ctx_client *clients )
{
   struct dirent* dent;
   //struct debugfs_ctx_client *it = clients->head;
   char PID[128],  CTX[128];

   DIR* srcdir = opendir("/sys/kernel/debug/mali0/ctx");

   if (srcdir == NULL)
   {
       fprintf(stderr,"open ctx dir error\n");
       return -1;
   }
   
   while((dent = readdir(srcdir)) != NULL)
   {
       struct stat st;
   
       if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0 || strcmp(dent->d_name, "defaults") == 0)
           continue;
   
       if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
       {
           fprintf(stderr,"ctx dir stat %s error \n", dent->d_name);
           continue;
       }
       //struct debugfs_ctx_client *it_next = it->next;
       struct debugfs_ctx_client *client;
       client = calloc(1, sizeof(*client));

       if (S_ISDIR(st.st_mode))
       {
           static int count = 0;
           int  i=0, j=0;
           clients-> ctx_no++;
           strcpy(client->ctx_dir_name, dent->d_name);
           while(dent->d_name[i] != '_')
           {
              PID[i]=dent->d_name[i];
              i++;
           }
           PID[i++] = 0;
           while((dent->d_name[i] != 0) && (dent->d_name[i] != '\n') )
           {
              CTX[j++]=dent->d_name[i++];
           }
           CTX[j] = 0;
           sscanf(PID, "%d", &client->PID);
           sscanf(CTX, "%d", &client->CTX);
           client->process_full_path_name=get_process_name_by_pid(PID);
           //fprintf(stdout," mem ctx dir %s  PID: %d Process :%s\n", client->ctx_dir_name, client->PID, client->process_full_path_name);
           client->ctx_no=count++;
           client->next =clients->head; 
           clients->head=client;

       }
    //   it = it_next;
   }
   closedir(srcdir);
   return 0;

}

void gtop_set_display_flags(enum flags_type flag)
{
   switch(flag)
   {
      case FLAG_SHOW_CONTEXTS:
         if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS))
            REMOVE_FLAG(flags, FLAG_SHOW_CONTEXTS);
         else
            SET_FLAG(flags, FLAG_SHOW_CONTEXTS);
         break;
      default:
          break;
   }
}

int debugfs_get_gpu_kctx(struct debugfs_kctx_client *clients, const char *path )
{
   FILE *file = NULL;
   char buf[1024];
   //struct debugfs_mali_info info;
   
   memset(clients, 0, sizeof(*clients));
   if (!path) {
      file = debugfs_fopen("gpu_memory", "r");
   } else {
      file = fopen(path, "r");
   }

   if (!file){
      fprintf(stderr, "Failed to open gpu_memory file\n");
      return -1;
   }
   memset(buf, 0, sizeof(char) * 1024);
   while ((fgets(buf, 1024, file)) != NULL) {
       char *line = buf;
       char *name= info.name;
       int i=0;
       int err;
       if(!strncmp(line, "mali", 4)){
          err=sscanf (line, "%s  %u\n", name, &info.total_mem_used);
          if(err != 2) {
            fprintf(stderr, "Failed to get kctx name and mem total\n");
            return -1;
         }
          //fprintf(stdout, "Total  kctx memory %d  \n", info.total_mem_used);
          continue;
       }
       struct debugfs_kctx_client *client;
       client = calloc(1, sizeof(*client));
       char *name1 = client->kctx_name;
       line= line+2;
       //fprintf(stdout, "%s \n", line);
       err = sscanf(line, "%s         %d\n", name1, &(client->mem).mem_used);
       if(err !=2){
          fprintf(stderr, "Failed to get kctx %s and mem used err %d\n", client->kctx_name, err);
          free(client);
          return -1;
       }
      client->kctx_no=i++;
      client->next =clients->head; 
      clients->head=client;
   }
   fclose(file);
   return 0;
}

static int get_mem_class(struct debugfs_ctx_client *client,char *line, char *cmp_str, int idx)
{
   int count = strlen(cmp_str);
   if(!strncmp(line, cmp_str, count)){
   line= line+count;
   //fprintf(stdout, "%d  %s \n", idx, line);
   while(*line !=')'){
      (client->mem).class_mem[idx]=((client->mem).class_mem[idx])*10+(*line -'0');
       line=line+1;
   }
   client->mem.class_mem_total += (client->mem).class_mem[idx];
   return 0;
   }
   return 1;
}
int debugfs_get_gpu_ctx(struct debugfs_ctx_client *clients, const char *path)
{
   FILE *file = NULL;
   char buf[1024];
   char buf_ctx[1024];
   int chan_cnt= strlen("Channel: ");
   memset(clients, 0, sizeof(*clients));
   if(get_gpu_ctx_dir(clients)){
      fprintf(stderr, "Failed to get gpu memory ctx sub directory\n");
      return -1;
   }
   struct debugfs_ctx_client  *curr_client;
   list_for_each(curr_client, clients->head) {
   memset(buf_ctx, 0, sizeof(buf_ctx));
   int mem_class_idx=0;
  // fprintf(stdout, "%d  %d   %s \n",  curr_client->ctx_no,curr_client->PID, curr_client->ctx_dir_name );

   if (!path) {
      snprintf(buf_ctx, sizeof(buf_ctx), "%s/%s/%s",  "ctx", curr_client->ctx_dir_name,"mem_profile");
      file = debugfs_fopen(buf_ctx, "r");
   } else {
      file = fopen(path, "r");
   }
   if (!file){
      //fprintf(stdout, "Failed to open file %s \n", buf_ctx);
      continue;
   }
   if((fgets(curr_client->process_name, 128, file)) == NULL)
   {
      fprintf(stderr, "Failed to process_name\n");
      return -1;
   }
   // fprintf(stdout, "fopen file  %s  %s \n", buf_ctx, curr_client->process_name);
   memset(buf, 0, sizeof(char) * 1024);
   //parsing meminfo from each application
   while ((fgets(buf, 1024, file)) != NULL) {
       char *line = buf;
   if(!strncmp(line, "Total allocated GPU memory:", strlen("Total allocated GPU memory:"))){
      line=line+strlen("Total allocated GPU memory:");
      sscanf(line, "%d", &curr_client->ctx_gpu_mem_total);
      clients->gpu_mem_total += curr_client->ctx_gpu_mem_total;
      //fprintf(stdout, "Total  allocated gpu  memory :%d \n", curr_client->ctx_gpu_mem_total);
      break;
   }
   if(strncmp(line, "Channel: ", chan_cnt))
      continue;
   line = buf+chan_cnt;
   //fprintf(stdout, " %s \n", line);
   while(get_mem_class(curr_client, line, class_mem[mem_class_idx],mem_class_idx))
   {
      mem_class_idx++;
      if(mem_class_idx >=CCTX_MEMORY_CLASS_COUNT){
         fprintf(stdout, "Error:Exceed Maximum class mem  %s \n", line);
         mem_class_idx=0;
         break;
      }   
      continue;
   }
   mem_class_idx++;
   //continue;
   } 
   clients->ctx_mem_total += curr_client->mem.class_mem_total;
   fclose(file);
}
   return 0;
}


int debugfs_get_gpu_meminfo(struct debugfs_kctx_client *kctx_client, struct debugfs_ctx_client *ctx_client)
{
   if(debugfs_get_gpu_kctx(kctx_client, NULL))
      return -1;
   if(debugfs_get_gpu_ctx(ctx_client, NULL))
      return -1;
   return 0;
}

int debugfs_get_gpu_usage(struct debugfs_mali_info *info, const char *path)
{
   FILE *file = NULL;
   char buf[1024];
   char busy[128];
   char idle[128];
   char protm[128];
   char frag[128];
   char shader[128];
   char tiler[128];

   char *line = buf;
   char *name= busy;
   char *name1= idle;
   char *name2= protm;
   char *name3= shader;
   char *name4= frag;
   char *name5= tiler;


   if (!path) {
      file = debugfs_fopen("dvfs_utilization", "r");
   } else {
      file = fopen(path, "r");
   }

   if (!file){
      fprintf(stderr, "Failed to open dvfs_utilization file\n");
      return -1;
   }

   memset(buf, 0, sizeof(char) * 1024);
   int err;
   if( fgets(buf, 1024, file) != NULL){
      err = sscanf(line, "%s  %"PRIu64" %s%" PRIu64" %s  %"PRIu64"\n", name, &info->busy_time, name1, &info->idle_time, name2, &info->protm_time);
      if(err != 6){
         fprintf(stderr, "Failed to sscanf usage\n"); 
         return -1;
      }
    if( fgets(buf, 1024, file) != NULL){
       err = sscanf(line, "%s  %"PRIu64" %s%" PRIu64" %s  %"PRIu64"\n",name3, &info->frag_time, name4, &info->compute_time, name5, &info->tiler_time);
       if(err != 6){
           fprintf(stderr, "Failed to sscanf usage\n");
          return -1;
          }
       info->no_shader_usage=false;
    }
    else info->no_shader_usage=true;
      //fprintf(stdout, "busy time %d and idle time %d\n", info->busy_time, info->idle_time);
      fclose(file);
   }
   else fprintf(stderr, "Failed to fget dvfs usage\n");
   file = fopen("/sys/kernel/debug/clk/gpu/clk_rate", "r");

   if (!file){
      fprintf(stderr, "Failed to open clk/gpu/clk_rate file\n");
      return -1;
   }

   memset(buf, 0, sizeof(char) * 1024);
   if( fgets(buf, 1024, file) != NULL){
      err = sscanf(line, " %d\n", &info->last_render_freq);
      if(err !=1){
         fprintf(stderr, "Failed to sscanf last period render frequency\n");
         return -1;
      }
      // fprintf(stdout, "%d\n", info->last_render_freq);
      fclose(file);
      return 0;
   }
   fprintf(stderr, "Failed to fget dvfs usage\n");

   return -1;
}

void debugfs_free_kctx_clients(struct debugfs_kctx_client *kctx_clients){
   struct debugfs_kctx_client *it = kctx_clients->head;

   while (it) {
      /* save next so we can remove the current node */
      struct debugfs_kctx_client *it_next = it->next;
      free(it);
      it = it_next;
   }
   memset(kctx_clients, 0, sizeof(*kctx_clients));
}

void debugfs_free_ctx_clients(struct debugfs_ctx_client *ctx_clients){
   struct debugfs_ctx_client *it = ctx_clients->head;

   while (it) {
      /* save next so we can remove the current node */
      struct debugfs_ctx_client *it_next = it->next;
      free(it->process_full_path_name);
      free(it);

      it = it_next;
   }

   memset(ctx_clients, 0, sizeof(*ctx_clients));
}

static void display_mali_debugfs_ctx_info(struct debugfs_ctx_client *clients){
   struct debugfs_ctx_client  *curr_client;
   int i;

   if(clients->ctx_mem_total != 0)
      fprintf(stdout, "class Mem: %d bytes  total ", clients->ctx_mem_total);
   if(clients->ctx_mem_total != 0)
      fprintf(stdout, "     allocated gpu Mem: %d bytes total \n", clients->gpu_mem_total);

   list_for_each(curr_client, clients->head) {
      if((curr_client->mem).class_mem_total == 0)
         continue;
      fprintf(stdout, "\n");
      fprintf(stdout, "%s", underlined_color);
      if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS))
         fprintf(stdout, "PID    %d   CTX   %d    %s \n", curr_client->PID, curr_client->CTX, curr_client->process_full_path_name);
      else
         fprintf(stdout, "PID    %d   %s \n", curr_client->PID, curr_client->process_full_path_name);
      fprintf(stdout, "%s", regular_color);
      fprintf(stdout, "class Mem: %d bytes total\n", (curr_client->mem).class_mem_total);
      for(i=0; i<CCTX_MEMORY_CLASS_COUNT;i++){
         if((curr_client->mem).class_mem[i] != 0)
         fprintf(stdout, " %s: %d bytes\n", class_mem_name[i], (curr_client->mem).class_mem[i]);
      }
      fprintf(stdout, "allocated gpu Mem: %d bytes total\n", curr_client->ctx_gpu_mem_total);
      fprintf(stdout, "\n");
      
   }
}

void gtop_display_mali_debugfs_info(void){
   struct debugfs_kctx_client  kctx_clients;
   struct debugfs_ctx_client  ctx_clients;

// struct debugfs_ctx_client *ctx_client;
   if(debugfs_get_gpu_usage(&info, NULL)){
      fprintf(stderr, "Failed to get gpu dvfc usages\n");
      return;
   }
   if(debugfs_get_gpu_kctx(&kctx_clients, NULL)){
      fprintf(stderr, "Failed to get gpu memory\n");
      return;
   }
   info.busy_delta_time = info.busy_time -info.last_busy_time ;
   info.idle_delta_time = info.idle_time -info.last_idle_time ;
   info.last_busy_time= info.busy_time;
   info.last_idle_time = info.idle_time;
   fprintf(stdout, "%s", regular_color);
   if(info.busy_delta_time == 0)
      fprintf(stdout, "GPU last render period frequency : %dMHz  utilization  : %.2f%%\n", info.last_render_freq/1000000, 0.0);
   else 
      fprintf(stdout, "GPU last render period frequency : %dMHz  utilization : %.2f%%\n", info.last_render_freq/1000000, info.busy_delta_time*100.0/(info.busy_delta_time+info.idle_delta_time));
   fprintf(stdout, "gpu kernel Mem: %dKB total\n", info.total_mem_used*4);
   debugfs_free_kctx_clients(&kctx_clients);

   if(debugfs_get_gpu_ctx(&ctx_clients, NULL)){
      fprintf(stderr, "Failed to get gpu ctx memory\n");
      return;
   }
   struct debugfs_ctx_client  *clients = &ctx_clients;
   if(clients->ctx_mem_total != 0){
      fprintf(stdout, "\n");
      fprintf(stdout, "class Mem: %d kB total    ", clients->ctx_mem_total/1024);
      fprintf(stdout, "allocated gpu Mem: %d kB total\n", clients->gpu_mem_total/1024);
      fprintf(stdout, "%s", underlined_color);
      if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS))
         fprintf(stdout, " %7s %7s%16s %20s %22s\n",
            "PID", "CTX",  "Class MEM(kB)",  "   Allocated GPU MEM(kB)", "      CMD                                                                   ");
      else
         fprintf(stdout, " %7s %16s %20s %22s\n",
            "PID",  "Class MEM(kB)",  "   Allocated GPU MEM(kB)", "      CMD                                                                   ");
   }
   struct debugfs_ctx_client  *curr_ctx_client;
   fprintf(stdout, "%s", regular_color);

   list_for_each(curr_ctx_client, clients->head) {
      if((curr_ctx_client->mem).class_mem_total == 0)
         continue;
      fprintf(stdout, "%1s%7u%1s", "",
            curr_ctx_client->PID, "");
      if (FLAG_IS_SET(flags, FLAG_SHOW_CONTEXTS))
         fprintf(stdout, "%1s%7u%1s", "",
            curr_ctx_client->CTX, "");
      fprintf(stdout, "%3s%8"PRIu32"%12s%8"PRIu32,
            "", (curr_ctx_client->mem).class_mem_total / (1024), 
            "", curr_ctx_client->ctx_gpu_mem_total / (1024));
      if(curr_ctx_client->process_full_path_name[0] == '/')
         fprintf(stdout, "                  %s", strrchr(curr_ctx_client->process_full_path_name, '/')+1);
      else
         fprintf(stdout, "                  %s", curr_ctx_client->process_full_path_name);
      fprintf(stdout, "\n");
      
   }

   //display_mali_debugfs_ctx_info(&ctx_clients);
   debugfs_free_ctx_clients(&ctx_clients);
}

void gtop_display_mali_debugfs_ktx_info(void){
   //struct debugfs_mali_info info;
   struct debugfs_kctx_client  kctx_clients;
   struct debugfs_kctx_client  *curr_client;
   memset(&info, 0, sizeof(info));

   if(debugfs_get_gpu_kctx(&kctx_clients, NULL)){
      fprintf(stderr, "Failed to get gpu memory\n");
      return;
   }
   fprintf(stdout, "\n");
   /* draw with bold */
   fprintf(stdout, "%s", underlined_color);       
   fprintf(stdout, "Gpu kernel Mem : %dKB total \n", info.total_mem_used*4);
   fprintf(stdout, "%s", regular_color);
   list_for_each(curr_client, kctx_clients.head) {
      fprintf(stdout, "    %s                %dKB\n", curr_client->kctx_name, (curr_client->mem).mem_used*4);
   }
   fprintf(stdout, "\n");
   debugfs_free_kctx_clients(&kctx_clients);
}

void gtop_display_mali_debugfs_pid_mem_info(void){
   struct debugfs_ctx_client  ctx_clients;

   fprintf(stdout, "\n");
   if(debugfs_get_gpu_ctx(&ctx_clients, NULL)){
      fprintf(stderr, "Failed to get gpu ctx memory\n");
      return;
   }

   display_mali_debugfs_ctx_info(&ctx_clients);
   debugfs_free_ctx_clients(&ctx_clients);
}

void gtop_display_mali_debugfs_dvfs_utilization_info()
{
   fprintf(stdout, "\n");
   if(debugfs_get_gpu_usage(&info, NULL)){
      fprintf(stderr, "Failed to get gpu dvfc usages\n");
      return;
   }
   uint64_t total_counter_time;
   info.busy_delta_time = info.busy_time -info.last_busy_time ;
   info.idle_delta_time = info.idle_time -info.last_idle_time ;
   info.protm_delta_time = info.protm_time -info.last_protm_time ;
   total_counter_time = info.busy_delta_time+info.idle_delta_time;
   info.last_protm_time = info.protm_time;
   info.last_busy_time= info.busy_time;
   info.last_idle_time = info.idle_time;

   info.compute_delta_time = info.compute_time -info.last_compute_time ;
   info.frag_delta_time = info.frag_time -info.last_frag_time ;
   info.tiler_delta_time = info.tiler_time -info.last_tiler_time ;

   info.last_compute_time= info.compute_time;
   info.last_frag_time = info.frag_time;
   info.last_tiler_time = info.tiler_time;

   //if gpu is idle, dvfs will not add both gpu active and non-active counter, busy time and idle time will not change, just report 0.0 usage
   if(total_counter_time == 0 ||info.busy_delta_time ==0 )
   {
      fprintf(stdout, "GPU last render period frequency :  %dMHz\n", info.last_render_freq/1000000);
      fprintf(stdout, "GPU utilization : %.2f%%\n", 0.0);
      fprintf(stdout, "GPU protect mode utilization : %.2f%%\n", 0.0);
      if(!info.no_shader_usage)
      {
          fprintf(stdout, "Fragment shader utilization : %.2f%%\n", 0.0);
          fprintf(stdout, "Non Fragment shader utilization : %.2f%%\n", 0.0);
          fprintf(stdout, "Tiler utilization : %.2f%%\n", 0.0);
      }
      return;
   }
   fprintf(stdout, "GPU last render period frequency :  %dMHz\n",  info.last_render_freq/1000000);
   fprintf(stdout, "GPU utilization : %.2f%%\n", info.busy_delta_time*100.0/(info.busy_delta_time+info.idle_delta_time));
   fprintf(stdout, "GPU protect mode utilization : %.2f%%\n", info.protm_delta_time*100.0/(info.busy_delta_time+info.idle_delta_time));

   if(!info.no_shader_usage)
   {
      fprintf(stdout, "Fragment shader utilization : %.2f%%\n", info.frag_delta_time*100.0/(info.busy_delta_time+info.idle_delta_time));
      fprintf(stdout, "Non Fragment shader utilization : %.2f%%\n",(info.compute_delta_time)*100.0/(info.busy_delta_time+info.idle_delta_time));
      fprintf(stdout, "Tiler utilization : %.2f%%\n", info.tiler_delta_time*100.0/(info.busy_delta_time+info.idle_delta_time));
   }
}

#endif
