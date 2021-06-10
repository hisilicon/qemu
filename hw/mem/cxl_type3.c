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
#include "qemu/pmem.h"
#include "qemu/range.h"
#include "qemu/rcu.h"
#include "sysemu/hostmem.h"
#include "hw/cxl/cxl.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"

#define DWORD_BYTE 4

bool cxl_doe_compliance_rsp(DOECap *doe_cap)
{
    CompRsp *rsp = &CT3(doe_cap->pdev)->cxl_cstate.compliance.response;
    struct compliance_req_header *req = pcie_doe_get_write_mbox_ptr(doe_cap);
    uint32_t type, req_len = 0, rsp_len = 0;

    type = req->req_code;

    switch (type) {
    case CXL_COMP_MODE_CAP:
        req_len = sizeof(struct cxl_compliance_cap_req);
        rsp_len = sizeof(struct cxl_compliance_cap_rsp);
        rsp->cap_rsp.status = 0x0;
        rsp->cap_rsp.available_cap_bitmask = 0;
        rsp->cap_rsp.enabled_cap_bitmask = 0;
        break;
    case CXL_COMP_MODE_STATUS:
        req_len = sizeof(struct cxl_compliance_status_req);
        rsp_len = sizeof(struct cxl_compliance_status_rsp);
        rsp->status_rsp.cap_bitfield = 0;
        rsp->status_rsp.cache_size = 0;
        rsp->status_rsp.cache_size_units = 0;
        break;
    case CXL_COMP_MODE_HALT:
        req_len = sizeof(struct cxl_compliance_halt_req);
        rsp_len = sizeof(struct cxl_compliance_halt_rsp);
        break;
    case CXL_COMP_MODE_MULT_WR_STREAM:
        req_len = sizeof(struct cxl_compliance_multi_write_streaming_req);
        rsp_len = sizeof(struct cxl_compliance_multi_write_streaming_rsp);
        break;
    case CXL_COMP_MODE_PRO_CON:
        req_len = sizeof(struct cxl_compliance_producer_consumer_req);
        rsp_len = sizeof(struct cxl_compliance_producer_consumer_rsp);
        break;
    case CXL_COMP_MODE_BOGUS:
        req_len = sizeof(struct cxl_compliance_bogus_writes_req);
        rsp_len = sizeof(struct cxl_compliance_bogus_writes_rsp);
        break;
    case CXL_COMP_MODE_INJ_POISON:
        req_len = sizeof(struct cxl_compliance_inject_poison_req);
        rsp_len = sizeof(struct cxl_compliance_inject_poison_rsp);
        break;
    case CXL_COMP_MODE_INJ_CRC:
        req_len = sizeof(struct cxl_compliance_inject_crc_req);
        rsp_len = sizeof(struct cxl_compliance_inject_crc_rsp);
        break;
    case CXL_COMP_MODE_INJ_FC:
        req_len = sizeof(struct cxl_compliance_inject_flow_ctrl_req);
        rsp_len = sizeof(struct cxl_compliance_inject_flow_ctrl_rsp);
        break;
    case CXL_COMP_MODE_TOGGLE_CACHE:
        req_len = sizeof(struct cxl_compliance_toggle_cache_flush_req);
        rsp_len = sizeof(struct cxl_compliance_toggle_cache_flush_rsp);
        break;
    case CXL_COMP_MODE_INJ_MAC:
        req_len = sizeof(struct cxl_compliance_inject_mac_delay_req);
        rsp_len = sizeof(struct cxl_compliance_inject_mac_delay_rsp);
        break;
    case CXL_COMP_MODE_INS_UNEXP_MAC:
        req_len = sizeof(struct cxl_compliance_insert_unexp_mac_req);
        rsp_len = sizeof(struct cxl_compliance_insert_unexp_mac_rsp);
        break;
    case CXL_COMP_MODE_INJ_VIRAL:
        req_len = sizeof(struct cxl_compliance_inject_viral_req);
        rsp_len = sizeof(struct cxl_compliance_inject_viral_rsp);
        break;
    case CXL_COMP_MODE_INJ_ALMP:
        req_len = sizeof(struct cxl_compliance_inject_almp_req);
        rsp_len = sizeof(struct cxl_compliance_inject_almp_rsp);
        break;
    case CXL_COMP_MODE_IGN_ALMP:
        req_len = sizeof(struct cxl_compliance_ignore_almp_req);
        rsp_len = sizeof(struct cxl_compliance_ignore_almp_rsp);
        break;
    case CXL_COMP_MODE_INJ_BIT_ERR:
        req_len = sizeof(struct cxl_compliance_inject_bit_err_in_flit_req);
        rsp_len = sizeof(struct cxl_compliance_inject_bit_err_in_flit_rsp);
        break;
    default:
        break;
    }

    /* Discard if request length mismatched */
    if (pcie_doe_get_obj_len(req) < DIV_ROUND_UP(req_len, DWORD_BYTE)) {
        return false;
    }

    /* Common fields for each compliance type */
    rsp->header.doe_header.vendor_id = CXL_VENDOR_ID;
    rsp->header.doe_header.data_obj_type = CXL_DOE_COMPLIANCE;
    rsp->header.doe_header.length = DIV_ROUND_UP(rsp_len, DWORD_BYTE);
    rsp->header.rsp_code = type;
    rsp->header.version = 0x1;
    rsp->header.length = rsp_len;

    memcpy(doe_cap->read_mbox, rsp, rsp_len);

    doe_cap->read_mbox_len += rsp->header.doe_header.length;

    return true;
}

