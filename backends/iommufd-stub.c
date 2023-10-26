/*
 * iommufd container backend stub
 *
 * Copyright (C) 2023 Intel Corporation.
 * Copyright Red Hat, Inc. 2023
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
#include "sysemu/iommufd.h"
#include "qemu/error-report.h"

int iommufd_backend_connect(IOMMUFDBackend *be, Error **errp)
{
    return 0;
}
void iommufd_backend_disconnect(IOMMUFDBackend *be)
{
}
void iommufd_backend_free_id(int fd, uint32_t id)
{
}
int iommufd_backend_get_ioas(IOMMUFDBackend *be, uint32_t *ioas_id)
{
    return 0;
}
void iommufd_backend_put_ioas(IOMMUFDBackend *be, uint32_t ioas_id)
{
}
int iommufd_backend_map_dma(IOMMUFDBackend *be, uint32_t ioas_id, hwaddr iova,
                            ram_addr_t size, void *vaddr, bool readonly)
{
    return 0;
}
int iommufd_backend_unmap_dma(IOMMUFDBackend *be, uint32_t ioas_id,
                              hwaddr iova, ram_addr_t size)
{
    return 0;
}
int iommufd_backend_alloc_hwpt(int iommufd, uint32_t dev_id,
                               uint32_t pt_id, uint32_t *out_hwpt)
{
    return 0;
}
