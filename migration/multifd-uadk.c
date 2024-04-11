/*
 * Multifd UADK compression accelerator implementation
 *
 * Copyright (c) 2024 Huawei Technologies R & D (UK) Ltd
 *
 * Authors:
 *  Shameer Kolothum <shameerali.kolothum.thodi@huawei.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "migration.h"
#include "multifd.h"
#include "options.h"
#include "uadk/wd_comp.h"
#include "uadk/wd_sched.h"

struct wd_data {
    handle_t handle;
    uint32_t data_size;
    uint8_t *buf;
    uint32_t buf_size;
    uint32_t *buf_hdr;
};

static bool uadk_comp_init_done;

static int uadk_alloc_buf(struct wd_data *wd, uint32_t count,
                          uint32_t page_size)
{
    int flags = MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS;
    int prot = PROT_READ | PROT_WRITE;

    wd->buf = mmap(NULL, count * page_size, prot, flags, -1, 0);
    if (wd->buf == MAP_FAILED) {
        return -ENOMEM;
    }

    wd->buf_hdr = g_new0(uint32_t, count);
    wd->buf_size = count * page_size;
    wd->data_size = page_size;
    return 0;
}

static void uadk_free_buf(struct wd_data *wd)
{
    if (wd->buf) {
        munmap(wd->buf, wd->buf_size);
        wd->buf = NULL;
    }
    g_free(wd->buf_hdr);
}

static struct wd_data *uadk_init_sess(uint32_t count, uint32_t page_size,
                                      bool compress, Error **errp)
{
    struct wd_comp_sess_setup ss = {0};
    struct sched_params param = {0};
    char alg[] = "zlib";
    const char *err_msg;
    struct wd_data *wd;
    handle_t handle;
    int ret;

    if (!uadk_comp_init_done) {
        ret = wd_comp_init2(alg, SCHED_POLICY_RR, TASK_HW);
        if (ret) {
            error_setg(errp, "multifd: failed wd_comp_init2");
            return NULL;
        }
        uadk_comp_init_done = true;
    }

    ss.alg_type = WD_ZLIB;
    if (compress) {
        ss.op_type = WD_DIR_COMPRESS;
        ss.comp_lv = migrate_multifd_uadk_level();
    } else {
        ss.op_type = WD_DIR_DECOMPRESS;
    }
    param.type = ss.op_type;
    ss.sched_param = &param;

    handle = wd_comp_alloc_sess(&ss);
    if (!handle) {
        err_msg = "failed wd_comp_alloc_sess";
        goto out;
    }

    wd = g_new0(struct wd_data, 1);
    wd->handle = handle;

    ret = uadk_alloc_buf(wd, count, page_size);
    if (ret) {
        err_msg = "out of mem for uadk buf";
        goto out_end;
    }

    return wd;

out_end:
    wd_comp_free_sess(handle);
out:
    wd_comp_uninit2();
    error_setg(errp, "multifd: %s", err_msg);
    return NULL;
}

static void uadk_uninit_sess(struct wd_data *wd)
{
    wd_comp_free_sess(wd->handle);
    wd_comp_uninit2();
    uadk_free_buf(wd);
    g_free(wd);
}

/**
 * uadk_send_setup: setup send side
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int uadk_send_setup(MultiFDSendParams *p, Error **errp)
{
    struct wd_data *wd;

    wd = uadk_init_sess(p->page_count, p->page_size, true, errp);
    if (!wd) {
        return -1;
    }

    p->compress_data = wd;
    assert(p->iov == NULL);
    /*
     * Each page will be compressed independently and sent using an IOV. The
     * additional two IOVs are used to store packet header and compressed data
     * length
     */

    p->iov = g_new0(struct iovec, p->page_count + 2);
    return 0;
}

/**
 * uadk_send_cleanup: cleanup send side
 *
 * Close the channel and return memory.
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static void uadk_send_cleanup(MultiFDSendParams *p, Error **errp)
{
    struct wd_data *wd = p->compress_data;

    uadk_uninit_sess(wd);
    p->compress_data = NULL;
}

static inline void prepare_next_iov(MultiFDSendParams *p, void *base,
                                    uint32_t len)
{
    p->iov[p->iovs_num].iov_base = (uint8_t *)base;
    p->iov[p->iovs_num].iov_len = len;
    p->iovs_num++;
}
/**
 * uadk_send_prepare: prepare data to be able to send
 *
 * Create a compressed buffer with all the pages that we are going to
 * send.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int uadk_send_prepare(MultiFDSendParams *p, Error **errp)
{
    return -1;
}

/**
 * uadk_recv_setup: setup receive side
 *
 * Create the compressed channel and buffer.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int uadk_recv_setup(MultiFDRecvParams *p, Error **errp)
{
    struct wd_data *wd;

    wd = uadk_init_sess(p->page_count, p->page_size, false, errp);
    if (!wd) {
        return -1;
    }
    p->compress_data = wd;
    return 0;
}

/**
 * uadk_recv_cleanup: setup receive side
 *
 * For no compression this function does nothing.
 *
 * @p: Params for the channel that we are using
 */
static void uadk_recv_cleanup(MultiFDRecvParams *p)
{
    struct wd_data *wd = p->compress_data;

    uadk_uninit_sess(wd);
    p->compress_data = NULL;
}

/**
 * uadk_recv: read the data from the channel into actual pages
 *
 * Read the compressed buffer, and uncompress it into the actual
 * pages.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int uadk_recv(MultiFDRecvParams *p, Error **errp)
{
    return -1;
}

static MultiFDMethods multifd_uadk_ops = {
    .send_setup = uadk_send_setup,
    .send_cleanup = uadk_send_cleanup,
    .send_prepare = uadk_send_prepare,
    .recv_setup = uadk_recv_setup,
    .recv_cleanup = uadk_recv_cleanup,
    .recv = uadk_recv,
};

static void multifd_uadk_register(void)
{
    multifd_register_ops(MULTIFD_COMPRESSION_UADK,
                               &multifd_uadk_ops);
}
migration_init(multifd_uadk_register);
