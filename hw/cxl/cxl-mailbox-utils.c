/*
 * CXL Utility library for mailbox interface
 *
 * Copyright(C) 2020 Intel Corporation.
 *
 * This work is licensed under the terms of the GNU GPL, version 2. See the
 * COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "hw/cxl/cxl.h"
#include "hw/pci/pci.h"
#include "qemu/log.h"
#include "qemu/uuid.h"

/*
 * How to add a new command, example. The command set FOO, with cmd BAR.
 *  1. Add the command set and cmd to the enum.
 *     FOO    = 0x7f,
 *          #define BAR 0
 *  2. Forward declare the handler.
 *     declare_mailbox_handler(FOO_BAR);
 *  3. Add the command to the cxl_cmd_set[][]
 *     CXL_CMD(FOO, BAR, 0, 0),
 *  4. Implement your handler
 *     define_mailbox_handler(FOO_BAR) { ... return CXL_MBOX_SUCCESS; }
 *
 *
 *  Writing the handler:
 *    The handler will provide the &struct cxl_cmd, the &CXLDeviceState, and the
 *    in/out length of the payload. The handler is responsible for consuming the
 *    payload from cmd->payload and operating upon it as necessary. It must then
 *    fill the output data into cmd->payload (overwriting what was there),
 *    setting the length, and returning a valid return code.
 *
 *  XXX: The handler need not worry about endianess. The payload is read out of
 *  a register interface that already deals with it.
 */

enum {
    EVENTS      = 0x01,
        #define GET_RECORDS   0x0
        #define CLEAR_RECORDS   0x1
        #define GET_INTERRUPT_POLICY   0x2
        #define SET_INTERRUPT_POLICY   0x3
};

/* 8.2.8.4.5.1 Command Return Codes */
typedef enum {
    CXL_MBOX_SUCCESS = 0x0,
    CXL_MBOX_BG_STARTED = 0x1,
    CXL_MBOX_INVALID_INPUT = 0x2,
    CXL_MBOX_UNSUPPORTED = 0x3,
    CXL_MBOX_INTERNAL_ERROR = 0x4,
    CXL_MBOX_RETRY_REQUIRED = 0x5,
    CXL_MBOX_BUSY = 0x6,
    CXL_MBOX_MEDIA_DISABLED = 0x7,
    CXL_MBOX_FW_XFER_IN_PROGRESS = 0x8,
    CXL_MBOX_FW_XFER_OUT_OF_ORDER = 0x9,
    CXL_MBOX_FW_AUTH_FAILED = 0xa,
    CXL_MBOX_FW_INVALID_SLOT = 0xb,
    CXL_MBOX_FW_ROLLEDBACK = 0xc,
    CXL_MBOX_FW_REST_REQD = 0xd,
    CXL_MBOX_INVALID_HANDLE = 0xe,
    CXL_MBOX_INVALID_PA = 0xf,
    CXL_MBOX_INJECT_POISON_LIMIT = 0x10,
    CXL_MBOX_PERMANENT_MEDIA_FAILURE = 0x11,
    CXL_MBOX_ABORTED = 0x12,
    CXL_MBOX_INVALID_SECURITY_STATE = 0x13,
    CXL_MBOX_INCORRECT_PASSPHRASE = 0x14,
    CXL_MBOX_UNSUPPORTED_MAILBOX = 0x15,
    CXL_MBOX_INVALID_PAYLOAD_LENGTH = 0x16,
    CXL_MBOX_MAX = 0x17
} ret_code;

struct cxl_cmd;
typedef ret_code (*opcode_handler)(struct cxl_cmd *cmd,
                                   CXLDeviceState *cxl_dstate, uint16_t *len);
struct cxl_cmd {
    const char *name;
    opcode_handler handler;
    ssize_t in;
    uint16_t effect; /* Reported in CEL */
    uint8_t *payload;
};

#define define_mailbox_handler(name)                \
    static ret_code cmd_##name(struct cxl_cmd *cmd, \
                               CXLDeviceState *cxl_dstate, uint16_t *len)
#define declare_mailbox_handler(name) define_mailbox_handler(name)

#define define_mailbox_handler_zeroed(name, size)                         \
    uint16_t __zero##name = size;                                         \
    static ret_code cmd_##name(struct cxl_cmd *cmd,                       \
                               CXLDeviceState *cxl_dstate, uint16_t *len) \
    {                                                                     \
        *len = __zero##name;                                              \
        memset(cmd->payload, 0, *len);                                    \
        return CXL_MBOX_SUCCESS;                                          \
    }
#define define_mailbox_handler_const(name, data)                          \
    static ret_code cmd_##name(struct cxl_cmd *cmd,                       \
                               CXLDeviceState *cxl_dstate, uint16_t *len) \
    {                                                                     \
        *len = sizeof(data);                                              \
        memcpy(cmd->payload, data, *len);                                 \
        return CXL_MBOX_SUCCESS;                                          \
    }
#define define_mailbox_handler_nop(name)                                  \
    static ret_code cmd_##name(struct cxl_cmd *cmd,                       \
                               CXLDeviceState *cxl_dstate, uint16_t *len) \
    {                                                                     \
        return CXL_MBOX_SUCCESS;                                          \
    }

