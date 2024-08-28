/*
 * HiSilicon Kunpeng 920 registers
 *
 * This code is licensed under the GNU GPL v2 or later.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "cpregs.h"

int kunpeng920_update_registers(ARMCPU *cpu)
{
    /*
     * Check host is indeed a supported HiSilicon 920 platform and
     * kernel supports writable ID reg.
     * We now only supports 920B & 920C.
     */
    if ((cpu->midr != 0x480fd020 && cpu->midr != 0x480fd030) ||
        !cpu->writable_masks) {
        return -EINVAL;
    }

    /*
     * Set a miniumum set of features between 920B and 920C.
     * This is to enable migration between these two.
     * ToDo: We need to make use of writable_masks to check
     * whether we can actually change the host returned values.
     * If not, KVM write reg will fail later.
     */
    cpu->isar.id_aa64pfr0 = 0x1101001121111111;
    cpu->isar.id_aa64pfr1 = 0x21;
    cpu->isar.id_aa64isar2 = 0;
    cpu->isar.id_aa64mmfr1 = 0x110212122;
    cpu->isar.id_aa64dfr0 = 0xf010305408;
    cpu->isar.id_aa64zfr0 = 0;
    cpu->ctr = 0x84448004;

    return 0;
}
