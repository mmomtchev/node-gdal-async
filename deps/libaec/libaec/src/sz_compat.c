/**
 * @file sz_compat.c
 *
 * @section LICENSE
 * Copyright 2024 Mathis Rosenhauer, Moritz Hanke, Joerg Behrens, Luis Kornblueh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * Adaptive Entropy Coding library
 *
 */

#include "config.h"
#include "szlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOPTS 129
#define MIN(a, b) (((a) < (b))? (a): (b))

static int convert_options(int sz_opts)
{
    int co[NOPTS];
    int opts = 0;

    memset(co, 0, sizeof(int) * NOPTS);
    co[SZ_MSB_OPTION_MASK] = AEC_DATA_MSB;
    co[SZ_NN_OPTION_MASK] = AEC_DATA_PREPROCESS;

    for (int i = 1; i < NOPTS; i <<= 1)
        if (sz_opts & i)
            opts |= co[i];

    return opts;
}

static int bits_to_bytes(int bit_length)
{
    if (bit_length > 16)
        return 4;
    else if (bit_length > 8)
        return 2;
    else
        return 1;
}

static void interleave_buffer(void *dest, const void *src,
                              size_t n, size_t wordsize)
{
    const unsigned char *src8 = (unsigned char *)src;
    unsigned char *dest8 = (unsigned char *)dest;

    for (size_t i = 0; i < n / wordsize; i++)
        for (size_t j = 0; j < wordsize; j++)
            dest8[j * (n / wordsize) + i] = src8[i * wordsize + j];
}

static void deinterleave_buffer(void *dest, const void *src,
                                size_t n, size_t wordsize)
{
    const unsigned char *src8 = (unsigned char *)src;
    unsigned char *dest8 = (unsigned char *)dest;

    for (size_t i = 0; i < n / wordsize; i++)
        for (size_t j = 0; j < wordsize; j++)
            dest8[i * wordsize + j] = src8[j * (n / wordsize) + i];
}

static void add_padding(void *dest, const void *src, size_t src_length,
                        size_t line_size, size_t padding_size,
                        int pixel_size, int pp)
{
    const char zero_pixel[] = {0, 0, 0, 0};

    const char *pixel = zero_pixel;
    size_t j = 0;
    size_t i = 0;
    while (i < src_length) {
        size_t ps;
        size_t ls = MIN(src_length - i, line_size);
        memcpy((char *)dest + j, (char *)src + i, ls);
        j += ls;
        i += ls;
        if (pp)
            pixel = (char *)src + i - pixel_size;
        ps = line_size + padding_size - ls;
        for (size_t k = 0; k < ps; k += pixel_size)
            memcpy((char *)dest + j + k, pixel, pixel_size);
        j += ps;
    }
}

static void remove_padding(void *buf, size_t buf_length,
                           size_t line_size, size_t padding_size,
                           int pixel_size)
{
    size_t padded_line_size = line_size + padding_size;

    size_t i = line_size;
    for (size_t j = padded_line_size; j < buf_length; j += padded_line_size) {
        memmove((char *)buf + i, (char *)buf + j, line_size);
        i += line_size;
    }
}

