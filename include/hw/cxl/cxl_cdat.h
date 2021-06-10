/*
 * CXL CDAT Structure
 *
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef CXL_CDAT_H
#define CXL_CDAT_H

#include "hw/cxl/cxl_pci.h"

/*
 * Reference:
 *   Coherent Device Attribute Table (CDAT) Specification, Rev. 1.02, Oct. 2020
 *   Compute Express Link (CXL) Specification, Rev. 2.0, Oct. 2020
 */
/* Table Access DOE - CXL 8.1.11 */
#define CXL_DOE_TABLE_ACCESS      2
#define CXL_DOE_PROTOCOL_CDAT     ((CXL_DOE_TABLE_ACCESS << 16) | CXL_VENDOR_ID)

/* Read Entry - CXL 8.1.11.1 */
#define CXL_DOE_TAB_TYPE_CDAT 0
#define CXL_DOE_TAB_ENT_MAX 0xFFFF

/* Read Entry Request - CXL 8.1.11.1 Table 134 */
#define CXL_DOE_TAB_REQ 0
struct cxl_cdat_req {
    DOEHeader header;
    uint8_t req_code;
    uint8_t table_type;
    uint16_t entry_handle;
} QEMU_PACKED;

/* Read Entry Response - CXL 8.1.11.1 Table 135 */
#define CXL_DOE_TAB_RSP 0
struct cxl_cdat_rsp {
    DOEHeader header;
    uint8_t rsp_code;
    uint8_t table_type;
    uint16_t entry_handle;
} QEMU_PACKED;

/* CDAT Table Format - CDAT Table 1 */
#define CXL_CDAT_REV 1
struct cdat_table_header {
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t reserved[6];
    uint32_t sequence;
} QEMU_PACKED;

/* CDAT Structure Types - CDAT Table 2 */
enum cdat_type {
    CDAT_TYPE_DSMAS = 0,
    CDAT_TYPE_DSLBIS = 1,
    CDAT_TYPE_DSMSCIS = 2,
    CDAT_TYPE_DSIS = 3,
    CDAT_TYPE_DSEMTS = 4,
    CDAT_TYPE_SSLBIS = 5,
};

struct cdat_sub_header {
    uint8_t type;
    uint8_t reserved;
    uint16_t length;
};

/* Device Scoped Memory Affinity Structure - CDAT Table 3 */
struct cdat_dsmas {
    struct cdat_sub_header header;
    uint8_t DSMADhandle;
    uint8_t flags;
    uint16_t reserved;
    uint64_t DPA_base;
    uint64_t DPA_length;
} QEMU_PACKED;

/* Device Scoped Latency and Bandwidth Information Structure - CDAT Table 5 */
struct cdat_dslbis {
    struct cdat_sub_header header;
    uint8_t handle;
    uint8_t flags;
    uint8_t data_type;
    uint8_t reserved;
    uint64_t entry_base_unit;
    uint16_t entry[3];
    uint16_t reserved2;
} QEMU_PACKED;

/* Device Scoped Memory Side Cache Information Structure - CDAT Table 6 */
struct cdat_dsmscis {
    struct cdat_sub_header header;
    uint8_t DSMAS_handle;
    uint8_t reserved[3];
    uint64_t memory_side_cache_size;
    uint32_t cache_attributes;
} QEMU_PACKED;

/* Device Scoped Initiator Structure - CDAT Table 7 */
struct cdat_dsis {
    struct cdat_sub_header header;
    uint8_t flags;
    uint8_t handle;
    uint16_t reserved;
} QEMU_PACKED;

/* Device Scoped EFI Memory Type Structure - CDAT Table 8 */
struct cdat_dsemts {
    struct cdat_sub_header header;
    uint8_t DSMAS_handle;
    uint8_t EFI_memory_type_attr;
    uint16_t reserved;
    uint64_t DPA_offset;
    uint64_t DPA_length;
} QEMU_PACKED;

/* Switch Scoped Latency and Bandwidth Information Structure - CDAT Table 9 */
struct cdat_sslbis_header {
    struct cdat_sub_header header;
    uint8_t data_type;
    uint8_t reserved[3];
    uint64_t entry_base_unit;
} QEMU_PACKED;

/* Switch Scoped Latency and Bandwidth Entry - CDAT Table 10 */
struct cdat_sslbe {
    uint16_t port_x_id;
    uint16_t port_y_id;
    uint16_t latency_bandwidth;
    uint16_t reserved;
} QEMU_PACKED;

typedef struct CDATEntry {
    void *base;
    uint32_t length;
} CDATEntry;

typedef struct CDATObject {
    CDATEntry *entry;
    int entry_len;

    void (*build_cdat_table)(void ***, int *);
    char *filename;
    char *buf;
} CDATObject;
#endif /* CXL_CDAT_H */
