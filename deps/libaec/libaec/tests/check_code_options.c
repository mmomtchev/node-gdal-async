#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check_aec.h"

#define BUF_SIZE 1024 * 3

int check_block_sizes(struct test_state *state, int id, int id_len)
{
    state->id = id;
    for (int bs = 8; bs <= 64; bs *= 2) {
        int max_rsi;

        state->strm->block_size = bs;
        max_rsi = (int)(state->buf_len / (bs * state->bytes_per_sample));
        if (max_rsi > 4096)
            max_rsi = 4096;

        for (int rsi = 1; rsi <= max_rsi; rsi++) {
            int status;
            state->strm->rsi = rsi;
            status = state->codec(state);
            if (status)
                return status;

            if ((state->cbuf[0] >> (8 - id_len)) != id) {
                printf(
                    "%s: block of size %i created with ID:%x, expected %x.\n",
                    CHECK_FAIL, bs, state->cbuf[0] >> (8 - id_len), id
                    );
                return 99;
            }
        }
    }
    return 0;
}

int check_zero(struct test_state *state)
{
    int status;

    if (state->strm->flags & AEC_DATA_PREPROCESS)
        memset(state->ubuf, 0x55, state->buf_len);
    else
        memset(state->ubuf, 0, state->buf_len);

    printf("Checking zero blocks ... ");
    status = check_block_sizes(state, 0, state->id_len + 1);
    if (status)
        return status;

    printf ("%s\n", CHECK_PASS);
    return 0;
}

int check_splitting(struct test_state *state, int k)
{
    int status;
    unsigned char *tmp;

    int size = state->bytes_per_sample;

    if (state->strm->flags & AEC_DATA_PREPROCESS) {
        for (tmp = state->ubuf;
             tmp < state->ubuf + state->buf_len;
             tmp += 4 * size) {
            state->out(tmp, state->xmin + (1ULL << (k - 1)) - 1, size);
            state->out(tmp + size, state->xmin, size);
            state->out(tmp + 2 * size, state->xmin
                       + (1ULL << (k + 1)) - 1, size);
            state->out(tmp + 3 * size, state->xmin, size);
        }
    } else {
        for (tmp = state->ubuf;
             tmp < state->ubuf + state->buf_len;
             tmp += 4 * size) {
            state->out(tmp, 0, size);
            state->out(tmp + size, (1ULL << k) - 1, size);
            state->out(tmp + 2 * size, 0, size);
            state->out(tmp + 3 * size, (1ULL << (k + 2)) - 1, size);
        }
    }

    printf("Checking splitting with k=%i ... ", k);
    status = check_block_sizes(state, k + 1, state->id_len);
    if (status)
        return status;

    printf ("%s\n", CHECK_PASS);
    return 0;
}

int check_uncompressed(struct test_state *state)
{
    int status;
    unsigned char *tmp;

    int size = state->bytes_per_sample;

    for (tmp = state->ubuf;
         tmp < state->ubuf + state->buf_len;
         tmp += 2 * size) {
        state->out(tmp, state->xmax, size);
        state->out(tmp + size, state->xmin, size);
    }

    printf("Checking uncompressed ... ");
    status = check_block_sizes(state,
                               (1ULL << state->id_len) - 1,
                               state->id_len);
    if (status)
        return status;

    printf ("%s\n", CHECK_PASS);
    return 0;
}

int check_fs(struct test_state *state)
{
    int status;
    unsigned char *tmp;

    int size = state->bytes_per_sample;

    if (state->strm->flags & AEC_DATA_PREPROCESS) {
        for (tmp = state->ubuf;
             tmp < state->ubuf + state->buf_len;
             tmp += 4 * size) {
            state->out(tmp, state->xmin + 2, size);
            state->out(tmp + size, state->xmin, size);
            state->out(tmp + 2 * size, state->xmin, size);
            state->out(tmp + 3 * size, state->xmin, size);
        }
    } else {
        for (tmp = state->ubuf;
             tmp < state->ubuf + state->buf_len;
             tmp += 4 * size) {
            state->out(tmp, 0, size);
            state->out(tmp + size, 0, size);
            state->out(tmp + 2 * size, 0, size);
            state->out(tmp + 3 * size, 4, size);
        }
    }

    printf("Checking FS ... ");
    status = check_block_sizes(state, 1, state->id_len);
    if (status)
        return status;

    printf ("%s\n", CHECK_PASS);
    return 0;
}

