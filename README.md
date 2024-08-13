m6a.4xlarge
64 gb storage

Use the script in dev/git/python-1-billion-row-challenge/data to generate

The current machine does not support all of the following CPU features that are required by the
image: [CX8, CMOV, FXSR, HT, MMX, AMD_3DNOW_PREFETCH, SSE, SSE2, SSE3, SSSE3, SSE4A, SSE4_1,
SSE4_2, POPCNT, LZCNT, TSC, TSCINV_BIT, AVX, AVX2, AES, CLMUL, BMI1, BMI2, ADX, SHA, FMA,
VZEROUPPER, FLUSH, FLUSHOPT, HV, RDTSCP, RDPID, F16C].

AMD EPYC™ Genoa 9454P

// Target times against C reference
//
// cd ~/dev/git/1brc
// time bin/analyze /Users/badger/dev/git/python-1-billion-row-challenge/data/measurements_medium.txt > /dev/null
//
// TODO: WTF. They aren't getting any sys or user times :think:
// Full: 0.00s user 0.00s system 0% cpu 26.914 total
// Medium: 0.00s user 0.00s system 1% cpu 0.667 total

// I need to be just over twice as fast! That's actually not horrific.
// Although, I tried the winnign java implementation on a decent AWS
// instance and I'm about 4 times slower. So there's definitely work to be
// done.
// My times:
// Note that I'm not leverging multiple cores that well it seems
// Full:
//      One thread: 62.84s user 24.19s system 43% cpu 3:19.24 total
//                  64.36s user 31.52s system 42% cpu 3:46.73 total
//      Num Cores threads: 48.57s user 25.66s system 126% cpu 58.778 total
// Medium:
//      One thread: 4.27s user 0.35s system 99% cpu 4.651 total
//      Num Cores threads: 5.77s user 0.32s system 386% cpu 1.576 total


data/measurements_100m.txt  4.15s user 0.38s system 376% cpu 1.205 total
-t1 data/measurements_300m.txt  11.69s user 1.50s system 98% cpu 13.397 total
data/measurements_300m.txt  12.43s user 1.15s system 382% cpu 3.551 total

THIS IS MY BEST
data/measurements_full.txt  37.45s user 23.33s system 118% cpu 51.479 total
-t8 data/measurements_full.txt  39.98s user 19.43s system 207% cpu 28.595 total
