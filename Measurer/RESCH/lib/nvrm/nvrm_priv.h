#ifndef __NVRM_PRIV_H__
#define __NVRM_PRIV_H__

#include <sys/ioctl.h>
#include <inttypes.h>
#include <stdint.h>

#define NVRM_CHECK_VERSION_STR_CMD_STRICT		0
#define NVRM_CHECK_VERSION_STR_CMD_RELAXED		'1'
#define NVRM_CHECK_VERSION_STR_CMD_OVERRIDE		'2'
#define NVRM_CHECK_VERSION_STR_REPLY_UNRECOGNIZED	0
#define NVRM_CHECK_VERSION_STR_REPLY_RECOGNIZED		1

#define NVRM_FIFO_ENG_GRAPH	1
#define NVRM_FIFO_ENG_COPY0	2
#define NVRM_FIFO_ENG_COPY1	3
#define NVRM_FIFO_ENG_COPY2	4
#define NVRM_FIFO_ENG_VP	5
#define NVRM_FIFO_ENG_ME	6
#define NVRM_FIFO_ENG_PPP	7
#define NVRM_FIFO_ENG_BSP	8
#define NVRM_FIFO_ENG_MPEG	9
#define NVRM_FIFO_ENG_SOFTWARE	10
#define NVRM_FIFO_ENG_CRYPT	11
#define NVRM_FIFO_ENG_VCOMP	12
#define NVRM_FIFO_ENG_VENC	13
/* XXX 15? */
/* XXX 14? apparently considered to be valid by blob logic */
#define NVRM_FIFO_ENG_UNK16	16

#define NVRM_COMPUTE_MODE_DEFAULT		0
#define NVRM_COMPUTE_MODE_EXCLUSIVE_THREAD	1
#define NVRM_COMPUTE_MODE_PROHIBITED		2
#define NVRM_COMPUTE_MODE_EXCLUSIVE_PROCESS	3

#define NVRM_PERSISTENCE_MODE_ENABLED	0
#define NVRM_PERSISTENCE_MODE_DISABLED	1


#define NVRM_MAX_DEV 32

#define NVRM_GPU_ID_INVALID ((uint32_t)0xffffffffu)

struct nvrm_mthd_key_value {
	uint32_t key;
	uint32_t value;
};

/* context */

struct nvrm_mthd_context_unk0101 {
	uint32_t unk00;
	uint32_t unk04;
	uint64_t unk08_ptr;
	uint64_t unk10_ptr;
	uint64_t unk18_ptr;
	uint32_t unk20;
	uint32_t unk24;
};
#define NVRM_MTHD_CONTEXT_UNK0101 0x00000101

/* looks exactly like LIST_DEVICES, wtf? */
struct nvrm_mthd_context_unk0201 {
	uint32_t gpu_id[32];
};
#define NVRM_MTHD_CONTEXT_UNK0201 0x00000201

struct nvrm_mthd_context_unk0202 {
	uint32_t gpu_id;
	uint32_t unk04; /* out */
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c_gpu_id; /* out */
	uint32_t unk20;
	uint32_t unk24;
};
#define NVRM_MTHD_CONTEXT_UNK0202 0x00000202

struct nvrm_mthd_context_unk0301 {
	uint32_t unk00[12];
};
#define NVRM_MTHD_CONTEXT_UNK0301 0x00000301

struct nvrm_mthd_context_list_devices {
	uint32_t gpu_id[32];
};
#define NVRM_MTHD_CONTEXT_LIST_DEVICES 0x00000214

struct nvrm_mthd_context_enable_device {
	uint32_t gpu_id;
	uint32_t unk04[32];
};
#define NVRM_MTHD_CONTEXT_ENABLE_DEVICE 0x00000215

struct nvrm_mthd_context_disable_device {
	uint32_t gpu_id;
	uint32_t unk04[32];
};
#define NVRM_MTHD_CONTEXT_DISABLE_DEVICE 0x00000216

/* device */

struct nvrm_mthd_device_unk0201 {
	uint32_t cnt; /* out */
	uint32_t unk04;
	uint64_t ptr; /* XXX */
};
#define NVRM_MTHD_DEVICE_UNK0201 0x00800201

struct nvrm_mthd_device_unk0280 {
	uint32_t unk00; /* out */
};
#define NVRM_MTHD_DEVICE_UNK0280 0x00800280

struct nvrm_mthd_device_set_persistence_mode {
	uint32_t mode;
};
#define NVRM_MTHD_DEVICE_SET_PERSISTENCE_MODE 0x00800287

struct nvrm_mthd_device_get_persistence_mode {
	uint32_t mode;
};
#define NVRM_MTHD_DEVICE_GET_PERSISTENCE_MODE 0x00800288

struct nvrm_mthd_device_unk1102 {
	uint32_t unk00;
	uint32_t unk04;
	uint64_t ptr; /* XXX */
};
#define NVRM_MTHD_DEVICE_UNK1102 0x00801102

struct nvrm_mthd_device_unk1401 {
	uint32_t unk00;
	uint32_t unk04;
	uint64_t ptr; /* XXX */
};
#define NVRM_MTHD_DEVICE_UNK1401 0x00801401

struct nvrm_mthd_device_unk1701 {
	uint32_t unk00;
	uint32_t unk04;
	uint64_t ptr; /* XXX */
};
#define NVRM_MTHD_DEVICE_UNK1701 0x00801701

struct nvrm_mthd_device_unk170d {
	uint32_t unk00;
	uint32_t unk04;
	uint64_t ptr; /* XXX */
	uint64_t unk10;
};
#define NVRM_MTHD_DEVICE_UNK170D 0x0080170d

/* subdevice */

struct nvrm_mthd_subdevice_unk0101 {
	uint32_t unk00;
	uint32_t unk04;
	uint64_t ptr; /* XXX */
};
#define NVRM_MTHD_SUBDEVICE_UNK0101 0x20800101

struct nvrm_mthd_subdevice_get_name {
	uint32_t unk00;
	char name[0x80];
};
#define NVRM_MTHD_SUBDEVICE_GET_NAME 0x20800110

