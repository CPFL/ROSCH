/*
 * Copyright (C) Yusuke FUJII
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nvrm_priv.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

/* just in case */
#define MAX_HANDLE 0x40000000

uint32_t nvrm_handle_alloc(struct nvrm_context *ctx) {
	struct nvrm_handle **ptr = &ctx->hchain;
	uint32_t expected = 1;
	while (*ptr && (*ptr)->handle == expected) {
		ptr = &(*ptr)->next;
		expected++;
	}
	if (expected > MAX_HANDLE) {
		fprintf(stderr, "Out of handles!\n");
		abort();
	}
	struct nvrm_handle *nhandle = malloc(sizeof *nhandle);
	nhandle->next = *ptr;
	nhandle->handle = expected;	
	*ptr = nhandle;
	return nhandle->handle;
}

void nvrm_handle_free(struct nvrm_context *ctx, uint32_t handle) {
	struct nvrm_handle **ptr = &ctx->hchain;
	while (*ptr && (*ptr)->handle != handle) {
		ptr = &(*ptr)->next;
	}
	if (!*ptr) {
		fprintf(stderr, "Tried to free nonexistent handle %08x\n", handle);
		abort();
	}
	struct nvrm_handle *phandle = *ptr;
	*ptr = phandle->next;
	free(phandle);
}

int nvrm_xlat_device(struct nvrm_context *ctx, int idx) {
	int i;
	int oidx = idx;
	for (i = 0; i < NVRM_MAX_DEV; i++)
		if (ctx->devs[i].gpu_id != NVRM_GPU_ID_INVALID)
			if (!idx--)
				return i;
	fprintf(stderr, "nvrm: tried accessing OOB device %d\n", oidx);
	abort();
}

int nvrm_num_devices(struct nvrm_context *ctx) {
	int i;
	int cnt = 0;
	for (i = 0; i < NVRM_MAX_DEV; i++)
		if (ctx->devs[i].gpu_id != NVRM_GPU_ID_INVALID)
			cnt++;
	return cnt;
}

int nvrm_mthd_context_list_devices(struct nvrm_context *ctx, uint32_t handle, uint32_t *gpu_id) {
	struct nvrm_mthd_context_list_devices arg = {
	};
	int res = nvrm_ioctl_call(ctx, handle, NVRM_MTHD_CONTEXT_LIST_DEVICES, &arg, sizeof arg);
	int i;
	if (res)
		return res;
	for (i = 0; i < 32; i++)
		gpu_id[i] = arg.gpu_id[i];
	return 0;
}

int nvrm_mthd_context_enable_device(struct nvrm_context *ctx, uint32_t handle, uint32_t gpu_id) {
	struct nvrm_mthd_context_enable_device arg = {
		.gpu_id = gpu_id,
		.unk04 = { 0xffffffff },	
	};
	return nvrm_ioctl_call(ctx, handle, NVRM_MTHD_CONTEXT_ENABLE_DEVICE, &arg, sizeof arg);
}

int nvrm_mthd_context_disable_device(struct nvrm_context *ctx, uint32_t handle, uint32_t gpu_id) {
	struct nvrm_mthd_context_disable_device arg = {
		.gpu_id = gpu_id,
		.unk04 = { 0xffffffff },	
	};
	return nvrm_ioctl_call(ctx, handle, NVRM_MTHD_CONTEXT_DISABLE_DEVICE, &arg, sizeof arg);
}

int nvrm_device_get_chipset(struct nvrm_device *dev, uint32_t *major, uint32_t *minor, uint32_t *stepping) {
	struct nvrm_mthd_subdevice_get_chipset arg;
	int res = nvrm_ioctl_call(dev->ctx, dev->osubdev, NVRM_MTHD_SUBDEVICE_GET_CHIPSET, &arg, sizeof arg);
	if (res)
		return res;
	if (major) *major = arg.major;
	if (minor) *minor = arg.minor;
	if (stepping) *stepping = arg.stepping;
	return 0;
}

