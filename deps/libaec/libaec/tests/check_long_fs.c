#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check_aec.h"

#define BUF_SIZE (64 * 4)

int check_long_fs(struct test_state *state)
{
    int size = state->bytes_per_sample;
    int bs = state->strm->block_size;

    for (int i = 0; i < bs / 2; i++) {
        state->out(state->ubuf + size * i, state->xmin, size);
        state->out(state->ubuf + bs * size / 2 + size * i, 65000, size);
    }

    printf("Checking long fs ... ");

    const int status = state->codec(state);
    if (status == 0)
      printf ("%s\n", CHECK_PASS);

    return status;
}

int main(void)
{
    struct test_state state;
    state.dump = 0;
    state.buf_len = state.ibuf_len = BUF_SIZE;
    state.cbuf_len = 2 * BUF_SIZE;

    state.ubuf = (unsigned char *)malloc(state.buf_len);
    state.cbuf = (unsigned char *)malloc(state.cbuf_len);
    state.obuf = (unsigned char *)malloc(state.buf_len);

    if (!state.ubuf || !state.cbuf || !state.obuf) {
        fprintf(stderr, "Not enough memory.\n");

        free(state.ubuf);
        free(state.cbuf);
        free(state.obuf);

        return 99;
    }

    struct aec_stream strm;
    strm.flags = AEC_DATA_PREPROCESS;
    state.strm = &strm;
    strm.bits_per_sample = 16;
    strm.block_size = 64;
    strm.rsi = 1;
    state.codec = encode_decode_large;
    update_state(&state);

    const int status = check_long_fs(&state);

    free(state.ubuf);
    free(state.cbuf);
    free(state.obuf);

    return status;
}
