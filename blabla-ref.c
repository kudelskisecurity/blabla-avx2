/*
 * Optimized implementation of BlaBla for SSE2/SSSE3/AVX2.
 *
 * Copyright (C) 2017 Nagravision S.A.
*/

#ifdef SUPERCOP
#include "crypto_stream.h"
#endif

#include "blabla.h"

typedef struct
{
    uint64_t key[4];
    uint64_t counter[4];
    uint64_t v[16];
} blabla_ctxt;


#define ROTR64(word, count) (((word) >> (count)) ^ ((word) << (64 - (count))))


void blabla_ctxt_init (blabla_ctxt *ctxt, const uint8_t *key, const uint8_t *nonce)
{
    memcpy (ctxt->key, key, 32);
    ctxt->counter[0] = 1;
    memcpy (&ctxt->counter[1], nonce, 16);
}

void blabla_ctxt_init_zero (blabla_ctxt *ctxt, const uint8_t *key)
{
    memcpy (ctxt->key, key, 32);
    ctxt->counter[0] = 1;
    memset (&ctxt->counter[1], 0, 16);
}

void G (uint64_t *v, int a, int b, int c, int d)
{
    v[a] += v[b];
    v[d] ^= v[a];
    v[d] = ROTR64 (v[d], 32);
    v[c] += v[d];
    v[b] ^= v[c];
    v[b] = ROTR64 (v[b], 24);
    v[a] += v[b];
    v[d] ^= v[a];
    v[d] = ROTR64 (v[d], 16);
    v[c] += v[d];
    v[b] ^= v[c];
    v[b] = ROTR64 (v[b], 63);
}

void blabla_permuteadd (blabla_ctxt *ctxt)
{
    int i;
    uint64_t w[16];

    memcpy (w, ctxt->v, 128);
    for (i = 0; i < nROUNDS; ++i)
    {
        G (w, 0, 4, 8, 12);
        G (w, 1, 5, 9, 13);
        G (w, 2, 6, 10, 14);
        G (w, 3, 7, 11, 15);
        G (w, 0, 5, 10, 15);
        G (w, 1, 6, 11, 12);
        G (w, 2, 7, 8, 13);
        G (w, 3, 4, 9, 14);
    }

    for (i = 0; i < 16; ++i)
    {
        ctxt->v[i] += w[i];
    }
}

void blabla_ctxt_keystream_block (blabla_ctxt *ctxt, uint8_t *out)
{
    ctxt->v[0] = constants[0];
    ctxt->v[1] = constants[1];
    ctxt->v[2] = constants[2];
    ctxt->v[3] = constants[3];
    memcpy (&ctxt->v[4], ctxt->key, 32);
    ctxt->v[8] = constants[4];
    ctxt->v[9] = constants[5];
    ctxt->v[10] = constants[6];
    ctxt->v[11] = constants[7];
    ctxt->v[12] = constants[8];
    memcpy (&ctxt->v[13], ctxt->counter, 24);

    blabla_permuteadd (ctxt);

    memcpy (out, ctxt->v, 128);
    ++ctxt->counter[0];
}

void blabla_ctxt_keystream (blabla_ctxt *ctxt, uint8_t *out, uint64_t len)
{
    while (len >= BLOCK_LEN)
    {
        blabla_ctxt_keystream_block (ctxt, out);
        out += BLOCK_LEN;
        len -= BLOCK_LEN;
    }

    if (len > 0)
    {
        uint8_t block[BLOCK_LEN];

        blabla_ctxt_keystream_block (ctxt, block);
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

void blabla_ctxt_xor_block (blabla_ctxt *ctxt, const uint8_t *in, uint8_t *out)
{
    int i;

    ctxt->v[0] = constants[0];
    ctxt->v[1] = constants[1];
    ctxt->v[2] = constants[2];
    ctxt->v[3] = constants[3];
    memcpy (&ctxt->v[4], ctxt->key, 32);
    ctxt->v[8] = constants[4];
    ctxt->v[9] = constants[5];
    ctxt->v[10] = constants[6];
    ctxt->v[11] = constants[7];
    ctxt->v[12] = constants[8];
    memcpy (&ctxt->v[13], ctxt->counter, 24);

    blabla_permuteadd (ctxt);

    const uint64_t *lin = (const uint64_t *)in;
    uint64_t *lout = (uint64_t *)out;
    for (i = 0; i < 16; ++i)
    {
        lout[i] = ctxt->v[i] ^ lin[i];
    }
    ++ctxt->counter[0];
}

void blabla_ctxt_xor (blabla_ctxt *ctxt, const uint8_t *in, uint8_t *out, uint64_t len)
{
    while (len >= BLOCK_LEN)
    {
        blabla_ctxt_xor_block (ctxt, in, out);
        in += BLOCK_LEN;
        out += BLOCK_LEN;
        len -= BLOCK_LEN;
    }

    if (len > 0)
    {
        uint8_t inblock[BLOCK_LEN];
        uint8_t outblock[BLOCK_LEN];

        memcpy (inblock, in, len);
        blabla_ctxt_xor_block (ctxt, inblock, outblock);
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
