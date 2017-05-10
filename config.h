/*
 * Optimized implementation of BlaBla for SSE2/SSSE3/AVX2.
 *
 * Copyright (C) 2017 Nagravision S.A.
*/

#ifndef BLABLA_CONFIG_H
#define BLABLA_CONFIG_H

/* These don't work everywhere */
#if defined(__SSE2__) || defined(__x86_64__) || defined(__amd64__)
#pragma message "Detected SSE2."
#define HAVE_SSE2
#endif

#if defined(__SSSE3__)
#pragma message "Detected SSSE3."
#define HAVE_SSSE3
#endif

#if defined(__AVX2__)
#pragma message "Detected AVX2."
#define HAVE_AVX2
#endif


#ifdef HAVE_AVX2
#ifndef HAVE_SSSE3
#define HAVE_SSSE3
#endif
#endif

#ifdef HAVE_SSSE3
#define HAVE_SSE2
#endif

#if !defined(HAVE_SSE2)
#error "This code requires at least SSE2."
#endif

#endif
