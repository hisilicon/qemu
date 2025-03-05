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

#include "smmuv3-internal.h"

static int
smmuv3_accel_dev_get_info(SMMUv3AccelDevice *accel_dev, uint32_t *data_type,
                          uint32_t data_len, void *data)
{
    uint64_t caps;

    if (!accel_dev || !accel_dev->idev) {
        return -ENOENT;
    }

    return !iommufd_backend_get_device_info(accel_dev->idev->iommufd,
                                            accel_dev->idev->devid,
                                            data_type, data,
                                            data_len, &caps, NULL);
}

static void smmuv3_accel_init_regs(SMMUv3AccelState *s_accel)
{
    SMMUv3State *s = ARM_SMMUV3(s_accel);
    SMMUv3AccelDevice *accel_dev;
    uint32_t data_type;
    uint32_t val;
    int ret;

    if (!s_accel->viommu || QLIST_EMPTY(&s_accel->viommu->device_list)) {
        error_report("At least one cold-plugged vfio-pci is required for smmuv3-accel!");
        exit(1);
    }

    accel_dev = QLIST_FIRST(&s_accel->viommu->device_list);
    if (accel_dev->info.idr[0]) {
        info_report("reusing the previous hw_info");
        goto out;
    }

    ret = smmuv3_accel_dev_get_info(accel_dev, &data_type,
                                    sizeof(accel_dev->info), &accel_dev->info);
    if (ret) {
        error_report("failed to get SMMU device info");
        return;
    }

    if (data_type != IOMMU_HW_INFO_TYPE_ARM_SMMUV3) {
        error_report("Wrong data type (%d)!", data_type);
        return;
    }

out:
    trace_smmuv3_accel_get_device_info(accel_dev->info.idr[0],
                                       accel_dev->info.idr[1],
                                       accel_dev->info.idr[3],
                                       accel_dev->info.idr[5]);

    val = FIELD_EX32(accel_dev->info.idr[0], IDR0, BTM);
    s->idr[0] = FIELD_DP32(s->idr[0], IDR0, BTM, val);
    val = FIELD_EX32(accel_dev->info.idr[0], IDR0, ATS);
    s->idr[0] = FIELD_DP32(s->idr[0], IDR0, ATS, val);
    val = FIELD_EX32(accel_dev->info.idr[0], IDR0, ASID16);
    s->idr[0] = FIELD_DP32(s->idr[0], IDR0, ASID16, val);
    val = FIELD_EX32(accel_dev->info.idr[0], IDR0, TERM_MODEL);
    s->idr[0] = FIELD_DP32(s->idr[0], IDR0, TERM_MODEL, val);
    val = FIELD_EX32(accel_dev->info.idr[0], IDR0, STALL_MODEL);
    s->idr[0] = FIELD_DP32(s->idr[0], IDR0, STALL_MODEL, val);
    val = FIELD_EX32(accel_dev->info.idr[0], IDR0, STLEVEL);
    s->idr[0] = FIELD_DP32(s->idr[0], IDR0, STLEVEL, val);

    val = FIELD_EX32(accel_dev->info.idr[1], IDR1, SIDSIZE);
    s->idr[1] = FIELD_DP32(s->idr[1], IDR1, SIDSIZE, val);
    val = FIELD_EX32(accel_dev->info.idr[1], IDR1, SSIDSIZE);
    s->idr[1] = FIELD_DP32(s->idr[1], IDR1, SSIDSIZE, val);

    val = FIELD_EX32(accel_dev->info.idr[3], IDR3, HAD);
    s->idr[3] = FIELD_DP32(s->idr[3], IDR3, HAD, val);
    val = FIELD_EX32(accel_dev->info.idr[3], IDR3, RIL);
    s->idr[3] = FIELD_DP32(s->idr[3], IDR3, RIL, val);
    val = FIELD_EX32(accel_dev->info.idr[3], IDR3, BBML);
    s->idr[3] = FIELD_DP32(s->idr[3], IDR3, BBML, val);

    val = FIELD_EX32(accel_dev->info.idr[5], IDR5, GRAN4K);
    s->idr[5] = FIELD_DP32(s->idr[5], IDR5, GRAN4K, val);
    val = FIELD_EX32(accel_dev->info.idr[5], IDR5, GRAN16K);
    s->idr[5] = FIELD_DP32(s->idr[5], IDR5, GRAN16K, val);
    val = FIELD_EX32(accel_dev->info.idr[5], IDR5, GRAN64K);
    s->idr[5] = FIELD_DP32(s->idr[5], IDR5, GRAN64K, val);
    val = FIELD_EX32(accel_dev->info.idr[5], IDR5, OAS);
    s->idr[5] = FIELD_DP32(s->idr[5], IDR5, OAS, val);

    /* FIXME check iidr and aidr registrs too */
}

