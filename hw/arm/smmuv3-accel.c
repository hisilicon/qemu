/*
 * Copyright (c) 2025 Huawei Technologies R & D (UK) Ltd
 * Copyright (C) 2025 NVIDIA
 * Written by Nicolin Chen, Shameer Kolothum
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "trace.h"
#include "qemu/error-report.h"

#include "hw/arm/smmuv3.h"
#include "hw/iommu.h"
#include "hw/pci/pci_bridge.h"
#include "hw/pci-host/gpex.h"
#include "hw/vfio/pci.h"

#include "smmuv3-accel.h"

#define SMMU_STE_VALID      (1ULL << 0)
#define SMMU_STE_CFG_BYPASS (1ULL << 3)

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

static bool
smmuv3_accel_dev_alloc_viommu(SMMUv3AccelDevice *accel_dev,
                               HostIOMMUDeviceIOMMUFD *idev, Error **errp)
{
    struct iommu_hwpt_arm_smmuv3 bypass_data = {
        .ste = { SMMU_STE_CFG_BYPASS | SMMU_STE_VALID, 0x0ULL },
    };
    struct iommu_hwpt_arm_smmuv3 abort_data = {
        .ste = { SMMU_STE_VALID, 0x0ULL },
    };
    SMMUDevice *sdev = &accel_dev->sdev;
    SMMUState *bs = sdev->smmu;
    SMMUv3State *s = ARM_SMMUV3(bs);
    SMMUv3AccelState *s_accel = s->s_accel;
    uint32_t s2_hwpt_id = idev->hwpt_id;
    SMMUS2Hwpt *s2_hwpt;
    SMMUViommu *viommu;
    uint32_t viommu_id;

    if (s_accel->viommu) {
        accel_dev->viommu = s_accel->viommu;
        return true;
    }

    if (!iommufd_backend_alloc_viommu(idev->iommufd, idev->devid,
                                      IOMMU_VIOMMU_TYPE_ARM_SMMUV3,
                                      s2_hwpt_id, &viommu_id, errp)) {
        return false;
    }

    viommu = g_new0(SMMUViommu, 1);
    viommu->core.viommu_id = viommu_id;
    viommu->core.s2_hwpt_id = s2_hwpt_id;
    viommu->core.iommufd = idev->iommufd;

    if (!iommufd_backend_alloc_hwpt(idev->iommufd, idev->devid,
                                    viommu->core.viommu_id, 0,
                                    IOMMU_HWPT_DATA_ARM_SMMUV3,
                                    sizeof(abort_data), &abort_data,
                                    &viommu->abort_hwpt_id, errp)) {
        goto free_viommu;
    }

    if (!iommufd_backend_alloc_hwpt(idev->iommufd, idev->devid,
                                    viommu->core.viommu_id, 0,
                                    IOMMU_HWPT_DATA_ARM_SMMUV3,
                                    sizeof(bypass_data), &bypass_data,
                                    &viommu->bypass_hwpt_id, errp)) {
        goto free_abort_hwpt;
    }

    s2_hwpt = g_new(SMMUS2Hwpt, 1);
    s2_hwpt->iommufd = idev->iommufd;
    s2_hwpt->hwpt_id = s2_hwpt_id;

    viommu->iommufd = idev->iommufd;
    viommu->s2_hwpt = s2_hwpt;

    s_accel->viommu = viommu;
    accel_dev->viommu = viommu;
    return true;

free_abort_hwpt:
    iommufd_backend_free_id(idev->iommufd, viommu->abort_hwpt_id);
free_viommu:
    iommufd_backend_free_id(idev->iommufd, viommu->core.viommu_id);
    g_free(viommu);
    return false;
}

static bool smmuv3_accel_set_iommu_device(PCIBus *bus, void *opaque, int devfn,
                                          HostIOMMUDevice *hiod, Error **errp)
{
    HostIOMMUDeviceIOMMUFD *idev = HOST_IOMMU_DEVICE_IOMMUFD(hiod);
    SMMUState *bs = opaque;
    SMMUv3State *s = ARM_SMMUV3(bs);
    SMMUv3AccelState *s_accel = s->s_accel;
    SMMUPciBus *sbus = smmu_get_sbus(bs, bus);
    SMMUv3AccelDevice *accel_dev = smmuv3_accel_get_dev(bs, sbus, bus, devfn);
    SMMUDevice *sdev = &accel_dev->sdev;

    if (!idev) {
        return true;
    }

    if (accel_dev->idev) {
        if (accel_dev->idev != idev) {
            error_report("Device 0x%x already has an associated idev",
                         smmu_get_sid(sdev));
            return false;
        } else {
            return true;
        }
    }

    if (!smmuv3_accel_dev_alloc_viommu(accel_dev, idev, errp)) {
        error_report("Device 0x%x: Unable to alloc viommu", smmu_get_sid(sdev));
        return false;
    }

    accel_dev->idev = idev;
    QLIST_INSERT_HEAD(&s_accel->viommu->device_list, accel_dev, next);
    trace_smmuv3_accel_set_iommu_device(devfn, smmu_get_sid(sdev));
    return true;
}

static void smmuv3_accel_unset_iommu_device(PCIBus *bus, void *opaque,
                                            int devfn)
{
    SMMUState *bs = opaque;
    SMMUv3State *s = ARM_SMMUV3(bs);
    SMMUPciBus *sbus = g_hash_table_lookup(bs->smmu_pcibus_by_busptr, bus);
    SMMUv3AccelDevice *accel_dev;
    SMMUViommu *viommu;
    SMMUDevice *sdev;

    if (!sbus) {
        return;
    }

    sdev = sbus->pbdev[devfn];
    if (!sdev) {
        return;
    }

    accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
    if (!host_iommu_device_iommufd_attach_hwpt(accel_dev->idev,
                                               accel_dev->idev->hwpt_id,
                                               NULL)) {
        error_report("Unable to attach dev to the default HW pagetable");
    }

    accel_dev->idev = NULL;
    QLIST_REMOVE(accel_dev, next);
    trace_smmuv3_accel_unset_iommu_device(devfn, smmu_get_sid(sdev));

    viommu = s->s_accel->viommu;
    if (QLIST_EMPTY(&viommu->device_list)) {
        iommufd_backend_free_id(viommu->iommufd, viommu->bypass_hwpt_id);
        iommufd_backend_free_id(viommu->iommufd, viommu->abort_hwpt_id);
        iommufd_backend_free_id(viommu->iommufd, viommu->core.viommu_id);
        iommufd_backend_free_id(viommu->iommufd, viommu->s2_hwpt->hwpt_id);
        g_free(viommu->s2_hwpt);
        g_free(viommu);
        s->s_accel->viommu = NULL;
    }
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
    .set_iommu_device = smmuv3_accel_set_iommu_device,
    .unset_iommu_device = smmuv3_accel_unset_iommu_device,
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
