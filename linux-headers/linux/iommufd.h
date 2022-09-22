/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright (c) 2021-2022, NVIDIA CORPORATION & AFFILIATES.
 */
#ifndef _IOMMUFD_H
#define _IOMMUFD_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define IOMMUFD_TYPE (';')

/**
 * DOC: General ioctl format
 *
 * The ioctl mechanims follows a general format to allow for extensibility. Each
 * ioctl is passed in a structure pointer as the argument providing the size of
 * the structure in the first u32. The kernel checks that any structure space
 * beyond what it understands is 0. This allows userspace to use the backward
 * compatible portion while consistently using the newer, larger, structures.
 *
 * ioctls use a standard meaning for common errnos:
 *
 *  - ENOTTY: The IOCTL number itself is not supported at all
 *  - E2BIG: The IOCTL number is supported, but the provided structure has
 *    non-zero in a part the kernel does not understand.
 *  - EOPNOTSUPP: The IOCTL number is supported, and the structure is
 *    understood, however a known field has a value the kernel does not
 *    understand or support.
 *  - EINVAL: Everything about the IOCTL was understood, but a field is not
 *    correct.
 *  - ENOENT: An ID or IOVA provided does not exist.
 *  - ENOMEM: Out of memory.
 *  - EOVERFLOW: Mathematics oveflowed.
 *
 * As well as additional errnos. within specific ioctls.
 */
enum {
	IOMMUFD_CMD_BASE = 0x80,
	IOMMUFD_CMD_DESTROY = IOMMUFD_CMD_BASE,
	IOMMUFD_CMD_IOAS_ALLOC,
	IOMMUFD_CMD_IOAS_ALLOW_IOVAS,
	IOMMUFD_CMD_IOAS_COPY,
	IOMMUFD_CMD_IOAS_IOVA_RANGES,
	IOMMUFD_CMD_IOAS_MAP,
	IOMMUFD_CMD_IOAS_UNMAP,
	IOMMUFD_CMD_VFIO_IOAS,
	IOMMUFD_CMD_DEVICE_GET_INFO,
	IOMMUFD_CMD_HWPT_ALLOC,
	IOMMUFD_CMD_HWPT_INVALIDATE,
};

/**
 * struct iommu_destroy - ioctl(IOMMU_DESTROY)
 * @size: sizeof(struct iommu_destroy)
 * @id: iommufd object ID to destroy. Can by any destroyable object type.
 *
 * Destroy any object held within iommufd.
 */
struct iommu_destroy {
	__u32 size;
	__u32 id;
};
#define IOMMU_DESTROY _IO(IOMMUFD_TYPE, IOMMUFD_CMD_DESTROY)

/**
 * struct iommu_ioas_alloc - ioctl(IOMMU_IOAS_ALLOC)
 * @size: sizeof(struct iommu_ioas_alloc)
 * @flags: Must be 0
 * @out_ioas_id: Output IOAS ID for the allocated object
 *
 * Allocate an IO Address Space (IOAS) which holds an IO Virtual Address (IOVA)
 * to memory mapping.
 */
struct iommu_ioas_alloc {
	__u32 size;
	__u32 flags;
	__u32 out_ioas_id;
};
#define IOMMU_IOAS_ALLOC _IO(IOMMUFD_TYPE, IOMMUFD_CMD_IOAS_ALLOC)

/**
 * struct iommu_iova_range
 * @start: First IOVA
 * @last: Inclusive last IOVA
 *
 * An interval in IOVA space.
 */
struct iommu_iova_range {
	__aligned_u64 start;
	__aligned_u64 last;
};

