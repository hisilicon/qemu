/*
 * CXL ACPI Implementation
 *
 * Copyright(C) 2020 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */

#include "qemu/osdep.h"
#include "hw/cxl/cxl.h"
#include "hw/acpi/acpi.h"
#include "hw/acpi/aml-build.h"
#include "hw/acpi/bios-linker-loader.h"
#include "hw/acpi/cxl.h"
#include "qapi/error.h"
#include "qemu/uuid.h"

static Aml *__build_cxl_osc_method(void)
{
    Aml *method, *if_uuid, *else_uuid, *if_arg1_not_1, *if_cxl, *if_caps_masked;
    Aml *a_ctrl = aml_local(0);
    Aml *a_cdw1 = aml_name("CDW1");

    method = aml_method("_OSC", 4, AML_NOTSERIALIZED);
    aml_append(method, aml_create_dword_field(aml_arg(3), aml_int(0), "CDW1"));

    /* 9.14.2.1.4 */
    if_uuid = aml_if(
        aml_lor(aml_equal(aml_arg(0),
                          aml_touuid("33DB4D5B-1FF7-401C-9657-7441C03DD766")),
                aml_equal(aml_arg(0),
                          aml_touuid("68F2D50B-C469-4D8A-BD3D-941A103FD3FC"))));
    aml_append(if_uuid, aml_create_dword_field(aml_arg(3), aml_int(4), "CDW2"));
    aml_append(if_uuid, aml_create_dword_field(aml_arg(3), aml_int(8), "CDW3"));

    aml_append(if_uuid, aml_store(aml_name("CDW3"), a_ctrl));

    /* This is all the same as what's used for PCIe */
    aml_append(if_uuid,
               aml_and(aml_name("CTRL"), aml_int(0x1F), aml_name("CTRL")));

    if_arg1_not_1 = aml_if(aml_lnot(aml_equal(aml_arg(1), aml_int(0x1))));
    /* Unknown revision */
    aml_append(if_arg1_not_1, aml_or(a_cdw1, aml_int(0x08), a_cdw1));
    aml_append(if_uuid, if_arg1_not_1);

    if_caps_masked = aml_if(aml_lnot(aml_equal(aml_name("CDW3"), a_ctrl)));
    /* Capability bits were masked */
    aml_append(if_caps_masked, aml_or(a_cdw1, aml_int(0x10), a_cdw1));
    aml_append(if_uuid, if_caps_masked);

    aml_append(if_uuid, aml_store(aml_name("CDW2"), aml_name("SUPP")));
    aml_append(if_uuid, aml_store(aml_name("CDW3"), aml_name("CTRL")));

    if_cxl = aml_if(aml_equal(
        aml_arg(0), aml_touuid("68F2D50B-C469-4D8A-BD3D-941A103FD3FC")));
    /* CXL support field */
    aml_append(if_cxl, aml_create_dword_field(aml_arg(3), aml_int(12), "CDW4"));
    /* CXL capabilities */
    aml_append(if_cxl, aml_create_dword_field(aml_arg(3), aml_int(16), "CDW5"));
    aml_append(if_cxl, aml_store(aml_name("CDW4"), aml_name("SUPC")));
    aml_append(if_cxl, aml_store(aml_name("CDW5"), aml_name("CTRC")));

    /* CXL 2.0 Port/Device Register access */
    aml_append(if_cxl,
               aml_or(aml_name("CDW5"), aml_int(0x1), aml_name("CDW5")));
    aml_append(if_uuid, if_cxl);

    /* Update DWORD3 (the return value) */
    aml_append(if_uuid, aml_store(a_ctrl, aml_name("CDW3")));

    aml_append(if_uuid, aml_return(aml_arg(3)));
    aml_append(method, if_uuid);

    else_uuid = aml_else();

    /* unrecognized uuid */
    aml_append(else_uuid,
               aml_or(aml_name("CDW1"), aml_int(0x4), aml_name("CDW1")));
    aml_append(else_uuid, aml_return(aml_arg(3)));
    aml_append(method, else_uuid);

    return method;
}

void build_cxl_osc_method(Aml *dev)
{
    aml_append(dev, aml_name_decl("SUPP", aml_int(0)));
    aml_append(dev, aml_name_decl("CTRL", aml_int(0)));
    aml_append(dev, aml_name_decl("SUPC", aml_int(0)));
    aml_append(dev, aml_name_decl("CTRC", aml_int(0)));
    aml_append(dev, __build_cxl_osc_method());
}