struct nvrm_mthd_subdevice_unk0119 {
	uint32_t unk00;
};
#define NVRM_MTHD_SUBDEVICE_UNK0119 0x20800119

struct nvrm_mthd_subdevice_get_fifo_engines {
	uint32_t cnt;
	uint32_t _pad;
	uint64_t ptr; /* ints */
};
#define NVRM_MTHD_SUBDEVICE_GET_FIFO_ENGINES 0x20800123

struct nvrm_mthd_subdevice_get_fifo_classes {
	uint32_t eng;
	uint32_t cnt;
	uint64_t ptr; /* ints */
};
#define NVRM_MTHD_SUBDEVICE_GET_FIFO_CLASSES 0x20800124

struct nvrm_mthd_subdevice_set_compute_mode {
	uint32_t mode;
	uint32_t unk04;
};
#define NVRM_MTHD_SUBDEVICE_SET_COMPUTE_MODE 0x20800130

struct nvrm_mthd_subdevice_get_compute_mode {
	uint32_t mode;
};
#define NVRM_MTHD_SUBDEVICE_GET_COMPUTE_MODE 0x20800131

struct nvrm_mthd_subdevice_get_gpc_mask {
	uint32_t gpc_mask;
};
#define NVRM_MTHD_SUBDEVICE_GET_GPC_MASK 0x20800137

struct nvrm_mthd_subdevice_get_gpc_tp_mask {
	uint32_t gpc_id;
	uint32_t tp_mask;
};
#define NVRM_MTHD_SUBDEVICE_GET_GPC_TP_MASK 0x20800138

struct nvrm_mthd_subdevice_get_gpu_id {
	uint32_t gpu_id;
};
#define NVRM_MTHD_SUBDEVICE_GET_GPU_ID 0x20800142

/* no param */
#define NVRM_MTHD_SUBDEVICE_UNK0145 0x20800145

/* no param */
#define NVRM_MTHD_SUBDEVICE_UNK0146 0x20800146

struct nvrm_mthd_subdevice_get_fifo_joinable_engines {
	uint32_t eng;
	uint32_t cls;
	uint32_t cnt;
	uint32_t res[0x20];
};
#define NVRM_MTHD_SUBDEVICE_GET_FIFO_JOINABLE_ENGINES 0x20800147

struct nvrm_mthd_subdevice_get_uuid {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t uuid_len;
	char uuid[0x100];
};
#define NVRM_MTHD_SUBDEVICE_GET_UUID 0x2080014a

struct nvrm_mthd_subdevice_unk0303 {
	uint32_t handle_unk003e;
};
#define NVRM_MTHD_SUBDEVICE_UNK0303 0x20800303

struct nvrm_mthd_subdevice_get_time {
	uint64_t time;
};
#define NVRM_MTHD_SUBDEVICE_GET_TIME 0x20800403

struct nvrm_mthd_subdevice_unk0512 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10; /* out */
	uint32_t unk14;
	uint64_t ptr;
};
#define NVRM_MTHD_SUBDEVICE_UNK0512 0x20800512

struct nvrm_mthd_subdevice_unk0522 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10; /* out */
	uint32_t unk14;
	uint64_t ptr;
};
#define NVRM_MTHD_SUBDEVICE_UNK0522 0x20800522

struct nvrm_mthd_subdevice_unk1201 {
	/* XXX reads MP+0x9c on NVCF */
	uint32_t cnt;
	uint32_t _pad;
	uint64_t ptr; /* key:value */
};
#define NVRM_MTHD_SUBDEVICE_UNK1201 0x20801201

struct nvrm_mthd_subdevice_fb_get_params {
	uint32_t cnt;
	uint32_t _pad;
	uint64_t ptr; /* key:value */
};
#define NVRM_PARAM_SUBDEVICE_FB_BUS_WIDTH	11
#define NVRM_PARAM_SUBDEVICE_FB_UNK13		13	/* 5 for NV50; 8 for NVCF and NVE4 */
#define NVRM_PARAM_SUBDEVICE_FB_UNK23		23	/* 0 */
#define NVRM_PARAM_SUBDEVICE_FB_UNK24		24	/* 0 */
#define NVRM_PARAM_SUBDEVICE_FB_PART_COUNT	25
#define NVRM_PARAM_SUBDEVICE_FB_L2_CACHE_SIZE	27
#define NVRM_MTHD_SUBDEVICE_FB_GET_PARAMS 0x20801301

struct nvrm_mthd_subdevice_fb_get_surface_geometry {
	uint32_t width; /* in */
	uint32_t height; /* in */
	uint32_t bpp; /* in/out */
	uint32_t pitch; /* out */
	uint32_t size; /* out */
	uint32_t unk14;
};
#define NVRM_MTHD_SUBDEVICE_FB_GET_SURFACE_GEOMETRY 0x20801324

struct nvrm_mthd_subdevice_get_chipset {
	uint32_t major;
	uint32_t minor;
	uint32_t stepping;
};
#define NVRM_MTHD_SUBDEVICE_GET_CHIPSET 0x20801701

struct nvrm_mthd_subdevice_get_bus_id {
	uint32_t main_id;
	uint32_t subsystem_id;
	uint32_t stepping;
	uint32_t real_product_id;
};
#define NVRM_MTHD_SUBDEVICE_GET_BUS_ID 0x20801801

struct nvrm_mthd_subdevice_bus_get_params {
	uint32_t cnt;
	uint32_t _pad;
	uint64_t ptr; /* key:value */
};
#define NVRM_PARAM_SUBDEVICE_BUS_BUS_ID		29
#define NVRM_PARAM_SUBDEVICE_BUS_DEV_ID		30
#define NVRM_PARAM_SUBDEVICE_BUS_DOMAIN_ID	60
#define NVRM_MTHD_SUBDEVICE_BUS_GET_PARAMS 0x20801802