/**
 * struct iommu_ioas_iova_ranges - ioctl(IOMMU_IOAS_IOVA_RANGES)
 * @size: sizeof(struct iommu_ioas_iova_ranges)
 * @ioas_id: IOAS ID to read ranges from
 * @out_num_iovas: Output total number of ranges in the IOAS
 * @__reserved: Must be 0
 * @out_valid_iovas: Array of valid IOVA ranges. The array length is the smaller
 *                   of out_num_iovas or the length implied by size.
 * @out_valid_iovas.start: First IOVA in the allowed range
 * @out_valid_iovas.last: Inclusive last IOVA in the allowed range
 *
 * Query an IOAS for ranges of allowed IOVAs. Mapping IOVA outside these ranges
 * is not allowed. out_num_iovas will be set to the total number of iovas and
 * the out_valid_iovas[] will be filled in as space permits. size should include
 * the allocated flex array.
 *
 * The allowed ranges are dependent on the HW path the DMA operation takes, and
 * can change during the lifetime of the IOAS. A fresh empty IOAS will have a
 * full range, and each attached device will narrow the ranges based on that
 * devices HW restrictions. Detatching a device can widen the ranges. Userspace
 * should query ranges after every attach/detatch to know what IOVAs are valid
 * for mapping.
 */
struct iommu_ioas_iova_ranges {
	__u32 size;
	__u32 ioas_id;
	__u32 out_num_iovas;
	__u32 __reserved;
	struct iommu_valid_iovas {
		__aligned_u64 start;
		__aligned_u64 last;
	} out_valid_iovas[];
};
#define IOMMU_IOAS_IOVA_RANGES _IO(IOMMUFD_TYPE, IOMMUFD_CMD_IOAS_IOVA_RANGES)

/**
 * struct iommu_ioas_allow_iovas - ioctl(IOMMU_IOAS_ALLOW_IOVAS)
 * @size: sizeof(struct iommu_ioas_allow_iovas)
 * @ioas_id: IOAS ID to allow IOVAs from
 * @allowed_iovas: Pointer to array of struct iommu_iova_range
 *
 * Ensure a range of IOVAs are always available for allocation. If this call
 * succeeds then IOMMU_IOAS_IOVA_RANGES will never return a list of IOVA ranges
 * that are narrower than the ranges provided here. This call will fail if
 * IOMMU_IOAS_IOVA_RANGES is currently narrower than the given ranges.
 *
 * When an IOAS is first created the IOVA_RANGES will be maximally sized, and as
 * devices are attached the IOVA will narrow based on the device restrictions.
 * When an allowed range is specified any narrowing will be refused, ie device
 * attachment can fail if the device requires limiting within the allowed range.
 *
 * Automatic IOVA allocation is also impacted by this call, it MAP will allocate
 * within the allowed IOVAs if they are present.
 *
 * This call replaces the entire allowed list with the given list.
 */
struct iommu_ioas_allow_iovas {
	__u32 size;
	__u32 ioas_id;
	__u32 num_iovas;
	__u32 __reserved;
	__aligned_u64 allowed_iovas;
};
#define IOMMU_IOAS_ALLOW_IOVAS _IO(IOMMUFD_TYPE, IOMMUFD_CMD_IOAS_ALLOW_IOVAS)

/**
 * enum iommufd_ioas_map_flags - Flags for map and copy
 * @IOMMU_IOAS_MAP_FIXED_IOVA: If clear the kernel will compute an appropriate
 *                             IOVA to place the mapping at
 * @IOMMU_IOAS_MAP_WRITEABLE: DMA is allowed to write to this mapping
 * @IOMMU_IOAS_MAP_READABLE: DMA is allowed to read from this mapping
 */
enum iommufd_ioas_map_flags {
	IOMMU_IOAS_MAP_FIXED_IOVA = 1 << 0,
	IOMMU_IOAS_MAP_WRITEABLE = 1 << 1,
	IOMMU_IOAS_MAP_READABLE = 1 << 2,
};

