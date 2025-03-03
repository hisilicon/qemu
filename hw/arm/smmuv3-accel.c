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

#include "hw/arm/smmuv3-accel.h"
#include "hw/pci/pci_bridge.h"

static SMMUv3AccelDevice *smmuv3_accel_get_dev(SMMUState *s, SMMUPciBus *sbus,
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
        smmu_init_sdev(s, sdev, bus, devfn);
    }

    return accel_dev;
}

static bool
smmuv3_accel_dev_attach_viommu(SMMUv3AccelDevice *accel_dev,
                               HostIOMMUDeviceIOMMUFD *idev, Error **errp)
{
    struct iommu_hwpt_arm_smmuv3 bypass_data = {
        .ste = { 0x9ULL, 0x0ULL },
    };
    struct iommu_hwpt_arm_smmuv3 abort_data = {
        .ste = { 0x1ULL, 0x0ULL },
    };
    SMMUDevice *sdev = &accel_dev->sdev;
    SMMUState *s = sdev->smmu;
    SMMUv3AccelState *s_accel = ARM_SMMUV3_ACCEL(s);
    SMMUS2Hwpt *s2_hwpt;
    SMMUViommu *viommu;
    uint32_t s2_hwpt_id;
    uint32_t viommu_id;

    if (s_accel->viommu) {
        accel_dev->viommu = s_accel->viommu;
        return host_iommu_device_iommufd_attach_hwpt(
                       idev, s_accel->viommu->s2_hwpt->hwpt_id, errp);
    }

    if (!iommufd_backend_alloc_hwpt(idev->iommufd, idev->devid, idev->ioas_id,
                                    IOMMU_HWPT_ALLOC_NEST_PARENT,
                                    IOMMU_HWPT_DATA_NONE, 0, NULL,
                                    &s2_hwpt_id, errp)) {
        return false;
    }

    /* Attach to S2 for MSI cookie */
    if (!host_iommu_device_iommufd_attach_hwpt(idev, s2_hwpt_id, errp)) {
        goto free_s2_hwpt;
    }

    if (!iommufd_backend_alloc_viommu(idev->iommufd, idev->devid,
                                      IOMMU_VIOMMU_TYPE_ARM_SMMUV3,
                                      s2_hwpt_id, &viommu_id, errp)) {
        goto detach_s2_hwpt;
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
        error_report("failed to allocate an abort pagetable");
        goto free_viommu;
    }

    if (!iommufd_backend_alloc_hwpt(idev->iommufd, idev->devid,
                                    viommu->core.viommu_id, 0,
                                    IOMMU_HWPT_DATA_ARM_SMMUV3,
                                    sizeof(bypass_data), &bypass_data,
                                    &viommu->bypass_hwpt_id, errp)) {
        error_report("failed to allocate a bypass pagetable");
        goto free_abort_hwpt;
    }

    /*
     * Attach the bypass STE which means S1 bypass and S2 translate.
     * This is to make sure that the vIOMMU object is now associated
     * with the device and has this STE installed in the host SMMUV3.
     */
    if (!host_iommu_device_iommufd_attach_hwpt(
                idev, viommu->bypass_hwpt_id, errp)) {
        error_report("failed to attach the bypass pagetable");
        goto free_bypass_hwpt;
    }

    s2_hwpt = g_new0(SMMUS2Hwpt, 1);
    s2_hwpt->iommufd = idev->iommufd;
    s2_hwpt->hwpt_id = s2_hwpt_id;
    s2_hwpt->ioas_id = idev->ioas_id;

    viommu->iommufd = idev->iommufd;
    viommu->s2_hwpt = s2_hwpt;

    s_accel->viommu = viommu;
    accel_dev->viommu = viommu;
    return true;

free_bypass_hwpt:
    iommufd_backend_free_id(idev->iommufd, viommu->bypass_hwpt_id);
free_abort_hwpt:
    iommufd_backend_free_id(idev->iommufd, viommu->abort_hwpt_id);
free_viommu:
    iommufd_backend_free_id(idev->iommufd, viommu->core.viommu_id);
    g_free(viommu);
detach_s2_hwpt:
    host_iommu_device_iommufd_attach_hwpt(idev, accel_dev->idev->ioas_id, errp);
free_s2_hwpt:
    iommufd_backend_free_id(idev->iommufd, s2_hwpt_id);
    return false;
}

