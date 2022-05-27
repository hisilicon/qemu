/*
 * IOMMU Device
 *
 * Copyright (C) 2022 Intel Corporation.
 *
 * Authors: Yi Liu <yi.l.liu@intel.com>
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

#ifndef HW_IOMMU_DEVICE_H
#define HW_IOMMU_DEVICE_H

#include "qemu/queue.h"
#include "qemu/thread.h"
#include "qom/object.h"
#include <linux/iommu.h>
#include <linux/iommufd.h>
#ifndef CONFIG_USER_ONLY
#include "exec/hwaddr.h"
#endif

#define TYPE_IOMMU_DEVICE "qemu:iommu-device"
#define IOMMU_DEVICE(obj) \
        OBJECT_CHECK(IOMMUFDDevice, (obj), TYPE_IOMMU_DEVICE)
#define IOMMU_DEVICE_GET_CLASS(obj) \
        OBJECT_GET_CLASS(IOMMUFDDeviceClass, (obj), \
                         TYPE_IOMMU_DEVICE)
#define IOMMU_DEVICE_CLASS(klass) \
        OBJECT_CLASS_CHECK(IOMMUFDDeviceClass, (klass), \
                           TYPE_IOMMU_DEVICE)

typedef struct IOMMUFDDevice IOMMUFDDevice;

typedef struct IOMMUFDDeviceClass {
    /* private */
    ObjectClass parent_class;

    int (*attach_stage1)(IOMMUFDDevice *idev, uint32_t *pasid,
                         uint32_t hwpt_id);
    int (*detach_stage1)(IOMMUFDDevice *idev, uint32_t hwpt_id);
} IOMMUFDDeviceClass;

/*
 * This is an abstraction of host IOMMU with dual-stage capability
 */
struct IOMMUFDDevice {
    Object parent_obj;
    int iommufd;
    uint32_t dev_id;
    uint32_t hwpt_id;
    bool initialized;
};

int iommu_device_attach_stage1(IOMMUFDDevice *idev, uint32_t *pasid,
                               uint32_t hwpt_id);
int iommu_device_detach_stage1(IOMMUFDDevice *idev, uint32_t hwpt_id);
int iommu_device_get_info(IOMMUFDDevice *idev,
                          enum iommu_hw_type *type,
                          uint32_t len, void *data);
void iommu_device_init(void *_idev, size_t instance_size,
                       const char *mrtypename, int fd,
                       uint32_t dev_id, uint32_t s2_hwpt_id);
void iommu_device_destroy(IOMMUFDDevice *idev);

#endif
