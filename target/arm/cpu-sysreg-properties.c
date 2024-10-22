/*
 * QEMU ARM CPU SYSREG PROPERTIES
 * to be generated from linux sysreg
 *
 * Copyright (c) Red Hat, Inc. 2024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 */

#include "cpu-custom.h"

ARM64SysReg arm64_id_regs[NUM_ID_IDX];

void initialize_cpu_sysreg_properties(void)
{
    memset(arm64_id_regs, 0, sizeof(ARM64SysReg) * NUM_ID_IDX);
    /* CTR_EL0 */
    ARM64SysReg *CTR_EL0 = arm64_sysreg_get(CTR_EL0_IDX);
    CTR_EL0->name = "CTR_EL0";
    arm64_sysreg_add_field(CTR_EL0, "TminLine", 32, 37);
    arm64_sysreg_add_field(CTR_EL0, "DIC", 29, 29);
    arm64_sysreg_add_field(CTR_EL0, "IDC", 28, 28);
    arm64_sysreg_add_field(CTR_EL0, "CWG", 24, 27);
    arm64_sysreg_add_field(CTR_EL0, "ERG", 20, 23);
    arm64_sysreg_add_field(CTR_EL0, "DminLine", 16, 19);
    arm64_sysreg_add_field(CTR_EL0, "L1Ip", 14, 15);
    arm64_sysreg_add_field(CTR_EL0, "IminLine", 0, 3);
}

