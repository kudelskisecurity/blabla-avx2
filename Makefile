CC=gcc
BENCH=bench.c
TEST=test.c

FLAGS=-Ofast -funroll-loops -Wall --std=c99 -Wpedantic 
FLAGSREF  =$(FLAGS)
FLAGSSSE2 =$(FLAGS) -msse2
FLAGSSSSE3=$(FLAGS) -mssse3
FLAGSAVX2 =$(FLAGS) -mavx2

all: test bench
.PHONY: all asm format clean

# merge bench and test?

bench:
	$(CC) $(FLAGSREF)   $(BENCH) blabla-ref.c -o bench-ref
	$(CC) $(FLAGSSSE2)  $(BENCH) blabla-opt.c -o bench-opt-sse2
	$(CC) $(FLAGSSSSE3) $(BENCH) blabla-opt.c -o bench-opt-ssse3
	$(CC) $(FLAGSAVX2)  $(BENCH) blabla-opt.c -o bench-opt-avx2

test:   # sanitizers not for bench as they slow down the code
	$(CC) $(FLAGSREF)   -fsanitize=address,undefined $(TEST) blabla-ref.c -o test-ref
	$(CC) $(FLAGSSSE2)  -fsanitize=address,undefined $(TEST) blabla-opt.c -o test-opt-sse2
	$(CC) $(FLAGSSSSE3) -fsanitize=address,undefined $(TEST) blabla-opt.c -o test-opt-ssse3
	$(CC) $(FLAGSAVX2)  -fsanitize=address,undefined $(TEST) blabla-opt.c -o test-opt-avx2
	./test-ref
	./test-opt-sse2
	./test-opt-ssse3
	./test-opt-avx2

asm:
	mkdir -p asm
	$(CC) $(FLAGSREF)   -o asm/blabla-ref.s             -S blabla-ref.c
	$(CC) $(FLAGSSSE2)  -o asm/blabla-opt-sse2.s        -S blabla-opt.c
	$(CC) $(FLAGSSSSE3) -o asm/blabla-opt-ssse3.s       -S blabla-opt.c
	$(CC) $(FLAGSAVX2)  -o asm/blabla-opt-avx2.s        -S blabla-opt.c

format: # used config from ./.clang-format
	clang-format -i *.c *.h

clean:
	rm -f bench-* test-* 
	rm -f *.s

