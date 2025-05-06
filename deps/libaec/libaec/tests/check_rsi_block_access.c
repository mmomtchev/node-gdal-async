#include "check_aec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

struct aec_context
{
    size_t nvalues;
    int flags;
    int rsi;
    int block_size;
    int bits_per_sample;
    int bytes_per_sample;
    unsigned char * obuf;
    unsigned char * ebuf;
    unsigned char * dbuf;
    size_t obuf_len;
    size_t ebuf_len;
    size_t dbuf_len;
    size_t ebuf_total;
};

typedef void (*data_generator_t)(struct aec_context *ctx);

static int get_input_bytes(struct aec_context *ctx)
{
    if (ctx->flags & AEC_DATA_3BYTE) {
        fprintf(stderr, "AES_DATA_3BYTE is not supported\n");
        exit(1);
    }
    if (ctx->bits_per_sample < 1 || ctx->bits_per_sample > 32) {
        fprintf(stderr, "Invalid bits_per_sample: %d\n", ctx->bits_per_sample);
        exit(1);
    }
    int nbytes =  (ctx->bits_per_sample + 7) / 8;
    if (nbytes == 3) nbytes = 4;
    return nbytes;
}

static void data_generator_zero(struct aec_context *ctx)
{
    size_t nbytes = ctx->bytes_per_sample;
    if (ctx->obuf_len % nbytes) {
        fprintf(stderr, "Invalid buffer_size: %zu\n", ctx->obuf_len);
        exit(1);
    }

    size_t nvalues = ctx->obuf_len / nbytes;

    for (size_t i = 0; i < nvalues; i++) {
        size_t value = 0;
        unsigned char *value_p = (unsigned char*) &value;
        for (size_t j = 0; j < nbytes; j++) {
            if (ctx->flags & AEC_DATA_MSB)
                ctx->obuf[i * nbytes + j] = value_p[nbytes - j - 1];
            else
                ctx->obuf[i * nbytes + j] = value_p[j];
        }
    }
}

static void data_generator_random(struct aec_context *ctx)
{
    size_t nbytes = ctx->bytes_per_sample;
    if (ctx->obuf_len % nbytes) {
        fprintf(stderr, "Invalid buffer_size: %zu\n", ctx->obuf_len);
        exit(1);
    }

    size_t nvalues = ctx->obuf_len / nbytes;
    size_t mask = (1 << (ctx->bits_per_sample - 1))-1;

    for (size_t i = 0; i < nvalues; i++) {
        size_t value = rand() & mask;
        unsigned char *value_p = (unsigned char*) &value;

        for (size_t j = 0; j < nbytes; j++) {
            if (ctx->flags & AEC_DATA_MSB) {
                ctx->obuf[i * nbytes + j] = value_p[nbytes - j - 1];
            }
            else {
                ctx->obuf[i * nbytes + j] = value_p[j];
            }
        }
    }
}

static void data_generator_incr(struct aec_context *ctx)
{
    size_t nbytes = ctx->bytes_per_sample;
    if (ctx->obuf_len % nbytes) {
        fprintf(stderr, "Invalid buffer_size: %zu\n", ctx->obuf_len);
        exit(1);
    }

    size_t nvalues = ctx->obuf_len / nbytes;
    size_t max_value = 1 << (ctx->bits_per_sample - 1);

    for (size_t i = 0; i < nvalues; i++) {
        size_t value = i % max_value;
        unsigned char *value_p = (unsigned char*) &value;
        for (size_t j = 0; j < nbytes; j++) {
            if (ctx->flags & AEC_DATA_MSB) {
                ctx->obuf[i * nbytes + j] = value_p[nbytes - j - 1];
            }
            else {
                ctx->obuf[i * nbytes + j] = value_p[j];
            }
        }
    }
}