int SZ_BufftoBuffCompress(void *dest, size_t *destLen,
                          const void *source, size_t sourceLen,
                          SZ_com_t *param)
{
    struct aec_stream strm;
    void *buf = 0;
    void *padbuf = 0;
    int status;
    int interleave;
    int pixel_size;
    int aec_status;
    size_t scanlines;
    size_t padbuf_size;
    size_t padding_size;

    strm.block_size = param->pixels_per_block;
    strm.rsi = (param->pixels_per_scanline + param->pixels_per_block - 1)
        / param->pixels_per_block;
    strm.flags = AEC_NOT_ENFORCE | convert_options(param->options_mask);
    strm.avail_out = *destLen;
    strm.next_out = dest;

    interleave = param->bits_per_pixel == 32 || param->bits_per_pixel == 64;
    if (interleave) {
        strm.bits_per_sample = 8;
        buf = malloc(sourceLen);
        if (buf == NULL) {
            status = SZ_MEM_ERROR;
            goto CLEANUP;
        }
        interleave_buffer(buf, source, sourceLen, param->bits_per_pixel / 8);
    } else {
        strm.bits_per_sample = param->bits_per_pixel;
        buf = (void *)source;
    }

    pixel_size = bits_to_bytes(strm.bits_per_sample);

    scanlines = (sourceLen / pixel_size + param->pixels_per_scanline - 1)
        / param->pixels_per_scanline;
    padbuf_size = strm.rsi * strm.block_size * pixel_size * scanlines;
    padbuf = malloc(padbuf_size);
    if (padbuf == NULL) {
        status = SZ_MEM_ERROR;
        goto CLEANUP;
    }

    padding_size =
        (strm.rsi * strm.block_size - param->pixels_per_scanline)
        * pixel_size;

    add_padding(padbuf, buf, sourceLen,
                param->pixels_per_scanline * pixel_size,
                padding_size, pixel_size,
                strm.flags & AEC_DATA_PREPROCESS);
    strm.next_in = padbuf;
    strm.avail_in = padbuf_size;

    aec_status = aec_buffer_encode(&strm);
    if (aec_status == AEC_STREAM_ERROR)
        status = SZ_OUTBUFF_FULL;
    else
        status = aec_status;
    *destLen = strm.total_out;

CLEANUP:
    if (padbuf)
        free(padbuf);
    if (interleave && buf)
        free(buf);
    return status;
}

int SZ_BufftoBuffDecompress(void *dest, size_t *destLen,
                            const void *source, size_t sourceLen,
                            SZ_com_t *param)
{
    struct aec_stream strm;
    void *buf = 0;
    int status;
    int pad_scanline;
    int deinterleave;
    int extra_buffer;
    int pixel_size;
    size_t total_out;
    size_t scanlines;

    strm.block_size = param->pixels_per_block;
    strm.rsi = (param->pixels_per_scanline + param->pixels_per_block - 1)
        / param->pixels_per_block;
    strm.flags = convert_options(param->options_mask);
    strm.avail_in = sourceLen;
    strm.next_in = source;

    pad_scanline = param->pixels_per_scanline % param->pixels_per_block;
    deinterleave = (param->bits_per_pixel == 32
                        || param->bits_per_pixel == 64);
    extra_buffer = pad_scanline || deinterleave;

    if (deinterleave)
        strm.bits_per_sample = 8;
    else
        strm.bits_per_sample = param->bits_per_pixel;
    pixel_size = bits_to_bytes(strm.bits_per_sample);


    if (extra_buffer) {
        size_t buf_size;
        if (pad_scanline) {
            scanlines = (*destLen / pixel_size + param->pixels_per_scanline - 1)
                / param->pixels_per_scanline;
            buf_size = strm.rsi * strm.block_size * pixel_size * scanlines;
        } else {
            buf_size = *destLen;
        }
        buf = malloc(buf_size);
        if (buf == NULL) {
            status = SZ_MEM_ERROR;
            goto CLEANUP;
        }
        strm.next_out = buf;
        strm.avail_out = buf_size;
    } else {
        strm.next_out = dest;
        strm.avail_out = *destLen;
    }

    status = aec_buffer_decode(&strm);
    if (status != AEC_OK)
        goto CLEANUP;

    if (pad_scanline) {
        size_t padding_size =
            (strm.rsi * strm.block_size - param->pixels_per_scanline)
            * pixel_size;
        remove_padding(buf, strm.total_out,
                       param->pixels_per_scanline * pixel_size,
                       padding_size, pixel_size);
        total_out = scanlines * param->pixels_per_scanline * pixel_size;
    } else {
        total_out = strm.total_out;
    }

    if (total_out < *destLen)
        *destLen = total_out;

    if (deinterleave)
        deinterleave_buffer(dest, buf, *destLen, param->bits_per_pixel / 8);
    else if (pad_scanline)
        memcpy(dest, buf, *destLen);

CLEANUP:
    if (extra_buffer && buf)
        free(buf);

    return status;
}

int SZ_encoder_enabled(void)
{
    return 1;
}

/* netcdf searches for SZ_Compress in configure */
char SZ_Compress() { return SZ_OK; }
