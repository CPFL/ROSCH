#ifndef __NVRM_FIFO_H__
#define __NVRM_FIFO_H__

#include <sched.h>
struct nvrm_desc;

#define IOREAD32(addr) *(uint32_t *)(addr)
#define IOWRITE32(addr,val) *(uint32_t *)(addr) = val
#define SCHED_YIELD sched_yield

static inline void __nvrm_fire_ring(struct nvrm_desc *desc)
{
	if (desc->fifo.pb_pos != desc->fifo.pb_put) {
		uint64_t base = desc->fifo.pb_base + desc->fifo.pb_put;
		uint32_t len;
		if (desc->fifo.pb_pos > desc->fifo.pb_put) {
			len = desc->fifo.pb_pos - desc->fifo.pb_put;
		} else {
			len = desc->fifo.pb_size - desc->fifo.pb_put;
			desc->fifo.push(desc, base, len, 0);
			desc->fifo.pb_put = 0;
			base = desc->fifo.pb_base;
			len = desc->fifo.pb_pos;
		}
		if (len > 0)
			desc->fifo.push(desc, base, len, 0);
		desc->fifo.pb_put = desc->fifo.pb_pos;
		if (desc->fifo.kick)
			desc->fifo.kick(desc);
	}
}

static inline void __nvrm_out_ring(struct nvrm_desc *desc, uint32_t word)
{
	while (((desc->fifo.pb_pos + 4) & desc->fifo.pb_mask) == desc->fifo.pb_get) {
		uint32_t old = desc->fifo.pb_get;
		__nvrm_fire_ring(desc);
		desc->fifo.update_get(desc);
		if (old == desc->fifo.pb_get) {
		    SCHED_YIELD();
		}
	}
	desc->fifo.pb_map[desc->fifo.pb_pos/4] = word;
	desc->fifo.pb_pos += 4;
	desc->fifo.pb_pos &= desc->fifo.pb_mask;
}

static inline void __nvrm_ring_space(struct nvrm_desc *desc, uint32_t word)
{
	if (desc->fifo.space)
		desc->fifo.space(desc, word);
}

static inline void __nvrm_begin_ring_nv50(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, mthd | (subc<<13) | (len<<18));
}

static inline void __nvrm_begin_ring_nv50_const(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, mthd | (subc<<13) | (len<<18) | (0x4<<28));
}

static inline void __nvrm_begin_ring_nvc0(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, (0x2<<28) | (len<<16) | (subc<<13) | (mthd>>2));
}

static inline void __nvrm_begin_ring_nvc0_const(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, (0x6<<28) | (len<<16) | (subc<<13) | (mthd>>2));
}


static inline void __nvrm_begin_ring_nve4(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, (0x2<<28) | (len<<16) | (subc<<13) | (mthd>>2));

}

static inline void __nvrm_begin_ring_nve4_const(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, (0x6<<28) | (len<<16) | (subc<<13) | (mthd>>2));
}

static inline void __nvrm_begin_ring_nve4_il(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, (0x8<<28) | (len<<16) | (subc<<13) | (mthd>>2));
}
static inline void __nvrm_begin_ring_nve4_1l(struct nvrm_desc *desc, int subc, int mthd, int len)
{
	__nvrm_out_ring(desc, (0xa<<28) | (len<<16) | (subc<<13) | (mthd>>2));
}

static inline uint32_t __resch_fifo_read_reg(struct nvrm_desc *desc, uint32_t reg)
{
    return IOREAD32((unsigned long)desc->fifo.regs + reg);
}

static inline void __resch_fifo_write_reg(struct nvrm_desc *desc, uint32_t reg, uint32_t val)
{
    IOWRITE32((unsigned long)desc->fifo.regs + reg, val);
}

static void resch_fifo_push(struct nvrm_desc *desc, uint64_t base, uint32_t len, int flags)
{
    uint64_t w = base | (uint64_t)len << 40 | (uint64_t)flags << 40;
    while (((desc->fifo.ib_put + 1) & desc->fifo.ib_mask) == desc->fifo.ib_get) {
	uint32_t old = desc->fifo.ib_get;
	desc->fifo.ib_get = __resch_fifo_read_reg(desc, 0x88);
	if (old == desc->fifo.ib_get) {
	    sched_yield();
	}
    }
    desc->fifo.ib_map[desc->fifo.ib_put * 2] = w;
    desc->fifo.ib_map[desc->fifo.ib_put * 2 + 1] = w >> 32;
    desc->fifo.ib_put++;
    desc->fifo.ib_put &= desc->fifo.ib_mask;
    __sync_synchronize();
    desc->dummy = desc->fifo.ib_map[0];
    __resch_fifo_write_reg(desc, 0x8c, desc->fifo.ib_put);
}

static void resch_fifo_update_get(struct nvrm_desc *desc)
{
    uint32_t lo = __resch_fifo_read_reg(desc, 0x58);
    uint32_t hi = __resch_fifo_read_reg(desc, 0x5c);
    if (hi & 0x80000000) {
	uint64_t mg = ((uint64_t)hi << 32 | lo) & 0xffffffffffull;
	desc->fifo.pb_get = mg - desc->fifo.pb_base;
    } else {
	desc->fifo.pb_get = 0;
    }
}



#endif
