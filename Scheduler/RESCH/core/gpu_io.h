#ifndef __GPU_IO_H__
#define __GPU_IO_H__

#define MMIO_BAR0 0

#define nv_rd08(addr,offset) ioread8(addr+offset)
#define nv_rd32(addr,offset) ioread32(addr+offset)
#define nv_rd32_native(addr,offset) ioread32be(addr+offset)

#if 0 /* DETAIL_DEBUG */

#define nv_wr32(addr,offset, value) do{ \
    iowrite32(value, addr+offset); \
}while(0)

#define nv_wr08(addr,offset, value) do{ \
    iowrite8(value, addr+offset); \
}while(0)

#define nv_wr32_native(addr,offset, value) do{ \
    iowrite32be(value, addr+offset); \
}while(0)

#else

#define nv_wr32(addr,offset, value) do{ \
    iowrite32(value, addr+offset); \
}while(0)

#define nv_wr08(addr,offset, value) do{ \
    iowrite8(value, addr+offset); \
}while(0)

#define nv_wr32_native(addr,offset, value) do{ \
    iowrite32be(value, addr+offset); \
}while(0)

#endif


#endif
