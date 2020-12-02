/*
 * common header for iommu devices
 *
 * Copyright Red Hat, Inc. 2019
 *
 * Authors:
 *  Eric Auger <eric.auger@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef QEMU_HW_IOMMU_IOMMU_H
#define QEMU_HW_IOMMU_IOMMU_H
#ifdef __linux__
#include <linux/iommu.h>
#endif

typedef struct IOMMUConfig {
    union {
#ifdef __linux__
        struct iommu_pasid_table_config pasid_cfg;
        struct iommu_inv_pasid_info inv_pasid_info;
#endif
          };
} IOMMUConfig;

typedef struct IOMMUPageResponse {
    union {
#ifdef __linux__
        struct iommu_page_response resp;
#endif
          };
} IOMMUPageResponse;


#endif /* QEMU_HW_IOMMU_IOMMU_H */