int check_se(struct test_state *state)
{
    int status;
    unsigned char *tmp;

    int size = state->bytes_per_sample;

    if (state->strm->flags & AEC_DATA_PREPROCESS) {
        for (tmp = state->ubuf;
             tmp < state->ubuf + state->buf_len;
             tmp += 8 * size) {
            state->out(tmp, state->xmax - 1, size);
            state->out(tmp + size, state->xmax - 1, size);
            state->out(tmp + 2 * size, state->xmax - 1, size);
            state->out(tmp + 3 * size, state->xmax - 1, size);
            state->out(tmp + 4 * size, state->xmax, size);
            state->out(tmp + 5 * size, state->xmax, size);
            state->out(tmp + 6 * size, state->xmax, size);
            state->out(tmp + 7 * size, state->xmax, size);
        }
    } else {
        for (tmp = state->ubuf;
             tmp < state->ubuf + state->buf_len;
             tmp += 8 * size) {
            state->out(tmp, 0, size);
            state->out(tmp + size, 0, size);
            state->out(tmp + 2 * size, 0, size);
            state->out(tmp + 3 * size, 0, size);
            state->out(tmp + 4 * size, 1, size);
            state->out(tmp + 5 * size, 0, size);
            state->out(tmp + 6 * size, 0, size);
            state->out(tmp + 7 * size, 2, size);
        }
    }

    printf("Checking Second Extension ... ");
    status = check_block_sizes(state, 1, state->id_len + 1);
    if (status)
        return status;

    printf ("%s\n", CHECK_PASS);
    return 0;
}

int check_bps(struct test_state *state)
{
    int status;

    for (int bps = 8; bps <= 32; bps += 8) {
        state->strm->bits_per_sample = bps;
        if (bps == 24)
            state->strm->flags |= AEC_DATA_3BYTE;
        else
            state->strm->flags &= ~AEC_DATA_3BYTE;

        update_state(state);

        status = check_zero(state);
        if (status)
            return status;

        status = check_se(state);
        if (status)
            return status;

        status = check_uncompressed(state);
        if (status)
            return status;

        status = check_fs(state);
        if (status)
            return status;

        for (int k = 1; k < bps - 2; k++) {
            status = check_splitting(state, k);
            if (status)
                return status;
        }
        printf("All checks with %i bit per sample passed.\n", bps);
    }
    return 0;
}

int check_byte_orderings(struct test_state *state)
{
    int status;

    printf("-----------------------------------\n");
    printf("Checking no PP, LSB first, unsigned\n");
    printf("-----------------------------------\n");
    status = check_bps(state);
    if (status)
        return status;

    printf("-----------------------------------\n");
    printf("Checking PP, LSB first, unsigned\n");
    printf("-----------------------------------\n");
    state->strm->flags |= AEC_DATA_PREPROCESS;
    status = check_bps(state);
    if (status)
        return status;

    printf("-----------------------------------\n");
    printf("Checking PP, LSB first, signed\n");
    printf("-----------------------------------\n");
    state->strm->flags |= AEC_DATA_SIGNED;

    status = check_bps(state);
    if (status)
        return status;

    state->strm->flags &= ~AEC_DATA_SIGNED;
    state->strm->flags |= AEC_DATA_MSB;

    printf("-----------------------------------\n");
    printf("Checking PP, MSB first, unsigned\n");
    printf("-----------------------------------\n");
    status = check_bps(state);
    if (status)
        return status;

    printf("-----------------------------------\n");
    printf("Checking PP, MSB first, signed\n");
    printf("-----------------------------------\n");
    state->strm->flags |= AEC_DATA_SIGNED;

    status = check_bps(state);
    if (status)
        return status;
    return 0;
}

int main(int argc, char *argv[])
{
    int status;
    struct aec_stream strm;
    struct test_state state;

    if (argc > 1 && strncmp(argv[1], "-d", 2) == 0)
        state.dump = 1;
    else
        state.dump = 0;

    state.buf_len = state.ibuf_len = BUF_SIZE;
    state.cbuf_len = 2 * BUF_SIZE;

    state.ubuf = (unsigned char *)malloc(state.buf_len);
    state.cbuf = (unsigned char *)malloc(state.cbuf_len);
    state.obuf = (unsigned char *)malloc(state.buf_len);

    if (!state.ubuf || !state.cbuf || !state.obuf) {
        printf("Not enough memory.\n");
        status = 99;
        goto DESTRUCT;
    }

    strm.flags = 0;
    state.strm = &strm;

    printf("***************************\n");
    printf("Checking with small buffers\n");
    printf("***************************\n");
    state.codec = encode_decode_small;
    status = check_byte_orderings(&state);
    if (status)
        goto DESTRUCT;

    printf("***************************\n");
    printf("Checking with large buffers\n");
    printf("***************************\n");
    state.codec = encode_decode_large;
    status = check_byte_orderings(&state);

DESTRUCT:
    if (state.ubuf)
        free(state.ubuf);
    if (state.cbuf)
        free(state.cbuf);
    if (state.obuf)
        free(state.obuf);

    return status;
}