struct nvrm_mthd_subdevice_get_bus_info {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t regs_size_mb;
	uint64_t regs_base;
	uint32_t unk18;
	uint32_t fb_size_mb;
	uint64_t fb_base;
	uint32_t unk28;
	uint32_t ramin_size_mb;
	uint64_t ramin_base;
	uint32_t unk38;
	uint32_t unk3c;
	uint64_t unk40;
	uint64_t unk48;
	uint64_t unk50;
	uint64_t unk58;
	uint64_t unk60;
	uint64_t unk68;
	uint64_t unk70;
	uint64_t unk78;
	uint64_t unk80;
};
#define NVRM_MTHD_SUBDEVICE_GET_BUS_INFO 0x20801803

struct nvrm_mthd_subdevice_get_vm_info {
	uint32_t total_addr_bits;
	uint32_t pde_addr_bits;
	/* XXX: ... */
	uint32_t unk00[0xa0/4]; /* out */
};
#define NVRM_MTHD_SUBDEVICE_GET_VM_INFO 0x20801806

struct nvrm_mthd_subdevice_unk200a {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_MTHD_SUBDEVICE_UNK200A 0x2080200a

/* FIFO */

struct nvrm_mthd_fifo_ib_object_info {
	uint32_t handle;
	uint32_t name;
	uint32_t hwcls;
	uint32_t eng;
#define NVRM_FIFO_ENG_GRAPH 1
#define NVRM_FIFO_ENG_COPY0 2
};
#define NVRM_MTHD_FIFO_IB_OBJECT_INFO 0x906f0101

struct nvrm_mthd_fifo_ib_activate {
	uint8_t unk00;
};
#define NVRM_MTHD_FIFO_IB_ACTIVATE 0xa06f0103

/* ??? */

struct nvrm_mthd_unk85b6_unk0201 {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_MTHD_UNK85B6_UNK0201 0x85b60201

struct nvrm_mthd_unk85b6_unk0202 {
	uint8_t unk00;
};
#define NVRM_MTHD_UNK85B6_UNK0202 0x85b60202

/* IOCTL_H_ */

#define NVRM_IOCTL_MAGIC 'F'

/* used on dev fd */
struct nvrm_ioctl_create_vspace {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t cls;
	uint32_t flags;
	uint32_t unk14; /* maybe pad */
	uint64_t foffset;
	uint64_t limit;
	uint32_t status;
	uint32_t _pad2;
};
#define NVRM_IOCTL_CREATE_VSPACE _IOWR(NVRM_IOCTL_MAGIC, 0x27, struct nvrm_ioctl_create_vspace)

struct nvrm_ioctl_create_simple {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t cls;
	uint32_t status;
};
#define NVRM_IOCTL_CREATE_SIMPLE _IOWR(NVRM_IOCTL_MAGIC, 0x28, struct nvrm_ioctl_create_simple)

struct nvrm_ioctl_destroy {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t status;
};
#define NVRM_IOCTL_DESTROY _IOWR(NVRM_IOCTL_MAGIC, 0x29, struct nvrm_ioctl_destroy)

struct nvrm_ioctl_call {
	uint32_t cid;
	uint32_t handle;
	uint32_t mthd;
	uint32_t _pad;
	uint64_t ptr;
	uint32_t size;
	uint32_t status;
};
#define NVRM_IOCTL_CALL _IOWR(NVRM_IOCTL_MAGIC, 0x2a, struct nvrm_ioctl_call)

struct nvrm_ioctl_create {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t cls;
	uint64_t ptr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_CREATE _IOWR(NVRM_IOCTL_MAGIC, 0x2b, struct nvrm_ioctl_create)

/* used on dev fd */
struct nvrm_ioctl_get_param {
	uint32_t cid;
	uint32_t handle; /* device */
	uint32_t key;
	uint32_t value; /* out */
	uint32_t status;
};
#define NVRM_IOCTL_GET_PARAM _IOWR(NVRM_IOCTL_MAGIC, 0x32, struct nvrm_ioctl_get_param)

/* used on dev fd */
struct nvrm_ioctl_query {
	uint32_t cid;
	uint32_t handle;
	uint32_t query;
	uint32_t size;
	uint64_t ptr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_QUERY _IOWR(NVRM_IOCTL_MAGIC, 0x37, struct nvrm_ioctl_query)

struct nvrm_ioctl_memory {
	uint32_t cid;
	uint32_t parent;
	uint32_t cls;
	uint32_t unk0c;
	uint32_t status;
	uint32_t unk14;
	uint64_t vram_total;
	uint64_t vram_free;
	uint32_t vspace;
	uint32_t handle;
	uint32_t unk30;
	uint32_t flags1;
#define NVRM_IOCTL_MEMORY_FLAGS1_USER_HANDLE	0x00004000 /* otherwise 0xcaf..... allocated */
	uint64_t unk38;
	uint32_t flags2;
	uint32_t unk44;
	uint64_t unk48;
	uint32_t flags3;
	uint32_t unk54;
	uint64_t unk58;
	uint64_t size;
	uint64_t align;
	uint64_t base;
	uint64_t limit;
	uint64_t unk80;
	uint64_t unk88;
	uint64_t unk90;
	uint64_t unk98;
};
#define NVRM_IOCTL_MEMORY _IOWR(NVRM_IOCTL_MAGIC, 0x4a, struct nvrm_ioctl_memory)

struct nvrm_ioctl_unk4d {
	uint32_t cid;
	uint32_t handle;
	uint64_t unk08;
	uint64_t unk10;
	uint64_t slen;
	uint64_t sptr;
	uint64_t unk28;
	uint64_t unk30;
	uint64_t unk38; /* out */
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_UNK4D _IOWR(NVRM_IOCTL_MAGIC, 0x4d, struct nvrm_ioctl_unk4d)

struct nvrm_ioctl_host_map {
	uint32_t cid;
	uint32_t subdev;
	uint32_t handle;
	uint32_t _pad;
	uint64_t base;
	uint64_t limit;
	uint64_t foffset;
	uint32_t status;
	uint32_t _pad2;
};
#define NVRM_IOCTL_HOST_MAP _IOWR(NVRM_IOCTL_MAGIC, 0x4e, struct nvrm_ioctl_host_map)

struct nvrm_ioctl_host_unmap {
	uint32_t cid;
	uint32_t subdev;
	uint32_t handle;
	uint32_t _pad;
	uint64_t foffset;
	uint32_t status;
	uint32_t _pad2;
};
#define NVRM_IOCTL_HOST_UNMAP _IOWR(NVRM_IOCTL_MAGIC, 0x4f, struct nvrm_ioctl_host_unmap)

struct nvrm_ioctl_create_dma {
	uint32_t cid;
	uint32_t handle;
	uint32_t cls;
	uint32_t flags;
	uint32_t unk10;
	uint32_t parent;
	uint64_t base;
	uint64_t limit;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_CREATE_DMA _IOWR(NVRM_IOCTL_MAGIC, 0x54, struct nvrm_ioctl_create_dma)

struct nvrm_ioctl_vspace_map {
	uint32_t cid;
	uint32_t dev;
	uint32_t vspace;
	uint32_t handle;
	uint64_t base;
	uint64_t size;
	uint32_t flags;
	uint32_t unk24;
	uint64_t addr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_VSPACE_MAP _IOWR(NVRM_IOCTL_MAGIC, 0x57, struct nvrm_ioctl_vspace_map)

struct nvrm_ioctl_vspace_unmap {
	uint32_t cid;
	uint32_t dev;
	uint32_t vspace;
	uint32_t handle;
	uint64_t unk10;
	uint64_t addr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_VSPACE_UNMAP _IOWR(NVRM_IOCTL_MAGIC, 0x58, struct nvrm_ioctl_vspace_unmap)

struct nvrm_ioctl_bind {
	uint32_t cid;
	uint32_t target;
	uint32_t handle;
	uint32_t status;
};
#define NVRM_IOCTL_BIND _IOWR(NVRM_IOCTL_MAGIC, 0x59, struct nvrm_ioctl_bind)

struct nvrm_ioctl_unk5e {
	uint32_t cid;
	uint32_t subdev;
	uint32_t handle;
	uint32_t _pad;
	uint64_t foffset;
	uint64_t ptr; /* to just-mmapped thing */
	uint32_t unk20;
	uint32_t unk24;
};
#define NVRM_IOCTL_UNK5E _IOWR(NVRM_IOCTL_MAGIC, 0x5e, struct nvrm_ioctl_unk5e)

#define NVRM_IOCTL_ESC_BASE 200

struct nvrm_ioctl_card_info {
	struct {
		uint32_t flags;
		uint32_t domain;
		uint8_t bus;
		uint8_t slot;
		uint16_t vendor_id;
		uint16_t device_id;
		uint16_t _pad;
		uint32_t gpu_id;
		uint16_t interrupt;
		uint16_t _pad2;
		uint64_t reg_address;
		uint64_t reg_size;
		uint64_t fb_address;
		uint64_t fb_size;
	} card[32];
};
struct nvrm_ioctl_card_info2 {
	struct {
		uint32_t flags;
		uint32_t domain;
		uint16_t bus;
		uint16_t slot;
		uint16_t vendor_id;
		uint16_t device_id;
		uint32_t _pad;
		uint32_t gpu_id;
		uint32_t interrupt;
		uint32_t _pad2;
		uint64_t reg_address;
		uint64_t reg_size;
		uint64_t fb_address;
		uint64_t fb_size;
		uint32_t index;
		uint32_t _pad3;
	} card[32];
};
#define NVRM_IOCTL_CARD_INFO _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+0, struct nvrm_ioctl_card_info)

struct nvrm_ioctl_env_info {
	uint32_t pat_supported;
};
#define NVRM_IOCTL_ENV_INFO _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+2, struct nvrm_ioctl_env_info)

struct nvrm_ioctl_create_os_event {
	uint32_t cid;
	uint32_t handle;
	uint32_t ehandle;
	uint32_t fd;
	uint32_t status;
};
#define NVRM_IOCTL_CREATE_OS_EVENT _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+6, struct nvrm_ioctl_create_os_event)

struct nvrm_ioctl_destroy_os_event {
	uint32_t cid;
	uint32_t handle;
	uint32_t fd;
	uint32_t status;
};
#define NVRM_IOCTL_DESTROY_OS_EVENT _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+7, struct nvrm_ioctl_destroy_os_event)

struct nvrm_ioctl_check_version_str {
	uint32_t cmd;
	uint32_t reply;
	char vernum[0x40];
};
#define NVRM_IOCTL_CHECK_VERSION_STR _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+10, struct nvrm_ioctl_check_version_str)


#define NVRM_STATUS_SUCCESS		0
#define NVRM_STATUS_ALREADY_EXISTS_SUB	5	/* like 6, but for subdevice-relative stuff */
#define NVRM_STATUS_ALREADY_EXISTS	6	/* tried to create object for eg. device that already has one */
#define NVRM_STATUS_INVALID_PARAM	8	/* NULL param to create, ... */
#define NVRM_STATUS_INVALID_DEVICE	11	/* NVRM_CLASS_DEVICE devid out of range */
#define NVRM_STATUS_INVALID_MTHD	12	/* invalid mthd to call */
#define NVRM_STATUS_OBJECT_ERROR	26	/* object model violation - wrong parent class, tried to create object with existing handle, nonexistent object, etc. */
#define NVRM_STATUS_NO_HW		29	/* not supported on this card */
#define NVRM_STATUS_MTHD_SIZE_MISMATCH	32	/* invalid param size for a mthd */
#define NVRM_STATUS_ADDRESS_FAULT	34	/* basically -EFAULT */
#define NVRM_STATUS_MTHD_CLASS_MISMATCH	41	/* invalid mthd for given class */



/* sw objects */

struct nvrm_create_context {
	uint32_t cid;
};
#define NVRM_CLASS_CONTEXT_NEW		0x0000
#define NVRM_CLASS_CONTEXT		0x0041

struct nvrm_create_device {
	uint32_t idx;
	uint32_t cid;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};
#define NVRM_CLASS_DEVICE_0		0x0080	/* wrt context */
#define NVRM_CLASS_DEVICE_1		0x0081	/* wrt context */
#define NVRM_CLASS_DEVICE_2		0x0082	/* wrt context */
#define NVRM_CLASS_DEVICE_3		0x0083	/* wrt context */
#define NVRM_CLASS_DEVICE_4		0x0084	/* wrt context */
#define NVRM_CLASS_DEVICE_5		0x0085	/* wrt context */
#define NVRM_CLASS_DEVICE_6		0x0086	/* wrt context */
#define NVRM_CLASS_DEVICE_7		0x0087	/* wrt context */

struct nvrm_create_subdevice {
	uint32_t idx;
};
#define NVRM_CLASS_SUBDEVICE_0		0x2080	/* wrt device */
#define NVRM_CLASS_SUBDEVICE_1		0x2081	/* wrt device */
#define NVRM_CLASS_SUBDEVICE_2		0x2082	/* wrt device */
#define NVRM_CLASS_SUBDEVICE_3		0x2083	/* wrt device */

/* no create param */
#define NVRM_CLASS_TIMER		0x0004	/* wrt subdevice */

/* created by create_memory */
#define NVRM_CLASS_MEMORY_SYSRAM	0x003e	/* wrt device */
#define NVRM_CLASS_MEMORY_UNK003F	0x003f	/* wrt device */
#define NVRM_CLASS_MEMORY_UNK0040	0x0040	/* wrt device */
#define NVRM_CLASS_MEMORY_VM		0x0070	/* wrt device */
#define NVRM_CLASS_MEMORY_UNK0071	0x0071	/* wrt device */

/* no create param */
#define NVRM_CLASS_UNK0073		0x0073	/* wrt device; singleton */

/* no create param */
#define NVRM_CLASS_UNK208F		0x208f	/* wrt subdevice */

struct nvrm_create_event {
	uint32_t cid;
	uint32_t cls;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t ehandle;
	uint32_t unk14;
};
#define NVRM_CLASS_EVENT		0x0079	/* wrt graph, etc. */

#define NVRM_CLASS_DMA_READ		0x0002	/* wrt memory */
#define NVRM_CLASS_DMA_WRITE		0x0003	/* wrt memory */
#define NVRM_CLASS_DMA_RW		0x003d	/* wrt memory */

/* no create param */
#define NVRM_CLASS_PEEPHOLE_NV30	0x307e
#define NVRM_CLASS_PEEPHOLE_GF100	0x9068

struct nvrm_create_unk83de {
	uint32_t unk00;
	uint32_t cid;
	uint32_t handle; /* seen with compute */
};
#define NVRM_CLASS_UNK83DE		0x83de	/* wrt context */

/* no create param */
#define NVRM_CLASS_UNK85B6		0x85b6	/* wrt subdevice */

/* no create param */
#define NVRM_CLASS_UNK9096		0x9096	/* wrt subdevice */

struct nvrm_create_unk0005 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};
#define NVRM_CLASS_UNK0005		0x0005

/* FIFO */

struct nvrm_create_fifo_pio {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_FIFO_PIO_NV4		0x006d

struct nvrm_create_fifo_dma {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};
#define NVRM_CLASS_FIFO_DMA_NV40	0x406e
#define NVRM_CLASS_FIFO_DMA_NV44	0x446e

struct nvrm_create_fifo_ib {
	uint32_t error_notify;
	uint32_t dma;
	uint64_t ib_addr;
	uint64_t ib_entries;
	uint32_t unk18;
	uint32_t unk1c;
};
#define NVRM_CLASS_FIFO_IB_G80		0x506f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_G82		0x826f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_MCP89	0x866f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_GF100	0x906f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_GK104	0xa06f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_GK110	0xa16f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_UNKA2	0xa26f	/* wrt device */
#define NVRM_CLASS_FIFO_IB_UNKB0	0xb06f	/* wrt device */

/* graph */

struct nvrm_create_graph {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define NVRM_CLASS_GR_NULL		0x0030

#define NVRM_CLASS_GR_BETA_NV1		0x0012
#define NVRM_CLASS_GR_CLIP_NV1		0x0019
#define NVRM_CLASS_GR_ROP_NV3		0x0043
#define NVRM_CLASS_GR_BETA4_NV4		0x0072
#define NVRM_CLASS_GR_CHROMA_NV4	0x0057
#define NVRM_CLASS_GR_PATTERN_NV4	0x0044
#define NVRM_CLASS_GR_GDI_NV4		0x004a
#define NVRM_CLASS_GR_BLIT_NV4		0x005f
#define NVRM_CLASS_GR_TRI_NV4		0x005d
#define NVRM_CLASS_GR_IFC_NV30		0x308a
#define NVRM_CLASS_GR_IIFC_NV30		0x3064
#define NVRM_CLASS_GR_LIN_NV30		0x305c
#define NVRM_CLASS_GR_SIFC_NV30		0x3066
#define NVRM_CLASS_GR_TEX_NV30		0x307b
#define NVRM_CLASS_GR_SURF2D_G80	0x5062
#define NVRM_CLASS_GR_SIFM_G80		0x5089
/* XXX: 0052, 0062, 0077, 007b, 009e, 309e still exist??? */

#define NVRM_CLASS_GR_2D_G80		0x502d
#define NVRM_CLASS_GR_M2MF_G80		0x5039
#define NVRM_CLASS_GR_3D_G80		0x5097
#define NVRM_CLASS_GR_COMPUTE_G80	0x50c0

#define NVRM_CLASS_GR_3D_G82		0x8297
#define NVRM_CLASS_GR_3D_G200		0x8397
#define NVRM_CLASS_GR_3D_GT212		0x8597
#define NVRM_CLASS_GR_COMPUTE_GT212	0x85c0 /* XXX: no create param for that one?? */
#define NVRM_CLASS_GR_3D_MCP89		0x8697

#define NVRM_CLASS_GR_2D_GF100		0x902d
#define NVRM_CLASS_GR_M2MF_GF100	0x9039
#define NVRM_CLASS_GR_3D_GF100		0x9097
#define NVRM_CLASS_GR_COMPUTE_GF100	0x90c0

#define NVRM_CLASS_GR_COMPUTE_GF110	0x91c0
#define NVRM_CLASS_GR_3D_GF108		0x9197
#define NVRM_CLASS_GR_3D_GF110		0x9297

#define NVRM_CLASS_GR_UPLOAD_GK104	0xa040
#define NVRM_CLASS_GR_3D_GK104		0xa097
#define NVRM_CLASS_GR_COMPUTE_GK104	0xa0c0

#define NVRM_CLASS_GR_UPLOAD_GK110	0xa140
#define NVRM_CLASS_GR_3D_GK110		0xa197
#define NVRM_CLASS_GR_COMPUTE_GK110	0xa1c0

#define NVRM_CLASS_GR_3D_GK208		0xa297

#define NVRM_CLASS_GR_3D_GM107		0xb097
#define NVRM_CLASS_GR_COMPUTE_GM107	0xb0c0

#define NVRM_CLASS_GR_3D_UNKB1		0xb197

/* copy */

struct nvrm_create_copy {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_COPY_GT212		0x85b5
#define NVRM_CLASS_COPY_GF100_0		0x90b5
#define NVRM_CLASS_COPY_GF100_1		0x90b8 /* XXX: wtf? */
#define NVRM_CLASS_COPY_GK104		0xa0b5
#define NVRM_CLASS_COPY_GM107		0xb0b5

/* vdec etc. */

/* no create param */
#define NVRM_CLASS_MPEG_NV31		0x3174
#define NVRM_CLASS_MPEG_G82		0x8274

struct nvrm_create_me {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_ME_NV40		0x4075

struct nvrm_create_vp {
	uint32_t unk00[0x50/4];
};
#define NVRM_CLASS_VP_G80		0x5076
#define NVRM_CLASS_VP_G74		0x7476
#define NVRM_CLASS_VP_G98		0x88b2
#define NVRM_CLASS_VP_GT212		0x85b2
#define NVRM_CLASS_VP_GF100		0x90b2
#define NVRM_CLASS_VP_GF119		0x95b2

struct nvrm_create_bsp {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_BSP_G74		0x74b0
#define NVRM_CLASS_BSP_G98		0x88b1
#define NVRM_CLASS_BSP_GT212		0x85b1
#define NVRM_CLASS_BSP_MCP89		0x86b1
#define NVRM_CLASS_BSP_GF100		0x90b1
#define NVRM_CLASS_BSP_GF119		0x95b1
#define NVRM_CLASS_BSP_GM107		0xa0b0 /* hm. */

struct nvrm_create_ppp {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_PPP_G98		0x88b3
#define NVRM_CLASS_PPP_GT212		0x85b3
#define NVRM_CLASS_PPP_GF100		0x90b3

struct nvrm_create_venc {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_VENC_GK104		0x90b7
#define NVRM_CLASS_VENC_GM107		0xc0b7

/* no create param */
#define NVRM_CLASS_VCOMP_MCP89		0x86b6

/* engine 16 - no create param */

#define NVRM_CLASS_UNK95A1		0x95a1

/* evil stuff */

/* no create param */
#define NVRM_CLASS_CRYPT_G74		0x74c1
#define NVRM_CLASS_CRYPT_G98		0x88b4

/* software fifo eng */

struct nvrm_create_sw_unk0075 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define NVRM_CLASS_SW_UNK0075		0x0075 /* not on G98 nor MCP79 */

/* no create param */
#define NVRM_CLASS_SW_UNK007D		0x007d
#define NVRM_CLASS_SW_UNK208A		0x208a /* not on G98 nor MCP79 */
#define NVRM_CLASS_SW_UNK5080		0x5080
#define NVRM_CLASS_SW_UNK50B0		0x50b0 /* G80:G98 and MCP79 */

struct nvrm_create_sw_unk9072 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};
#define NVRM_CLASS_SW_UNK9072		0x9072 /* GF100+ */

/* no create param */
#define NVRM_CLASS_SW_UNK9074		0x9074 /* GF100+ */

/* display */

/* no create param */
#define NVRM_CLASS_DISP_ROOT_G80	0x5070 /* wrt device; singleton */
#define NVRM_CLASS_DISP_ROOT_G82	0x8270
#define NVRM_CLASS_DISP_ROOT_G200	0x8370
#define NVRM_CLASS_DISP_ROOT_G94	0x8870
#define NVRM_CLASS_DISP_ROOT_GT212	0x8570
#define NVRM_CLASS_DISP_ROOT_GF119	0x9070
#define NVRM_CLASS_DISP_ROOT_GK104	0x9170
#define NVRM_CLASS_DISP_ROOT_GK110	0x9270
#define NVRM_CLASS_DISP_ROOT_GM107	0x9470

struct nvrm_create_disp_fifo {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_DISP_FIFO		0x5079 /* ... allows you to bind to any of the FIFOs??? */

struct nvrm_create_disp_fifo_pio {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define NVRM_CLASS_DISP_CURSOR_G80	0x507a
#define NVRM_CLASS_DISP_CURSOR_G82	0x827a
#define NVRM_CLASS_DISP_CURSOR_GT212	0x857a
#define NVRM_CLASS_DISP_CURSOR_GF119	0x907a
#define NVRM_CLASS_DISP_CURSOR_GK104	0x917a

#define NVRM_CLASS_DISP_OVPOS_G80	0x507b
#define NVRM_CLASS_DISP_OVPOS_G82	0x827b
#define NVRM_CLASS_DISP_OVPOS_GT212	0x857b
#define NVRM_CLASS_DISP_OVPOS_GF119	0x907b
#define NVRM_CLASS_DISP_OVPOS_GK104	0x917b

struct nvrm_create_disp_fifo_dma {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};
#define NVRM_CLASS_DISP_FLIP_G80	0x507c
#define NVRM_CLASS_DISP_FLIP_G82	0x827c
#define NVRM_CLASS_DISP_FLIP_G200	0x837c
#define NVRM_CLASS_DISP_FLIP_GT212	0x857c
#define NVRM_CLASS_DISP_FLIP_GF119	0x907c
#define NVRM_CLASS_DISP_FLIP_GK104	0x917c
#define NVRM_CLASS_DISP_FLIP_GK110	0x927c

#define NVRM_CLASS_DISP_MASTER_G80	0x507d
#define NVRM_CLASS_DISP_MASTER_G82	0x827d
#define NVRM_CLASS_DISP_MASTER_G200	0x837d
#define NVRM_CLASS_DISP_MASTER_G94	0x887d
#define NVRM_CLASS_DISP_MASTER_GT212	0x857d
#define NVRM_CLASS_DISP_MASTER_GF119	0x907d
#define NVRM_CLASS_DISP_MASTER_GK104	0x917d
#define NVRM_CLASS_DISP_MASTER_GK110	0x927d
#define NVRM_CLASS_DISP_MASTER_GM107	0x947d

#define NVRM_CLASS_DISP_OVERLAY_G80	0x507e
#define NVRM_CLASS_DISP_OVERLAY_G82	0x827e
#define NVRM_CLASS_DISP_OVERLAY_G200	0x837e
#define NVRM_CLASS_DISP_OVERLAY_GT212	0x857e
#define NVRM_CLASS_DISP_OVERLAY_GF119	0x907e
#define NVRM_CLASS_DISP_OVERLAY_GK104	0x917e

/* scan results */

/* no create param */
#define NVRM_CLASS_UNK0001		0x0001
#define NVRM_CLASS_UNK0074		0x0074
#define NVRM_CLASS_UNK007F		0x007f
#define NVRM_CLASS_UNK402C		0x402c /* wrt subdevice */
#define NVRM_CLASS_UNK507F		0x507f
#define NVRM_CLASS_UNK907F		0x907f
#define NVRM_CLASS_UNK50A0		0x50a0
#define NVRM_CLASS_UNK50E0		0x50e0
#define NVRM_CLASS_UNK50E2		0x50e2
#define NVRM_CLASS_UNK824D		0x824d
#define NVRM_CLASS_UNK884D		0x884d
#define NVRM_CLASS_UNK83CC		0x83cc
#define NVRM_CLASS_UNK844C		0x844c
#define NVRM_CLASS_UNK9067		0x9067 /* wrt device */
#define NVRM_CLASS_UNK90DD		0x90dd /* wrt subdevice */
#define NVRM_CLASS_UNK90E0		0x90e0 /* wrt subdevice */
#define NVRM_CLASS_UNK90E1		0x90e1 /* wrt subdevice */
#define NVRM_CLASS_UNK90E2		0x90e2 /* wrt subdevice */
#define NVRM_CLASS_UNK90E3		0x90e3 /* wrt subdevice */
#define NVRM_CLASS_UNK90E4		0x90e4
#define NVRM_CLASS_UNK90E5		0x90e5
#define NVRM_CLASS_UNK90E6		0x90e6 /* wrt subdevice */
#define NVRM_CLASS_UNK90EC		0x90ec /* wrt device */
#define NVRM_CLASS_UNK9171		0x9171
#define NVRM_CLASS_UNK9271		0x9271
#define NVRM_CLASS_UNK9471		0x9471
#define NVRM_CLASS_UNKA080		0xa080
#define NVRM_CLASS_UNKA0B6		0xa0b6
#define NVRM_CLASS_UNKA0E0		0xa0e0
#define NVRM_CLASS_UNKA0E1		0xa0e1

struct nvrm_create_unk0078 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};
#define NVRM_CLASS_UNK0078		0x0078

struct nvrm_create_unk007e {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};
#define NVRM_CLASS_UNK007E		0x007e

struct nvrm_create_unk00db {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_UNK00DB		0x00db

struct nvrm_create_unk00f1 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};
#define NVRM_CLASS_UNK00F1		0x00f1

struct nvrm_create_unk00ff {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};
#define NVRM_CLASS_UNK00FF		0x00ff

struct nvrm_create_unk25a0 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};
#define NVRM_CLASS_UNK25A0		0x25a0

struct nvrm_create_unk30f1 {
	uint32_t unk00;
};
#define NVRM_CLASS_UNK30F1		0x30f1

struct nvrm_create_unk30f2 {
	uint32_t unk00;
};
#define NVRM_CLASS_UNK30F2		0x30f2

struct nvrm_create_unk83f3 {
	uint32_t unk00;
};
#define NVRM_CLASS_UNK83F3		0x83f3

struct nvrm_create_unk40ca {
	uint32_t unk00;
};
#define NVRM_CLASS_UNK40CA		0x40ca /* wrt device or subdevice */

struct nvrm_create_unk503b {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_UNK503B		0x503b

struct nvrm_create_unk503c {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define NVRM_CLASS_UNK503C		0x503c /* wrt subdevice */

struct nvrm_create_unk5072 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};
#define NVRM_CLASS_UNK5072		0x5072

struct nvrm_create_unk8d75 {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_UNK8D75		0x8d75

struct nvrm_create_unk906d {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_UNK906D		0x906d

struct nvrm_create_unk906e {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_UNK906E		0x906e

struct nvrm_create_unk90f1 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define NVRM_CLASS_UNK90F1		0x90f1 /* wrt device */

struct nvrm_create_unka06c {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};
#define NVRM_CLASS_UNKA06C		0xa06c

struct nvrm_create_unka0b7 {
	uint32_t unk00;
	uint32_t unk04;
};
#define NVRM_CLASS_UNKA0B7		0xa0b7

/* only seen on device so far */

/* XXX reads PBFB.CFG1 on NVCF */
struct nvrm_query_gpu_params {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t compressible_vram_size;
	uint32_t unk18;
	uint32_t unk1c;
	uint32_t unk20;
	uint32_t nv50_gpu_units;
	uint32_t unk28;
	uint32_t unk2c;
};
#define NVRM_QUERY_GPU_PARAMS 0x00000125

struct nvrm_query_object_classes {
	uint32_t cnt;
	uint32_t _pad;
	uint64_t ptr;
};
#define NVRM_QUERY_OBJECT_CLASSES 0x0000014c

struct nvrm_query_unk019a {
	uint32_t unk00;
};
#define NVRM_QUERY_UNK019A 0x0000019a

struct nvrm_handle {
	struct nvrm_handle *next;
	uint32_t handle;
};

struct nvrm_device {
	struct nvrm_context *ctx;
	int idx;
	int open;
	uint32_t gpu_id;
	int fd;
	uint32_t odev;
	uint32_t osubdev;
};

struct nvrm_context {
	int fd_ctl;
	uint32_t cid;
	struct nvrm_device devs[NVRM_MAX_DEV];
	struct nvrm_handle *hchain;
	int ver_major;
	int ver_minor;
};

struct nvrm_vspace {
	struct nvrm_context *ctx;
	struct nvrm_device *dev;
	uint32_t ovas;
	uint32_t odma;
};

struct nvrm_bo {
	struct nvrm_context *ctx;
	struct nvrm_device *dev;
	struct nvrm_vspace *vas;
	uint32_t handle;
	uint64_t size;
	uint64_t gpu_addr;
	uint64_t foffset;
	void *mmap;
};

struct nvrm_channel {
	struct nvrm_context *ctx;
	struct nvrm_device *dev;
	struct nvrm_vspace *vas;
	struct nvrm_eng *echain;
	uint32_t ofifo;
	void *fifo_mmap;
	uint64_t fifo_foffset;
	uint32_t oedma;
	uint32_t oerr;
	uint32_t cls;
};

struct nvrm_eng {
	struct nvrm_channel *chan;
	struct nvrm_eng *next;
	uint32_t cls;
	uint32_t handle;
};


int nvrm_num_devices(struct nvrm_context *ctx);
int nvrm_device_get_chipset(struct nvrm_device *dev, uint32_t *major, uint32_t *minor, uint32_t *stepping);
int nvrm_device_get_fb_size(struct nvrm_device *dev, uint64_t *fb_size);
int nvrm_device_get_vendor_id(struct nvrm_device *dev, uint16_t *vendor_id);
int nvrm_device_get_device_id(struct nvrm_device *dev, uint16_t *device_id);
int nvrm_device_get_gpc_mask(struct nvrm_device *dev, uint32_t *mask);
int nvrm_device_get_gpc_tp_mask(struct nvrm_device *dev, int gpc_id, uint32_t *mask);
int nvrm_device_get_total_tp_count(struct nvrm_device *dev, int *count);

struct nvrm_vspace *nvrm_vspace_create(struct nvrm_device *dev);
void nvrm_vspace_destroy(struct nvrm_vspace *vas);

struct nvrm_bo *nvrm_bo_create(struct nvrm_vspace *vas, uint64_t size, int sysram);
void nvrm_bo_destroy(struct nvrm_bo *bo);
void *nvrm_bo_host_map(struct nvrm_bo *bo);
uint64_t nvrm_bo_gpu_addr(struct nvrm_bo *bo);
void nvrm_bo_host_unmap(struct nvrm_bo *bo);

struct nvrm_channel *nvrm_channel_create_ib(struct nvrm_vspace *vas, uint32_t cls, struct nvrm_bo *ib);
int nvrm_channel_activate(struct nvrm_channel *chan);
void nvrm_channel_destroy(struct nvrm_channel *chan);
void *nvrm_channel_host_map_regs(struct nvrm_channel *chan);
void *nvrm_channel_host_map_errnot(struct nvrm_channel *chan);

struct nvrm_eng *nvrm_eng_create(struct nvrm_channel *chan, uint32_t eid, uint32_t cls);

/* handles */
uint32_t nvrm_handle_alloc(struct nvrm_context *ctx);
void nvrm_handle_free(struct nvrm_context *ctx, uint32_t handle);

/* ioctls */
int nvrm_ioctl_create_vspace(struct nvrm_device *dev, uint32_t parent, uint32_t handle, uint32_t cls, uint32_t flags, uint64_t *limit, uint64_t *foffset);
int nvrm_ioctl_create_dma(struct nvrm_context *ctx, uint32_t parent, uint32_t handle, uint32_t cls, uint32_t flags, uint64_t base, uint64_t limit);
int nvrm_ioctl_call(struct nvrm_context *ctx, uint32_t handle, uint32_t mthd, void *ptr, uint32_t size);
int nvrm_ioctl_create(struct nvrm_context *ctx, uint32_t parent, uint32_t handle, uint32_t cls, void *ptr);
int nvrm_ioctl_destroy(struct nvrm_context *ctx, uint32_t parent, uint32_t handle);
int nvrm_ioctl_unk4d(struct nvrm_context *ctx, uint32_t handle, const char *str);
int nvrm_ioctl_card_info(struct nvrm_context *ctx);
int nvrm_ioctl_get_fb_size(struct nvrm_context *ctx, int idx, uint64_t *size);
int nvrm_ioctl_get_vendor_id(struct nvrm_context *ctx, int idx, uint16_t *id);
int nvrm_ioctl_get_device_id(struct nvrm_context *ctx, int idx, uint16_t *id);
int nvrm_ioctl_env_info(struct nvrm_context *ctx, uint32_t *pat_supported);
int nvrm_ioctl_check_version_str(struct nvrm_context *ctx, uint32_t cmd, const char *vernum);
int nvrm_ioctl_memory(struct nvrm_context *ctx, uint32_t parent, uint32_t vspace, uint32_t handle, uint32_t flags1, uint32_t flags2, uint64_t base, uint64_t size);
int nvrm_ioctl_vspace_map(struct nvrm_context *ctx, uint32_t dev, uint32_t vspace, uint32_t handle, uint64_t base, uint64_t size, uint64_t *addr);
int nvrm_ioctl_vspace_unmap(struct nvrm_context *ctx, uint32_t dev, uint32_t vspace, uint32_t handle, uint64_t addr);
int nvrm_ioctl_host_map(struct nvrm_context *ctx, uint32_t subdev, uint32_t handle, uint64_t base, uint64_t size, uint64_t *foffset);
int nvrm_ioctl_host_unmap(struct nvrm_context *ctx, uint32_t subdev, uint32_t handle, uint64_t foffset);

/* mthds */
int nvrm_mthd_context_list_devices(struct nvrm_context *ctx, uint32_t handle, uint32_t *pciid);
int nvrm_mthd_context_enable_device(struct nvrm_context *ctx, uint32_t handle, uint32_t pciid);
int nvrm_mthd_context_disable_device(struct nvrm_context *ctx, uint32_t handle, uint32_t pciid);

int nvrm_create_cid(struct nvrm_context *ctx);
int nvrm_xlat_device(struct nvrm_context *ctx, int idx);



#endif