int nvrm_device_get_fb_size(struct nvrm_device *dev, uint64_t *fb_size) {
	int res = nvrm_ioctl_get_fb_size(dev->ctx, dev->idx, fb_size);
	if (res)
		return res;
	return 0;
}

int nvrm_device_get_vendor_id(struct nvrm_device *dev, uint16_t *vendor_id) {
	int res = nvrm_ioctl_get_vendor_id(dev->ctx, dev->idx, vendor_id);
	if (res)
		return res;
	return 0;
}

int nvrm_device_get_device_id(struct nvrm_device *dev, uint16_t *device_id) {
	int res = nvrm_ioctl_get_device_id(dev->ctx, dev->idx, device_id);
	if (res)
		return res;
	return 0;
}

int nvrm_device_get_gpc_mask(struct nvrm_device *dev, uint32_t *mask) {
	struct nvrm_mthd_subdevice_get_gpc_mask arg = {
	};
	int res = nvrm_ioctl_call(dev->ctx, dev->osubdev, NVRM_MTHD_SUBDEVICE_GET_GPC_MASK, &arg, sizeof arg);
	if (res)
		return res;
	*mask = arg.gpc_mask;
	return 0;
}

int nvrm_device_get_gpc_tp_mask(struct nvrm_device *dev, int gpc_id, uint32_t *mask) {
	struct nvrm_mthd_subdevice_get_gpc_tp_mask arg = {
		.gpc_id = gpc_id,
	};
	int res = nvrm_ioctl_call(dev->ctx, dev->osubdev, NVRM_MTHD_SUBDEVICE_GET_GPC_TP_MASK, &arg, sizeof arg);
	if (res)
		return res;
	*mask = arg.tp_mask;
	return 0;
}

int nvrm_device_get_total_tp_count(struct nvrm_device *dev, int *count) {
	int c = 0;
	uint32_t gpc_mask, tp_mask;
	int res = nvrm_device_get_gpc_mask(dev, &gpc_mask);
	if (res)
		return res;
	int i;
	for (i = 0; gpc_mask; i++, gpc_mask >>= 1) {
		if (gpc_mask & 1) {
			res = nvrm_device_get_gpc_tp_mask(dev, i, &tp_mask);
			if (res)
				return res;
			c += __builtin_popcount(tp_mask);
		}
	}
	*count = c;
	return 0;
}

struct nvrm_vspace *nvrm_vspace_create(struct nvrm_device *dev) {
	struct nvrm_vspace *vas = calloc(sizeof *vas, 1);
	uint64_t limit = 0;
	if (!vas)
		goto out_alloc;
	vas->ctx = dev->ctx;
	vas->dev = dev;
	vas->ovas = nvrm_handle_alloc(vas->ctx);
	vas->odma = nvrm_handle_alloc(vas->ctx);
	if (nvrm_ioctl_create_vspace(vas->dev, vas->dev->odev, vas->ovas, NVRM_CLASS_MEMORY_VM, 0x00010000, &limit, 0))
		goto out_vspace;
	if (nvrm_ioctl_create_dma(vas->ctx, vas->ovas, vas->odma, NVRM_CLASS_DMA_READ, 0x20000000, 0, limit))
		goto out_dma;

	return vas;

out_dma:
	nvrm_ioctl_destroy(vas->ctx, vas->dev->odev, vas->ovas);
out_vspace:
	nvrm_handle_free(vas->ctx, vas->odma);
	nvrm_handle_free(vas->ctx, vas->ovas);
	free(vas);
out_alloc:
	return 0;
}

void nvrm_vspace_destroy(struct nvrm_vspace *vas) {
	nvrm_ioctl_destroy(vas->ctx, vas->ovas, vas->odma);
	nvrm_ioctl_destroy(vas->ctx, vas->dev->odev, vas->ovas);
	nvrm_handle_free(vas->ctx, vas->odma);
	nvrm_handle_free(vas->ctx, vas->ovas);
	free(vas);
}

