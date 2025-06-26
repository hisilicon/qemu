/*
 * Copyright (c) 2025 Huawei Technologies R & D (UK) Ltd
 * Copyright (C) 2025 NVIDIA
 * Written by Nicolin Chen, Shameer Kolothum
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"

#include "hw/arm/smmuv3.h"
#include "smmuv3-accel.h"

static SMMUv3AccelDevice *smmuv3_accel_get_dev(SMMUState *bs, SMMUPciBus *sbus,
                                                PCIBus *bus, int devfn)
{
    SMMUDevice *sdev = sbus->pbdev[devfn];
    SMMUv3AccelDevice *accel_dev;

    if (sdev) {
        accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
    } else {
        accel_dev = g_new0(SMMUv3AccelDevice, 1);
        sdev = &accel_dev->sdev;

        sbus->pbdev[devfn] = sdev;
        smmu_init_sdev(bs, sdev, bus, devfn);
    }

    return accel_dev;
}

static AddressSpace *smmuv3_accel_find_add_as(PCIBus *bus, void *opaque,
                                              int devfn)
{
    SMMUState *bs = opaque;
    SMMUPciBus *sbus;
    SMMUv3AccelDevice *accel_dev;
    SMMUDevice *sdev;

    sbus = smmu_get_sbus(bs, bus);
    accel_dev = smmuv3_accel_get_dev(bs, sbus, bus, devfn);
    sdev = &accel_dev->sdev;

    return &sdev->as;
}

static const PCIIOMMUOps smmuv3_accel_ops = {
    .get_address_space = smmuv3_accel_find_add_as,
};

static void smmuv3_accel_class_init(ObjectClass *oc, const void *data)
{
    SMMUBaseClass *sbc = ARM_SMMU_CLASS(oc);

    sbc->iommu_ops = &smmuv3_accel_ops;
}

static const TypeInfo types[] = {
    {
        .name = TYPE_ARM_SMMUV3_ACCEL,
        .parent = TYPE_ARM_SMMUV3,
        .class_init = smmuv3_accel_class_init,
    }
};
DEFINE_TYPES(types)