/**
 * struct iommu_ioas_map - ioctl(IOMMU_IOAS_MAP)
 * @size: sizeof(struct iommu_ioas_map)
 * @flags: Combination of enum iommufd_ioas_map_flags
 * @ioas_id: IOAS ID to change the mapping of
 * @__reserved: Must be 0
 * @user_va: Userspace pointer to start mapping from
 * @length: Number of bytes to map
 * @iova: IOVA the mapping was placed at. If IOMMU_IOAS_MAP_FIXED_IOVA is set
 *        then this must be provided as input.
 *
 * Set an IOVA mapping from a user pointer. If FIXED_IOVA is specified then the
 * mapping will be established at iova, otherwise a suitable location will be
 * automatically selected and returned in iova.
 */
struct iommu_ioas_map {
	__u32 size;
	__u32 flags;
	__u32 ioas_id;
	__u32 __reserved;
	__aligned_u64 user_va;
	__aligned_u64 length;
	__aligned_u64 iova;
};
#define IOMMU_IOAS_MAP _IO(IOMMUFD_TYPE, IOMMUFD_CMD_IOAS_MAP)

/**
 * struct iommu_ioas_copy - ioctl(IOMMU_IOAS_COPY)
 * @size: sizeof(struct iommu_ioas_copy)
 * @flags: Combination of enum iommufd_ioas_map_flags
 * @dst_ioas_id: IOAS ID to change the mapping of
 * @src_ioas_id: IOAS ID to copy from
 * @length: Number of bytes to copy and map
 * @dst_iova: IOVA the mapping was placed at. If IOMMU_IOAS_MAP_FIXED_IOVA is
 *            set then this must be provided as input.
 * @src_iova: IOVA to start the copy
 *
 * Copy an already existing mapping from src_ioas_id and establish it in
 * dst_ioas_id. The src iova/length must exactly match a range used with
 * IOMMU_IOAS_MAP.
 *
 * This may be used to efficiently clone a subset of an IOAS to another, or as a
 * kind of 'cache' to speed up mapping. Copy has an effciency advantage over
 * establishing equivilant new mappings, as internal resources are shared, and
 * the kernel will pin the user memory only once.
 */
struct iommu_ioas_copy {
	__u32 size;
	__u32 flags;
	__u32 dst_ioas_id;
	__u32 src_ioas_id;
	__aligned_u64 length;
	__aligned_u64 dst_iova;
	__aligned_u64 src_iova;
};
#define IOMMU_IOAS_COPY _IO(IOMMUFD_TYPE, IOMMUFD_CMD_IOAS_COPY)

/**
 * struct iommu_ioas_unmap - ioctl(IOMMU_IOAS_UNMAP)
 * @size: sizeof(struct iommu_ioas_copy)
 * @ioas_id: IOAS ID to change the mapping of
 * @iova: IOVA to start the unmapping at
 * @length: Number of bytes to unmap, and return back the bytes unmapped
 *
 * Unmap an IOVA range. The iova/length must be a superset of a previously
 * mapped range used with IOMMU_IOAS_PAGETABLE_MAP or COPY. Splitting or
 * truncating ranges is not allowed. The values 0 to U64_MAX will unmap
 * everything.
 */
struct iommu_ioas_unmap {
	__u32 size;
	__u32 ioas_id;
	__aligned_u64 iova;
	__aligned_u64 length;
};
#define IOMMU_IOAS_UNMAP _IO(IOMMUFD_TYPE, IOMMUFD_CMD_IOAS_UNMAP)

/**
 * enum iommufd_vfio_ioas_op
 * @IOMMU_VFIO_IOAS_GET: Get the current compatibility IOAS
 * @IOMMU_VFIO_IOAS_SET: Change the current compatibility IOAS
 * @IOMMU_VFIO_IOAS_CLEAR: Disable VFIO compatibility
 */
enum iommufd_vfio_ioas_op {
	IOMMU_VFIO_IOAS_GET = 0,
	IOMMU_VFIO_IOAS_SET = 1,
	IOMMU_VFIO_IOAS_CLEAR = 2,
};