static SMMUv3AccelDevice *smmuv3_accel_get_dev(SMMUState *s, SMMUPciBus *sbus,
                                                PCIBus *bus, int devfn)
{
    SMMUv3AccelState *s_accel = ARM_SMMUV3_ACCEL(s);
    SMMUDevice *sdev = sbus->pbdev[devfn];
    SMMUv3AccelDevice *accel_dev;

    if (sdev) {
        accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
    } else {
        accel_dev = g_new0(SMMUv3AccelDevice, 1);
        sdev = &accel_dev->sdev;

        sbus->pbdev[devfn] = sdev;
        smmu_init_sdev(s, sdev, bus, devfn);
        address_space_init(&accel_dev->as_sysmem, &s_accel->root,
                           "smmuv3-accel-sysmem");
    }

    return accel_dev;
}

static void
smmuv3_accel_dev_uninstall_nested_ste(SMMUv3AccelDevice *accel_dev, bool abort)
{
    HostIOMMUDeviceIOMMUFD *idev = accel_dev->idev;
    SMMUS1Hwpt *s1_hwpt = accel_dev->s1_hwpt;
    uint32_t hwpt_id;

    if (!s1_hwpt || !accel_dev->viommu) {
        return;
    }

    if (abort) {
        hwpt_id = accel_dev->viommu->abort_hwpt_id;
    } else {
        hwpt_id = accel_dev->viommu->bypass_hwpt_id;
    }

    host_iommu_device_iommufd_attach_hwpt(idev, hwpt_id, &error_abort);
    iommufd_backend_free_id(s1_hwpt->iommufd, s1_hwpt->hwpt_id);
    accel_dev->s1_hwpt = NULL;
    g_free(s1_hwpt);
}

static int
smmuv3_accel_dev_install_nested_ste(SMMUv3AccelDevice *accel_dev,
                                    uint32_t data_type, uint32_t data_len,
                                    void *data)
{
    SMMUViommu *viommu = accel_dev->viommu;
    SMMUS1Hwpt *s1_hwpt = accel_dev->s1_hwpt;
    HostIOMMUDeviceIOMMUFD *idev = accel_dev->idev;

    if (!idev || !viommu) {
        return -ENOENT;
    }

    if (s1_hwpt) {
        smmuv3_accel_dev_uninstall_nested_ste(accel_dev, false);
    }

    s1_hwpt = g_new0(SMMUS1Hwpt, 1);
    if (!s1_hwpt) {
        return -ENOMEM;
    }

    s1_hwpt->iommufd = idev->iommufd;
    iommufd_backend_alloc_hwpt(idev->iommufd, idev->devid,
                               viommu->core.viommu_id, 0, data_type, data_len,
                               data, &s1_hwpt->hwpt_id, &error_abort);
    host_iommu_device_iommufd_attach_hwpt(idev, s1_hwpt->hwpt_id, &error_abort);
    accel_dev->s1_hwpt = s1_hwpt;
    return 0;
}

void smmuv3_accel_install_nested_ste(SMMUDevice *sdev, int sid)
{
    SMMUv3AccelDevice *accel_dev;
    SMMUEventInfo event = {.type = SMMU_EVT_NONE, .sid = sid,
                           .inval_ste_allowed = true};
    struct iommu_hwpt_arm_smmuv3 nested_data = {};
    SMMUv3State *s = sdev->smmu;
    SMMUState *bs = &s->smmu_state;
    uint32_t config;
    STE ste;
    int ret;

    if (!bs->accel) {
        return;
    }

    accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
    if (!accel_dev->viommu) {
        return;
    }

    if (!accel_dev->vdev && accel_dev->idev) {
        SMMUVdev *vdev;
        uint32_t vdev_id;
        SMMUViommu *viommu = accel_dev->viommu;

        iommufd_backend_alloc_vdev(viommu->core.iommufd, accel_dev->idev->devid,
                                   viommu->core.viommu_id, sid, &vdev_id,
                                   &error_abort);
        vdev = g_new0(SMMUVdev, 1);
        vdev->vdev_id = vdev_id;
        vdev->sid = sid;
        accel_dev->vdev = vdev;
    }

    ret = smmu_find_ste(sdev->smmu, sid, &ste, &event);
    if (ret) {
        /*
         * For a 2-level Stream Table, the level-2 table might not be ready
         * until the device gets inserted to the stream table. Ignore this.
         */
        return;
    }

    config = STE_CONFIG(&ste);
    if (!STE_VALID(&ste) || !STE_CFG_S1_ENABLED(config)) {
        smmuv3_accel_dev_uninstall_nested_ste(accel_dev, STE_CFG_ABORT(config));
        smmuv3_flush_config(sdev);
        return;
    }

    nested_data.ste[0] = (uint64_t)ste.word[0] | (uint64_t)ste.word[1] << 32;
    nested_data.ste[1] = (uint64_t)ste.word[2] | (uint64_t)ste.word[3] << 32;
    /* V | CONFIG | S1FMT | S1CTXPTR | S1CDMAX */
    nested_data.ste[0] &= 0xf80fffffffffffffULL;
    /* S1DSS | S1CIR | S1COR | S1CSH | S1STALLD | EATS */
    nested_data.ste[1] &= 0x380000ffULL;
    ret = smmuv3_accel_dev_install_nested_ste(accel_dev,
                                              IOMMU_HWPT_DATA_ARM_SMMUV3,
                                              sizeof(nested_data),
                                              &nested_data);
    if (ret) {
        error_report("Unable to install nested STE=%16LX:%16LX, ret=%d",
                     nested_data.ste[1], nested_data.ste[0], ret);
    }
    trace_smmuv3_accel_install_nested_ste(sid, nested_data.ste[1],
                                          nested_data.ste[0]);
}

