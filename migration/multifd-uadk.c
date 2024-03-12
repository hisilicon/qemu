/*
 * Multifd UADK Zlib compression accelerator implementation
 *
 * Copyright (c) 2024 Huawei
 *
 * Authors:
 *  Shameer Kolothum <shameerali.kolothum.thodi@huawei.com>
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
#include "uadk/uadk/wd_comp.h"
#include "uadk/uadk/wd_sched.h"
#include "uadk/uadk/wd_zlibwrapper.h"

static const int ZLIB_MIN_WBITS = 8;

#define CHUNK_SIZE MULTIFD_PACKET_SIZE
#define MAX_BUF_SIZE (MULTIFD_PACKET_SIZE * 2)

struct wd_zlib_data {
    z_stream zs;
    uint8_t *src;
    uint8_t *dst;
};

static bool support_compression_methods[MULTIFD_COMPRESSION__MAX];

static int uadk_alloc_buf(struct wd_zlib_data *wd_data)
{
    int flags = MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS;
    int prot = PROT_READ | PROT_WRITE;

    wd_data->src = mmap(NULL, MAX_BUF_SIZE, prot, flags, -1, 0);
    if (wd_data->src == MAP_FAILED) {
        return -ENOMEM;
    }
    wd_data->dst = mmap(NULL, MAX_BUF_SIZE, prot, flags, -1, 0);
    if (wd_data->dst == MAP_FAILED) {
        munmap(wd_data->src, MAX_BUF_SIZE);
        wd_data->src = NULL;
        return -ENOMEM;
    }
    return 0;
}

static void uadk_free_buf(struct wd_zlib_data *wd_data)
{
    if (wd_data->src) {
        munmap(wd_data->src, MAX_BUF_SIZE);
        wd_data->src = NULL;
    }
    if (wd_data->dst) {
        munmap(wd_data->dst, MAX_BUF_SIZE);
        wd_data->dst = NULL;
    }
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
    struct wd_zlib_data *data = g_new0(struct wd_zlib_data, 1);
    z_stream *zs = &data->zs;
    const char *err_msg;
    int ret = 0;

    ret = wd_deflate_init(zs, 1, ZLIB_MIN_WBITS);
    if (ret) {
        err_msg = "wd_deflate init failed";
        goto out;
    }

    ret = uadk_alloc_buf(data);
    if (ret) {
        err_msg = "out of mem for uadk buf";
        goto out_end;
    }
    p->data = data;
    return 0;

out_end:
    wd_deflate_end(zs);
out:
    error_setg(errp, "multifd %u: %s", p->id, err_msg);
    return -1;
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
    struct wd_zlib_data *data = p->data;

    wd_deflate_end(&data->zs);
    uadk_free_buf(data);
    g_free(data);
    p->data = NULL;
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
    MultiFDPages_t *pages = p->pages;
    struct wd_zlib_data *z = p->data;
    z_stream *zs = &z->zs;
    uint8_t *dst = z->dst;
    uint32_t out_size = 0;
    uint32_t in_size = pages->num * p->page_size;
    int ret = 0, deflated;
    int flush;

    multifd_send_prepare_header(p);

    for (int i = 0; i < pages->num; i++) {
        memcpy(z->src + (i * p->page_size),
               p->pages->block->host + pages->offset[i], p->page_size);
    }

    zs->next_in = z->src;
    do {
        uint32_t available = MAX_BUF_SIZE - out_size;

        if (in_size > CHUNK_SIZE) {
            zs->avail_in = CHUNK_SIZE;
            in_size -= CHUNK_SIZE;
        } else {
            zs->avail_in = in_size;
            in_size = 0;
        }

        flush = in_size ? Z_SYNC_FLUSH : Z_FINISH;

        do {
            zs->avail_out = CHUNK_SIZE;
            zs->next_out = dst;
            ret = wd_deflate(zs, flush);
            if (ret < 0) {
                error_setg(errp, "multifd %u: wd_deflate returned %d",
                           p->id, ret);
                return -1;
            }
            deflated = CHUNK_SIZE - zs->avail_out;
            dst += deflated;
        } while (zs->avail_in > 0);

        out_size += available - zs->avail_out;
    } while (flush != Z_FINISH);

    p->iov[p->iovs_num].iov_base = z->dst;
    p->iov[p->iovs_num].iov_len = out_size;
    p->iovs_num++;
    p->next_packet_size = out_size;
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
    struct wd_zlib_data *data = g_new0(struct wd_zlib_data, 1);
    z_stream *zs = &data->zs;
    const char *err_msg;
    int ret = 0;

    ret = wd_inflate_init(zs, ZLIB_MIN_WBITS);
    if (ret) {
        err_msg = "wd_inflate init failed";
        goto out;
    }

    ret = uadk_alloc_buf(data);
    if (ret) {
        err_msg = "out of mem for uadk buf";
        goto out_end;
    }
    p->data = data;
    return 0;

out_end:
    wd_inflate_end(zs);
out:
    error_setg(errp, "multifd %u: %s", p->id, err_msg);
    return -1;
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
    struct wd_zlib_data *data = p->data;

    wd_inflate_end(&data->zs);
    uadk_free_buf(data);
    g_free(data);
    p->data = NULL;
}

/**
 * uadk_recv_pages: read the data from the channel into actual pages
 *
 * Read the compressed buffer, and uncompress it into the actual
 * pages.
 *
 * Returns 0 for success or -1 for error
 *
 * @p: Params for the channel that we are using
 * @errp: pointer to an error
 */
