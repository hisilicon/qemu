/*
 * iommufd container backend
 *
 * Copyright (C) 2023 Intel Corporation.
 * Copyright Red Hat, Inc. 2023
 *
 * Authors: Yi Liu <yi.l.liu@intel.com>
 *          Eric Auger <eric.auger@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "sysemu/iommufd.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qemu/module.h"
#include "qom/object_interfaces.h"
#include "qemu/error-report.h"
#include "monitor/monitor.h"
#include "trace.h"
#include <sys/ioctl.h>
#include <linux/iommufd.h>

static void iommufd_backend_init(Object *obj)
{
    IOMMUFDBackend *be = IOMMUFD_BACKEND(obj);

    be->fd = -1;
    be->users = 0;
    be->owned = true;
    be->hugepages = 1;
    qemu_mutex_init(&be->lock);
}

static void iommufd_backend_finalize(Object *obj)
{
    IOMMUFDBackend *be = IOMMUFD_BACKEND(obj);

    if (be->owned) {
        close(be->fd);
        be->fd = -1;
    }
}

static void iommufd_backend_set_fd(Object *obj, const char *str, Error **errp)
{
    IOMMUFDBackend *be = IOMMUFD_BACKEND(obj);
    int fd = -1;

    fd = monitor_fd_param(monitor_cur(), str, errp);
    if (fd == -1) {
        error_prepend(errp, "Could not parse remote object fd %s:", str);
        return;
    }
    qemu_mutex_lock(&be->lock);
    be->fd = fd;
    be->owned = false;
    qemu_mutex_unlock(&be->lock);
    trace_iommu_backend_set_fd(be->fd);
}

static void iommufd_backend_set_hugepages(Object *obj, bool enabled, Error **errp)
{
    IOMMUFDBackend *be = IOMMUFD_BACKEND(obj);

    qemu_mutex_lock(&be->lock);
    be->hugepages = enabled;
    qemu_mutex_unlock(&be->lock);
}

static void iommufd_backend_class_init(ObjectClass *oc, void *data)
{
    object_class_property_add_str(oc, "fd", NULL, iommufd_backend_set_fd);

    object_class_property_add_bool(oc, "hugepages", NULL, iommufd_backend_set_hugepages);
    object_class_property_set_description(oc, "hugepages",
        "Set to 'off' to disable hugepages");
}

int iommufd_backend_connect(IOMMUFDBackend *be, Error **errp)
{
    int fd, ret = 0;

    qemu_mutex_lock(&be->lock);
    if (be->users == UINT32_MAX) {
        error_setg(errp, "too many connections");
        ret = -E2BIG;
        goto out;
    }
    if (be->owned && !be->users) {
        fd = qemu_open_old("/dev/iommu", O_RDWR);
        if (fd < 0) {
            error_setg_errno(errp, errno, "/dev/iommu opening failed");
            ret = fd;
            goto out;
        }
        be->fd = fd;
    }
    be->users++;
out:
    trace_iommufd_backend_connect(be->fd, be->owned,
                                  be->users, ret);
    qemu_mutex_unlock(&be->lock);
    return ret;
}

void iommufd_backend_disconnect(IOMMUFDBackend *be)
{
    qemu_mutex_lock(&be->lock);
    if (!be->users) {
        goto out;
    }
    be->users--;
    if (!be->users && be->owned) {
        close(be->fd);
        be->fd = -1;
    }
out:
    trace_iommufd_backend_disconnect(be->fd, be->users);
    qemu_mutex_unlock(&be->lock);
}

static int iommufd_backend_set_option(int fd, uint32_t object_id,
                                      enum iommufd_option option_id,
                                      uint64_t val64)
{
    int ret;
    struct iommu_option option = {
        .size = sizeof(option),
        .option_id = option_id,
        .op = IOMMU_OPTION_OP_SET,
        .val64 = val64,
        .object_id = object_id,
    };

    ret = ioctl(fd, IOMMU_OPTION, &option);
    if (ret) {
        error_report("Failed to set option %x to value %"PRIx64" %m", option_id, val64);
    }
    trace_iommufd_backend_set_option(fd, object_id, option_id, val64, ret);

    return ret;
}

static int iommufd_backend_alloc_ioas(int fd, uint32_t *ioas_id)
{
    int ret;
    struct iommu_ioas_alloc alloc_data  = {
        .size = sizeof(alloc_data),
        .flags = 0,
    };

    ret = ioctl(fd, IOMMU_IOAS_ALLOC, &alloc_data);
    if (ret) {
        error_report("Failed to allocate ioas %m");
    }

    *ioas_id = alloc_data.out_ioas_id;
    trace_iommufd_backend_alloc_ioas(fd, *ioas_id, ret);

    return ret;
}

void iommufd_backend_free_id(int fd, uint32_t id)
{
    int ret;
    struct iommu_destroy des = {
        .size = sizeof(des),
        .id = id,
    };

    ret = ioctl(fd, IOMMU_DESTROY, &des);
    trace_iommufd_backend_free_id(fd, id, ret);
    if (ret) {
        error_report("Failed to free id: %u %m", id);
    }
}

int iommufd_backend_get_ioas(IOMMUFDBackend *be, uint32_t *ioas_id)
{
    int ret;

    ret = iommufd_backend_alloc_ioas(be->fd, ioas_id);
    if (!ret && !be->hugepages) {
        iommufd_backend_set_option(be->fd, *ioas_id,
                                   IOMMU_OPTION_HUGE_PAGES, 0);
    }

    trace_iommufd_backend_get_ioas(be->fd, *ioas_id, ret);
    return ret;
}

void iommufd_backend_put_ioas(IOMMUFDBackend *be, uint32_t ioas_id)
{
    iommufd_backend_free_id(be->fd, ioas_id);
    trace_iommufd_backend_put_ioas(be->fd, ioas_id);
}

int iommufd_backend_map_dma(IOMMUFDBackend *be, uint32_t ioas_id, hwaddr iova,
                            ram_addr_t size, void *vaddr, bool readonly)
{
    int ret;
    struct iommu_ioas_map map = {
        .size = sizeof(map),
        .flags = IOMMU_IOAS_MAP_READABLE |
                 IOMMU_IOAS_MAP_FIXED_IOVA,
        .ioas_id = ioas_id,
        .__reserved = 0,
        .user_va = (uintptr_t)vaddr,
        .iova = iova,
        .length = size,
    };

    if (!readonly) {
        map.flags |= IOMMU_IOAS_MAP_WRITEABLE;
    }

    ret = ioctl(be->fd, IOMMU_IOAS_MAP, &map);
    trace_iommufd_backend_map_dma(be->fd, ioas_id, iova, size,
                                  vaddr, readonly, ret);
    if (ret) {
        error_report("IOMMU_IOAS_MAP failed: %m");
    }
    return !ret ? 0 : -errno;
}

int iommufd_backend_unmap_dma(IOMMUFDBackend *be, uint32_t ioas_id,
                              hwaddr iova, ram_addr_t size)
{
    int ret;
    struct iommu_ioas_unmap unmap = {
        .size = sizeof(unmap),
        .ioas_id = ioas_id,
        .iova = iova,
        .length = size,
    };

    ret = ioctl(be->fd, IOMMU_IOAS_UNMAP, &unmap);
    trace_iommufd_backend_unmap_dma(be->fd, ioas_id, iova, size, ret);
    /*
     * TODO: IOMMUFD doesn't support mapping PCI BARs for now.
     * It's not a problem if there is no p2p dma, relax it here
     * and avoid many noisy trigger from vIOMMU side.
     */
    if (ret && errno == ENOENT) {
        ret = 0;
    }
    if (ret) {
        error_report("IOMMU_IOAS_UNMAP failed: %m");
    }
    return !ret ? 0 : -errno;
}