static uint32_t ct3d_config_read(PCIDevice *pci_dev, uint32_t addr, int size)
{
    CXLType3Dev *ct3d = CT3(pci_dev);
    uint32_t val;

    if (pcie_doe_read_config(&ct3d->doe_comp, addr, size, &val)) {
        return val;
    }

    return pci_default_read_config(pci_dev, addr, size);
}

static void ct3d_config_write(PCIDevice *pci_dev, uint32_t addr, uint32_t val,
                              int size)
{
    CXLType3Dev *ct3d = CT3(pci_dev);

    pcie_doe_write_config(&ct3d->doe_comp, addr, val, size);
    pci_default_write_config(pci_dev, addr, val, size);
}

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

static void cxl_set_addr(CXLType3Dev *ct3d, hwaddr addr, Error **errp)
{
    MemoryDeviceClass *mdc = MEMORY_DEVICE_GET_CLASS(ct3d);
    mdc->set_addr(MEMORY_DEVICE(ct3d), addr, errp);
}

static void hdm_decoder_commit(CXLType3Dev *ct3d, int which)
{
    MemoryRegion *pmem = ct3d->cxl_dstate.pmem;
    MemoryRegion *mr = host_memory_backend_get_memory(ct3d->hostmem);
    Range window, device;
    ComponentRegisters *cregs = &ct3d->cxl_cstate.crb;
    uint32_t *cache_mem = cregs->cache_mem_registers;
    uint64_t offset, size;
    Error *err = NULL;

    assert(which == 0);

    ARRAY_FIELD_DP32(cache_mem, CXL_HDM_DECODER0_CTRL, COMMIT, 0);
    ARRAY_FIELD_DP32(cache_mem, CXL_HDM_DECODER0_CTRL, ERROR, 0);

    offset = ((uint64_t)cache_mem[R_CXL_HDM_DECODER0_BASE_HI] << 32) |
             cache_mem[R_CXL_HDM_DECODER0_BASE_LO];
    size = ((uint64_t)cache_mem[R_CXL_HDM_DECODER0_SIZE_HI] << 32) |
           cache_mem[R_CXL_HDM_DECODER0_SIZE_LO];

    range_init_nofail(&window, mr->addr, memory_region_size(mr));
    range_init_nofail(&device, offset, size);

    if (!range_contains_range(&window, &device)) {
        ARRAY_FIELD_DP32(cache_mem, CXL_HDM_DECODER0_CTRL, ERROR, 1);
        return;
    }

    /*
     * FIXME: Support resizing.
     * Maybe just memory_region_ram_resize(pmem, size, &err)?
     */
    if (size != memory_region_size(pmem)) {
        ARRAY_FIELD_DP32(cache_mem, CXL_HDM_DECODER0_CTRL, ERROR, 1);
        return;
    }

    cxl_set_addr(ct3d, offset, &err);
    if (err) {
        ARRAY_FIELD_DP32(cache_mem, CXL_HDM_DECODER0_CTRL, ERROR, 1);
        return;
    }
    memory_region_set_enabled(pmem, true);

    ARRAY_FIELD_DP32(cache_mem, CXL_HDM_DECODER0_CTRL, COMMITTED, 1);
}

static void ct3d_reg_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    CXLComponentState *cxl_cstate = opaque;
    ComponentRegisters *cregs = &cxl_cstate->crb;
    CXLType3Dev *ct3d = container_of(cxl_cstate, CXLType3Dev, cxl_cstate);
    uint32_t *cache_mem = cregs->cache_mem_registers;
    bool should_commit = false;
    int which_hdm = -1;

    assert(size == 4);

    switch (offset) {
    case A_CXL_HDM_DECODER0_CTRL:
        should_commit = FIELD_EX32(value, CXL_HDM_DECODER0_CTRL, COMMIT);
        which_hdm = 0;
        break;
    default:
        break;
    }

    stl_le_p((uint8_t *)cache_mem + offset, value);
    if (should_commit)
        hdm_decoder_commit(ct3d, which_hdm);
}