/**
 * struct iommu_vfio_ioas - ioctl(IOMMU_VFIO_IOAS)
 * @size: sizeof(struct iommu_ioas_copy)
 * @ioas_id: For IOMMU_VFIO_IOAS_SET the input IOAS ID to set
 *           For IOMMU_VFIO_IOAS_GET will output the IOAS ID
 * @op: One of enum iommufd_vfio_ioas_op
 * @__reserved: Must be 0
 *
 * The VFIO compatibility support uses a single ioas because VFIO APIs do not
 * support the ID field. Set or Get the IOAS that VFIO compatibility will use.
 * When VFIO_GROUP_SET_CONTAINER is used on an iommufd it will get the
 * compatibility ioas, either by taking what is already set, or auto creating
 * one. From then on VFIO will continue to use that ioas and is not effected by
 * this ioctl. SET or CLEAR does not destroy any auto-created IOAS.
 */
struct iommu_vfio_ioas {
	__u32 size;
	__u32 ioas_id;
	__u16 op;
	__u16 __reserved;
};
#define IOMMU_VFIO_IOAS _IO(IOMMUFD_TYPE, IOMMUFD_CMD_VFIO_IOAS)

enum iommu_device_data_type {
	IOMMU_DEVICE_DATA_NONE = 0,
	IOMMU_DEVICE_DATA_INTEL_VTD,
	IOMMU_DEVICE_DATA_ARM_SMMUV3,
};

/**
 * struct iommu_device_info_vtd - Intel VT-d device info
 *
 * @flags: Must be set to 0
 * @__reserved: Must be 0
 * @cap_reg: Basic capability register value
 * @ecap_reg: Extended capability register value
  */
struct iommu_device_info_vtd {
	__u32 flags;
	__u32 __reserved;
	__aligned_u64 cap_reg;
	__aligned_u64 ecap_reg;
};

/**
 * struct iommu_device_info_smmuv3 - ARM SMMUv3 device info
 *
 * @flags: Must be set to 0
 * @__reserved: Must be 0
 * @idr_regs: Information
 * @idr5: Extended capability register value
 */
struct iommu_device_info_smmuv3 {
	__u32 flags;
	__u32 __reserved;
	__u32 idr[6];
};

/**
 * struct iommu_device_info - ioctl(IOMMU_DEVICE_GET_INFO)
 * @size: sizeof(struct iommu_device_info)
 * @flags: Must be 0
 * @dev_id: the device being attached to the IOMMU
 * @__reserved: Must be 0
 * @out_data_type: type of the output data, i.e. enum iommu_device_data_type
 * @out_data_len: legnth of the type specific data
 * @out_data_ptr: pointer to the type specific data
 */
struct iommu_device_info {
	__u32 size;
	__u32 flags;
	__u32 dev_id;
	__u32 __reserved;
	__u32 out_device_type;
	__u32 out_data_len;
	__aligned_u64 out_data_ptr;
};
#define IOMMU_DEVICE_GET_INFO _IO(IOMMUFD_TYPE, IOMMUFD_CMD_DEVICE_GET_INFO)

/**
 * struct iommu_hwpt_intel_vtd - Intel VT-d specific page table data
 *
 * @flags: VT-d page table entry attributes
 * @s1_pgtbl: the stage1 (a.k.a user managed page table) pointer.
 *            This pointer should be subjected to stage2 translation
 * @pat: Page attribute table data to compute effective memory type
 * @emt: Extended memory type
 * @addr_width: the input address width of VT-d page table
 * @__reserved: Must be 0
 */