define_mailbox_handler_zeroed(EVENTS_GET_RECORDS, 0x20);
define_mailbox_handler_nop(EVENTS_CLEAR_RECORDS);
define_mailbox_handler_zeroed(EVENTS_GET_INTERRUPT_POLICY, 4);
define_mailbox_handler_nop(EVENTS_SET_INTERRUPT_POLICY);

#define IMMEDIATE_CONFIG_CHANGE (1 << 1)
#define IMMEDIATE_LOG_CHANGE (1 << 4)

#define CXL_CMD(s, c, in, cel_effect) \
    [s][c] = { stringify(s##_##c), cmd_##s##_##c, in, cel_effect }

static struct cxl_cmd cxl_cmd_set[256][256] = {
    CXL_CMD(EVENTS, GET_RECORDS, 1, 0),
    CXL_CMD(EVENTS, CLEAR_RECORDS, ~0, IMMEDIATE_LOG_CHANGE),
    CXL_CMD(EVENTS, GET_INTERRUPT_POLICY, 0, 0),
    CXL_CMD(EVENTS, SET_INTERRUPT_POLICY, 4, IMMEDIATE_CONFIG_CHANGE),
};

#undef CXL_CMD

QemuUUID cel_uuid;

void cxl_process_mailbox(CXLDeviceState *cxl_dstate)
{
    uint16_t ret = CXL_MBOX_SUCCESS;
    struct cxl_cmd *cxl_cmd;
    uint64_t status_reg;
    opcode_handler h;

    /*
     * current state of mailbox interface
     *  mbox_cap_reg = cxl_dstate->reg_state32[R_CXL_DEV_MAILBOX_CAP];
     *  mbox_ctrl_reg = cxl_dstate->reg_state32[R_CXL_DEV_MAILBOX_CTRL];
     *  status_reg = *(uint64_t *)&cxl_dstate->reg_state[A_CXL_DEV_MAILBOX_STS];
     */
    uint64_t command_reg =
        *(uint64_t *)&cxl_dstate->mbox_reg_state[A_CXL_DEV_MAILBOX_CMD];

    /* Check if we have to do anything */
    if (!ARRAY_FIELD_EX32(cxl_dstate->mbox_reg_state32, CXL_DEV_MAILBOX_CTRL,
                          DOORBELL)) {
        qemu_log_mask(LOG_UNIMP, "Corrupt internal state for firmware\n");
        return;
    }

    uint8_t set = FIELD_EX64(command_reg, CXL_DEV_MAILBOX_CMD, COMMAND_SET);
    uint8_t cmd = FIELD_EX64(command_reg, CXL_DEV_MAILBOX_CMD, COMMAND);
    uint16_t len = FIELD_EX64(command_reg, CXL_DEV_MAILBOX_CMD, LENGTH);
    cxl_cmd = &cxl_cmd_set[set][cmd];
    h = cxl_cmd->handler;
    if (!h) {
        qemu_log_mask(LOG_UNIMP, "Command %04xh not implemented\n",
                                 set << 8 | cmd);
        goto handled;
    }

    if (len != cxl_cmd->in) {
        ret = CXL_MBOX_INVALID_PAYLOAD_LENGTH;
    }

    cxl_cmd->payload = cxl_dstate->mbox_reg_state + A_CXL_DEV_CMD_PAYLOAD;
    ret = (*h)(cxl_cmd, cxl_dstate, &len);
    assert(len <= cxl_dstate->payload_size);

handled:
    /*
     * Set the return code
     * XXX: it's a 64b register, but we're not setting the vendor, so we can get
     * away with this
     */
    status_reg = FIELD_DP64(0, CXL_DEV_MAILBOX_STS, ERRNO, ret);

    /*
     * Set the return length
     */
    command_reg = FIELD_DP64(command_reg, CXL_DEV_MAILBOX_CMD, COMMAND_SET, 0);
    command_reg = FIELD_DP64(command_reg, CXL_DEV_MAILBOX_CMD, COMMAND, 0);
    command_reg = FIELD_DP64(command_reg, CXL_DEV_MAILBOX_CMD, LENGTH, len);

    cxl_dstate->mbox_reg_state64[A_CXL_DEV_MAILBOX_CMD / 8] = command_reg;
    cxl_dstate->mbox_reg_state64[A_CXL_DEV_MAILBOX_STS / 8] = status_reg;

    /* Tell the host we're done */
    ARRAY_FIELD_DP32(cxl_dstate->mbox_reg_state32, CXL_DEV_MAILBOX_CTRL,
                     DOORBELL, 0);
}

int cxl_initialize_mailbox(CXLDeviceState *cxl_dstate)
{
    const char *cel_uuidstr = "0da9c0b5-bf41-4b78-8f79-96b1623b3f17";

    for (int set = 0; set < 256; set++) {
        for (int cmd = 0; cmd < 256; cmd++) {
            if (cxl_cmd_set[set][cmd].handler) {
                struct cxl_cmd *c = &cxl_cmd_set[set][cmd];
                struct cel_log *log =
                    &cxl_dstate->cel_log[cxl_dstate->cel_size];

                log->opcode = (set << 8) | cmd;
                log->effect = c->effect;
                cxl_dstate->cel_size++;
            }
        }
    }

    return qemu_uuid_parse(cel_uuidstr, &cel_uuid);
}
