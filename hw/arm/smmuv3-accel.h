/*
 * Copyright (c) 2025 Huawei Technologies R & D (UK) Ltd
 * Copyright (C) 2025 NVIDIA
 * Written by Nicolin Chen, Shameer Kolothum
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_ARM_SMMUV3_ACCEL_H
#define HW_ARM_SMMUV3_ACCEL_H

#include "hw/arm/smmuv3.h"
#include "hw/arm/smmu-common.h"
#include CONFIG_DEVICES

typedef struct SMMUv3AccelDevice {
    SMMUDevice  sdev;
    AddressSpace as_sysmem;
} SMMUv3AccelDevice;

typedef struct SMMUv3AccelState {
    MemoryRegion root;
    MemoryRegion sysmem;
} SMMUv3AccelState;

#if defined(CONFIG_ARM_SMMUV3) && defined(CONFIG_IOMMUFD)
void smmuv3_accel_init(SMMUv3State *s);
#else
static inline void smmuv3_accel_init(SMMUv3State *d)
{
}
#endif

#endif /* HW_ARM_SMMUV3_ACCEL_H */
