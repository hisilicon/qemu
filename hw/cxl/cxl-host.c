/*
 * CXL host parameter parsing routines
 *
 * Copyright (c) 2022 Huawei
 * Modeled loosely on the NUMA options handling in hw/core/numa.c
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/bitmap.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "sysemu/qtest.h"
#include "hw/boards.h"

#include "qapi/opts-visitor.h"
#include "qapi/qapi-visit-machine.h"
#include "qemu/option.h"
#include "hw/cxl/cxl.h"
#include "hw/pci/pci_bus.h"
#include "hw/pci/pci_bridge.h"
#include "hw/pci/pci_host.h"
#include "hw/pci/pcie_port.h"

QemuOptsList qemu_cxl_fixed_window_opts = {
    .name = "cxl-fixed-memory-window",
    .implied_opt_name = "type",
    .head = QTAILQ_HEAD_INITIALIZER(qemu_cxl_fixed_window_opts.head),
    .desc = { { 0 } }
};

static void set_cxl_fixed_memory_window_options(MachineState *ms,
                                                CXLFixedMemoryWindowOptions *object,
                                                Error **errp)
{
    CXLFixedWindow *fw = g_malloc0(sizeof(*fw));
    strList *target;
    int i;

    for (target = object->targets; target; target = target->next) {
        fw->num_targets++;
    }

    fw->enc_int_ways = cxl_interleave_ways_enc(fw->num_targets, errp);
    if (*errp) {
        return;
    }

    fw->targets = g_malloc0_n(fw->num_targets, sizeof(*fw->targets));
    for (i = 0, target = object->targets; target; i++, target = target->next) {
        /* This link cannot be resolved yet, so stash the name for now */
        fw->targets[i] = g_strdup(target->value);
    }

    if (object->size % (256 * MiB)) {
        error_setg(errp,
                   "Size of a CXL fixed memory window must my a multiple of 256MiB");
        return;
    }
    fw->size = object->size;

    if (object->has_interleave_granularity) {
        fw->enc_int_gran =
            cxl_interleave_granularity_enc(object->interleave_granularity,
                                           errp);
        if (*errp) {
            return;
        }
    } else {
        /* Default to 256 byte interleave */
        fw->enc_int_gran = 0;
    }

    ms->cxl_devices_state->fixed_windows =
        g_list_append(ms->cxl_devices_state->fixed_windows, fw);

    return;
}

static int parse_cxl_fixed_memory_window(void *opaque, QemuOpts *opts,
                                         Error **errp)
{
    CXLFixedMemoryWindowOptions *object = NULL;
    MachineState *ms = MACHINE(opaque);
    Error *err = NULL;
    Visitor *v = opts_visitor_new(opts);

    visit_type_CXLFixedMemoryWindowOptions(v, NULL, &object, errp);
    visit_free(v);
    if (!object) {
        return -1;
    }

    set_cxl_fixed_memory_window_options(ms, object, &err);

    qapi_free_CXLFixedMemoryWindowOptions(object);
    if (err) {
        error_propagate(errp, err);
        return -1;
    }

    return 0;
}

void parse_cxl_fixed_memory_window_opts(MachineState *ms)
{
    qemu_opts_foreach(qemu_find_opts("cxl-fixed-memory-window"),
                      parse_cxl_fixed_memory_window, ms, &error_fatal);
}

void cxl_fixed_memory_window_link_targets(Error **errp)
{
    MachineState *ms = MACHINE(qdev_get_machine());

    if (ms->cxl_devices_state && ms->cxl_devices_state->fixed_windows) {
        GList *it;

        for (it = ms->cxl_devices_state->fixed_windows; it; it = it->next) {
            CXLFixedWindow *fw = it->data;
            int i;

            for (i = 0; i < fw->num_targets; i++) {
                Object *o;
                bool ambig;

                o = object_resolve_path_type(fw->targets[i],
                                             TYPE_PXB_CXL_DEVICE,
                                             &ambig);
                if (!o) {
                    error_setg(errp, "Could not resolve CXLFM target %s",
                               fw->targets[i]);
                    return;
                }
                fw->target_hbs[i] = PXB_CXL_DEV(o);
            }
        }
    }
}
