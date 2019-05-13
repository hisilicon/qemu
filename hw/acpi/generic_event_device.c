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
#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "hw/acpi/acpi.h"
#include "hw/acpi/generic_event_device.h"
#include "hw/mem/pc-dimm.h"
#include "qemu/error-report.h"

#define ACPI_POWER_BUTTON_DEVICE "PWRB"

/*
 * The ACPI Generic Event Device (GED) is a hardware-reduced specific
 * device[ACPI v6.1 Section 5.6.9] that handles all platform events,
 * including the hotplug ones. Platforms need to specify their own
 * GedEvent array to describe what kind of events they want to support
 * through GED. This routine uses a single interrupt for the GED device,
 * relying on IO memory region to communicate the type of device
 * affected by the interrupt. This way, we can support up to 32 events
 * with a unique interrupt.
 */
void build_ged_aml(Aml *table, const char *name, HotplugHandler *hotplug_dev,
                   uint32_t ged_irq, AmlRegionSpace rs)
{
    AcpiGedState *s = ACPI_GED(hotplug_dev);
    GedEvent *ged_events = s->ged_events;
    Aml *crs = aml_resource_template();
    Aml *evt, *field;
    Aml *dev = aml_device("%s", name);
    Aml *irq_sel = aml_local(0);
    Aml *isel = aml_name(AML_GED_IRQ_SEL);
    uint32_t i;

    if (!s->ged_base || !ged_events || !s->ged_events_size) {
        return;
    }

    /* _CRS interrupt */
    aml_append(crs, aml_interrupt(AML_CONSUMER, AML_EDGE, AML_ACTIVE_HIGH,
                                  AML_EXCLUSIVE, &ged_irq, 1));

    aml_append(dev, aml_name_decl("_HID", aml_string("ACPI0013")));
    aml_append(dev, aml_name_decl("_UID", aml_string(GED_DEVICE)));
    aml_append(dev, aml_name_decl("_CRS", crs));

    /* Append IO region */
    aml_append(dev, aml_operation_region(AML_GED_IRQ_REG, rs,
               aml_int(s->ged_base + ACPI_GED_IRQ_SEL_OFFSET),
               ACPI_GED_IRQ_SEL_LEN));
    field = aml_field(AML_GED_IRQ_REG, AML_DWORD_ACC, AML_NOLOCK,
                      AML_WRITE_AS_ZEROS);
    aml_append(field, aml_named_field(AML_GED_IRQ_SEL,
                                      ACPI_GED_IRQ_SEL_LEN * BITS_PER_BYTE));
    aml_append(dev, field);
    /*
     * For each GED event we:
     * - Add an interrupt to the CRS section.
     * - Add a conditional block for each event, inside a while loop.
     *   This is semantically equivalent to a switch/case implementation.
     */
    evt = aml_method("_EVT", 1, AML_SERIALIZED);
    {
        Aml *if_ctx;

        /* Local0 = ISEL */
        aml_append(evt, aml_store(isel, irq_sel));

        /*
         * Here we want to call a method for each supported GED event type.
         * The resulting ASL code looks like:
         *
         * Local0 = ISEL
         * If ((Local0 & irq0) == irq0)
         * {
         *     MethodEvent0()
         * }
         *
         * If ((Local0 & irq1) == irq1)
         * {
         *     MethodEvent1()
         * }
         * ...
         */

        for (i = 0; i < s->ged_events_size; i++) {
            if_ctx = aml_if(aml_equal(aml_and(irq_sel,
                                      aml_int(ged_events[i].selector), NULL),
                                      aml_int(ged_events[i].selector)));
            switch (ged_events[i].event) {
            case GED_MEMORY_HOTPLUG:
                aml_append(if_ctx, aml_call0(MEMORY_DEVICES_CONTAINER "."
                                             MEMORY_SLOT_SCAN_METHOD));
                break;
            case GED_PWR_DOWN:
                aml_append(if_ctx, 
                           aml_notify(aml_name(ACPI_POWER_BUTTON_DEVICE),
                                      aml_int(0x80)));
		break;
            default:
                warn_report("GED doesn't have any Notify() object for the "
                            "platform event %u. It may not be handled properly",
                             ged_events[i].event);

            }
            aml_append(evt, if_ctx);
        }
    }

    /* Append _EVT method */
    aml_append(dev, evt);

    aml_append(table, dev);
}