static bool smmuv3_accel_set_iommu_device(PCIBus *bus, void *opaque, int devfn,
                                          HostIOMMUDevice *hiod, Error **errp)
{
    HostIOMMUDeviceIOMMUFD *idev = HOST_IOMMU_DEVICE_IOMMUFD(hiod);
    SMMUState *s = opaque;
    SMMUv3AccelState *s_accel = ARM_SMMUV3_ACCEL(s);
    SMMUPciBus *sbus = smmu_get_sbus(s, bus);
    SMMUv3AccelDevice *accel_dev = smmuv3_accel_get_dev(s, sbus, bus, devfn);
    SMMUDevice *sdev = &accel_dev->sdev;

    if (!idev) {
        return true;
    }

    if (accel_dev->idev) {
        if (accel_dev->idev != idev) {
            error_report("Device 0x%x already ha an associated idev",
                         smmu_get_sid(sdev));
            return false;
        } else {
            return true;
        }
    }

    if (!smmuv3_accel_dev_attach_viommu(accel_dev, idev, errp)) {
        error_report("Unable to attach viommu");
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
    SMMUDevice *sdev;
    SMMUv3AccelDevice *accel_dev;
    SMMUViommu *viommu;
    SMMUState *s = opaque;
    SMMUv3AccelState *s_accel = ARM_SMMUV3_ACCEL(s);
    SMMUPciBus *sbus = g_hash_table_lookup(s->smmu_pcibus_by_busptr, bus);

    if (!sbus) {
        return;
    }

    sdev = sbus->pbdev[devfn];
    if (!sdev) {
        return;
    }

    accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
    if (!host_iommu_device_iommufd_attach_hwpt(accel_dev->idev,
                                               accel_dev->idev->ioas_id,
                                               NULL)) {
        error_report("Unable to attach dev to the default HW pagetable");
    }


    accel_dev->idev = NULL;
    QLIST_REMOVE(accel_dev, next);
    trace_smmuv3_accel_unset_iommu_device(devfn, smmu_get_sid(sdev));

    viommu = s_accel->viommu;
    if (QLIST_EMPTY(&viommu->device_list)) {
        iommufd_backend_free_id(viommu->iommufd, viommu->bypass_hwpt_id);
        iommufd_backend_free_id(viommu->iommufd, viommu->abort_hwpt_id);
        iommufd_backend_free_id(viommu->iommufd, viommu->core.viommu_id);
        iommufd_backend_free_id(viommu->iommufd, viommu->s2_hwpt->hwpt_id);
        g_free(viommu->s2_hwpt);
        g_free(viommu);
        s_accel->viommu = NULL;
    }
}
static AddressSpace *smmuv3_accel_find_add_as(PCIBus *bus, void *opaque,
                                              int devfn)
{
    SMMUState *s = opaque;
    SMMUPciBus *sbus;
    SMMUv3AccelDevice *accel_dev;
    SMMUDevice *sdev;

    sbus = smmu_get_sbus(s, bus);
    accel_dev = smmuv3_accel_get_dev(s, sbus, bus, devfn);
    sdev = &accel_dev->sdev;

    return &sdev->as;
}

static int smmuv3_accel_pxb_pcie_bus(Object *obj, void *opaque)
{
    DeviceState *d = opaque;

    if (object_dynamic_cast(obj, "pxb-pcie-bus")) {
        PCIBus *bus = PCI_HOST_BRIDGE(obj->parent)->bus;
        if (d->parent_bus && !strcmp(bus->qbus.name, d->parent_bus->name)) {
            object_property_set_link(OBJECT(d), "primary-bus", OBJECT(bus),
                                     &error_abort);
        }
    }
    return 0;
}

static void smmu_accel_realize(DeviceState *d, Error **errp)
{
    SMMUv3AccelState *s_accel = ARM_SMMUV3_ACCEL(d);
    SMMUv3AccelClass *c = ARM_SMMUV3_ACCEL_GET_CLASS(s_accel);
    SysBusDevice *dev = SYS_BUS_DEVICE(d);
    SMMUState *bs = ARM_SMMU(d);
    Error *local_err = NULL;

    object_child_foreach_recursive(object_get_root(),
                                   smmuv3_accel_pxb_pcie_bus, d);

    object_property_set_bool(OBJECT(dev), "accel", true, &error_abort);
    c->parent_realize(d, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }
    bs->get_address_space = smmuv3_accel_find_add_as;
    bs->set_iommu_device = smmuv3_accel_set_iommu_device;
    bs->unset_iommu_device = smmuv3_accel_unset_iommu_device;
}

static void smmuv3_accel_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SMMUv3AccelClass *c = ARM_SMMUV3_ACCEL_CLASS(klass);

    device_class_set_parent_realize(dc, smmu_accel_realize,
                                    &c->parent_realize);
    dc->hotpluggable = false;
    dc->bus_type = TYPE_PCIE_BUS;
}

static const TypeInfo smmuv3_accel_type_info = {
    .name          = TYPE_ARM_SMMUV3_ACCEL,
    .parent        = TYPE_ARM_SMMUV3,
    .instance_size = sizeof(SMMUv3AccelState),
    .class_size    = sizeof(SMMUv3AccelClass),
    .class_init    = smmuv3_accel_class_init,
};

static void smmuv3_accel_register_types(void)
{
    type_register_static(&smmuv3_accel_type_info);
}

type_init(smmuv3_accel_register_types)
