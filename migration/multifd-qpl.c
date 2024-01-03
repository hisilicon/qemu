/*
 * Multifd qpl compression accelerator implementation
 *
 * Copyright (c) 2023 Intel Corporation
 *
 * Authors:
 *  Yuan Liu<yuan1.liu@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/rcu.h"
#include "exec/ramblock.h"
#include "exec/target_page.h"
#include "qapi/error.h"
#include "migration.h"
#include "trace.h"
#include "options.h"
#include "multifd.h"
#include "qpl/qpl.h"

#define MAX_BUF_SIZE (MULTIFD_PACKET_SIZE * 2)
static bool support_compression_methods[MULTIFD_COMPRESSION__MAX];

struct qpl_data {
    qpl_job *job;
    /* compressed data buffer */
    uint8_t *buf;
    /* decompressed data buffer */
    uint8_t *zbuf;
};

static int init_qpl(struct qpl_data *qpl, uint8_t channel_id,  Error **errp)
{
    qpl_status status;
    qpl_path_t path = qpl_path_auto;
    uint32_t job_size = 0;

    status = qpl_get_job_size(path, &job_size);
    if (status != QPL_STS_OK) {
        error_setg(errp, "multfd: %u: failed to get QPL size, error %d",
                   channel_id, status);
        return -1;
    }

    qpl->job = g_try_malloc0(job_size);
    if (!qpl->job) {
        error_setg(errp, "multfd: %u: failed to allocate QPL job", channel_id);
        return -1;
    }

    status = qpl_init_job(path, qpl->job);
    if (status != QPL_STS_OK) {
        error_setg(errp, "multfd: %u: failed to init QPL hardware, error %d",
                   channel_id, status);
        return -1;
    }
    return 0;
}

static void deinit_qpl(struct qpl_data *qpl)
{
    if (qpl->job) {
        qpl_fini_job(qpl->job);
        g_free(qpl->job);
    }
}

/**
 * qpl_send_setup: setup send side
 *
 * Setup each channel with QPL compression.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int qpl_send_setup(MultiFDSendParams *p, Error **errp)
{
    struct qpl_data *qpl = g_new0(struct qpl_data, 1);
    int flags = MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS;
    const char *err_msg;

    if (init_qpl(qpl, p->id, errp) != 0) {
        err_msg = "failed to initialize QPL\n";
        goto err_qpl_init;
    }
    qpl->zbuf = mmap(NULL, MAX_BUF_SIZE, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (qpl->zbuf == MAP_FAILED) {
        err_msg = "failed to allocate QPL zbuf\n";
        goto err_zbuf_mmap;
    }
    p->data = qpl;
    return 0;

err_zbuf_mmap:
    deinit_qpl(qpl);
err_qpl_init:
    g_free(qpl);
    error_setg(errp, "multifd %u: %s", p->id, err_msg);
    return -1;
}

/**
 * qpl_send_cleanup: cleanup send side
 *
 * Close the channel and return memory.
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static void qpl_send_cleanup(MultiFDSendParams *p, Error **errp)
{
    struct qpl_data *qpl = p->data;

    deinit_qpl(qpl);
    if (qpl->zbuf) {
        munmap(qpl->zbuf, MAX_BUF_SIZE);
        qpl->zbuf = NULL;
    }
    g_free(p->data);
    p->data = NULL;
}

/**
 * qpl_send_prepare: prepare data to be able to send
 *
 * Create a compressed buffer with all the pages that we are going to
 * send.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int qpl_send_prepare(MultiFDSendParams *p, Error **errp)
{
    struct qpl_data *qpl = p->data;
    qpl_job *job = qpl->job;
    qpl_status status;

    job->op = qpl_op_compress;
    job->next_out_ptr = qpl->zbuf;
    job->available_out = MAX_BUF_SIZE;
    job->flags = QPL_FLAG_FIRST | QPL_FLAG_OMIT_VERIFY | QPL_FLAG_ZLIB_MODE;
    /* QPL supports compression level 1 */
    job->level = 1;
    for (int i = 0; i < p->normal_num; i++) {
        if (i == p->normal_num - 1) {
            job->flags |= (QPL_FLAG_LAST | QPL_FLAG_OMIT_VERIFY);
        }
        job->next_in_ptr = p->pages->block->host + p->normal[i];
        job->available_in = p->page_size;
        status = qpl_execute_job(job);
        if (status != QPL_STS_OK) {
            error_setg(errp, "multifd %u: execute job error %d ",
                       p->id, status);
            return -1;
        }
        job->flags &= ~QPL_FLAG_FIRST;
    }
    p->iov[p->iovs_num].iov_base = qpl->zbuf;
    p->iov[p->iovs_num].iov_len = job->total_out;
    p->iovs_num++;
    p->next_packet_size += job->total_out;
    p->flags |= MULTIFD_FLAG_ZLIB;
    return 0;
}

