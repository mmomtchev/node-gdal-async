#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check_aec.h"

static void out_lsb(unsigned char *dest, unsigned long long int val, int size)
{
    for (int i = 0; i < size; i++)
        dest[i] = (unsigned char)(val >> (8 * i));
}

static void out_msb(unsigned char *dest, unsigned long long int val, int size)
{
    for (int i = 0; i < size; i++)
        dest[i] = (unsigned char)(val >> (8 * (size - 1 - i)));
}

int update_state(struct test_state *state)
{
    struct aec_stream *strm = state->strm;

    if (strm->bits_per_sample > 16) {
        state->id_len = 5;

        if (strm->bits_per_sample <= 24 && strm->flags & AEC_DATA_3BYTE) {
            state->bytes_per_sample = 3;
        } else {
            state->bytes_per_sample = 4;
        }
    }
    else if (strm->bits_per_sample > 8) {
        state->id_len = 4;
        state->bytes_per_sample = 2;
    } else {
        state->id_len = 3;
        state->bytes_per_sample = 1;
    }

    if (strm->flags & AEC_DATA_MSB)
        state->out = out_msb;
    else
        state->out = out_lsb;

    if (strm->flags & AEC_DATA_SIGNED) {
        state->xmin = -(1LL << (strm->bits_per_sample - 1));
        state->xmax = (1ULL << (strm->bits_per_sample - 1)) - 1;
    } else {
        state->xmin = 0;
        state->xmax = (1ULL << strm->bits_per_sample) - 1;
    }

    return 0;
}

int encode_decode_small(struct test_state *state)
{
    size_t compressed_size;
    size_t n_in, avail_in, avail_out, total_out;
    struct aec_stream *strm = state->strm;

    int status = aec_encode_init(strm);
    if (status != AEC_OK) {
        printf("Init failed.\n");
        return 99;
    }

    n_in = 0;
    avail_in = 1;
    avail_out = 1;
    total_out = 0;
    strm->next_in = state->ubuf;
    strm->avail_in = state->bytes_per_sample;
    strm->avail_out = 1;
    strm->next_out = state->cbuf;

    while ((avail_in || avail_out) && total_out < state->cbuf_len) {
        if (strm->avail_in == 0 && avail_in) {
            n_in += state->bytes_per_sample;
            if (n_in < state->buf_len) {
                strm->avail_in = state->bytes_per_sample;
                strm->next_in = state->ubuf + n_in;
            } else {
                avail_in = 0;
            }
        }

        status = aec_encode(strm, AEC_NO_FLUSH);
        if (status != AEC_OK) {
            printf("Decode failed.\n");
            return 99;
        }

        if (strm->total_out - total_out > 0
            && total_out < state->cbuf_len) {
            total_out = strm->total_out;
            strm->avail_out = 1;
            strm->next_out = state->cbuf + total_out;
            avail_out = 1;
        } else {
            avail_out = 0;
        }
    }

    status = aec_encode(strm, AEC_FLUSH);
    if (status != AEC_OK) {
        printf("Encode failed.\n");
        return 99;
    }

    aec_encode_end(strm);

    compressed_size = strm->total_out;

    strm->avail_in = 1;
    strm->next_in = state->cbuf;

    strm->avail_out = state->bytes_per_sample;
    strm->next_out = state->obuf;

    status = aec_decode_init(strm);
    if (status != AEC_OK) {
        printf("Init failed.\n");
        return 99;
    }

    n_in = 0;
    avail_in = 1;
    avail_out = 1;
    total_out = 0;
    strm->next_in = state->cbuf;
    strm->avail_in = 1;
    strm->avail_out = state->bytes_per_sample;
    strm->next_out = state->obuf;

    while ((avail_in || avail_out) && total_out < state->buf_len) {
        if (strm->avail_in == 0 && avail_in) {
            n_in++;
            if (n_in < compressed_size) {
                strm->avail_in = 1;
                strm->next_in = state->cbuf + n_in;
            } else {
                avail_in = 0;
            }
        }

        status = aec_decode(strm, AEC_NO_FLUSH);
        if (status != AEC_OK) {
            printf("Decode failed.\n");
            return 99;
        }

        if (strm->total_out - total_out > 0
            && total_out < state->buf_len) {
            total_out = strm->total_out;
            strm->avail_out = state->bytes_per_sample;
            strm->next_out = state->obuf + total_out;
            avail_out = 1;
        } else {
            avail_out = 0;
        }
    }

    status = aec_decode(strm, AEC_FLUSH);
    if (status != AEC_OK) {
        printf("Decode failed.\n");
        return 99;
    }

    if (memcmp(state->ubuf, state->obuf, state->ibuf_len)) {
        printf("\n%s: Uncompressed output differs from input.\n", CHECK_FAIL);

        printf("\nuncompressed buf");
        for (int i = 0; i < 80; i++) {
            if (i % 8 == 0)
                printf("\n");
            printf("%02x ", state->ubuf[i]);
        }
        printf("\n\ncompressed buf len %zu", compressed_size);
        for (int i = 0; i < 80; i++) {
            if (i % 8 == 0)
                printf("\n");
            printf("%02x ", state->cbuf[i]);
        }
        printf("\n\ndecompressed buf");
        for (int i = 0; i < 80; i++) {
            if (i % 8 == 0)
                printf("\n%04i ", i);
            printf("%02x ", state->obuf[i]);
        }
        printf("\n");
        return 99;
    }
    aec_decode_end(strm);
    return 0;
}

