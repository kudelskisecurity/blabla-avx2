/*
 * Optimized implementation of BlaBla for SSE2/SSSE3/AVX2.
 *
 * Copyright (C) 2017 Nagravision S.A.
*/

#ifdef SUPERCOP
#include "crypto_stream.h"
#endif

#include "blabla.h"
#include "config.h"
/* Intel intrinsics */
#include <immintrin.h>

typedef struct
{
    uint64_t key[4];
    uint64_t counter[4];
} blabla_ctxt;


void blabla_ctxt_init (blabla_ctxt *ctxt, const uint8_t *key, const uint8_t *nonce)
{
    memcpy (ctxt->key, key, 32);
    ctxt->counter[0] = constants[8];
    ctxt->counter[1] = 1;
    memcpy (&ctxt->counter[2], nonce, 16);
}

void blabla_ctxt_init_zero (blabla_ctxt *ctxt, const uint8_t *key)
{
    memcpy (ctxt->key, key, 32);
    ctxt->counter[0] = constants[8];
    ctxt->counter[1] = 1;
    memset (&ctxt->counter[2], 0, 16);
}


#ifdef HAVE_AVX2

#define BLOCKS_PER_CORE 4
#define MM_TYPE        __m256i
#define LOADU(m)       _mm256_loadu_si256 ((const __m256i *)(m))
#define STOREU(m, v)   _mm256_storeu_si256 ((__m256i *)(m), (v))
#define SET1_EPI64x(v) _mm256_set1_epi64x (v)
#define INIT_COUNTER   _mm256_set_epi64x (3, 2, 1, 0)

#define ADD(A, B) _mm256_add_epi64 (A, B)
#define XOR(A, B) _mm256_xor_si256 (A, B)

#define ROT16                                                                  \
    _mm256_setr_epi8 (2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9,    \
                      2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9)

#define ROT24                                                                  \
    _mm256_setr_epi8 (3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10,    \
                      3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10)

#define ROT(X, R)                                                              \
    (R) == 32 ? _mm256_shuffle_epi32 ((X), _MM_SHUFFLE (2, 3, 0, 1))           \
  : (R) == 24 ? _mm256_shuffle_epi8  ((X), ROT24)                              \
  : (R) == 16 ? _mm256_shuffle_epi8  ((X), ROT16)                              \
  :             XOR (_mm256_srli_epi64 ((X), (R)),                             \
                     _mm256_slli_epi64 ((X), 64 - (R)))

#else /* !HAVE_AVX2 */

#define BLOCKS_PER_CORE 2
#define MM_TYPE        __m128i
#define LOADU(m)       _mm_loadu_si128 ((const __m128i *)(m))
#define STOREU(m, v)   _mm_storeu_si128 ((__m128i *)(m), (v))
#define SET1_EPI64x(v) _mm_set1_epi64x (v)
#define INIT_COUNTER   _mm_set_epi64x (1, 0)

#define ADD(A, B) _mm_add_epi64 (A, B)
#define XOR(A, B) _mm_xor_si128 (A, B)


#ifdef HAVE_SSSE3

#define ROT16                                                                  \
    _mm_setr_epi8 (2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9)

#define ROT24                                                                  \
    _mm_setr_epi8 (3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10)

#define ROT(X, R)                                                              \
    (R) == 32 ? _mm_shuffle_epi32 ((X), _MM_SHUFFLE (2, 3, 0, 1))              \
  : (R) == 24 ? _mm_shuffle_epi8  ((X), ROT24)                                 \
  : (R) == 16 ? _mm_shuffle_epi8  ((X), ROT16)                                 \
  :             XOR (_mm_srli_epi64 ((X), (R)),                                \
                     _mm_slli_epi64 ((X), 64 - (R)))

#else /* !HAVE_SSSE3 */

#define ROT(X, R)                                                              \
    (R) == 32 ? _mm_shuffle_epi32 ((X), _MM_SHUFFLE (2, 3, 0, 1))              \
  :             XOR (_mm_srli_epi64 ((X), (R)),                                \
                     _mm_slli_epi64 ((X), 64 - (R)))