static void ct3_instance_init(Object *obj)
{
    /* MemoryDeviceClass *mdc = MEMORY_DEVICE_GET_CLASS(obj); */
}

static void ct3_finalize(Object *obj)
{
    CXLType3Dev *ct3d = CT3(obj);
    CXLComponentState *cxl_cstate = &ct3d->cxl_cstate;
    ComponentRegisters *regs = &cxl_cstate->crb;

    g_free((void *)regs->special_ops);
    g_free(ct3d->cxl_dstate.pmem);
}

static void cxl_setup_memory(CXLType3Dev *ct3d, Error **errp)
{
    MemoryRegionSection mrs;
    MemoryRegion *pmem;
    MemoryRegion *mr;
    uint64_t offset = 0;
    size_t remaining_size;

    if (!ct3d->hostmem) {
        error_setg(errp, "memdev property must be set");
        return;
    }

    if (!ct3d->lsa) {
        error_setg(errp, "lsa property must be set");
        return;
    }

    /* FIXME: need to check mr is the host bridge's MR */
    mr = host_memory_backend_get_memory(ct3d->hostmem);

    /* Create our new subregion */
    pmem = g_new(MemoryRegion, 1);

    /* Find the first free space in the window */
    WITH_RCU_READ_LOCK_GUARD()
    {
        mrs = memory_region_find(mr, offset, 1);
        while (mrs.mr && mrs.mr != mr) {
            offset += memory_region_size(mrs.mr);
            mrs = memory_region_find(mr, offset, 1);
        }
    }

    remaining_size = memory_region_size(mr) - offset;
    if (remaining_size < ct3d->size) {
        g_free(pmem);
        error_setg(errp,
                   "Not enough free space (%zd) required for device (%" PRId64  ")",
                   remaining_size, ct3d->size);
    }

    memory_region_set_nonvolatile(pmem, true);
    memory_region_set_enabled(pmem, false);
    memory_region_init_alias(pmem, OBJECT(ct3d), "cxl_type3-memory", mr, 0,
                             ct3d->size);
    ct3d->cxl_dstate.pmem = pmem;

#ifdef SET_PMEM_PADDR
    /* This path will initialize the memory device as if BIOS had done it */
    cxl_set_addr(ct3d, mr->addr + offset, errp);
    memory_region_set_enabled(pmem, true);
#endif
}

static MemoryRegion *cxl_md_get_memory_region(MemoryDeviceState *md,
                                              Error **errp)
{
    CXLType3Dev *ct3d = CT3(md);

    if (!ct3d->cxl_dstate.pmem) {
        cxl_setup_memory(ct3d, errp);
    }

    return ct3d->cxl_dstate.pmem;
}

static DOEProtocol doe_comp_prot[] = {
    {CXL_VENDOR_ID, CXL_DOE_COMPLIANCE, cxl_doe_compliance_rsp},
    {},
};

static void ct3_realize(PCIDevice *pci_dev, Error **errp)
{
    CXLType3Dev *ct3d = CT3(pci_dev);
    CXLComponentState *cxl_cstate = &ct3d->cxl_cstate;
    ComponentRegisters *regs = &cxl_cstate->crb;
    MemoryRegion *mr = &regs->component_registers;
    uint8_t *pci_conf = pci_dev->config;
    unsigned short msix_num = 1;
    int i;

    if (!ct3d->cxl_dstate.pmem) {
        cxl_setup_memory(ct3d, errp);
    }

    pci_config_set_prog_interface(pci_conf, 0x10);
    pci_config_set_class(pci_conf, PCI_CLASS_MEMORY_CXL);

    pcie_endpoint_cap_init(pci_dev, 0x80);
    cxl_cstate->dvsec_offset = 0x100;

    ct3d->cxl_cstate.pdev = pci_dev;
    build_dvsecs(ct3d);

    regs->special_ops = g_new0(MemoryRegionOps, 1);
    regs->special_ops->write = ct3d_reg_write;

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

    /* MSI(-X) Initailization */
    msix_init_exclusive_bar(pci_dev, msix_num, 4, NULL);
    for (i = 0; i < msix_num; i++) {
        msix_vector_use(pci_dev, i);
    }

    /* DOE Initailization */
    pcie_doe_init(pci_dev, &ct3d->doe_comp, 0x160, doe_comp_prot, true, 0);
}

static uint64_t cxl_md_get_addr(const MemoryDeviceState *md)
{
    CXLType3Dev *ct3d = CT3(md);
    MemoryRegion *pmem = ct3d->cxl_dstate.pmem;

    assert(pmem->alias);
    return pmem->alias_offset;
}

