#ifndef CHECK_AEC_H
#define CHECK_AEC_H 1

#include <config.h>
#include "libaec.h"

struct test_state {
    int (* codec)(struct test_state *state);
    int id;
    int id_len;
    int bytes_per_sample;
    unsigned char *ubuf;
    unsigned char *cbuf;
    unsigned char *obuf;
    size_t ibuf_len; /* input buffer length may be shorter than buf_len */
    size_t buf_len;
    size_t cbuf_len;
    long long int xmax;
    long long int xmin;
    void (*out)(unsigned char *dest, unsigned long long int val, int size);
    int dump; /* dump buffer to file for fuzzing corpus */
    struct aec_stream *strm;
};

int update_state(struct test_state *state);
int encode_decode_small(struct test_state *state);
int encode_decode_large(struct test_state *state);

#ifndef HAVE_SNPRINTF
#ifdef HAVE__SNPRINTF_S
#define snprintf(d, n, ...) _snprintf_s((d), (n), _TRUNCATE, __VA_ARGS__)
#else
#ifdef HAVE__SNPRINTF
#define snprintf _snprintf
#else
#error "no snprintf compatible function found"
#endif /* HAVE__SNPRINTF */
#endif /* HAVE__SNPRINTF_S */
#endif /* HAVE_SNPRINTF */

#ifdef _WIN32
#define CHECK_PASS "PASS"
#define CHECK_FAIL "FAIL"
#else
#define CHECK_PASS "[0;32mPASS[0m"
#define CHECK_FAIL "[0;31mFAIL[0m"
#endif

#endif /* CHECK_AEC_H */