#endif /* HAVE_SSSE3 */

#endif /* HAVE_AVX2 */


#ifdef MANUAL_SCHEDULING /* This is slower in practice */

#define HALF_ROUND(                                                            \
x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, R1, R2)  \
    do                                                                         \
    {                                                                          \
        x0 = ADD (x0, x4);                                                     \
        x1 = ADD (x1, x5);                                                     \
        x2 = ADD (x2, x6);                                                     \
        x3 = ADD (x3, x7);                                                     \
        x12 = XOR (x12, x0);                                                   \
        x13 = XOR (x13, x1);                                                   \
        x14 = XOR (x14, x2);                                                   \
        x15 = XOR (x15, x3);                                                   \
        x12 = ROT (x12, R1);                                                   \
        x13 = ROT (x13, R1);                                                   \
        x14 = ROT (x14, R1);                                                   \
        x15 = ROT (x15, R1);                                                   \
        x8 = ADD (x8, x12);                                                    \
        x9 = ADD (x9, x13);                                                    \
        x10 = ADD (x10, x14);                                                  \
        x11 = ADD (x11, x15);                                                  \
        x4 = XOR (x4, x8);                                                     \
        x5 = XOR (x5, x9);                                                     \
        x6 = XOR (x6, x10);                                                    \
        x7 = XOR (x7, x11);                                                    \
        x4 = ROT (x4, R2);                                                     \
        x5 = ROT (x5, R2);                                                     \
        x6 = ROT (x6, R2);                                                     \
        x7 = ROT (x7, R2);                                                     \
    } while (0)

#define ROUND(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15)                \
    do                                                                                             \
    {                                                                                              \
        HALF_ROUND (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, 32, 24); \
        HALF_ROUND (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, 16, 63); \
    } while (0)

#define DOUBLE_ROUND(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) \
    do                                                                                     \
    {                                                                                      \
        /* Column round */                                                                 \
        ROUND (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15);      \
        /* Diagonal round */                                                               \
        ROUND (x0, x1, x2, x3, x5, x6, x7, x4, x10, x11, x8, x9, x15, x12, x13, x14);      \
    } while (0)

#else /* !MANUAL_SCHEDULING */

#define QUARTER_ROUND(A, B, C, D)                                              \
    do                                                                         \
    {                                                                          \
        A = ADD (A, B);                                                        \
        D = XOR (D, A);                                                        \
        D = ROT (D, 32);                                                       \
        C = ADD (C, D);                                                        \
        B = XOR (B, C);                                                        \
        B = ROT (B, 24);                                                       \
        A = ADD (A, B);                                                        \
        D = XOR (D, A);                                                        \
        D = ROT (D, 16);                                                       \
        C = ADD (C, D);                                                        \
        B = XOR (B, C);                                                        \
        B = ROT (B, 63);                                                       \
    } while (0)

#define DOUBLE_ROUND(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) \
    do                                                                                     \
    {                                                                                      \
        /* Column round */                                                                 \
        QUARTER_ROUND (x0, x4, x8, x12);                                                   \
        QUARTER_ROUND (x1, x5, x9, x13);                                                   \
        QUARTER_ROUND (x2, x6, x10, x14);                                                  \
        QUARTER_ROUND (x3, x7, x11, x15);                                                  \
        /* Diagonal round */                                                               \
        QUARTER_ROUND (x0, x5, x10, x15);                                                  \
        QUARTER_ROUND (x1, x6, x11, x12);                                                  \
        QUARTER_ROUND (x2, x7, x8, x13);                                                   \
        QUARTER_ROUND (x3, x4, x9, x14);                                                   \
    } while (0)

#endif /* MANUAL_SCHEDULING */