#define PREPARE_ENCODE(strm_e, ctx, flags) \
{ \
    (strm_e)->flags = flags; \
    (strm_e)->rsi = (ctx)->rsi; \
    (strm_e)->block_size = (ctx)->block_size; \
    (strm_e)->bits_per_sample = (ctx)->bits_per_sample; \
    (strm_e)->next_in = (ctx)->obuf; \
    (strm_e)->avail_in = (ctx)->obuf_len; \
    (strm_e)->next_out = (ctx)->ebuf; \
    (strm_e)->avail_out = (ctx)->ebuf_len; \
    int status = 0; \
    if ((status = aec_buffer_encode((strm_e))) != 0) { \
        return status; \
    } \
    (ctx)->ebuf_total = (strm_e)->total_out; \
    \
    struct aec_stream strm_d; \
    strm_d = (*strm_e); \
    strm_d.next_in = (ctx)->ebuf; \
    strm_d.avail_in = (ctx)->ebuf_total; \
    strm_d.next_out = (ctx)->dbuf; \
    strm_d.avail_out = (ctx)->dbuf_len; \
    if ((status = aec_buffer_decode((&strm_d))) != 0) { \
        return status; \
    } \
}

#define PREPARE_ENCODE_WITH_OFFSETS(strm_eo, ctx, flags, offsets_ptr, offsets_count_ptr) \
{ \
    (strm_eo)->flags = flags; \
    (strm_eo)->rsi = (ctx)->rsi; \
    (strm_eo)->block_size = (ctx)->block_size; \
    (strm_eo)->bits_per_sample = (ctx)->bits_per_sample; \
    (strm_eo)->next_in = (ctx)->obuf; \
    (strm_eo)->avail_in = (ctx)->obuf_len; \
    (strm_eo)->next_out = (ctx)->ebuf; \
    (strm_eo)->avail_out = (ctx)->ebuf_len; \
    int status = 0; \
    if ((status = aec_encode_init((strm_eo))) != AEC_OK) { \
        return(status); \
    } \
    aec_encode_enable_offsets((strm_eo)); \
    if ((status = aec_encode((strm_eo), AEC_FLUSH)) != 0) { \
        return(status); \
    } \
    aec_encode_count_offsets((strm_eo), (offsets_count_ptr)); \
    (offsets_ptr) = (size_t*) malloc(sizeof(*(offsets_ptr)) * *(offsets_count_ptr)); \
    if ((status = aec_encode_get_offsets((strm_eo), (offsets_ptr), *(offsets_count_ptr)))) { \
        return(status); \
    } \
    aec_encode_end((strm_eo)); \
    ctx->ebuf_total = (strm_eo)->total_out; \
}

#define PREPARE_DECODE_WITH_OFFSETS(strm_do, ctx, flags, offsets_ptr, offsets_count_ptr) \
{ \
    (strm_do)->flags = (ctx)->flags; \
    (strm_do)->rsi = (ctx)->rsi; \
    (strm_do)->block_size = (ctx)->block_size; \
    (strm_do)->bits_per_sample = (ctx)->bits_per_sample; \
    (strm_do)->next_in = (ctx)->ebuf; \
    (strm_do)->avail_in = (ctx)->ebuf_total; \
    (strm_do)->next_out = (ctx)->dbuf; \
    (strm_do)->avail_out = (ctx)->dbuf_len; \
    if ((status = aec_decode_init((strm_do))) != AEC_OK) { \
        return status; \
    } \
    if ((status = aec_decode_enable_offsets((strm_do))) != AEC_OK) { \
        return status; \
    }; \
    if ((status = aec_decode((strm_do), AEC_FLUSH)) != AEC_OK) { \
        return status; \
    } \
    if ((status = aec_decode_count_offsets((strm_do), (offsets_count_ptr))) != AEC_OK) { \
        return status; \
    } \
    (offsets_ptr) = (size_t*) malloc(sizeof(*(offsets_ptr)) * *(offsets_count_ptr)); \
    if ((status = aec_decode_get_offsets((strm_do), (offsets_ptr), *(offsets_count_ptr))) != AEC_OK) { \
        return status; \
    }; \
    aec_decode_end((strm_do)); \
    for (size_t i = 0; i < (strm_do)->total_out; ++i) { \
        if ((ctx)->dbuf[i] != (ctx)->obuf[i]) { \
            return 104; \
        } \
    } \
}