static void cxl_md_set_addr(MemoryDeviceState *md, uint64_t addr, Error **errp)
{
    CXLType3Dev *ct3d = CT3(md);
    MemoryRegion *pmem = ct3d->cxl_dstate.pmem;

    assert(pmem->alias);
    memory_region_set_alias_offset(pmem, addr);
    memory_region_set_address(pmem, addr);
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
    DEFINE_PROP_LINK("lsa", CXLType3Dev, lsa, TYPE_MEMORY_BACKEND,
                     HostMemoryBackend *),
    DEFINE_PROP_END_OF_LIST(),
};

static void pc_dimm_md_fill_device_info(const MemoryDeviceState *md,
                                        MemoryDeviceInfo *info)
{
    PCDIMMDeviceInfo *di = g_new0(PCDIMMDeviceInfo, 1);
    const DeviceClass *dc = DEVICE_GET_CLASS(md);
    const DeviceState *dev = DEVICE(md);
    CXLType3Dev *ct3d = CT3(md);

    if (dev->id) {
        di->has_id = true;
        di->id = g_strdup(dev->id);
    }

    di->hotplugged = dev->hotplugged;
    di->hotpluggable = dc->hotpluggable;
    di->addr = cxl_md_get_addr(md);
    di->slot = 0;
    di->node = 0;
    di->size = memory_device_get_region_size(md, NULL);
    di->memdev = object_get_canonical_path(OBJECT(ct3d->hostmem));

    info->u.cxl.data = di;
    info->type = MEMORY_DEVICE_INFO_KIND_CXL;
}

static uint64_t get_lsa_size(CXLType3Dev *ct3d)
{
    MemoryRegion *mr;

    mr = host_memory_backend_get_memory(ct3d->lsa);
    return memory_region_size(mr);
}

static void validate_lsa_access(MemoryRegion *mr, uint64_t size,
                                uint64_t offset)
{
    assert(offset + size <= memory_region_size(mr));
    assert(offset + size > offset);
}

static uint64_t get_lsa(CXLType3Dev *ct3d, void *buf, uint64_t size,
                    uint64_t offset)
{
    MemoryRegion *mr;
    void *lsa;

    mr = host_memory_backend_get_memory(ct3d->lsa);
    validate_lsa_access(mr, size, offset);

    lsa = memory_region_get_ram_ptr(mr) + offset;
    memcpy(buf, lsa, size);

    return size;
}

static void set_lsa(CXLType3Dev *ct3d, const void *buf, uint64_t size,
                    uint64_t offset)
{
    MemoryRegion *mr;
    void *lsa;

    mr = host_memory_backend_get_memory(ct3d->lsa);
    validate_lsa_access(mr, size, offset);

    lsa = memory_region_get_ram_ptr(mr) + offset;
    memcpy(lsa, buf, size);
    memory_region_set_dirty(mr, offset, size);

    /*
     * Just like the PMEM, if the guest is not allowed to exit gracefully, label
     * updates will get lost.
     */
}

static void ct3_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(oc);
    MemoryDeviceClass *mdc = MEMORY_DEVICE_CLASS(oc);

    pc->config_write = ct3d_config_write;
    pc->config_read = ct3d_config_read;
    CXLType3Class *cvc = CXL_TYPE3_DEV_CLASS(oc);

    pc->realize = ct3_realize;
    pc->class_id = PCI_CLASS_STORAGE_EXPRESS;
    pc->vendor_id = PCI_VENDOR_ID_INTEL;
    pc->device_id = 0xd93; /* LVF for now */
    pc->revision = 1;

    set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
    dc->desc = "CXL PMEM Device (Type 3)";
    dc->reset = ct3d_reset;
    device_class_set_props(dc, ct3_props);

    mdc->get_memory_region = cxl_md_get_memory_region;
    mdc->get_addr = cxl_md_get_addr;
    mdc->fill_device_info = pc_dimm_md_fill_device_info;
    mdc->get_plugged_size = memory_device_get_region_size;
    mdc->set_addr = cxl_md_set_addr;

    cvc->get_lsa_size = get_lsa_size;
    cvc->get_lsa = get_lsa;
    cvc->set_lsa = set_lsa;
}

static const TypeInfo ct3d_info = {
    .name = TYPE_CXL_TYPE3_DEV,
    .parent = TYPE_PCI_DEVICE,
    .class_size = sizeof(struct CXLType3Class),
    .class_init = ct3_class_init,
    .instance_size = sizeof(CXLType3Dev),
    .instance_init = ct3_instance_init,
    .instance_finalize = ct3_finalize,
    .interfaces = (InterfaceInfo[]) {
        { TYPE_MEMORY_DEVICE },
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