#define BLABLA_CORE(z0, z1, z2, z3, z4, z5, z6, z7,                                              \
                    z8, z9,z10,z11,z12,z13,z14,z15,                                              \
                    x0, x1, x2, x3, x4, x5, x6, x7,                                              \
                    x8, x9,x10,x11,x12,x13,x14,x15)                                              \
    do                                                                                           \
    {                                                                                            \
        int i;                                                                                   \
        z0 = x0, z1 = x1, z2 = x2, z3 = x3, z4 = x4, z5 = x5, z6 = x6,                           \
        z7 = x7, z8 = x8, z9 = x9, z10 = x10, z11 = x11, z12 = x12, z13 = x13,                   \
        z14 = x14, z15 = x15;                                                                    \
        for (i = 0; i < nROUNDS; ++i)                                                            \
            DOUBLE_ROUND (z0, z1, z2, z3, z4, z5, z6, z7, z8, z9, z10, z11, z12, z13, z14, z15); \
                                                                                                 \
        z0 = ADD (x0, z0);                                                                       \
        z1 = ADD (x1, z1);                                                                       \
        z2 = ADD (x2, z2);                                                                       \
        z3 = ADD (x3, z3);                                                                       \
        z4 = ADD (x4, z4);                                                                       \
        z5 = ADD (x5, z5);                                                                       \
        z6 = ADD (x6, z6);                                                                       \
        z7 = ADD (x7, z7);                                                                       \
        z8 = ADD (x8, z8);                                                                       \
        z9 = ADD (x9, z9);                                                                       \
        z10 = ADD (x10, z10);                                                                    \
        z11 = ADD (x11, z11);                                                                    \
        z12 = ADD (x12, z12);                                                                    \
        z13 = ADD (x13, z13);                                                                    \
        z14 = ADD (x14, z14);                                                                    \
        z15 = ADD (x15, z15);                                                                    \
    } while (0)


#ifdef HAVE_AVX2

#define TRANSPOSE(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) \
    do                                                                                  \
    {                                                                                   \
        MM_TYPE t0, t1, t2, t3, t4, t5, t6, t7;                                         \
        MM_TYPE t8, t9, t10, t11, t12, t13, t14, t15;                                   \
                                                                                        \
        t0 = _mm256_unpacklo_epi64 (x0, x1);                                            \
        t1 = _mm256_unpackhi_epi64 (x0, x1);                                            \
        t2 = _mm256_unpacklo_epi64 (x2, x3);                                            \
        t3 = _mm256_unpackhi_epi64 (x2, x3);                                            \
        t4 = _mm256_unpacklo_epi64 (x4, x5);                                            \
        t5 = _mm256_unpackhi_epi64 (x4, x5);                                            \
        t6 = _mm256_unpacklo_epi64 (x6, x7);                                            \
        t7 = _mm256_unpackhi_epi64 (x6, x7);                                            \
        t8 = _mm256_unpacklo_epi64 (x8, x9);                                            \
        t9 = _mm256_unpackhi_epi64 (x8, x9);                                            \
        t10 = _mm256_unpacklo_epi64 (x10, x11);                                         \
        t11 = _mm256_unpackhi_epi64 (x10, x11);                                         \
        t12 = _mm256_unpacklo_epi64 (x12, x13);                                         \
        t13 = _mm256_unpackhi_epi64 (x12, x13);                                         \
        t14 = _mm256_unpacklo_epi64 (x14, x15);                                         \
        t15 = _mm256_unpackhi_epi64 (x14, x15);                                         \
                                                                                        \
        x0 = _mm256_permute2x128_si256 (t0, t2, 0x20);                                  \
        x8 = _mm256_permute2x128_si256 (t0, t2, 0x31);                                  \
        x4 = _mm256_permute2x128_si256 (t1, t3, 0x20);                                  \
        x12 = _mm256_permute2x128_si256 (t1, t3, 0x31);                                 \
        x1 = _mm256_permute2x128_si256 (t4, t6, 0x20);                                  \
        x9 = _mm256_permute2x128_si256 (t4, t6, 0x31);                                  \
        x5 = _mm256_permute2x128_si256 (t5, t7, 0x20);                                  \
        x13 = _mm256_permute2x128_si256 (t5, t7, 0x31);                                 \
        x2 = _mm256_permute2x128_si256 (t8, t10, 0x20);                                 \
        x10 = _mm256_permute2x128_si256 (t8, t10, 0x31);                                \
        x6 = _mm256_permute2x128_si256 (t9, t11, 0x20);                                 \
        x14 = _mm256_permute2x128_si256 (t9, t11, 0x31);                                \
        x3 = _mm256_permute2x128_si256 (t12, t14, 0x20);                                \
        x11 = _mm256_permute2x128_si256 (t12, t14, 0x31);                               \
        x7 = _mm256_permute2x128_si256 (t13, t15, 0x20);                                \
        x15 = _mm256_permute2x128_si256 (t13, t15, 0x31);                               \
    } while (0)