static int aec_rsi_at(struct aec_stream *strm, const size_t *offsets,
               size_t offsets_count, size_t idx)
{
    if (offsets == NULL || idx >= offsets_count) return AEC_RSI_OFFSETS_ERROR;

    int status = 0;
    size_t rsi_offset = offsets[idx];

    if ((status = aec_decode_init(strm)) != AEC_OK)
        return status;
    if ((status = aec_buffer_seek(strm, rsi_offset)) != AEC_OK)
        return status;
    if ((status = aec_decode(strm, AEC_FLUSH)) != AEC_OK)
        return status;
    aec_decode_end(strm);

    return AEC_OK;
}

static int test_rsi_at(struct aec_context *ctx)
{
    int status = AEC_OK;
    int flags = ctx->flags;

    struct aec_stream strm_encode;
    PREPARE_ENCODE(&strm_encode, ctx, flags);

    struct aec_stream strm_decode;
    size_t *offsets;
    size_t offsets_count;
    PREPARE_DECODE_WITH_OFFSETS(&strm_decode, ctx, flags, offsets, &offsets_count);

    size_t rsi_len = ctx->rsi * ctx->block_size * ctx->bytes_per_sample;
    unsigned char *rsi_buf = malloc(rsi_len);
    if (rsi_buf == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate rsi buffer\n");
        exit(1);
    }

    for (size_t i = 0; i < offsets_count; ++i) {
        struct aec_stream strm_at;
        strm_at.flags = flags;
        strm_at.rsi = ctx->rsi;
        strm_at.block_size = ctx->block_size;
        strm_at.bits_per_sample = ctx->bits_per_sample;
        strm_at.next_in = ctx->ebuf;
        strm_at.avail_in = ctx->ebuf_total;
        strm_at.next_out = rsi_buf;
        strm_at.avail_out = ctx->dbuf_len - i * rsi_len > rsi_len ? rsi_len : ctx->dbuf_len % rsi_len;

        if ((status = aec_rsi_at(&strm_at, offsets, offsets_count, i)) != AEC_OK) {
            return status;
        }
        for (size_t j = 0; j < strm_at.total_out; j++) {
            if (j == (ctx->rsi * ctx->block_size * ctx->bytes_per_sample + j > ctx->obuf_len)) {
                break;
            }
            if (rsi_buf[j] != ctx->obuf[i * ctx->block_size * ctx->rsi * ctx->bytes_per_sample + j]) {
                return 101;
            }
        }
    }

    free(offsets);
    free(rsi_buf);
    return status;
}

