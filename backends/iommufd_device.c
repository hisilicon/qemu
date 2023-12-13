/*
 * QEMU abstract of Host IOMMU
 *
 * Copyright (C) 2024 Intel Corporation.
 *
 * Authors: Yi Liu <yi.l.liu@intel.com>
 *          Zhenzhong Duan <zhenzhong.duan@intel.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <sys/ioctl.h>
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "sysemu/iommufd_device.h"
#include "trace.h"

int iommufd_device_attach_hwpt(IOMMUFDDevice *idev, uint32_t hwpt_id)
{
    g_assert(idev->ops->attach_hwpt);
    return idev->ops->attach_hwpt(idev, hwpt_id);
}

int iommufd_device_detach_hwpt(IOMMUFDDevice *idev)
{
    g_assert(idev->ops->detach_hwpt);
    return idev->ops->detach_hwpt(idev);
}

int iommufd_device_get_info(IOMMUFDDevice *idev,
                            enum iommu_hw_info_type *type,
                            uint32_t len, void *data)
{
    struct iommu_hw_info info = {
        .size = sizeof(info),
        .flags = 0,
        .dev_id = idev->dev_id,
        .data_len = len,
        .__reserved = 0,
        .data_uptr = (uintptr_t)data,
    };
    int ret;

    ret = ioctl(idev->iommufd->fd, IOMMU_GET_HW_INFO, &info);
    if (ret) {
        error_report("Failed to get info %m");
    } else {
        *type = info.out_data_type;
    }

    return ret;
}

int iommufd_device_invalidate_cache(IOMMUFDDevice *idev,
                                    uint32_t data_type, uint32_t entry_len,
                                    uint32_t *entry_num, void *data_ptr)
{
    int ret, fd = idev->iommufd->fd;
    struct iommu_dev_invalidate cache = {
        .size = sizeof(cache),
        .dev_id = idev->dev_id,
        .data_type = data_type,
        .entry_len = entry_len,
        .data_uptr = (uint64_t)data_ptr,
    };

    cache.entry_num = *entry_num;
    ret = ioctl(fd, IOMMU_DEV_INVALIDATE, &cache);

    trace_iommufd_device_invalidate_cache(fd, idev->dev_id, data_type, entry_len,
                                          *entry_num, cache.entry_num,
                                          (uint64_t)data_ptr, ret);
    if (ret) {
        ret = -errno;
        error_report("IOMMU_DEV_INVALIDATE failed: %s", strerror(errno));
    } else {
        *entry_num = cache.entry_num;
    }

    return ret;
}

void iommufd_device_init(void *_idev, size_t instance_size,
                         IOMMUFDBackend *iommufd, uint32_t dev_id,
                         uint32_t ioas_id, IOMMUFDDeviceOps *ops)
{
    IOMMUFDDevice *idev = (IOMMUFDDevice *)_idev;

    g_assert(sizeof(IOMMUFDDevice) <= instance_size);

    idev->iommufd = iommufd;
    idev->dev_id = dev_id;
    idev->ioas_id = ioas_id;
    idev->ops = ops;
}