#else /* !HAVE_AVX2 */

#define TRANSPOSE(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) \
    do                                                                                  \
    {                                                                                   \
        MM_TYPE t0, t1, t2, t3, t4, t5, t6, t7;                                         \
        MM_TYPE t8, t9, t10, t11, t12, t13, t14, t15;                                   \
                                                                                        \
        t0 = _mm_unpacklo_epi64 (x0, x1);                                               \
        t1 = _mm_unpackhi_epi64 (x0, x1);                                               \
        t2 = _mm_unpacklo_epi64 (x2, x3);                                               \
        t3 = _mm_unpackhi_epi64 (x2, x3);                                               \
        t4 = _mm_unpacklo_epi64 (x4, x5);                                               \
        t5 = _mm_unpackhi_epi64 (x4, x5);                                               \
        t6 = _mm_unpacklo_epi64 (x6, x7);                                               \
        t7 = _mm_unpackhi_epi64 (x6, x7);                                               \
        t8 = _mm_unpacklo_epi64 (x8, x9);                                               \
        t9 = _mm_unpackhi_epi64 (x8, x9);                                               \
        t10 = _mm_unpacklo_epi64 (x10, x11);                                            \
        t11 = _mm_unpackhi_epi64 (x10, x11);                                            \
        t12 = _mm_unpacklo_epi64 (x12, x13);                                            \
        t13 = _mm_unpackhi_epi64 (x12, x13);                                            \
        t14 = _mm_unpacklo_epi64 (x14, x15);                                            \
        t15 = _mm_unpackhi_epi64 (x14, x15);                                            \
                                                                                        \
        x0 = t0;                                                                        \
        x1 = t2;                                                                        \
        x2 = t4;                                                                        \
        x3 = t6;                                                                        \
        x4 = t8;                                                                        \
        x5 = t10;                                                                       \
        x6 = t12;                                                                       \
        x7 = t14;                                                                       \
        x8 = t1;                                                                        \
        x9 = t3;                                                                        \
        x10 = t5;                                                                       \
        x11 = t7;                                                                       \
        x12 = t9;                                                                       \
        x13 = t11;                                                                      \
        x14 = t13;                                                                      \
        x15 = t15;                                                                      \
    } while (0)

#endif /* HAVE_AVX2 */

