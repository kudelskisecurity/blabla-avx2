# Optimized implementation of BlaBla for SSE2/SSSE3/AVX2

This project is an optimized implementation of [BlaBla](https://github.com/veorq/blabla) for CPUs supporting SSE2, SSSE3 or AVX2 instructions.
A reference C implementation is also provided for comparison. Another
reference C implementation was written by [Frank
Denis](https://github.com/jedisct1/blabla).

The optimization strategy is inspired by the [AVX2 ChaCha implementation](https://github.com/sneves/chacha-avx2) by Samuel Neves.


## Benchmarks

The project still lacks extensive benchmarks on multiple architectures,
but current tests suggest ~15% performance improvement over [AVX2 ChaCha
implementation](https://github.com/sneves/chacha-avx2) for the same
number of rounds.

## Testing

You can check that the code compiles and benchmark the various implementations as follows.

```
make
./bench-ref
./bench-opt-sse2
./bench-opt-ssse3
./bench-opt-avx2
```

## Authors

Guillaume Endignoux, while intern at Kudelski Security