int iommufd_backend_alloc_hwpt(int iommufd, uint32_t dev_id,
                               uint32_t pt_id, uint32_t *out_hwpt)
{
    int ret;
    struct iommu_hwpt_alloc alloc_hwpt = {
        .size = sizeof(struct iommu_hwpt_alloc),
        .flags = IOMMU_HWPT_ALLOC_DIRTY_TRACKING,
        .dev_id = dev_id,
        .pt_id = pt_id,
        .__reserved = 0,
    };

    ret = ioctl(iommufd, IOMMU_HWPT_ALLOC, &alloc_hwpt);
    trace_iommufd_backend_alloc_hwpt(iommufd, dev_id, pt_id,
                                     alloc_hwpt.out_hwpt_id, ret);

    if (ret) {
        error_report("IOMMU_HWPT_ALLOC failed: %m");
    } else {
        *out_hwpt = alloc_hwpt.out_hwpt_id;
    }
    return !ret ? 0 : -errno;
}

int iommufd_backend_set_dirty_tracking(IOMMUFDBackend *be, uint32_t hwpt_id,
                                       bool start)
{
    int ret;
    struct iommu_hwpt_set_dirty_tracking set_dirty = {
            .size = sizeof(set_dirty),
            .hwpt_id = hwpt_id,
            .flags = start ? IOMMU_HWPT_DIRTY_TRACKING_ENABLE : 0,
    };

    ret = ioctl(be->fd, IOMMU_HWPT_SET_DIRTY_TRACKING, &set_dirty);
    trace_iommufd_backend_set_dirty(be->fd, hwpt_id, start, ret);
    if (ret) {
        error_report("IOMMU_HWPT_SET_DIRTY failed: %s", strerror(errno));
    }
    return !ret ? 0 : -errno;
}

int iommufd_backend_get_dirty_iova(IOMMUFDBackend *be, uint32_t hwpt_id,
                                   uint64_t iova, ram_addr_t size,
                                   uint64_t page_size, uint64_t *data)
{
    int ret;
    struct iommu_hwpt_get_dirty_bitmap get_dirty_bitmap = {
        .size = sizeof(get_dirty_bitmap),
        .hwpt_id = hwpt_id,
        .iova = iova,
        .length = size,
        .page_size = page_size,
        .data = (__u64)data,
    };

    ret = ioctl(be->fd, IOMMU_HWPT_GET_DIRTY_BITMAP, &get_dirty_bitmap);
    trace_iommufd_backend_get_dirty_iova(be->fd, hwpt_id, iova, size,
                                         page_size, ret);
    if (ret) {
        error_report("IOMMU_HWPT_GET_DIRTY_IOVA (iova: 0x%"PRIx64
                     " size: 0x%"PRIx64") failed: %s", iova,
                     size, strerror(errno));
    }

    return !ret ? 0 : -errno;
}

static const TypeInfo iommufd_backend_info = {
    .name = TYPE_IOMMUFD_BACKEND,
    .parent = TYPE_OBJECT,
    .instance_size = sizeof(IOMMUFDBackend),
    .instance_init = iommufd_backend_init,
    .instance_finalize = iommufd_backend_finalize,
    .class_size = sizeof(IOMMUFDBackendClass),
    .class_init = iommufd_backend_class_init,
    .interfaces = (InterfaceInfo[]) {
        { TYPE_USER_CREATABLE },
        { }
    }
};

static void register_types(void)
{
    type_register_static(&iommufd_backend_info);
}

type_init(register_types);
