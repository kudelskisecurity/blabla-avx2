/*
 * Optimized implementation of BlaBla for SSE2/SSSE3/AVX2.
 *
 * Copyright (C) 2017 Nagravision S.A.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_LEN 128
#define nROUNDS 10

#ifdef USE_SHA2_CONSTANTS
/* Fractional part of sqrt(p) for the first prime numbers p (similar to SHA-2) */
static const uint64_t constants[9] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL, 0xcbbb9d5dc1059ed8ULL,
};
#else
/* Constants originally proposed by JP Aumasson */
static const uint64_t constants[9] = {
    0x6170786593810fabULL, 0x3320646ec7398aeeULL, 0x79622d3217318274ULL,
    0x6b206574babadadaULL, 0x2ae36e593e46ad5fULL, 0xb68f143029225fc9ULL,
    0x8da1e08468303aa6ULL, 0xa48a209acd50a4a7ULL, 0x7fdc12f23f90778cULL,
};
#endif

int blabla_keystream (uint8_t *out, uint64_t outlen, const uint8_t *n, const uint8_t *k);
int blabla_xor (uint8_t *out, const uint8_t *in, uint64_t inlen, const uint8_t *n, const uint8_t *k);
