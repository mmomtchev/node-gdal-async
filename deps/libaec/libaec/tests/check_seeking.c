#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check_aec.h"

#define BUF_SIZE 1024 * 3


void shift_cdata(struct test_state *state, unsigned char *cbuf_unshifted,
                 int byte_offset, int bit_offset)
{
    struct aec_stream *strm = state->strm;
    unsigned char *dst = state->cbuf + byte_offset;

    memset(state->cbuf, 0, state->buf_len);
    for (size_t i = 0; i < strm->avail_in; i++) {
        dst[i] |= cbuf_unshifted[i] >> bit_offset;
        dst[i + 1] |= cbuf_unshifted[i] << (8 - bit_offset);
    }
}

int encode_decode_large_seek(struct test_state *state)
{
    int status;
    int bflags = 0;
    int c_len;
    size_t to;
    char fbase[1024];
    unsigned char *cbuf_unshifted;
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

    cbuf_unshifted = (unsigned char *)malloc(state->cbuf_len);
    if (!cbuf_unshifted) {
        fprintf(stderr, "Not enough memory.\n");
        return 99;
    }
    c_len = strm->total_out;
    memcpy(cbuf_unshifted, state->cbuf, c_len);


    for (int byte_offset = 0; byte_offset < 256; byte_offset++) {
        for (int bit_offset = 0; bit_offset < 8; bit_offset++) {

            strm->avail_in = c_len;
            strm->avail_out = state->buf_len;
            strm->next_in = state->cbuf;
            strm->next_out = state->obuf;
            to = strm->total_out;
            shift_cdata(state, cbuf_unshifted, byte_offset, bit_offset);

            status = aec_decode_init(strm);
            if (status != AEC_OK) {
                printf("Init failed.\n");
                return 99;
            }
            status = aec_buffer_seek(strm, byte_offset * 8 + bit_offset);
            if (status != AEC_OK) {
                printf("Seeking failed.\n");
                return 99;
            }
            status = aec_decode(strm, AEC_FLUSH);
            if (status != AEC_OK) {
                printf("Decode failed.\n");
                return 99;
            }

            if (memcmp(state->ubuf, state->obuf, state->ibuf_len)) {
                printf("\n%s: Uncompressed output differs from input.\n",
                       CHECK_FAIL);

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
        }
    }
    return 0;
}

int check_block_sizes_seek(struct test_state *state)
{
    for (int bs = 8; bs <= 64; bs *= 2) {
        int status;
        state->strm->block_size = bs;
        state->strm->rsi = (int)(state->buf_len
                                 / (bs * state->bytes_per_sample));

        status = encode_decode_large_seek(state);
        if (status)
            return status;
    }
    return 0;
}

int check_rsi_seek(struct test_state *state)
{
    int status;
    int size = state->bytes_per_sample;

    for (unsigned char *tmp = state->ubuf;
         tmp < state->ubuf + state->buf_len;
         tmp += 2 * state->bytes_per_sample) {
        state->out(tmp, state->xmax - ((state->ubuf - tmp) % 64), size);
        state->out(tmp + size, state->xmin, size);
    }

    printf("Checking seeking ... ");
    status = check_block_sizes_seek(state);
    if (status)
        return status;

    printf ("%s\n", CHECK_PASS);
    return 0;
}

int main (void)
{
    int status;
    struct aec_stream strm;
    struct test_state state;

    state.dump = 0;
    state.buf_len = state.ibuf_len = BUF_SIZE;
    state.cbuf_len = 3 * BUF_SIZE;

    state.ubuf = (unsigned char *)malloc(state.buf_len);
    state.cbuf = (unsigned char *)malloc(state.cbuf_len);
    state.obuf = (unsigned char *)malloc(state.buf_len);

    if (!state.ubuf || !state.cbuf || !state.obuf) {
        printf("Not enough memory.\n");
        status = 99;
        goto DESTRUCT;
    }

    strm.flags = AEC_DATA_PREPROCESS;
    state.strm = &strm;
    strm.bits_per_sample = 32;
    update_state(&state);

    status = check_rsi_seek(&state);
    if (status)
        goto DESTRUCT;

DESTRUCT:
    if (state.ubuf)
        free(state.ubuf);
    if (state.cbuf)
        free(state.cbuf);
    if (state.obuf)
        free(state.obuf);

    return status;
}