struct nvrm_bo *nvrm_bo_create(struct nvrm_vspace *vas, uint64_t size, int sysram) {
	struct nvrm_bo *bo = calloc(sizeof *bo, 1);
	if (!bo)
		goto out_alloc;
	bo->ctx = vas->ctx;
	bo->dev = vas->dev;
	bo->vas = vas;
	bo->size = size;
	bo->handle = nvrm_handle_alloc(bo->ctx);
	uint32_t flags1 = sysram ? 0xd001 : 0x1d101;
#if 0
	uint32_t flags2 = sysram ? 0x3a000000 : 0x18000000; /* snooped. */
#else
	uint32_t flags2 = sysram ? 0x5a000000 : 0x18000000;
#endif
	if (nvrm_ioctl_memory(bo->ctx, bo->dev->odev, bo->vas->ovas, bo->handle, flags1, flags2, 0, size))
		goto out_bo;
	if (nvrm_ioctl_vspace_map(bo->ctx, bo->dev->odev, bo->vas->odma, bo->handle, 0, size, &bo->gpu_addr))
		goto out_map;
	return bo;

out_map:
	nvrm_ioctl_destroy(bo->ctx, bo->dev->odev, bo->handle);
out_bo:
	nvrm_handle_free(bo->ctx, bo->handle);
	free(bo);
out_alloc:
	return 0;
}

uint64_t nvrm_bo_gpu_addr(struct nvrm_bo *bo) {
	return bo->gpu_addr;
}

void *nvrm_bo_host_map(struct nvrm_bo *bo) {
	if (bo->mmap)
		return bo->mmap;
	if (nvrm_ioctl_host_map(bo->ctx, bo->dev->osubdev, bo->handle, 0, bo->size, &bo->foffset))
		goto out_host_map;
	void *res = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, bo->dev->fd, bo->foffset);
	if (res == MAP_FAILED)
		goto out_mmap;
	bo->mmap = res;
	return bo->mmap;

out_mmap:
	nvrm_ioctl_host_unmap(bo->ctx, bo->dev->osubdev, bo->handle, bo->foffset);
out_host_map:
	return 0;
}

void nvrm_bo_host_unmap(struct nvrm_bo *bo) {
	if (bo->mmap) {
		munmap(bo->mmap, bo->size);
		nvrm_ioctl_host_unmap(bo->ctx, bo->dev->osubdev, bo->handle, bo->foffset);
	}
}

void nvrm_bo_destroy(struct nvrm_bo *bo) {
	nvrm_bo_host_unmap(bo);
	nvrm_ioctl_vspace_unmap(bo->ctx, bo->dev->odev, bo->vas->odma, bo->handle, bo->gpu_addr);
	nvrm_ioctl_destroy(bo->ctx, bo->dev->odev, bo->handle);
	nvrm_handle_free(bo->ctx, bo->handle);
	free(bo);
}


struct nvrm_channel *nvrm_channel_create_ib(struct nvrm_vspace *vas, uint32_t cls, struct nvrm_bo *ib) {
	struct nvrm_channel *chan = calloc(sizeof *chan, 1);
	if (!chan)
		goto out_alloc;
	chan->ctx = vas->ctx;
	chan->dev = vas->dev;
	chan->vas = vas;
	chan->cls = cls;
	chan->oerr = nvrm_handle_alloc(chan->ctx);
	chan->oedma = nvrm_handle_alloc(chan->ctx);
	chan->ofifo = nvrm_handle_alloc(chan->ctx);

	if (nvrm_ioctl_memory(chan->ctx, chan->dev->odev, chan->dev->odev, chan->oerr, 0xd001, 0x3a000000, 0, 0x1000))
		goto out_err;

	if (nvrm_ioctl_create_dma(chan->ctx, chan->oerr, chan->oedma, NVRM_CLASS_DMA_READ, 0x20100000, 0, 0xfff))
		goto out_edma;

