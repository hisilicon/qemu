/*
 * QEMU IOMMUFD
 *
 * Copyright (C) 2022 Intel Corporation.
 * Copyright Red Hat, Inc. 2022
 *
 * Authors: Yi Liu <yi.l.liu@intel.com>
 *          Eric Auger <eric.auger@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/thread.h"
#include "qemu/module.h"
#include <sys/ioctl.h>
#include <linux/iommufd.h>
#include "hw/iommufd/iommufd.h"
#include "trace.h"

static QemuMutex iommufd_lock;
static uint32_t iommufd_users;
static int iommufd = -1;

int iommufd_get(void)
{
    qemu_mutex_lock(&iommufd_lock);
    if (iommufd == -1) {
        iommufd = qemu_open_old("/dev/iommu", O_RDWR);
        if (iommufd < 0) {
            error_report("Failed to open /dev/iommu!");
        } else {
            iommufd_users = 1;
        }
        trace_iommufd_get(iommufd);
    } else if (++iommufd_users == UINT32_MAX) {
        error_report("Failed to get iommufd: %d, count overflow", iommufd);
        iommufd_users--;
        qemu_mutex_unlock(&iommufd_lock);
        return -E2BIG;
    }
    qemu_mutex_unlock(&iommufd_lock);
    return iommufd;
}

void iommufd_put(int fd)
{
    qemu_mutex_lock(&iommufd_lock);
    if (--iommufd_users) {
        qemu_mutex_unlock(&iommufd_lock);
        return;
    }
    iommufd = -1;
    trace_iommufd_put(fd);
    close(fd);
    qemu_mutex_unlock(&iommufd_lock);
}

static int iommufd_alloc_ioas(int iommufd, uint32_t *ioas)
{
    int ret;
    struct iommu_ioas_alloc alloc_data  = {
        .size = sizeof(alloc_data),
        .flags = 0,
    };

    ret = ioctl(iommufd, IOMMU_IOAS_ALLOC, &alloc_data);
    if (ret) {
        error_report("Failed to allocate ioas %m");
    }

    *ioas = alloc_data.out_ioas_id;
    trace_iommufd_alloc_ioas(iommufd, *ioas, ret);

    return ret;
}

void iommufd_free_id(int iommufd, uint32_t id)
{
    int ret;
    struct iommu_destroy des = {
        .size = sizeof(des),
        .id = id,
    };

    ret = ioctl(iommufd, IOMMU_DESTROY, &des);
    trace_iommufd_free_id(iommufd, id, ret);
    if (ret) {
        error_report("Failed to free id: %u %m", id);
    }
}

int iommufd_get_ioas(int *fd, uint32_t *ioas_id)
{
    int ret;

    *fd = iommufd_get();
    if (*fd < 0) {
        return *fd;
    }

    ret = iommufd_alloc_ioas(*fd, ioas_id);
    trace_iommufd_get_ioas(*fd, *ioas_id, ret);
    if (ret) {
        iommufd_put(*fd);
    }
    return ret;
}

void iommufd_put_ioas(int iommufd, uint32_t ioas)
{
    trace_iommufd_put_ioas(iommufd, ioas);
    iommufd_free_id(iommufd, ioas);
    iommufd_put(iommufd);
}

int iommufd_unmap_dma(int iommufd, uint32_t ioas,
                      hwaddr iova, ram_addr_t size)
{
    int ret;
    struct iommu_ioas_unmap unmap = {
        .size = sizeof(unmap),
        .ioas_id = ioas,
        .iova = iova,
        .length = size,
    };

    ret = ioctl(iommufd, IOMMU_IOAS_UNMAP, &unmap);
    trace_iommufd_unmap_dma(iommufd, ioas, iova, size, ret);
    if (ret) {
        error_report("IOMMU_IOAS_UNMAP failed: %s", strerror(errno));
    }
    return !ret ? 0 : -errno;
}

int iommufd_map_dma(int iommufd, uint32_t ioas, hwaddr iova,
                    ram_addr_t size, void *vaddr, bool readonly)
{
    int ret;
    struct iommu_ioas_map map = {
        .size = sizeof(map),
        .flags = IOMMU_IOAS_MAP_READABLE |
                 IOMMU_IOAS_MAP_FIXED_IOVA,
        .ioas_id = ioas,
        .__reserved = 0,
        .user_va = (int64_t)vaddr,
        .iova = iova,
        .length = size,
    };

    if (!readonly) {
        map.flags |= IOMMU_IOAS_MAP_WRITEABLE;
    }

    ret = ioctl(iommufd, IOMMU_IOAS_MAP, &map);
    trace_iommufd_map_dma(iommufd, ioas, iova, size, vaddr, readonly, ret);
    if (ret) {
        error_report("IOMMU_IOAS_MAP failed: %s", strerror(errno));
    }
    return !ret ? 0 : -errno;
}

int iommufd_copy_dma(int iommufd, uint32_t src_ioas, uint32_t dst_ioas,
                     hwaddr iova, ram_addr_t size, bool readonly)
{
    int ret;
    struct iommu_ioas_copy copy = {
        .size = sizeof(copy),
        .flags = IOMMU_IOAS_MAP_READABLE |
                 IOMMU_IOAS_MAP_FIXED_IOVA,
        .dst_ioas_id = dst_ioas,
        .src_ioas_id = src_ioas,
        .length = size,
        .dst_iova = iova,
        .src_iova = iova,
    };

    if (!readonly) {
        copy.flags |= IOMMU_IOAS_MAP_WRITEABLE;
    }

    ret = ioctl(iommufd, IOMMU_IOAS_COPY, &copy);
    trace_iommufd_copy_dma(iommufd, src_ioas, dst_ioas,
                           iova, size, readonly, ret);
    if (ret) {
        error_report("IOMMU_IOAS_COPY failed: %s", strerror(errno));
    }
    return !ret ? 0 : -errno;
}

int iommufd_alloc_s1_hwpt(int iommufd, uint32_t dev_id,
                          hwaddr s1_ptr, uint32_t s2_hwpt,
                          int fd, union iommu_stage1_config *s1_config,
                          uint32_t *out_s1_hwpt, int *out_fault_fd)
{
    int ret;
    struct iommu_alloc_s1_hwpt hwpt = {
        .size = sizeof(struct iommu_alloc_s1_hwpt),
        .flags = 0,
        .dev_id = dev_id,
        .stage2_hwpt_id = s2_hwpt,
        .eventfd = fd,
        .stage1_config_len = sizeof(*s1_config),
        .stage1_config_uptr = (uint64_t)s1_config,
        .stage1_ptr = s1_ptr,
    };

    ret = ioctl(iommufd, IOMMU_ALLOC_S1_HWPT, &hwpt);
    trace_iommufd_alloc_s1_hwpt(iommufd, dev_id, s1_ptr,
                                s2_hwpt, fd, (uint64_t)s1_config, ret);
    if (ret) {
        error_report("IOMMU_ALLOC_S1_HWPT failed: %s", strerror(errno));
    } else {
        *out_fault_fd = hwpt.out_fault_fd;
        *out_s1_hwpt = hwpt.out_hwpt_id;
    }
    return !ret ? 0 : -errno;
}

int iommufd_alloc_pasid(int iommufd, uint32_t min, uint32_t max,
                        bool identical, uint32_t *pasid)
{
    int ret;
    uint32_t upasid = *pasid;
    struct iommu_alloc_pasid alloc = {
        .size = sizeof(alloc),
        .flags = identical ? IOMMU_ALLOC_PASID_IDENTICAL : 0,
        .range.min = min,
        .range.max = max,
        .pasid = upasid,
    };

    ret = ioctl(iommufd, IOMMU_ALLOC_PASID, &alloc);
    if (ret) {
        error_report("IOMMU_ALLOC_PASID failed: %s", strerror(errno));
    } else {
        *pasid = alloc.pasid;
    }
    trace_iommufd_alloc_pasid(iommufd, min, max,
                              identical, upasid, *pasid, ret);
    return !ret ? 0 : -errno;
}

int iommufd_free_pasid(int iommufd, uint32_t pasid)
{
    int ret;
    struct iommu_free_pasid free = {
        .size = sizeof(free),
        .flags = 0,
        .pasid = pasid,
    };

    ret = ioctl(iommufd, IOMMU_FREE_PASID, &free);
    if (ret) {
        error_report("IOMMU_FREE_PASID failed: %s", strerror(errno));
    }
    trace_iommufd_free_pasid(iommufd, pasid, ret);
    return !ret ? 0 : -errno;
}

int iommufd_invalidate_cache(int iommufd, uint32_t hwpt_id,
                             struct iommu_cache_invalidate_info *info)
{
    int ret;
    struct iommu_hwpt_invalidate_s1_cache cache = {
        .size = sizeof(cache),
        .flags = 0,
        .hwpt_id = hwpt_id,
        .info = *info,
    };

    ret = ioctl(iommufd, IOMMU_HWPT_INVAL_S1_CACHE, &cache);
    if (ret) {
        error_report("IOMMU_HWPT_INVAL_S1_CACHE failed: %s", strerror(errno));
    }
    trace_iommufd_invalidate_cache(iommufd, hwpt_id, ret);
    return !ret ? 0 : -errno;
}

int iommufd_page_response(int iommufd, uint32_t hwpt_id,
                          uint32_t dev_id, struct iommu_page_response *resp)
{
    int ret;
    struct iommu_hwpt_page_response page = {
        .size = sizeof(page),
        .flags = 0,
        .hwpt_id = hwpt_id,
        .dev_id = dev_id,
        .resp = *resp,
    };

    ret = ioctl(iommufd, IOMMU_PAGE_RESPONSE, &page);
    if (ret) {
        error_report("IOMMU_PAGE_RESPONSE failed: %s", strerror(errno));
    }
    trace_iommufd_page_response(iommufd, hwpt_id, dev_id, ret);
    return !ret ? 0 : -errno;
}

static void iommufd_register_types(void)
{
    qemu_mutex_init(&iommufd_lock);
}

type_init(iommufd_register_types)