/* Update batch->ncmds to the number of execute cmds */
int smmuv3_accel_issue_cmd_batch(SMMUState *bs, SMMUCommandBatch *batch)
{
    SMMUv3AccelState *s_accel = ARM_SMMUV3_ACCEL(bs);
    uint32_t total = batch->ncmds;
    IOMMUFDViommu *viommu_core;
    int ret;

    if (!bs->accel) {
        return 0;
    }

    if (!s_accel->viommu) {
        return 0;
    }
    viommu_core = &s_accel->viommu->core;
    ret = iommufd_backend_invalidate_cache(viommu_core->iommufd,
                                           viommu_core->viommu_id,
                                           IOMMU_VIOMMU_INVALIDATE_DATA_ARM_SMMUV3,
                                           sizeof(Cmd), &batch->ncmds,
                                           batch->cmds);
    if (total != batch->ncmds) {
        error_report("%s failed: ret=%d, total=%d, done=%d",
                      __func__, ret, total, batch->ncmds);
        return ret;
    }

    batch->ncmds = 0;
    batch->dev_cache = false;
    return ret;
}

int smmuv3_accel_batch_cmds(SMMUState *bs, SMMUDevice *sdev,
                            SMMUCommandBatch *batch, Cmd *cmd,
                            uint32_t *cons, bool dev_cache)
{
    int ret;

    if (!bs->accel) {
        return 0;
    }

    if (sdev) {
        SMMUv3AccelDevice *accel_dev;
        accel_dev = container_of(sdev, SMMUv3AccelDevice, sdev);
        if (!accel_dev->s1_hwpt) {
            return 0;
        }
    }

    /*
     * Currently separate out dev_cache and hwpt for safety, which might
     * not be necessary if underlying HW SMMU does not have the errata.
     *
     * TODO check IIDR register values read from hw_info.
     */
    if (batch->ncmds && (dev_cache != batch->dev_cache)) {
        ret = smmuv3_accel_issue_cmd_batch(bs, batch);
        if (ret) {
            *cons = batch->cons[batch->ncmds];
            return ret;
        }
    }
    batch->dev_cache = dev_cache;
    batch->cmds[batch->ncmds] = *cmd;
    batch->cons[batch->ncmds++] = *cons;
    return 0;
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
    SMMUVdev *vdev;
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
    vdev = accel_dev->vdev;
    if (vdev) {
        iommufd_backend_free_id(viommu->iommufd, vdev->vdev_id);
        g_free(vdev);
        accel_dev->vdev = NULL;
    }

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
    PCIDevice *pdev = pci_find_device(bus, pci_bus_num(bus), devfn);
    bool has_iommufd = false;

    if (pdev) {
        has_iommufd = object_property_find(OBJECT(pdev), "iommufd");
    }

    sbus = smmu_get_sbus(s, bus);
    accel_dev = smmuv3_accel_get_dev(s, sbus, bus, devfn);
    sdev = &accel_dev->sdev;

    /* Return the system as if the device uses stage-2 only */
    if (has_iommufd && !accel_dev->s1_hwpt) {
        return &accel_dev->as_sysmem;
    } else {
        return &sdev->as;
    }
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

    memory_region_init(&s_accel->root, OBJECT(s_accel), "root", UINT64_MAX);
    memory_region_init_alias(&s_accel->sysmem, OBJECT(s_accel),
                             "smmuv3-accel-sysmem", get_system_memory(), 0,
                             memory_region_size(get_system_memory()));
    memory_region_add_subregion(&s_accel->root, 0, &s_accel->sysmem);
    bs->get_address_space = smmuv3_accel_find_add_as;
    bs->set_iommu_device = smmuv3_accel_set_iommu_device;
    bs->unset_iommu_device = smmuv3_accel_unset_iommu_device;
}

static void smmuv3_accel_reset_hold(Object *obj, ResetType type)
{
    SMMUv3AccelState *s = ARM_SMMUV3_ACCEL(obj);
    SMMUv3AccelClass *c = ARM_SMMUV3_ACCEL_GET_CLASS(s);

    if (c->parent_phases.hold) {
        c->parent_phases.hold(obj, type);
    }
    smmuv3_accel_init_regs(s);
}

static void smmuv3_accel_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    SMMUv3AccelClass *c = ARM_SMMUV3_ACCEL_CLASS(klass);

    resettable_class_set_parent_phases(rc, NULL, smmuv3_accel_reset_hold, NULL,
                                       &c->parent_phases);
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
