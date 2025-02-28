/*
 * Copyright (c) 2025 Huawei Technologies R & D (UK) Ltd
 * Copyright (C) 2025 NVIDIA
 * Written by Nicolin Chen, Shameer Kolothum
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"

#include "hw/arm/smmuv3-accel.h"
#include "hw/pci/pci_bridge.h"

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
    Error *local_err = NULL;

    object_child_foreach_recursive(object_get_root(),
                                   smmuv3_accel_pxb_pcie_bus, d);

    object_property_set_bool(OBJECT(dev), "accel", true, &error_abort);
    c->parent_realize(d, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }
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
