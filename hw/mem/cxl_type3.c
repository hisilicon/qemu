#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/error-report.h"
#include "hw/mem/memory-device.h"
#include "hw/mem/pc-dimm.h"
#include "hw/pci/pci.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/range.h"
#include "qemu/rcu.h"
#include "sysemu/hostmem.h"
#include "hw/cxl/cxl.h"

typedef struct cxl_type3_dev {
    /* Private */
    PCIDevice parent_obj;

    /* Properties */
    uint64_t size;
    HostMemoryBackend *hostmem;

    /* State */
    CXLComponentState cxl_cstate;
    CXLDeviceState cxl_dstate;
} CXLType3Dev;

#define CT3(obj) OBJECT_CHECK(CXLType3Dev, (obj), TYPE_CXL_TYPE3_DEV)

static void build_dvsecs(CXLType3Dev *ct3d)
{
    CXLComponentState *cxl_cstate = &ct3d->cxl_cstate;
    uint8_t *dvsec;

    dvsec = (uint8_t *)&(struct cxl_dvsec_device){
        .cap = 0x1e,
        .ctrl = 0x6,
        .status2 = 0x2,
        .range1_size_hi = 0,
#ifdef SET_PMEM_PADDR
        .range1_size_lo = (2 << 5) | (2 << 2) | 0x3 | ct3d->size,
#else
        .range1_size_lo = 0x3,
#endif
        .range1_base_hi = 0,
        .range1_base_lo = 0,
    };
    cxl_component_create_dvsec(cxl_cstate, PCIE_CXL_DEVICE_DVSEC_LENGTH,
                               PCIE_CXL_DEVICE_DVSEC,
                               PCIE_CXL2_DEVICE_DVSEC_REVID, dvsec);

    dvsec = (uint8_t *)&(struct cxl_dvsec_register_locator){
        .rsvd         = 0,
        .reg0_base_lo = RBI_COMPONENT_REG | CXL_COMPONENT_REG_BAR_IDX,
        .reg0_base_hi = 0,
        .reg1_base_lo = RBI_CXL_DEVICE_REG | CXL_DEVICE_REG_BAR_IDX,
        .reg1_base_hi = 0,
    };
    cxl_component_create_dvsec(cxl_cstate, REG_LOC_DVSEC_LENGTH, REG_LOC_DVSEC,
                               REG_LOC_DVSEC_REVID, dvsec);
}

static void cxl_setup_memory(CXLType3Dev *ct3d, Error **errp)
{
    MemoryRegion *mr;

    if (!ct3d->hostmem) {
        error_setg(errp, "memdev property must be set");
        return;
    }

    mr = host_memory_backend_get_memory(ct3d->hostmem);
    if (!mr) {
        error_setg(errp, "memdev property must be set");
        return;
    }
    memory_region_set_nonvolatile(mr, true);
    memory_region_set_enabled(mr, true);
    host_memory_backend_set_mapped(ct3d->hostmem, true);
    ct3d->cxl_dstate.pmem_size = ct3d->hostmem->size;
}


static void ct3_realize(PCIDevice *pci_dev, Error **errp)
{
    CXLType3Dev *ct3d = CT3(pci_dev);
    CXLComponentState *cxl_cstate = &ct3d->cxl_cstate;
    ComponentRegisters *regs = &cxl_cstate->crb;
    MemoryRegion *mr = &regs->component_registers;
    uint8_t *pci_conf = pci_dev->config;

    if (!ct3d->hostmem) {
        cxl_setup_memory(ct3d, errp);
    }

    pci_config_set_prog_interface(pci_conf, 0x10);
    pci_config_set_class(pci_conf, PCI_CLASS_MEMORY_CXL);

    pcie_endpoint_cap_init(pci_dev, 0x80);
    cxl_cstate->dvsec_offset = 0x100;

    ct3d->cxl_cstate.pdev = pci_dev;
    build_dvsecs(ct3d);

    cxl_component_register_block_init(OBJECT(pci_dev), cxl_cstate,
                                      TYPE_CXL_TYPE3_DEV);

    pci_register_bar(
        pci_dev, CXL_COMPONENT_REG_BAR_IDX,
        PCI_BASE_ADDRESS_SPACE_MEMORY | PCI_BASE_ADDRESS_MEM_TYPE_64, mr);

    cxl_device_register_block_init(OBJECT(pci_dev), &ct3d->cxl_dstate);
    pci_register_bar(pci_dev, CXL_DEVICE_REG_BAR_IDX,
                     PCI_BASE_ADDRESS_SPACE_MEMORY |
                         PCI_BASE_ADDRESS_MEM_TYPE_64,
                     &ct3d->cxl_dstate.device_registers);
}

static void ct3d_reset(DeviceState *dev)
{
    CXLType3Dev *ct3d = CT3(dev);
    uint32_t *reg_state = ct3d->cxl_cstate.crb.cache_mem_registers;

    cxl_component_register_init_common(reg_state, CXL2_TYPE3_DEVICE);
    cxl_device_register_init_common(&ct3d->cxl_dstate);
}

static Property ct3_props[] = {
    DEFINE_PROP_SIZE("size", CXLType3Dev, size, -1),
    DEFINE_PROP_LINK("memdev", CXLType3Dev, hostmem, TYPE_MEMORY_BACKEND,
                     HostMemoryBackend *),
    DEFINE_PROP_END_OF_LIST(),
};

static void ct3_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(oc);

    pc->realize = ct3_realize;
    pc->class_id = PCI_CLASS_STORAGE_EXPRESS;
    pc->vendor_id = PCI_VENDOR_ID_INTEL;
    pc->device_id = 0xd93; /* LVF for now */
    pc->revision = 1;

    set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
    dc->desc = "CXL PMEM Device (Type 3)";
    dc->reset = ct3d_reset;
    device_class_set_props(dc, ct3_props);
}

static const TypeInfo ct3d_info = {
    .name = TYPE_CXL_TYPE3_DEV,
    .parent = TYPE_PCI_DEVICE,
    .class_init = ct3_class_init,
    .instance_size = sizeof(CXLType3Dev),
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CXL_DEVICE },
        { INTERFACE_PCIE_DEVICE },
        {}
    },
};

static void ct3d_registers(void)
{
    type_register_static(&ct3d_info);
}

type_init(ct3d_registers);
