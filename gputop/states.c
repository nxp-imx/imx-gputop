/*
 * Copyright (C) 2012-2017 by the following authors:
 * - Wladimir J. van der Laan <laanwj@gmail.com>
 * - Christian Gmeiner <christian.gmeiner@gmail.com>
 * - Lucas Stach <l.stach@pengutronix.de>
 * - Russell King <rmk@arm.linux.org.uk>
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

#include "states.h"

struct vivante_idle_module_name vivante_idle_module_names[] = {
	{"FE (Graphics Pipeline Front End) ",		VIVS_HI_IDLE_STATE_FE, 		true},
	{"DE (Draw Engine)                 ", 		VIVS_HI_IDLE_STATE_DE, 		true},
	{"PE (Pixel Engine)                ", 		VIVS_HI_IDLE_STATE_PE, 		true},
	{"SH (Shader Engine)               ", 		VIVS_HI_IDLE_STATE_SH, 		true},
	{"PA (Primitive Assembly)          ", 		VIVS_HI_IDLE_STATE_PA, 		true},
	{"SE (Setup Engine)                ", 		VIVS_HI_IDLE_STATE_SE, 		true},
	{"RA (Rasterizer)                  ", 		VIVS_HI_IDLE_STATE_RA, 		true},
	{"TX (Texture Engine)              ", 		VIVS_HI_IDLE_STATE_TX, 		true},
	{"VG (Vector Graphics)             ", 		VIVS_HI_IDLE_STATE_VG, 		true},
	{"IM                               ", 		VIVS_HI_IDLE_STATE_IM, 		true},
	{"FP (Fragment processor)          ", 		VIVS_HI_IDLE_STATE_FP, 		true},
	{"TS (Tile status)                 ", 		VIVS_HI_IDLE_STATE_TS, 		true},
	{"BL                               ",		VIVS_HI_IDLE_STATE_BL, 		true},
	{"BP                               ",		VIVS_HI_IDLE_STATE_BP, 		true},
	{"MC (Memory Controller)           ",		VIVS_HI_IDLE_STATE_MC, 		true},
	{"AXI_LP (AXI bus in low power)    ", 		VIVS_HI_IDLE_STATE_AXI_LP, 	false}
};


#define NUM_VIV_IDLE_MODULES \
	(sizeof(vivante_idle_module_names)/sizeof(vivante_idle_module_names[0]))

const char *viv_cmd_state_names[]= {
	"IDLE", "DEC", "ADR0", "LOAD0", "ADR1", "LOAD1", "3DADR", "3DCMD",
	"3DCNTL", "3DIDXCNTL", "INITREQDMA", "DRAWIDX", "DRAW", "2DRECT0",
	"2DRECT1", "2DDATA0",
	"2DDATA1", "WAITFIFO", "WAIT", "LINK", "END", "STALL", "UNKNOWN"
};
#define NUM_VIV_CMD_STATE_NAMES \
	(sizeof(viv_cmd_state_names)/sizeof(viv_cmd_state_names[0]))

const char *viv_cmd_dma_state_names[]= {
    "IDLE", "START", "REQ", "END"
};
#define NUM_VIV_CMD_DMA_STATE_NAMES \
	(sizeof(viv_cmd_dma_state_names)/sizeof(viv_cmd_dma_state_names[0]))

const char *viv_cmd_fetch_state_names[]= {
    "IDLE", "RAMVALID", "VALID"
};
#define NUM_VIV_CMD_FETCH_STATE_NAMES \
	(sizeof(viv_cmd_fetch_state_names)/sizeof(viv_cmd_fetch_state_names[0]))

const char *viv_req_dma_state_names[]= {
    "IDLE", "WAITIDX", "CAL"
};

#define NUM_VIV_REQ_DMA_STATE_NAMES \
	(sizeof(viv_req_dma_state_names)/sizeof(viv_req_dma_state_names[0]))

const char *viv_cal_state_names[]= {
    "IDLE", "LDADR", "IDXCALC"
};

#define NUM_VIV_CAL_STATE_NAMES \
	(sizeof(viv_cal_state_names)/sizeof(viv_cal_state_names[0]))

const char *viv_ve_req_state_names[]= {
    "IDLE", "CKCACHE", "MISS"
};

#define NUM_VIV_VE_REQ_STATE_NAMES \
	(sizeof(viv_ve_req_state_names)/sizeof(viv_ve_req_state_names[0]))

struct vivante_gpu_state {
	uint32_t viv_idle_states[NUM_VIV_IDLE_MODULES];

	uint32_t viv_cmd_state[32];

	uint32_t viv_cmd_dma_state[4];
	uint32_t viv_cmd_fetch_state[3];

	uint32_t viv_req_dma_state[3];
	uint32_t viv_cal_state[3];
	uint32_t viv_ve_req_state[3];

	uint32_t idle_cycles_core0;
	uint32_t idle_cycles_core1;

	uint32_t total_idle_cycles_core0;
	uint32_t total_idle_cycles_core1;

};
