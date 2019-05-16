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
 *
 * The ACPI Generic Event Device (GED) is a hardware-reduced specific
 * device[ACPI v6.1 Section 5.6.9] that handles all platform events,
 * including the hotplug ones. Generic Event Device allows platforms
 * to handle interrupts in ACPI ASL statements. It follows a very
 * similar approach like the _EVT method from GPIO events. All
 * interrupts are listed in  _CRS and the handler is written in _EVT
 * method. Here, we use a single interrupt for the GED device, relying
 * on IO memory region to communicate the type of device affected by
 * the interrupt. This way, we can support up to 32 events with a
 * unique interrupt.
 *
 * Here is an example.
 *
 * Device (\_SB.GED)
 * {
 *     Name (_HID, "ACPI0013")
 *     Name (_UID, Zero)
 *     Name (_CRS, ResourceTemplate ()
 *     {
 *         Interrupt (ResourceConsumer, Edge, ActiveHigh, Exclusive, ,, )
 *         {
 *              0x00000029,
 *         }
 *     })
 *     OperationRegion (IREG, SystemMemory, 0x09080000, 0x04)
 *     Field (IREG, DWordAcc, NoLock, WriteAsZeros)
 *     {
 *         ISEL,   32
 *     }
 *
 *     Method (_EVT, 1, Serialized)  // _EVT: Event
 *     {
 *         Local0 = ISEL // ISEL = IO memory region which specifies the
 *                       // device type.
 *         If (((Local0 & irq0) == irq0))
 *         {
 *             MethodEvent0()
 *         }
 *         ElseIf ((Local0 & irq1) == irq1)
 *         {
 *             MethodEvent1()
 *         }
 *         ...
 *     }
 * }
 *
 */

#ifndef HW_ACPI_GED_H
#define HW_ACPI_GED_H

#include "hw/acpi/memory_hotplug.h"

#define TYPE_ACPI_GED "acpi-ged"
#define ACPI_GED(obj) \
    OBJECT_CHECK(AcpiGedState, (obj), TYPE_ACPI_GED)

#define ACPI_GED_IRQ_SEL_OFFSET    0x0
#define ACPI_GED_IRQ_SEL_LEN       0x4
#define ACPI_GED_REG_LEN           0x4

#define GED_DEVICE      "GED"
#define AML_GED_IRQ_REG "IREG"
#define AML_GED_IRQ_SEL "ISEL"

/*
 * Platforms need to specify the GED event bitmap
 * to describe what kind of events they want to support
 * through GED.
 */
#define ACPI_GED_MEM_HOTPLUG_EVT   0x1
#define ACPI_GED_PWR_DOWN_EVT      0x2

typedef struct GEDState {
    MemoryRegion io;
    uint32_t     sel;
    QemuMutex    lock;
} GEDState;

typedef struct AcpiGedState {
    DeviceClass parent_obj;
    MemoryRegion iomem;
    Notifier powerdown_notifier;
    MemHotplugState memhp_state;
    hwaddr memhp_base;
    hwaddr ged_base;
    GEDState ged_state;
    uint32_t ged_event_bitmap;
    qemu_irq irq;
} AcpiGedState;

void build_ged_aml(Aml *table, const char* name, HotplugHandler *hotplug_dev,
                   hwaddr ged_base, uint32_t ged_irq, AmlRegionSpace rs);

#endif
