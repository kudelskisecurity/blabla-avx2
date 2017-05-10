/*
   BLAKE2 reference source code package - benchmark tool

   Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.

   Adapted for BlaBla by JP Aumasson and Guillaume Endignoux

   You may use this under the terms of the CC0, the OpenSSL Licence, or
   the Apache Public License 2.0, at your option.  The terms of these
   licenses can be found at:

   - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
   - OpenSSL license   : https://www.openssl.org/source/license.html
   - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0

   More information about the BLAKE2 hash function can be found at
   https://blake2.net.
*/
#include "blabla.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int bench_cmp (const void *x, const void *y)
{
    const int64_t *ix = (const int64_t *)x;
    const int64_t *iy = (const int64_t *)y;
    return *ix - *iy;
}

#if defined(__amd64__) || defined(__x86_64__)
static unsigned long long cpucycles (void)
{
    unsigned long long result;
    __asm__ __volatile__(".byte 15;.byte 49\n"
                         "shlq $32,%%rdx\n"
                         "orq %%rdx,%%rax\n"
                         : "=a"(result)::"%rdx");
    return result;
}
#elif defined(__i386__)
static unsigned long long cpucycles (void)
{
    unsigned long long result;
    __asm__ __volatile__(".byte 15;.byte 49;" : "=A"(result));
    return result;
}
#elif defined(_MSC_VER)
#include <intrin.h>
static unsigned long long cpucycles (void) { return __rdtsc (); }
#else
#error "Don't know how to count cycles on this platform!"
#endif

void bench ()
{
#define BENCH_WARMUP 1024 * 64
#define BENCH_TRIALS 32
#define BENCH_MAXLEN 4096
#define BENCH_INCREMENT 128
    static unsigned char out[BENCH_MAXLEN];
    static unsigned char key[32];
    static uint64_t nonce[2] = { 0, 0 };
    static unsigned long long median[BENCH_MAXLEN + 1];
    static unsigned char checksum = 0; // prevents compiler optimizations
    int i, j;
    printf ("#bytes  median  per byte\n");

    for (i = 0; i < 32; ++i) key[i] = i;

    for (i = 0; i <= BENCH_WARMUP; ++i)
    {
        j = BENCH_MAXLEN;
        ++nonce[0];
        blabla_keystream (out, j, (const uint8_t *)nonce, key);
        checksum ^= out[j - 1];
    }

    /* 1 ... BENCH_MAXLEN */
    for (j = 0; j <= BENCH_MAXLEN; j += BENCH_INCREMENT)
    {
        uint64_t cycles[BENCH_TRIALS + 1];

        for (i = 0; i <= BENCH_TRIALS; ++i)
        {
            cycles[i] = cpucycles ();
            ++nonce[0];
            blabla_keystream (out, j, (const uint8_t *)nonce, key);
            checksum ^= out[j - 1];
        }

        for (i = 0; i < BENCH_TRIALS; ++i)
            cycles[i] = cycles[i + 1] - cycles[i];

        qsort (cycles, BENCH_TRIALS, sizeof(uint64_t), bench_cmp);
        median[j] = cycles[BENCH_TRIALS / 2];
    }

    for (j = 0; j <= BENCH_MAXLEN; j += BENCH_INCREMENT)
        printf ("%5d, %7.2f\n", j, (double)median[j] / j);

    printf ("#2048   %6llu   %7.2f\n",
            median[BENCH_MAXLEN / 2],
            (double)median[BENCH_MAXLEN / 2] / (double)(BENCH_MAXLEN / 2));
    printf ("#4096   %6llu   %7.2f\n",
            median[BENCH_MAXLEN],
            (double)median[BENCH_MAXLEN] / (double)BENCH_MAXLEN);
    printf ("#long     long   %7.2f\n",
            (double)(median[BENCH_MAXLEN] - median[BENCH_MAXLEN / 2]) /
            (double)(BENCH_MAXLEN / 2));
    printf ("checksum: %02x\n", checksum);
}

int main ()
{
    bench ();
    return 0;
}
