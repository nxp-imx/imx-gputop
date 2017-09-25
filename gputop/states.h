/*
 * Copyright (C) NXP 2017
 * Copyright (C) 2012-2017 by the following authors:
 * - Wladimir J. van der Laan <laanwj@gmail.com>
 * - Christian Gmeiner <christian.gmeiner@gmail.com>
 * - Lucas Stach <l.stach@pengutronix.de>
 * - Russell King <rmk@arm.linux.org.uk>
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

#ifndef __GPUTOP_REGISTER_STATES
#define __GPUTOP_REGISTER_STATES

#ifdef __cplusplus
extern "C" {
#endif

#define VIVS_HI_IDLE_STATE		0x00000004

#define VIVS_HI_IDLE_STATE_FE		0x00000001
#define VIVS_HI_IDLE_STATE_DE		0x00000002
#define VIVS_HI_IDLE_STATE_PE		0x00000004
#define VIVS_HI_IDLE_STATE_SH		0x00000008
#define VIVS_HI_IDLE_STATE_PA		0x00000010
#define VIVS_HI_IDLE_STATE_SE		0x00000020
#define VIVS_HI_IDLE_STATE_RA		0x00000040
#define VIVS_HI_IDLE_STATE_TX		0x00000080
#define VIVS_HI_IDLE_STATE_VG		0x00000100
#define VIVS_HI_IDLE_STATE_IM		0x00000200
#define VIVS_HI_IDLE_STATE_FP		0x00000400
#define VIVS_HI_IDLE_STATE_TS		0x00000800
#define VIVS_HI_IDLE_STATE_BL		0x00001000
#define VIVS_HI_IDLE_STATE_BP		0x00002000
#define VIVS_HI_IDLE_STATE_MC		0x00004000
#define VIVS_HI_IDLE_STATE_AXI_LP	0x80000000

#define VIVS_FE_DMA_DEBUG_STATE		0x00000660
#define VIVS_FE_DMA_DEBUG_ADDR		0x00000664

#define AQ_HI_IDLE			0x00000001

#define GC_2000_TOTAL_CYCLES		0x00000438
#define GC_2000_TOTAL_IDLE_CYCLES	0x00000078

#define GC_TOTAL_CYCLES			0x00000078
#define GC_TOTAL_IDLE_CYCLES		0x0000007c

struct vivante_idle_module_name {
	const char *name;
	uint32_t bit;
	bool inv;
};

#endif /* __GPUTOP_REGISTER_STATES */