/* Memory read by the GED _EVT AML dynamic method */
static uint64_t ged_read(void *opaque, hwaddr addr, unsigned size)
{
    uint64_t val = 0;
    GEDState *ged_st = opaque;

    switch (addr) {
    case ACPI_GED_IRQ_SEL_OFFSET:
        /* Read the selector value and reset it */
        qemu_mutex_lock(&ged_st->lock);
        val = ged_st->sel;
        ged_st->sel = 0;
        qemu_mutex_unlock(&ged_st->lock);
        break;
    default:
        break;
    }

    return val;
}

/* Nothing is expected to be written to the GED memory region */
static void ged_write(void *opaque, hwaddr addr, uint64_t data,
                      unsigned int size)
{
}

static const MemoryRegionOps ged_ops = {
    .read = ged_read,
    .write = ged_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void acpi_ged_init(MemoryRegion *as, DeviceState *dev, GEDState *ged_st)
{
    AcpiGedState *s = ACPI_GED(dev);

    assert(s->ged_base);

    ged_st->irq = s->ged_irq;
    ged_st->gsi = *(qemu_irq *)s->gsi;
    warn_report("%s gsi %p s->gsi %p\n", __func__, ged_st->gsi, s->gsi);
    qemu_mutex_init(&ged_st->lock);
    memory_region_init_io(&ged_st->io, OBJECT(dev), &ged_ops, ged_st,
                          "acpi-ged-event", ACPI_GED_REG_LEN);
    memory_region_add_subregion(as, s->ged_base, &ged_st->io);
}

static void acpi_ged_device_plug_cb(HotplugHandler *hotplug_dev,
                                    DeviceState *dev, Error **errp)
{
    AcpiGedState *s = ACPI_GED(hotplug_dev);

    if (s->memhp_state.is_enabled &&
        object_dynamic_cast(OBJECT(dev), TYPE_PC_DIMM)) {
            acpi_memory_plug_cb(hotplug_dev, &s->memhp_state,
                                dev, errp);
    } else {
        error_setg(errp, "virt: device plug request for unsupported device"
                   " type: %s", object_get_typename(OBJECT(dev)));
    }
}

static void acpi_ged_event(GEDState *ged_st, uint32_t ged_irq_sel)
{
    /*
     * Set the GED IRQ selector to the expected device type value. This
     * way, the ACPI method will be able to trigger the right code based
     * on a unique IRQ.
     */
    qemu_mutex_lock(&ged_st->lock);
    ged_st->sel |= ged_irq_sel;
    qemu_mutex_unlock(&ged_st->lock);

    /* Trigger the event by sending an interrupt to the guest. */
    warn_report("%s gsi %p \n", __func__, ged_st->gsi);
    qemu_irq_pulse(ged_st->gsi);
}

static void acpi_ged_send_event(AcpiDeviceIf *adev, AcpiEventStatusBits ev)
{
    AcpiGedState *s = ACPI_GED(adev);
    GEDState *ged_st = &s->ged_state;
    uint32_t sel;

    if (ev & ACPI_MEMORY_HOTPLUG_STATUS) {
        sel = ACPI_GED_IRQ_SEL_MEM;
    } else {
        /* Unknown event. Return without generating interrupt. */
        return;
    }

    acpi_ged_event(ged_st, sel);
#if 0   
    /*
     * Set the GED IRQ selector to the expected device type value. This
     * way, the ACPI method will be able to trigger the right code based
     * on a unique IRQ.
     */
    qemu_mutex_lock(&ged_st->lock);
    ged_st->sel |= sel;
    qemu_mutex_unlock(&ged_st->lock);

    /* Trigger the event by sending an interrupt to the guest. */
    qemu_irq_pulse(ged_st->gsi);
#endif    
}

static void acpi_ged_pm_powerdown_req(Notifier *n, void *opaque)
{
    AcpiGedState *s = container_of(n, AcpiGedState, powerdown_notifier);
    GEDState *ged_st = &s->ged_state;

    warn_report("%s \n",__func__);
    acpi_ged_event(ged_st, ACPI_GED_IRQ_SEL_PWR_DOWN);
}

static void acpi_ged_device_realize(DeviceState *dev, Error **errp)
{
    AcpiGedState *s = ACPI_GED(dev);

    if (s->memhp_state.is_enabled) {
        acpi_memory_hotplug_init(get_system_memory(), OBJECT(dev),
                                 &s->memhp_state,
                                 s->memhp_base);
        acpi_ged_init(get_system_memory(), dev, &s->ged_state);
    }

    s->powerdown_notifier.notify = acpi_ged_pm_powerdown_req;
    qemu_register_powerdown_notifier(&s->powerdown_notifier);


}

static Property acpi_ged_properties[] = {
    /*
     * Memory hotplug base address is a property of GED here,
     * because GED handles memory hotplug event and MEMORY_HOTPLUG_DEVICE
     * gets initialized when GED device is realized.
     */
    DEFINE_PROP_UINT64("memhp-base", AcpiGedState, memhp_base, 0),
    DEFINE_PROP_BOOL("memory-hotplug-support", AcpiGedState,
                     memhp_state.is_enabled, true),
    DEFINE_PROP_PTR("gsi", AcpiGedState, gsi),
    DEFINE_PROP_LINK("new", AcpiGedState, new_gsi, TYPE_IRQ,
                     qemu_irq *),
    DEFINE_PROP_UINT64("ged-base", AcpiGedState, ged_base, 0),
    DEFINE_PROP_UINT32("ged-irq", AcpiGedState, ged_irq, 0),
    DEFINE_PROP_PTR("ged-events", AcpiGedState, ged_events),
    //DEFINE_PROP_LINK("ged-events", AcpiGedState, new_ged_events, "acpi-ged",
    //                 GedEvent *),
    DEFINE_PROP_LINK("downstream", AcpiGedState, downstream1, TYPE_MEMORY_REGION,
                     MemoryRegion *),
    DEFINE_PROP_UINT32("ged-events-size", AcpiGedState, ged_events_size, 0),
    DEFINE_PROP_END_OF_LIST(),
};
/*
static void acpi_ged_instance_init(Object *obj)
{
    AcpiGedState *s = ACPI_GED(obj);

    warn_report("%s \n",__func__);
    object_property_add_link(obj, "newgsi", TYPE_IRQ,
                                 (Object **)&s->new_gsi,
                                 object_property_allow_set_link,
                                 OBJ_PROP_LINK_STRONG,
                                 &error_abort);

}
*/
static void acpi_ged_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    HotplugHandlerClass *hc = HOTPLUG_HANDLER_CLASS(class);
    AcpiDeviceIfClass *adevc = ACPI_DEVICE_IF_CLASS(class);

    dc->desc = "ACPI";
    dc->props = acpi_ged_properties;
    dc->realize = acpi_ged_device_realize;

    /* Reason: pointer properties "gsi" and "ged_events" */
    dc->user_creatable = false;

    hc->plug = acpi_ged_device_plug_cb;

    adevc->send_event = acpi_ged_send_event;
}

static const TypeInfo acpi_ged_info = {
    .name          = TYPE_ACPI_GED,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AcpiGedState),
//    .instance_init = acpi_ged_instance_init,
    .class_init    = acpi_ged_class_init,
    .interfaces = (InterfaceInfo[]) {
        { TYPE_HOTPLUG_HANDLER },
        { TYPE_ACPI_DEVICE_IF },
        { }
    }
};

static TypeInfo acpi_ged_prop = {
    .name = TYPE_GED_PROP,
    .parent = TYPE_OBJECT,
    .instance_size = sizeof(struct GedProp),
};

static void acpi_ged_register_types(void)
{
    type_register_static(&acpi_ged_prop);
    type_register_static(&acpi_ged_info);
}

type_init(acpi_ged_register_types)