struct iommu_hwpt_intel_vtd {
#define IOMMU_VTD_PGTBL_SRE	(1 << 0) /* supervisor request */
#define IOMMU_VTD_PGTBL_EAFE	(1 << 1) /* extended access enable */
#define IOMMU_VTD_PGTBL_PCD	(1 << 2) /* page-level cache disable */
#define IOMMU_VTD_PGTBL_PWT	(1 << 3) /* page-level write through */
#define IOMMU_VTD_PGTBL_EMTE	(1 << 4) /* extended mem type enable */
#define IOMMU_VTD_PGTBL_CD	(1 << 5) /* PASID-level cache disable */
#define IOMMU_VTD_PGTBL_WPE	(1 << 6) /* Write protect enable */
#define IOMMU_VTD_PGTBL_LAST	(1 << 7)
	__u64 flags;
	__u64 s1_pgtbl;
	__u32 pat;
	__u32 emt;
	__u32 addr_width;
	__u32 __reserved;
};

/**
 * struct iommu_hwpt_arm_smmuv3 - ARM SMMUv3 specific page table data
 *
 * @flags: page table entry attributes
 * @config: Stream configuration
 * @s2vmid: Virtual machine identifier
 * @s1ctxptr: Stage 1 context descriptor pointer
 * @s1cdmax: Number of CDs pointed to by s1ContextPtr
 * @s1fmt: Stage 1 Format
 * @s1dss: Default substream
 */
struct iommu_hwpt_arm_smmuv3 {
#define IOMMU_SMMUV3_FLAG_S2	(1 << 0) /* if unset, stage1 */
#define IOMMU_SMMUV3_FLAG_VMID	(1 << 1) /* vmid override */
	__u64 flags;
#define IOMMU_SMMUV3_CONFIG_TRANSLATE	1
#define IOMMU_SMMUV3_CONFIG_BYPASS	2
#define IOMMU_SMMUV3_CONFIG_ABORT	3
	__u32 config;
	__u32 s2vmid;
	__u64 s1ctxptr;
	__u64 s1cdmax;
	__u64 s1fmt;
	__u64 s1dss;
};

/**
 * struct iommu_hwpt_alloc - ioctl(IOMMU_HWPT_ALLOC)
 * @size: sizeof(struct iommu_hwpt_alloc)
 * @flags: Must be 0
 * @dev_id: the device to allocate this HWPT for
 * @pt_id: the parent of this HWPT (IOAS or HWPT).
 * @data_type: type of the user data, i.e. enum iommu_device_data_type
 * @data_len: legnth of the type specific data
 * @data_uptr: user pointer to the type specific data
 * @out_hwpt_id: output HWPT ID for the allocated object
 * @__reserved: Must be 0
 *
 * Allocate a hardware page table for userspace
 */
struct iommu_hwpt_alloc {
	__u32 size;
	__u32 flags;
	__u32 dev_id;
	__u32 pt_id;
	__u32 data_type;
	__u32 data_len;
	__aligned_u64 data_uptr;
	__u32 out_hwpt_id;
	__u32 __reserved;
};
#define IOMMU_HWPT_ALLOC _IO(IOMMUFD_TYPE, IOMMUFD_CMD_HWPT_ALLOC)

/* Intel VT-d specific granularity of queued invalidation */
enum iommu_vtd_qi_granularity {
	IOMMU_VTD_QI_GRAN_DOMAIN,	/* domain-selective invalidation */
	IOMMU_VTD_QI_GRAN_PASID,	/* PASID-selective invalidation */
	IOMMU_VTD_QI_GRAN_ADDR,		/* page-selective invalidation */
	IOMMU_VTD_QI_GRAN_NR,		/* number of invalidation granularities */
};