#define BLABLA_STORE(dst, z, i)                                                \
    STOREU (dst + i * 8 * BLOCKS_PER_CORE, z ## i)

#define BLABLA_OUT(dst)                                                                   \
    do                                                                                    \
    {                                                                                     \
        TRANSPOSE (z0, z1, z2, z3, z4, z5, z6, z7, z8, z9, z10, z11, z12, z13, z14, z15); \
        BLABLA_STORE (dst, z, 0);                                                         \
        BLABLA_STORE (dst, z, 1);                                                         \
        BLABLA_STORE (dst, z, 2);                                                         \
        BLABLA_STORE (dst, z, 3);                                                         \
        BLABLA_STORE (dst, z, 4);                                                         \
        BLABLA_STORE (dst, z, 5);                                                         \
        BLABLA_STORE (dst, z, 6);                                                         \
        BLABLA_STORE (dst, z, 7);                                                         \
        BLABLA_STORE (dst, z, 8);                                                         \
        BLABLA_STORE (dst, z, 9);                                                         \
        BLABLA_STORE (dst, z, 10);                                                        \
        BLABLA_STORE (dst, z, 11);                                                        \
        BLABLA_STORE (dst, z, 12);                                                        \
        BLABLA_STORE (dst, z, 13);                                                        \
        BLABLA_STORE (dst, z, 14);                                                        \
        BLABLA_STORE (dst, z, 15);                                                        \
    } while (0)

#define BLABLA_XOR_STORE(src, dst, z, i)                                       \
    STOREU (dst + i * 8 * BLOCKS_PER_CORE,                                     \
            XOR (z ## i, LOADU (src + i * 8 * BLOCKS_PER_CORE)))

#define BLABLA_XOR_OUT(src, dst)                                                          \
    do                                                                                    \
    {                                                                                     \
        TRANSPOSE (z0, z1, z2, z3, z4, z5, z6, z7, z8, z9, z10, z11, z12, z13, z14, z15); \
        BLABLA_XOR_STORE (src, dst, z, 0);                                                \
        BLABLA_XOR_STORE (src, dst, z, 1);                                                \
        BLABLA_XOR_STORE (src, dst, z, 2);                                                \
        BLABLA_XOR_STORE (src, dst, z, 3);                                                \
        BLABLA_XOR_STORE (src, dst, z, 4);                                                \
        BLABLA_XOR_STORE (src, dst, z, 5);                                                \
        BLABLA_XOR_STORE (src, dst, z, 6);                                                \
        BLABLA_XOR_STORE (src, dst, z, 7);                                                \
        BLABLA_XOR_STORE (src, dst, z, 8);                                                \
        BLABLA_XOR_STORE (src, dst, z, 9);                                                \
        BLABLA_XOR_STORE (src, dst, z, 10);                                               \
        BLABLA_XOR_STORE (src, dst, z, 11);                                               \
        BLABLA_XOR_STORE (src, dst, z, 12);                                               \
        BLABLA_XOR_STORE (src, dst, z, 13);                                               \
        BLABLA_XOR_STORE (src, dst, z, 14);                                               \
        BLABLA_XOR_STORE (src, dst, z, 15);                                               \
    } while (0)


#define BLABLA_INIT(x0, x1, x2, x3, x4, x5, x6, x7,                            \
                    x8, x9,x10,x11,x12,x13,x14,x15,                            \
                    constants, key, counter)                                   \
    do                                                                         \
    {                                                                          \
        x0 = SET1_EPI64x (constants[0]);                                       \
        x1 = SET1_EPI64x (constants[1]);                                       \
        x2 = SET1_EPI64x (constants[2]);                                       \
        x3 = SET1_EPI64x (constants[3]);                                       \
                                                                               \
        x4 = SET1_EPI64x (key[0]);                                             \
        x5 = SET1_EPI64x (key[1]);                                             \
        x6 = SET1_EPI64x (key[2]);                                             \
        x7 = SET1_EPI64x (key[3]);                                             \
        x8 = SET1_EPI64x (constants[4]);                                       \
        x9 = SET1_EPI64x (constants[5]);                                       \
        x10 = SET1_EPI64x (constants[6]);                                      \
        x11 = SET1_EPI64x (constants[7]);                                      \
                                                                               \
        x12 = SET1_EPI64x (counter[0]);                                        \
        x13 = SET1_EPI64x (counter[1]);                                        \
        x14 = SET1_EPI64x (counter[2]);                                        \
        x15 = SET1_EPI64x (counter[3]);                                        \
    } while (0)


void blabla_ctxt_keystream (blabla_ctxt *ctxt, uint8_t *out, uint64_t len)
{
    MM_TYPE x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    MM_TYPE z0, z1, z2, z3, z4, z5, z6, z7, z8, z9, z10, z11, z12, z13, z14, z15;

    uint64_t *key = ctxt->key;
    uint64_t *counter = ctxt->counter;

    BLABLA_INIT (x0, x1, x2, x3, x4, x5, x6, x7,
                 x8, x9,x10,x11,x12,x13,x14,x15,
                 constants, key, counter);

    /* Increment counter */
    x13 = ADD (x13, INIT_COUNTER);

    while (len >= BLOCKS_PER_CORE * BLOCK_LEN)
    {
        BLABLA_CORE (z0, z1, z2, z3, z4, z5, z6, z7,
                     z8, z9,z10,z11,z12,z13,z14,z15,
                     x0, x1, x2, x3, x4, x5, x6, x7,
                     x8, x9,x10,x11,x12,x13,x14,x15);
        BLABLA_OUT (out);

        /* Increment counter */
        x13 = ADD (x13, SET1_EPI64x (BLOCKS_PER_CORE));

        out += BLOCKS_PER_CORE * BLOCK_LEN;
        len -= BLOCKS_PER_CORE * BLOCK_LEN;
    }

    if (len > 0) /* Should fallback to latency-oriented implementation here */
    {
        uint8_t block[BLOCKS_PER_CORE * BLOCK_LEN];
        BLABLA_CORE (z0, z1, z2, z3, z4, z5, z6, z7,
                     z8, z9,z10,z11,z12,z13,z14,z15,
                     x0, x1, x2, x3, x4, x5, x6, x7,
                     x8, x9,x10,x11,x12,x13,x14,x15);
        BLABLA_OUT (block);

        memcpy (out, block, len);
    }
}

int blabla_keystream (uint8_t *out, uint64_t outlen, const uint8_t *n, const uint8_t *k)
{
    blabla_ctxt ctxt;
    blabla_ctxt_init (&ctxt, k, n);
    blabla_ctxt_keystream (&ctxt, out, outlen);
    return 0;
}

#ifdef SUPERCOP
int crypto_stream (unsigned char *out,
                   unsigned long long outlen,
                   const unsigned char *n,
                   const unsigned char *k)
{
    return blabla_keystream (out, outlen, n, k);
}
#endif

void blabla_ctxt_xor (blabla_ctxt *ctxt, const uint8_t *in, uint8_t *out, uint64_t len)
{
    MM_TYPE x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    MM_TYPE z0, z1, z2, z3, z4, z5, z6, z7, z8, z9, z10, z11, z12, z13, z14, z15;

    uint64_t *key = ctxt->key;
    uint64_t *counter = ctxt->counter;

    BLABLA_INIT (x0, x1, x2, x3, x4, x5, x6, x7,
                 x8, x9,x10,x11,x12,x13,x14,x15,
                 constants, key, counter);

    /* Increment counter */
    x13 = ADD (x13, INIT_COUNTER);

    while (len >= BLOCKS_PER_CORE * BLOCK_LEN)
    {
        BLABLA_CORE (z0, z1, z2, z3, z4, z5, z6, z7,
                     z8, z9,z10,z11,z12,z13,z14,z15,
                     x0, x1, x2, x3, x4, x5, x6, x7,
                     x8, x9,x10,x11,x12,x13,x14,x15);
        BLABLA_XOR_OUT (in, out);

        /* Increment counter */
        x13 = ADD (x13, SET1_EPI64x (BLOCKS_PER_CORE));

        out += BLOCKS_PER_CORE * BLOCK_LEN;
        len -= BLOCKS_PER_CORE * BLOCK_LEN;
    }

    if (len > 0) /* Should fallback to latency-oriented implementation here */
    {
        uint8_t inblock[BLOCKS_PER_CORE * BLOCK_LEN];
        uint8_t outblock[BLOCKS_PER_CORE * BLOCK_LEN];

        memcpy (inblock, in, len);

        BLABLA_CORE (z0, z1, z2, z3, z4, z5, z6, z7,
                     z8, z9,z10,z11,z12,z13,z14,z15,
                     x0, x1, x2, x3, x4, x5, x6, x7,
                     x8, x9,x10,x11,x12,x13,x14,x15);
        BLABLA_XOR_OUT (inblock, outblock);

        memcpy (out, outblock, len);
    }
}

int blabla_xor (uint8_t *out, const uint8_t *in, uint64_t inlen, const uint8_t *n, const uint8_t *k)
{
    blabla_ctxt ctxt;
    blabla_ctxt_init (&ctxt, k, n);
    blabla_ctxt_xor (&ctxt, in, out, inlen);
    return 0;
}

#ifdef SUPERCOP
int crypto_stream_xor (unsigned char *out,
                       const unsigned char *in,
                       unsigned long long inlen,
                       const unsigned char *n,
                       const unsigned char *k)
{
    return blabla_xor (out, in, inlen, n, k);
}
#endif
