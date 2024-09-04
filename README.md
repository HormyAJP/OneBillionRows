# General

This is my attempt at the 1 bn rows challenge: https://1brc.dev/#.

It's not optimized to the nth degree, but it's pretty good. I didn't bother converting the data
to the expected format for output. It didn't seem worth it as I wasn't actually going to be able
to submit this to the challenge. The cost of doing that is minor compared to the cost of processing
the file.

# Results

The exact numbers below are pretty meaningless. I was running on a "noisy" machine. But the variance
in runs is low enough that they are pretty representative. Results use a "hot cache" - I run the code
once first then run again. The OS likely has pages cached with the file which means the second run
is always faster. It's better to do that to get a fairer comparison.

* My code:
  * 4 threads: 0.00s user 0.00s system 0% cpu 52.990 total
  * 8 threads: 0.00s user 0.00s system 0% cpu 28.212 total
* https://github.com/dannyvankooten/1brc:
  *            0.00s user 0.00s system 0% cpu 25.530 total

# Random Junk from implementation

Error from AWS when runnning the java implementation:

The current machine does not support all of the following CPU features that are required by the
image: [CX8, CMOV, FXSR, HT, MMX, AMD_3DNOW_PREFETCH, SSE, SSE2, SSE3, SSSE3, SSE4A, SSE4_1,
SSE4_2, POPCNT, LZCNT, TSC, TSCINV_BIT, AVX, AVX2, AES, CLMUL, BMI1, BMI2, ADX, SHA, FMA,
VZEROUPPER, FLUSH, FLUSHOPT, HV, RDTSCP, RDPID, F16C].