/**
 * struct iommu_hwpt_invalidate_intel_vtd - Intel VT-d cache invalidation info
 * @version: queued invalidation info version
 * @cache: bitfield that allows to select which caches to invalidate
 * @granularity: defines the lowest granularity used for the invalidation:
 *               domain > PASID > addr
 * @flags: indicates the granularity of invalidation
 *         - If the PASID bit is set, the @pasid field is populated and the
 *           invalidation relates to cache entries tagged with this PASID and
 *           matching the address range. If unset, global invalidation applies.
 *         - The LEAF flag indicates whether only the leaf PTE caching needs to
 *           be invalidated and other paging structure caches can be preserved.
 * @pasid: process address space ID
 * @addr: first stage/level input address
 * @granule_size: page/block size of the mapping in bytes
 * @nb_granules: number of contiguous granules to be invalidated
 *
 * Notes:
 *  - Valid combinations of cache/granularity are
 *    +--------------+---------------+---------------+---------------+
 *    | type /       |   DEV_IOTLB   |     IOTLB     |     PASID     |
 *    | granularity  |               |               |     cache     |
 *    +==============+===============+===============+===============+
 *    | DOMAIN       |      N/A      |       Y       |       Y       |
 *    +--------------+---------------+---------------+---------------+
 *    | PASID        |       Y       |       Y       |       Y       |
 *    +--------------+---------------+---------------+---------------+
 *    | ADDR         |       Y       |       Y       |      N/A      |
 *    +--------------+---------------+---------------+---------------+
 *  - Multiple caches can be selected, if they all support the used @granularity
 *  - IOMMU_VTD_QI_GRAN_DOMAIN only uses @version, @cache and @granularity
 *  - IOMMU_VTD_QI_GRAN_PASID uses @flags and @pasid additionally
 *  - IOMMU_VTD_QI_GRAN_ADDR uses all the info
 */
struct iommu_hwpt_invalidate_intel_vtd {
#define IOMMU_VTD_QI_INFO_VERSION_1 1
	__u32 version;
/* IOMMU paging structure cache type */
#define IOMMU_VTD_QI_TYPE_IOTLB		(1 << 0) /* IOMMU IOTLB */
#define IOMMU_VTD_QI_TYPE_DEV_IOTLB	(1 << 1) /* Device IOTLB */
#define IOMMU_VTD_QI_TYPE_PASID		(1 << 2) /* PASID cache */
#define IOMMU_VTD_QI_TYPE_NR		(3)
	__u8 cache;
	__u8 granularity;
	__u8 padding[6];
#define IOMMU_VTD_QI_FLAGS_PASID	(1 << 0)
#define IOMMU_VTD_QI_FLAGS_LEAF		(1 << 1)
	__u32 flags;
	__u64 pasid;
	__u64 addr;
	__u64 granule_size;
	__u64 nb_granules;
};

/**
 * struct iommu_hwpt_invalidate_arm_smmuv3 - ARM SMMUv3 cahce invalidation info
 * @flags: boolean attributes of cache invalidation command
 * @opcode: opcode of cache invalidation command
 * @ssid: SubStream ID
 * @granule_size: page/block size of the mapping in bytes
 * @range: IOVA range to invalidate
 */
struct iommu_hwpt_invalidate_arm_smmuv3 {
#define IOMMU_SMMUV3_CMDQ_TLBI_VA_LEAF	(1 << 0)
	__u64 flags;
	__u8 opcode;
	__u8 padding[3];
	__u32 asid;
	__u32 ssid;
	__u32 granule_size;
	struct iommu_iova_range range;
};

/**
 * struct iommu_hwpt_invalidate - ioctl(IOMMU_HWPT_INVALIDATE)
 * @size: sizeof(struct iommu_hwpt_invalidate)
 * @hwpt_id: HWPT ID of target hardware page table for the invalidation
 * @data_type: type of the user data, i.e. enum iommu_device_data_type
 * @data_len: legnth of the type specific data
 * @data_uptr: user pointer to the type specific data
 */
struct iommu_hwpt_invalidate {
	__u32 size;
	__u32 hwpt_id;
	__u32 data_type;
	__u32 data_len;
	__aligned_u64 data_uptr;
};
#define IOMMU_HWPT_INVALIDATE _IO(IOMMUFD_TYPE, IOMMUFD_CMD_HWPT_INVALIDATE)
#endif
