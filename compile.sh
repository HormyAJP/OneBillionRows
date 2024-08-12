#!/usr/bin env sh
cd source
gcc -O3 -march=native -mtune=native -flto -Wall -Wextra -o 1bn main.c hash_map.c logic.c
