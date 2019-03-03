/*
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/range.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"

#include "hw/hw.h"
#include "hw/hotplug.h"
#include "hw/sysbus.h"
#include "hw/acpi/ged.h"
#include "hw/arm/virt.h"

#include "hw/acpi/acpi.h"
#include "hw/acpi/acpi_dev_interface.h"
#include "hw/acpi/memory_hotplug.h"
#include "hw/acpi/ged.h"

typedef struct VirtAcpiState {
    SysBusDevice parent_obj;

    MemHotplugState memhp_state;
    GEDState ged_state;

    qemu_irq ged_irq;
} VirtAcpiState;

#define TYPE_VIRT_ACPI "virt-acpi"
#define VIRT_ACPI(obj) \
    OBJECT_CHECK(VirtAcpiState, (obj), TYPE_VIRT_ACPI)

#define VIRT_ACPI_GED_IRQ 0x10
static const VMStateDescription vmstate_acpi = {
    .name = "virt_acpi",
    .version_id = 1,
    .minimum_version_id = 1,
};

#if 1 
static void virt_device_plug_cb(HotplugHandler *hotplug_dev,
                                DeviceState *dev, Error **errp)
{
    VirtAcpiState *s = VIRT_ACPI(hotplug_dev);

    if (object_dynamic_cast(OBJECT(dev), TYPE_PC_DIMM)) {
        if (object_dynamic_cast(OBJECT(dev), TYPE_NVDIMM)) {
            nvdimm_acpi_plug_cb(hotplug_dev, dev);
        } else {
            acpi_memory_plug_cb(hotplug_dev, &s->memhp_state,
                                dev, errp);
        }
    } else {
        error_setg(errp, "virt: device plug request for unsupported device"
                   " type: %s", object_get_typename(OBJECT(dev)));
    }
}

static void virt_device_unplug_request_cb(HotplugHandler *hotplug_dev,
                                          DeviceState *dev, Error **errp)
{
    VirtAcpiState *s = VIRT_ACPI(hotplug_dev);

    if (object_dynamic_cast(OBJECT(dev), TYPE_PC_DIMM)) {
        acpi_memory_unplug_request_cb(hotplug_dev, &s->memhp_state, dev, errp);
    }else {
        error_setg(errp, "virt: device unplug request for unsupported device"
                   " type: %s", object_get_typename(OBJECT(dev)));
    }
}

static void virt_device_unplug_cb(HotplugHandler *hotplug_dev,
                                  DeviceState *dev, Error **errp)
{
    VirtAcpiState *s = VIRT_ACPI(hotplug_dev);

    if (object_dynamic_cast(OBJECT(dev), TYPE_PC_DIMM)) {
        acpi_memory_unplug_cb(&s->memhp_state, dev, errp);
    } else {
        error_setg(errp, "virt: device unplug for unsupported device"
                   " type: %s", object_get_typename(OBJECT(dev)));
    }
}

static void virt_ospm_status(AcpiDeviceIf *adev, ACPIOSTInfoList ***list)
{
}

static void virt_send_ged(AcpiDeviceIf *adev, AcpiEventStatusBits ev)
{
    VirtAcpiState *s = VIRT_ACPI(adev);
    uint32_t sel = ACPI_GED_IRQ_SEL_INIT;

    if (ev & ACPI_CPU_HOTPLUG_STATUS) {
        sel = ACPI_GED_IRQ_SEL_CPU;
    } else if (ev & ACPI_MEMORY_HOTPLUG_STATUS) {
        sel = ACPI_GED_IRQ_SEL_MEM;
    } else if (ev & ACPI_NVDIMM_HOTPLUG_STATUS) {
        sel = ACPI_GED_IRQ_SEL_NVDIMM;
    } else if (ev & ACPI_PCI_HOTPLUG_STATUS) {
        sel = ACPI_GED_IRQ_SEL_PCI;
    } else {
        /* Unknown event. Return without generating interrupt. */
        return;
    }

    /* We inject the hotplug interrupt.
     * The IRQ selector will make the difference from the ACPI table.
     */
    acpi_ged_event(&s->ged_state, sel);
}

#endif
static void virt_device_realize(DeviceState *dev, Error **errp)
{

    VirtMachineState *vms = VIRT_MACHINE(qdev_get_machine());
    const MemMapEntry *memmap = vms->memmap;    
    VirtAcpiState *s = VIRT_ACPI(dev);
    

    acpi_memory_hotplug_init(get_system_io(), OBJECT(dev), &s->memhp_state,
		             memmap[VIRT_PCDIMM_ACPI_IO].base);

    acpi_ged_init(get_system_io(), OBJECT(dev), &s->ged_state,
                  memmap[VIRT_GED_ACPI_IO].base, s->ged_irq);
		  
    printf("%s: Lx\n", __func__);
}
DeviceState *virt_acpi_init(qemu_irq ged_irq)
{
    DeviceState *dev;
    VirtAcpiState *s;
    
    printf("%s: L1\n", __func__);
    dev = sysbus_create_simple(TYPE_VIRT_ACPI, -1, NULL);

    s = VIRT_ACPI(dev);
    s->ged_irq = ged_irq;

    printf("%s: L2\n", __func__);
    return dev;
}
static Property virt_acpi_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};
static void virt_acpi_class_init(ObjectClass *class, void *data)
{
	
    DeviceClass *dc = DEVICE_CLASS(class);
    HotplugHandlerClass *hc = HOTPLUG_HANDLER_CLASS(class);
    AcpiDeviceIfClass *adevc = ACPI_DEVICE_IF_CLASS(class);

    dc->desc = "ACPI2";
    dc->vmsd = &vmstate_acpi;
    dc->props = virt_acpi_properties;
    dc->realize = virt_device_realize;

    hc->plug = virt_device_plug_cb;
    hc->unplug_request = virt_device_unplug_request_cb;
    hc->unplug = virt_device_unplug_cb;

    adevc->ospm_status = virt_ospm_status;
    adevc->send_event = virt_send_ged;
    
    printf("%s: Lx\n", __func__);
}

static const TypeInfo virt_acpi_info = {
    .name          = TYPE_VIRT_ACPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(VirtAcpiState),
    .class_init    = virt_acpi_class_init,
    .interfaces = (InterfaceInfo[]) {
        { TYPE_HOTPLUG_HANDLER },
        { TYPE_ACPI_DEVICE_IF },
        { }
    }
};

static void virt_acpi_register_types(void)
{
    type_register_static(&virt_acpi_info);
}

type_init(virt_acpi_register_types)
