/*
 * Copyright (c) 2025 Huawei Technologies R & D (UK) Ltd
 * Copyright (C) 2025 NVIDIA
 * Written by Nicolin Chen, Shameer Kolothum
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_ARM_SMMUV3_ACCEL_H
#define HW_ARM_SMMUV3_ACCEL_H

#include "hw/arm/smmu-common.h"
#include "hw/arm/smmuv3.h"
#include "qom/object.h"
#include "system/iommufd.h"

#include <linux/iommufd.h>

#define TYPE_ARM_SMMUV3_ACCEL   "arm-smmuv3-accel"
OBJECT_DECLARE_TYPE(SMMUv3AccelState, SMMUv3AccelClass, ARM_SMMUV3_ACCEL)

typedef struct SMMUS2Hwpt {
    IOMMUFDBackend *iommufd;
    uint32_t hwpt_id;
    uint32_t ioas_id;
} SMMUS2Hwpt;

typedef struct SMMUViommu {
    IOMMUFDBackend *iommufd;
    IOMMUFDViommu core;
    SMMUS2Hwpt *s2_hwpt;
    uint32_t bypass_hwpt_id;
    uint32_t abort_hwpt_id;
    QLIST_HEAD(, SMMUv3AccelDevice) device_list;
    QLIST_ENTRY(SMMUViommu) next;
} SMMUViommu;

typedef struct SMMUv3AccelDevice {
    SMMUDevice  sdev;
    HostIOMMUDeviceIOMMUFD *idev;
    SMMUViommu *viommu;
    QLIST_ENTRY(SMMUv3AccelDevice) next;
} SMMUv3AccelDevice;

struct SMMUv3AccelState {
    SMMUv3State smmuv3_state;
    SMMUViommu *viommu;
};

struct SMMUv3AccelClass {
    /*< private >*/
    SMMUv3Class smmuv3_class;
    /*< public >*/

    DeviceRealize parent_realize;
};

#endif /* HW_ARM_SMMUV3_ACCEL_H */