int test_read(struct aec_context *ctx)
{
    int status = AEC_OK;
    int flags = ctx->flags;

    struct aec_stream strm_encode;
    PREPARE_ENCODE(&strm_encode, ctx, flags);

    struct aec_stream strm_decode;
    size_t *offsets = NULL;
    size_t offsets_size = 0;
    PREPARE_DECODE_WITH_OFFSETS(&strm_decode, ctx, flags, offsets, &offsets_size);
    // Edge case: Imposible to get wanted number of slices
    size_t wanted_num_slices = 3;
    if (wanted_num_slices > ctx->obuf_len) {
        wanted_num_slices = ctx->obuf_len;
    }

    // Optimize the size of the last slice
    // Make sure that the last slice is not too small
    size_t slice_size = (ctx->obuf_len % ((ctx->obuf_len / wanted_num_slices) * wanted_num_slices)) == 0 ? ctx->obuf_len / wanted_num_slices : ctx->obuf_len / wanted_num_slices + 1;

    size_t num_slices = ctx->obuf_len / slice_size;
    size_t remainder = ctx->obuf_len % slice_size;

    size_t *slice_offsets = malloc((num_slices + 1) * sizeof(slice_offsets[0])); 
    size_t *slice_sizes = malloc((num_slices + 1) * sizeof(slice_sizes[0]));

    for (size_t i = 0; i < num_slices; ++i) {
        slice_offsets[i] = slice_size * i;
        slice_sizes[i] = slice_size;
    }
    if (remainder > 0) {
        slice_offsets[num_slices] = slice_size * num_slices;
        slice_sizes[num_slices] = remainder;
        ++num_slices;
    }

    struct aec_stream strm_read;
    strm_read.flags = ctx->flags;
    strm_read.rsi = ctx->rsi;
    strm_read.block_size = ctx->block_size;
    strm_read.bits_per_sample = ctx->bits_per_sample;
    strm_read.next_in = ctx->ebuf;
    strm_read.avail_in = strm_encode.total_out;
    strm_read.next_out = ctx->dbuf;
    strm_read.avail_out = ctx->dbuf_len;

    if ((status = aec_decode_init(&strm_read)) != AEC_OK)
        return status;

    // Test 1: Stream data
    for (size_t i = 0; i < num_slices; ++i) {
        if ((status = aec_decode_range(&strm_read, offsets, offsets_size, slice_offsets[i], slice_sizes[i])) != AEC_OK) {
            return status;
        }
    }

    for (size_t i = 0; i < ctx->obuf_len; ++i) {
        if (ctx->obuf[i] != ctx->dbuf[i]) {
            fprintf(stderr, "Index: %zu  Size: %zu strm_read.total_out: %zu\n", i, ctx->obuf_len, strm_read.total_out);
            fprintf(stderr, "Expected: %u  Got: %u\n", ctx->obuf[i], ctx->dbuf[i]);
            assert(0);
            return 102;
        }
    }

    // Test 2: Read slices
    for (size_t i = 0; i < num_slices; ++i) {
        size_t buf_size = slice_sizes[i];;
        unsigned char *buf = malloc(buf_size);
        if (buf == NULL) {
            fprintf(stderr, "Error: malloc failed\n");
            return 1;

        }
        strm_read.next_out = buf;
        strm_read.avail_out = buf_size;
        strm_read.total_out = 0;
        if ((status = aec_decode_range(&strm_read, offsets, offsets_size, slice_offsets[i], buf_size)) != AEC_OK) {
            return status;
        }
        for (size_t j = 0; j < buf_size; ++j) {
            if (ctx->obuf[slice_offsets[i] + j] != buf[j]) {
                return 103;
            }
        }
        free(buf);
    }

    aec_decode_end(&strm_read);

    free(offsets);
    free(slice_offsets);
    free(slice_sizes);
    return status;
}


int test_offsets(struct aec_context *ctx)
{
    int status = AEC_OK;
    int flags = ctx->flags;

    struct aec_stream strm1;
    size_t *encode_offsets_ptr;
    size_t encode_offsets_size;
    PREPARE_ENCODE_WITH_OFFSETS(&strm1, ctx, flags, encode_offsets_ptr, &encode_offsets_size);
    assert(encode_offsets_size > 0);

    struct aec_stream strm2;
    size_t *decode_offsets_ptr;
    size_t decode_offsets_size;
    PREPARE_DECODE_WITH_OFFSETS(&strm2, ctx, flags, decode_offsets_ptr, &decode_offsets_size);
    assert(decode_offsets_size > 0);

    if (encode_offsets_size != decode_offsets_size) {
        fprintf(stderr, "Error: encode_offsets_size = %zu, decode_offsets_size = %zu\n", encode_offsets_size, decode_offsets_size);
        return 102;
    }

    for (size_t i = 0; i < encode_offsets_size; ++i) {
        if (encode_offsets_ptr[i] != decode_offsets_ptr[i]) {
            fprintf(stderr, "Error: encode_offsets_ptr[%zu] = %zu, decode_offsets_ptr[%zu] = %zu\n", i, encode_offsets_ptr[i], i, decode_offsets_ptr[i]);
            return 103;
        }
    }

    free(decode_offsets_ptr);
    free(encode_offsets_ptr);
    return status;
}

