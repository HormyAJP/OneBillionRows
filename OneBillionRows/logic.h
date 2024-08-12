//
//  logic.h
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#ifndef logic_h
#define logic_h

#include "shared_definitions.h"
#include "hash_map.h"
#include <emmintrin.h>
#include <pthread.h>
#include <sys/types.h>

extern const char* FILE_NAME;

int physical_cores(void);
void run_single_threaded(const char* start, size_t length);
void spin_up_threads(int num_threads, char** splitpoints);
void parse_mapped_file_to_hash_map(const void* mapped_file, size_t mapped_size, hash_map* h);
void spin_up_threads(int num_threads, char** splitpoints);
char** split_input(char* start, size_t size, int num_cores);

static inline int index_of_newline(const char* p) {
    __m128i new_lines = _mm_set_epi64x(0x0A0A0A0A0A0A0A0A, 0x0A0A0A0A0A0A0A0A);
    __m128i bytes_to_check = _mm_set_epi64x(*((long long*)p + 1), *(long long*)p);
    __m128i comparison_result = _mm_cmpeq_epi8(bytes_to_check, new_lines);
    int mask = _mm_movemask_epi8(comparison_result);
    if (unlikely(!mask)) {
        return -1;
    }
    return __builtin_ctz(mask);
}

#endif /* logic_h */