static int uadk_recv_pages(MultiFDRecvParams *p, Error **errp)
{
    struct wd_zlib_data *wd = p->data;
    z_stream *zs = &wd->zs;
    uint32_t in_size = p->next_packet_size;
    uint8_t *dst = wd->dst;
    uint32_t out_size = zs->total_out;
    uint32_t expected_size = p->normal_num * p->page_size;
    uint32_t flags = p->flags & MULTIFD_FLAG_COMPRESSION_MASK;
    int ret, inflated;

    if (flags != MULTIFD_FLAG_ZLIB) {
        error_setg(errp, "multifd %u: flags received %x flags expected %x",
                   p->id, flags, MULTIFD_FLAG_ZLIB);
        return -1;
    }
    ret = qio_channel_read_all(p->c, (void *)wd->src, in_size, errp);
    if (ret != 0) {
        return ret;
    }

    zs->next_in = wd->src;
    do {
        if (in_size > CHUNK_SIZE) {
            zs->avail_in = CHUNK_SIZE;
            in_size -= CHUNK_SIZE;
        } else {
            zs->avail_in = in_size;
            in_size = 0;
        }

        do {
            zs->avail_out = CHUNK_SIZE;
            zs->next_out = dst;
            ret = wd_inflate(zs, Z_SYNC_FLUSH);
            if (ret < 0) {
                error_setg(errp, "multifd %u: wd_inflate returned %d",
                           p->id, ret);
                return -1;
            }
            inflated = CHUNK_SIZE - zs->avail_out;
            dst += inflated;
         } while (ret != Z_STREAM_END && zs->avail_in > 0);
    } while (ret != Z_STREAM_END);

    out_size = zs->total_out - out_size;
    if (out_size != expected_size) {
        error_setg(errp, "multifd %u: packet size received %u size expected %u",
                   p->id, out_size, expected_size);
        return -1;
    }

    for (int i = 0; i < p->normal_num; i++) {
        memcpy(p->host + p->normal[i], wd->dst + (i * p->page_size),
               p->page_size);
    }

    return 0;
}

static MultiFDMethods multifd_uadk_ops = {
    .send_setup = uadk_send_setup,
    .send_cleanup = uadk_send_cleanup,
    .send_prepare = uadk_send_prepare,
    .recv_setup = uadk_recv_setup,
    .recv_cleanup = uadk_recv_cleanup,
    .recv_pages = uadk_recv_pages
};

static bool is_supported(MultiFDCompression compression)
{
    return support_compression_methods[compression];
}

static MultiFDMethods *get_uadk_multifd_methods(void)
{
    return &multifd_uadk_ops;
}

static MultiFDAccelMethods multifd_uadk_accel_ops = {
    .is_supported = is_supported,
    .get_multifd_methods = get_uadk_multifd_methods,
};

static void multifd_uadk_register(void)
{
    multifd_register_accel_ops(MULTIFD_COMPRESSION_ACCEL_UADK,
                               &multifd_uadk_accel_ops);
    support_compression_methods[MULTIFD_COMPRESSION_ZLIB] = true;
}

migration_init(multifd_uadk_register);