int main(void)
{
    int status    = AEC_OK;
    size_t ns[]   = {1, 255, 256, 255*10, 256*10, 67000};  // number of samples
    size_t rsis[] = {1, 2, 255, 256, 512, 1024, 4095, 4096};  // RSI size
    size_t bss[]  = {8, 16, 32, 64};  // block size
    size_t bpss[] = {1, 7, 8, 9, 15, 16, 17, 23, 24, 25, 31, 32};  // bits per sample
    data_generator_t data_generators[] = {data_generator_zero, data_generator_random, data_generator_incr};

    for (size_t n_i = 0; n_i < sizeof(ns) / sizeof(ns[0]); ++n_i) {
        for (size_t rsi_i = 0; rsi_i < sizeof(rsis) / sizeof(rsis[0]); ++rsi_i) {
            for (size_t bs_i = 0; bs_i < sizeof(bss) / sizeof(bss[0]); ++bs_i) {
                for (size_t bps_i = 0; bps_i < sizeof(bpss) / sizeof(bpss[0]); ++bps_i) {
                    struct aec_context ctx;
                    ctx.nvalues = ns[n_i];
                    ctx.flags = AEC_DATA_PREPROCESS;
                    ctx.rsi = rsis[rsi_i];
                    ctx.block_size = bss[bs_i];
                    ctx.bits_per_sample = bpss[bps_i];
                    ctx.bytes_per_sample = get_input_bytes(&ctx);
                    size_t input_size = ctx.nvalues * ctx.bytes_per_sample;
                    ctx.obuf_len = input_size;
                    ctx.ebuf_len = input_size * 67 / 64 + 256;
                    ctx.dbuf_len = input_size;
                    ctx.obuf = malloc(ctx.obuf_len);
                    ctx.ebuf = malloc(ctx.ebuf_len);
                    ctx.dbuf = malloc(ctx.dbuf_len);
                    if (ctx.obuf == NULL || ctx.ebuf == NULL || ctx.dbuf == NULL) {
                        fprintf(stderr, "Error: Failed allocating memory\n");
                        exit(1);
                    }

                    for (size_t i = 0; i < sizeof(data_generators) / sizeof(data_generators[0]); ++i) {
                        data_generators[i](&ctx);

                        status = test_rsi_at(&ctx);
                        fprintf(stderr,
                                "Testing test_rsi_at()  "
                                "nvalues=%zu, rsi=%zu, block_size=%zu, bits_per_sample=%zu ... %s\n",
                                ns[n_i], rsis[rsi_i], bss[bs_i], bpss[bps_i], status == AEC_OK ? CHECK_PASS : CHECK_FAIL);
                        if (status != AEC_OK)
                            return status;

                        status = test_read(&ctx);
                        fprintf(stderr,
                                "Testing test_read()    "
                                "nvalues=%zu, rsi=%zu, block_size=%zu, bits_per_sample=%zu ... %s\n",
                                ns[n_i], rsis[rsi_i], bss[bs_i], bpss[bps_i], status == AEC_OK ? CHECK_PASS : CHECK_FAIL);
                        if (status != AEC_OK)
                            return status;

                        status = test_offsets(&ctx);
                        fprintf(stderr,
                                "Testing test_offsets() "
                                "nvalues=%zu, rsi=%zu, block_size=%zu, bits_per_sample=%zu ... %s\n",
                                ns[n_i], rsis[rsi_i], bss[bs_i], bpss[bps_i], status == AEC_OK ? CHECK_PASS : CHECK_FAIL);
                        if (status != AEC_OK)
                            return status;
                    }
                    free(ctx.obuf);
                    free(ctx.ebuf);
                    free(ctx.dbuf);
                }
            }
        }
    }
    return status;
}