/**
 * qpl_recv_setup: setup receive side
 *
 * Create the compressed channel and buffer.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int qpl_recv_setup(MultiFDRecvParams *p, Error **errp)
{
    struct qpl_data *qpl = g_new0(struct qpl_data, 1);
    int flags = MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS;
    const char *err_msg;

    if (init_qpl(qpl, p->id, errp) != 0) {
        err_msg = "failed to initialize QPL\n";
        goto err_qpl_init;
    }
    qpl->zbuf = mmap(NULL, MAX_BUF_SIZE, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (qpl->zbuf == MAP_FAILED) {
        err_msg = "failed to allocate QPL zbuf\n";
        goto err_zbuf_mmap;
    }
    qpl->buf = mmap(NULL, MAX_BUF_SIZE, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (qpl->buf == MAP_FAILED) {
        err_msg = "failed to allocate QPL buf\n";
        goto err_buf_mmap;
    }
    p->data = qpl;
    return 0;

err_buf_mmap:
    munmap(qpl->zbuf, MAX_BUF_SIZE);
    qpl->zbuf = NULL;
err_zbuf_mmap:
    deinit_qpl(qpl);
err_qpl_init:
    g_free(qpl);
    error_setg(errp, "multifd %u: %s", p->id, err_msg);
    return -1;
}

/**
 * qpl_recv_cleanup: setup receive side
 *
 * For no compression this function does nothing.
 *
 * @p: Params for the channel that we are using
 */
static void qpl_recv_cleanup(MultiFDRecvParams *p)
{
    struct qpl_data *qpl = p->data;

    deinit_qpl(qpl);
    if (qpl->zbuf) {
        munmap(qpl->zbuf, MAX_BUF_SIZE);
        qpl->zbuf = NULL;
    }
    if (qpl->buf) {
        munmap(qpl->buf, MAX_BUF_SIZE);
        qpl->buf = NULL;
    }
    g_free(p->data);
    p->data = NULL;
}

/**
 * qpl_recv_pages: read the data from the channel into actual pages
 *
 * Read the compressed buffer, and uncompress it into the actual
 * pages.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int qpl_recv_pages(MultiFDRecvParams *p, Error **errp)
{
    struct qpl_data *qpl = p->data;
    uint32_t in_size = p->next_packet_size;
    uint32_t expected_size = p->normal_num * p->page_size;
    uint32_t flags = p->flags & MULTIFD_FLAG_COMPRESSION_MASK;
    qpl_job *job = qpl->job;
    qpl_status status;
    int ret;

    if (flags != MULTIFD_FLAG_ZLIB) {
        error_setg(errp, "multifd %u: flags received %x flags expected %x",
                   p->id, flags, MULTIFD_FLAG_ZLIB);
        return -1;
    }
    ret = qio_channel_read_all(p->c, (void *)qpl->zbuf, in_size, errp);
    if (ret != 0) {
        return ret;
    }

    job->op = qpl_op_decompress;
    job->next_in_ptr = qpl->zbuf;
    job->available_in = in_size;
    job->next_out_ptr = qpl->buf;
    job->available_out = expected_size;
    job->flags = QPL_FLAG_FIRST | QPL_FLAG_LAST | QPL_FLAG_OMIT_VERIFY |
                 QPL_FLAG_ZLIB_MODE;
    status = qpl_execute_job(job);
    if ((status != QPL_STS_OK) || (job->total_out != expected_size)) {
        error_setg(errp, "multifd %u: execute job error %d, expect %u, out %u",
                   p->id, status, job->total_out, expected_size);
        return -1;
    }
    for (int i = 0; i < p->normal_num; i++) {
        memcpy(p->host + p->normal[i], qpl->buf + (i * p->page_size),
               p->page_size);
    }
    return 0;
}

static MultiFDMethods multifd_qpl_ops = {
    .send_setup = qpl_send_setup,
    .send_cleanup = qpl_send_cleanup,
    .send_prepare = qpl_send_prepare,
    .recv_setup = qpl_recv_setup,
    .recv_cleanup = qpl_recv_cleanup,
    .recv_pages = qpl_recv_pages
};

static bool is_supported(MultiFDCompression compression)
{
    return support_compression_methods[compression];
}

static MultiFDMethods *get_qpl_multifd_methods(void)
{
    return &multifd_qpl_ops;
}

static MultiFDAccelMethods multifd_qpl_accel_ops = {
    .is_supported = is_supported,
    .get_multifd_methods = get_qpl_multifd_methods,
};

static void multifd_qpl_register(void)
{
    multifd_register_accel_ops(MULTIFD_COMPRESSION_ACCEL_QPL,
                               &multifd_qpl_accel_ops);
    support_compression_methods[MULTIFD_COMPRESSION_ZLIB] = true;
}

migration_init(multifd_qpl_register);