int encode_decode_large(struct test_state *state)
{
    int status;
    int bflags = 0;
    size_t to;
    char fbase[1024];
    struct aec_stream *strm = state->strm;

    strm->avail_in = state->ibuf_len;
    strm->avail_out = state->cbuf_len;
    strm->next_in = state->ubuf;
    strm->next_out = state->cbuf;

    status = aec_encode_init(strm);
    if (status != AEC_OK) {
        printf("Init failed.\n");
        return 99;
    }
    if (state->dump) {
        char fname[1024 + 4];
        FILE *fp;
        snprintf(fbase, sizeof(fbase), "BPS%02iID%iBS%02iRSI%04iFLG%04i",
                 strm->bits_per_sample,
                 state->id,
                 strm->block_size,
                 strm->rsi,
                 strm->flags);
        snprintf(fname, sizeof(fname), "%s.dat", fbase);
        if ((fp = fopen(fname, "wb")) == NULL) {
            fprintf(stderr, "ERROR: cannot open dump file %s\n", fname);
            return 99;
        }
        fputc(strm->bits_per_sample, fp);
        bflags = strm->block_size >> 8;
        if (strm->flags | AEC_DATA_MSB)
            bflags |= 0x80;
        if (strm->flags | AEC_DATA_SIGNED)
            bflags |= 0x40;
        if (strm->flags | AEC_DATA_3BYTE)
            bflags |= 0x10;
        bflags |= 0x20; /* encode */
        fputc(bflags, fp);
        fwrite(strm->next_in, strm->avail_in, 1, fp);
        fclose(fp);
    }

    status = aec_encode(strm, AEC_FLUSH);
    if (status != AEC_OK) {
        printf("Encode failed.\n");
        return 99;
    }

    aec_encode_end(strm);

    if (state->dump) {
        char fname[1024 + 3];
        FILE *fp;
        snprintf(fname, sizeof(fname), "%s.rz", fbase);
        if ((fp = fopen(fname, "wb")) == NULL) {
            fprintf(stderr, "ERROR: cannot open dump file %s\n", fname);
            return 99;
        }
        fputc(strm->bits_per_sample, fp);
        bflags &= ~0x20;
        fputc(bflags, fp);
        fwrite(state->cbuf, strm->total_out, 1, fp);
        fclose(fp);
    }

    strm->avail_in = strm->total_out;
    strm->avail_out = state->buf_len;
    strm->next_in = state->cbuf;
    strm->next_out = state->obuf;
    to = strm->total_out;

    status = aec_decode_init(strm);
    if (status != AEC_OK) {
        printf("Init failed.\n");
        return 99;
    }

    status = aec_decode(strm, AEC_FLUSH);
    if (status != AEC_OK) {
        printf("Decode failed.\n");
        return 99;
    }

    if (memcmp(state->ubuf, state->obuf, state->ibuf_len)) {
        printf("\n%s: Uncompressed output differs from input.\n", CHECK_FAIL);

        printf("\nuncompressed buf");
        for (int i = 0; i < 80; i++) {
            if (i % 8 == 0)
                printf("\n");
            printf("%02x ", state->ubuf[i]);
        }
        printf("\n\ncompressed buf len %zu", to);
        for (int i = 0; i < 80; i++) {
            if (i % 8 == 0)
                printf("\n");
            printf("%02x ", state->cbuf[i]);
        }
        printf("\n\ndecompressed buf");
        for (int i = 0; i < 80; i++) {
            if (i % 8 == 0)
                printf("\n");
            printf("%02x ", state->obuf[i]);
        }
        printf("\n");
        return 99;
    }
    aec_decode_end(strm);
    return 0;
}
