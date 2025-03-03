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

#define TYPE_ARM_SMMUV3_ACCEL   "arm-smmuv3-accel"
OBJECT_DECLARE_TYPE(SMMUv3AccelState, SMMUv3AccelClass, ARM_SMMUV3_ACCEL)

struct SMMUv3AccelState {
    SMMUv3State smmuv3_state;
};

struct SMMUv3AccelClass {
    /*< private >*/
    SMMUv3Class smmuv3_class;
    /*< public >*/

    DeviceRealize parent_realize;
};

#endif /* HW_ARM_SMMUV3_ACCEL_H */
