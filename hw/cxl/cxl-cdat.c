/*
 * CXL CDAT Structure
 *
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/cxl/cxl.h"
#include "qapi/error.h"
#include "qemu/error-report.h"

static void cdat_len_check(struct cdat_sub_header *hdr, Error **errp)
{
    assert(hdr->length);
    assert(hdr->reserved == 0);

    switch (hdr->type) {
    case CDAT_TYPE_DSMAS:
        assert(hdr->length == sizeof(struct cdat_dsmas));
        break;
    case CDAT_TYPE_DSLBIS:
        assert(hdr->length == sizeof(struct cdat_dslbis));
        break;
    case CDAT_TYPE_DSMSCIS:
        assert(hdr->length == sizeof(struct cdat_dsmscis));
        break;
    case CDAT_TYPE_DSIS:
        assert(hdr->length == sizeof(struct cdat_dsis));
        break;
    case CDAT_TYPE_DSEMTS:
        assert(hdr->length == sizeof(struct cdat_dsemts));
        break;
    case CDAT_TYPE_SSLBIS:
        assert(hdr->length >= sizeof(struct cdat_sslbis_header));
        assert((hdr->length - sizeof(struct cdat_sslbis_header)) %
               sizeof(struct cdat_sslbe) == 0);
        break;
    default:
        error_setg(errp, "Type %d is reserved", hdr->type);
    }
}

void cxl_doe_cdat_init(CXLComponentState *cxl_cstate, Error **errp)
{
    CDATObject *cdat = &cxl_cstate->cdat;
    CDATEntry cdat_st[1024];
    uint8_t sum = 0, *buf;
    int i = 0, ent = 1, file_size = 0, cdat_table_len = 0;
    struct cdat_sub_header *hdr;
    struct cdat_table_header *cdat_header;
    FILE *fp;
    void **cdat_table = NULL;

    fp = fopen(cdat->filename, "r");

    if (fp) {
        /* Read CDAT file and create its cache */
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        cdat->buf = g_malloc0(file_size);

        if (fread(cdat->buf, file_size, 1, fp) == 0) {
            error_setg(errp, "File read failed");
        }

        fclose(fp);

        /* Set CDAT header, Entry = 0 */
        cdat_st[0].base = cdat->buf;
        cdat_st[0].length = sizeof(struct cdat_table_header);
        while (i < cdat_st[0].length) {
            sum += cdat->buf[i++];
        }

        /* Read CDAT structures */
        while (i < file_size) {
            hdr = (struct cdat_sub_header *)(cdat->buf + i);
            cdat_len_check(hdr, errp);

            cdat_st[ent].base = hdr;
            cdat_st[ent].length = hdr->length;

            while (cdat->buf + i < (char *)cdat_st[ent].base + cdat_st[ent].length) {
                assert(i < file_size);
                sum += cdat->buf[i++];
            }

            ent++;
        }

        if (sum != 0) {
            warn_report("Found checksum mismatch in %s\n", cdat->filename);
        }
    } else {
        /* Use default table if fopen == NULL */
        assert(cdat->build_cdat_table);
        cdat_header = g_malloc0(sizeof(struct cdat_table_header));
        cdat->build_cdat_table(&cdat_table, &cdat_table_len);

        /* Entry 0 for CDAT header, starts with Entry 1 */
        for (; ent < cdat_table_len + 1; ent++) {
            hdr = cdat_table[ent - 1];
            buf = cdat_table[ent - 1];

            cdat_st[ent].base = hdr;
            cdat_st[ent].length = hdr->length;

            cdat_header->length += hdr->length;
            for (i = 0; i < hdr->length; i++) {
                sum += buf[i];
            }
        }

        /* Generate CDAT header */
        cdat_header->revision = CXL_CDAT_REV;
        cdat_header->sequence = 0;
        cdat_header->length += sizeof(struct cdat_table_header);
        sum += cdat_header->revision + cdat_header->sequence +
               cdat_header->length;
        cdat_header->checksum = ~sum + 1;

        cdat_st[0].base = cdat_header;
        cdat_st[0].length = sizeof(struct cdat_table_header);
    }

    /* It is unlikely that CDAT entry number will exceed 1024,
     * but this assertion is still added for insurance */
    assert(ent <= 1024);

    /* Copy from temp struct */
    cdat->entry_len = ent;
    cdat->entry = g_malloc0(sizeof(CDATEntry) * ent);
    memcpy(cdat->entry, cdat_st, sizeof(CDATEntry) * ent);
}