	struct nvrm_create_fifo_ib arg = {
		.error_notify = chan->oedma,
		.dma = chan->vas->odma,
		.ib_addr = ib->gpu_addr,
		.ib_entries = ib->size / 8,
	};
	if (nvrm_ioctl_create(chan->ctx, chan->dev->odev, chan->ofifo, cls, &arg))
		goto out_fifo;

	if (nvrm_ioctl_host_map(chan->ctx, chan->dev->osubdev, chan->ofifo, 0, 0x200, &chan->fifo_foffset))
		goto out_fifo_map;
	chan->fifo_mmap = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, chan->dev->fd, chan->fifo_foffset & ~0xfff);
	if (chan->fifo_mmap == MAP_FAILED)
		goto out_mmap;

	return chan;

out_mmap:
	nvrm_ioctl_host_unmap(chan->ctx, chan->dev->osubdev, chan->ofifo, chan->fifo_foffset);
out_fifo_map:
	nvrm_ioctl_destroy(chan->ctx, chan->dev->odev, chan->ofifo);
out_fifo:
	nvrm_ioctl_destroy(chan->ctx, chan->oerr, chan->oedma);
out_edma:
	nvrm_ioctl_destroy(chan->ctx, chan->dev->odev, chan->oerr);
out_err:
	nvrm_handle_free(chan->ctx, chan->oerr);
	nvrm_handle_free(chan->ctx, chan->oedma);
	nvrm_handle_free(chan->ctx, chan->ofifo);
out_alloc:
	return 0;
}

int nvrm_channel_activate(struct nvrm_channel *chan) {
	if (chan->cls >= 0xa06f) {
		struct nvrm_mthd_fifo_ib_activate arg = {
			1,
		};
		return nvrm_ioctl_call(chan->ctx, chan->ofifo, NVRM_MTHD_FIFO_IB_ACTIVATE, &arg, sizeof arg);
	}
	return 0;
}

void nvrm_channel_destroy(struct nvrm_channel *chan) {
	while (chan->echain) {
		struct nvrm_eng *eng = chan->echain;
		chan->echain = eng->next;
		nvrm_ioctl_destroy(chan->ctx, chan->ofifo, eng->handle);
		nvrm_handle_free(chan->ctx, eng->handle);
		free(eng);
	}
	munmap(chan->fifo_mmap, 0x1000);
	nvrm_ioctl_host_unmap(chan->ctx, chan->dev->osubdev, chan->ofifo, chan->fifo_foffset);
	nvrm_ioctl_destroy(chan->ctx, chan->dev->odev, chan->ofifo);
	nvrm_ioctl_destroy(chan->ctx, chan->oerr, chan->oedma);
	nvrm_ioctl_destroy(chan->ctx, chan->dev->odev, chan->oerr);
	nvrm_handle_free(chan->ctx, chan->oerr);
	nvrm_handle_free(chan->ctx, chan->oedma);
	nvrm_handle_free(chan->ctx, chan->ofifo);
}

void *nvrm_channel_host_map_regs(struct nvrm_channel *chan) {
	return chan->fifo_mmap + (chan->fifo_foffset & 0xfff);
}

void *nvrm_channel_host_map_errnot(struct nvrm_channel *chan);

struct nvrm_eng *nvrm_eng_create(struct nvrm_channel *chan, uint32_t eid, uint32_t cls) {
	struct nvrm_eng *eng = calloc(sizeof *eng, 1);
	if (!eng)
		goto out_alloc;
	eng->chan = chan;
	eng->handle = nvrm_handle_alloc(chan->ctx);
	if (nvrm_ioctl_create(chan->ctx, chan->ofifo, eng->handle, cls, 0))
		goto out_eng;
	eng->next = chan->echain;
	chan->echain = eng;
	return eng;

out_eng:
	nvrm_handle_free(chan->ctx, eng->handle);
	free(eng);
out_alloc:
	return 0;
}
