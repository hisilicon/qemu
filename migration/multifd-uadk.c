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
#include "exec/ramblock.h"
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
    struct wd_data *uadk_data = p->compress_data;
    uint32_t hdr_size;
    struct wd_comp_req creq = {0};
    uint8_t *buf = uadk_data->buf;
    int ret = 0;
    uint32_t total_comp_len = 0;
    uint32_t total_recv_len = 0;

    if (!multifd_send_prepare_common(p)) {
        goto out;
    }

    hdr_size = p->pages->normal_num * sizeof(uint32_t);
    /* prepare the header that stores the lengths of all compressed data */
    prepare_next_iov(p, uadk_data->buf_hdr, hdr_size);
    p->next_packet_size += hdr_size;

    printf("\n%s: Shameer: hdr_size %d, num pages %d page_size %d\n", __func__, hdr_size, p->pages->normal_num, p->page_size);
    creq.op_type = WD_DIR_COMPRESS;
    for (int i = 0; i < p->pages->normal_num; i++) {
        creq.src = p->pages->block->host + p->pages->offset[i];
        creq.src_len = p->page_size;
        creq.dst = buf;
        creq.dst_len = uadk_data->data_size;

	total_recv_len += p->page_size;
        ret = wd_do_comp_sync(uadk_data->handle, &creq);
        if (ret || creq.status) {
            error_setg(errp, "multifd %u: wd_do_comp_sync returned %d",
                       p->id, ret);
            return -1;
        }
        if (creq.dst_len <= uadk_data->data_size) {
            uadk_data->buf_hdr[i] = cpu_to_be32(creq.dst_len);

            prepare_next_iov(p, buf, creq.dst_len);
            p->next_packet_size += creq.dst_len;
            buf += creq.dst_len;
	    total_comp_len += creq.dst_len;
        } else {
            /* The compressed output is larger than input. Send raw data. */
            uadk_data->buf_hdr[i] = cpu_to_be32(uadk_data->data_size);

            prepare_next_iov(p, p->pages->block->host + p->pages->offset[i],
                             uadk_data->data_size);
            p->next_packet_size += uadk_data->data_size;
            buf += uadk_data->data_size;
	    total_comp_len += uadk_data->data_size;
        }
    }

    printf("%s: Shameer: total_recv_len %d total_comp_len %d\n", __func__, total_recv_len, total_comp_len);
out:
    p->flags |= MULTIFD_FLAG_ZLIB;
    multifd_send_fill_packet(p);
    return 0;
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
    struct wd_data *uadk_data = p->compress_data;
    struct wd_comp_req creq = {0};
    uint32_t in_size = p->next_packet_size;
    uint32_t flags = p->flags & MULTIFD_FLAG_COMPRESSION_MASK;
    uint32_t hdr_len = p->normal_num * sizeof(uint32_t);
    uint32_t data_len = 0;
    uint8_t *buf = uadk_data->buf;
    int ret = 0;
    uint32_t total_decomp_len = 0;
    uint32_t total_rcvd_len = 0;

    if (flags != MULTIFD_FLAG_ZLIB) {
        error_setg(errp, "multifd %u: flags received %x flags expected %x",
                   p->id, flags, MULTIFD_FLAG_ZLIB);
        return -1;
    }

    multifd_recv_zero_page_process(p);
    if (!p->normal_num) {
        assert(in_size == 0);
        return 0;
    }

    printf("\n%s: Shameer: hdr_len %d in_size %d num pages %d page_size %d\n",__func__, hdr_len, in_size, p->normal_num, p->page_size);
    /* read compressed data lengths */
    assert(hdr_len < in_size);
    ret = qio_channel_read_all(p->c, (void *) uadk_data->buf_hdr,
                               hdr_len, errp);
    if (ret != 0) {
        return ret;
    }

    for (int i = 0; i < p->normal_num; i++) {
        uadk_data->buf_hdr[i] = be32_to_cpu(uadk_data->buf_hdr[i]);
        data_len += uadk_data->buf_hdr[i];
        assert(uadk_data->buf_hdr[i] <= uadk_data->data_size);
    }

    /* read compressed data */
    assert(in_size == hdr_len + data_len);
    ret = qio_channel_read_all(p->c, (void *)buf, data_len, errp);
    if (ret != 0) {
        return ret;
    }

    creq.op_type = WD_DIR_DECOMPRESS;
    creq.dst_len = p->page_size;
    for (int i = 0; i < p->normal_num; i++) {
        if (uadk_data->buf_hdr[i] == uadk_data->data_size) {
            memcpy(p->host + p->normal[i], buf, uadk_data->data_size);
            buf += uadk_data->data_size;
	    total_rcvd_len += uadk_data->data_size;
	    total_decomp_len += uadk_data->data_size;
            continue;
        }
        creq.src = buf;
        creq.src_len = uadk_data->buf_hdr[i];
        creq.dst = p->host + p->normal[i];
        ret = wd_do_comp_sync(uadk_data->handle, &creq);
        if (ret || creq.status) {
            error_setg(errp, "multifd %u: failed wd_do_comp_sync, ret %d status %d",
                       p->id, ret, creq.status);
            return -1;
        }
        if (creq.dst_len != uadk_data->data_size) {
            error_setg(errp, "multifd %u: decompressed length error", p->id);
            return -1;
        }
        buf += uadk_data->buf_hdr[i];
	total_rcvd_len += uadk_data->buf_hdr[i];;
	total_decomp_len += creq.dst_len;
     }
    printf("%s: Shameer total_rcvd_len %d, total_decomp_len %d\n", __func__, total_rcvd_len, total_decomp_len);
    return 0;
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
