/*
 * Copyright (c) 2025 Huawei Technologies R & D (UK) Ltd
 * Copyright (C) 2025 NVIDIA
 * Written by Nicolin Chen, Shameer Kolothum
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"

#include "hw/arm/smmuv3.h"
#include "hw/iommu.h"
#include "hw/pci/pci_bridge.h"
#include "hw/pci-host/gpex.h"
#include "hw/vfio/pci.h"

#include "smmuv3-accel.h"

static SMMUv3AccelDevice *smmuv3_accel_get_dev(SMMUState *bs, SMMUPciBus *sbus,
                                                PCIBus *bus, int devfn)
{
    SMMUv3State *s = ARM_SMMUV3(bs);
    SMMUDevice *sdev = sbus->pbdev[devfn];
    SMMUv3AccelDevice *accel_dev;

    if (sdev) {
        accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
    } else {
        accel_dev = g_new0(SMMUv3AccelDevice, 1);
        sdev = &accel_dev->sdev;

        sbus->pbdev[devfn] = sdev;
        smmu_init_sdev(bs, sdev, bus, devfn);
        address_space_init(&accel_dev->as_sysmem, &s->s_accel->root,
                           "smmuv3-accel-sysmem");
    }

    return accel_dev;
}

static bool smmuv3_accel_pdev_allowed(PCIDevice *pdev, bool *vfio_pci)
{

    if (object_dynamic_cast(OBJECT(pdev), TYPE_PCI_BRIDGE) ||
        object_dynamic_cast(OBJECT(pdev), "pxb-pcie") ||
        object_dynamic_cast(OBJECT(pdev), "gpex-root")) {
        return true;
    } else if ((object_dynamic_cast(OBJECT(pdev), TYPE_VFIO_PCI) &&
        object_property_find(OBJECT(pdev), "iommufd"))) {
        *vfio_pci = true;
        return true;
    }
    return false;
}

static AddressSpace *smmuv3_accel_find_add_as(PCIBus *bus, void *opaque,
                                              int devfn)
{
    PCIDevice *pdev = pci_find_device(bus, pci_bus_num(bus), devfn);
    SMMUState *bs = opaque;
    bool vfio_pci = false;
    SMMUPciBus *sbus;
    SMMUv3AccelDevice *accel_dev;
    SMMUDevice *sdev;

    if (pdev && !smmuv3_accel_pdev_allowed(pdev, &vfio_pci)) {
        error_report("Device(%s) not allowed. Only PCIe root complex devices "
                     "or PCI bridge devices or vfio-pci endpoint devices with "
                     "iommufd as backend is allowed with arm-smmuv3,accel=on",
                     pdev->name);
        exit(1);
    }
    sbus = smmu_get_sbus(bs, bus);
    accel_dev = smmuv3_accel_get_dev(bs, sbus, bus, devfn);
    sdev = &accel_dev->sdev;

    if (vfio_pci) {
        return &accel_dev->as_sysmem;
    } else {
        return &sdev->as;
    }
}

static uint64_t smmuv3_accel_get_viommu_cap(void *opaque)
{
    /*
     * Accelerated smmuv3 support only allowes Guest S1
     * configuration. Hence report VIOMMU_CAP_STAGE1
     * so that VFIO can create nested parent domain.
     * The real nested support should be reported from host
     * SMMUv3 and if it doesn't, the nested parent allocation
     * will fail anyway.
     */
    return VIOMMU_CAP_STAGE1;
}

static const PCIIOMMUOps smmuv3_accel_ops = {
    .get_address_space = smmuv3_accel_find_add_as,
    .get_viommu_cap = smmuv3_accel_get_viommu_cap,
};

void smmuv3_accel_init(SMMUv3State *s)
{
    SMMUv3AccelState *s_accel;

    s->s_accel = s_accel = g_new0(SMMUv3AccelState, 1);
    memory_region_init(&s_accel->root, OBJECT(s), "root", UINT64_MAX);
    memory_region_init_alias(&s_accel->sysmem, OBJECT(s),
                             "smmuv3-accel-sysmem", get_system_memory(), 0,
                             memory_region_size(get_system_memory()));
    memory_region_add_subregion(&s_accel->root, 0, &s_accel->sysmem);
}

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
