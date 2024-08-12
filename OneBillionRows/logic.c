//
//  logic.c
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#include "logic.h"
#include "shared_definitions.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>

typedef struct thread_data {
    char* start;
    size_t length;
    hash_map* h;
} thread_data;

#define MAX_THREADS 32

#if defined(__linux__)
int physical_cores(void) {
    FILE *fp;
    char line[256];
    int physical_cores = 0;
    int last_physical_id = -1;
    int last_core_id = -1;

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        perror("Error opening /proc/cpuinfo");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp)) {
        int physical_id = -1;
        int core_id = -1;

        if (sscanf(line, "physical id\t: %d", &physical_id) == 1) {
            // Do nothing, just get the physical_id
        } else if (sscanf(line, "core id\t\t: %d", &core_id) == 1) {
            // If we have a new physical_id or core_id, increase the count
            if (physical_id != last_physical_id || core_id != last_core_id) {
                physical_cores++;
                last_physical_id = physical_id;
                last_core_id = core_id;
            }
        }
    }

    fclose(fp);
    return physical_cores;
}
#elif defined(__APPLE__) && defined(__MACH__)
int physical_cores(void) {
    int physical_cores;
    size_t len = sizeof(physical_cores);

    if (sysctlbyname("hw.physicalcpu", &physical_cores, &len, NULL, 0) == -1) {
        perror("sysctlbyname");
        exit(EXIT_FAILURE);
    }

    return physical_cores;
}
#endif

// Crazy idea: Is it faster to do a lookup somehow here? Probably not
static inline int parse_number_and_move_pointer(const char** pstart) {
    int val = 0;
    if (**pstart == '-') {
        (*pstart)++;
        while ( **pstart != '.') {
            val = val * 10 - (**pstart - '0');
            (*pstart)++;
        }
        
        (*pstart)++;
        val = val * 10 - (**pstart - '0');
        
    } else {
        int val = 0;
        while ( **pstart != '.') {
            val = val * 10 + (**pstart - '0');
            (*pstart)++;
        }
        
        (*pstart)++;
        val = val * 10 + (**pstart - '0');
    }
    
    // Move past the last digit and the newline
    *pstart += 2;
    return val;
}

static inline int index_of_semicolon(const char* p) {
    __m128i new_lines = _mm_set_epi64x(0x3B3B3B3B3B3B3B3B, 0x3B3B3B3B3B3B3B3B);
    __m128i bytes_to_check = _mm_set_epi64x(*((long long*)p + 1), *(long long*)p);
    __m128i comparison_result = _mm_cmpeq_epi8(bytes_to_check, new_lines);
    int mask = _mm_movemask_epi8(comparison_result);
    if (unlikely(!mask)) {
        return -1;
    }
    return __builtin_ctz(mask);
}

static inline int atomic_max(atomic_int *ptr, int newval) {
    int expected, desired;

    do {
        expected = atomic_load(ptr);
        desired = (expected > newval) ? expected : newval;
    } while (!atomic_compare_exchange_weak(ptr, &expected, desired));

    return desired;
}

static inline int atomic_min(atomic_int *ptr, int newval) {
    int expected, desired;

    do {
        expected = atomic_load(ptr);
        desired = (expected < newval) ? expected : newval;
    } while (!atomic_compare_exchange_weak(ptr, &expected, desired));

    return desired;
}

static inline void enter_data_in_hash_map(hash_map* h, const char* weather_station_name, size_t str_len, int temp) {
    hash_data* data = hash_get_bucket(h, weather_station_name, str_len, NULL);
//    data->max = atomic_max(&data->max, temp);
    data->max = MYMAX(temp, data->max);
//    data->min = atomic_min(&data->min, temp);
    data->min = MYMIN(temp, data->min);
//    atomic_fetch_add(&data->total, temp);
    data->total += temp;
//    atomic_fetch_add(&data->count, 1);
    data->count++;
}

void split_rest_slow(hash_map* h, const char* start, const char* end) {
    const char* p = start;
    while (p < end) {
        while (*p != ';') {
            ++p;
        }
        
        size_t size = p - start;
        ++p;
        int num = parse_number_and_move_pointer(&p);
        enter_data_in_hash_map(h, start, size, num);
        start = p;
    }
}

const char* split_next(hash_map* h, const char* start) {
    const char* p = start;
    int index;
    while (1) {
        index = index_of_semicolon(p);
        if (likely(index != -1))
            break;
        // It should be impossible to overflow here. We make sure that we can't get anywhere near the end of the buffer
        // on the fast path. See calling function.
        p += 8;
    }
    const char* psemi = p + index;
    p = psemi + 1;
    // TODO: Idea. Aggregate data in a separate thread. Push things to some sort of queue. Overhead might be too
    // big?
    enter_data_in_hash_map(h, start, psemi - start, parse_number_and_move_pointer(&p));
    return p;
}

void parse_mapped_file_to_hash_map(const void* mapped_file, size_t mapped_size, hash_map* h) {
    const char* p = mapped_file;
    // Needs to be split up into blocks of 16 because my simd check processes blocks of
    // 128 bits.
    //Input value ranges are as follows: Station name: non null UTF-8 string of min length 1 character
    //and max length 100 bytes, containing neither ; nor \n characters. (i.e. this could be 100
    //one-byte characters, or 50 two-byte characters, etc.) Temperature value: non null double
    //between -99.9 (inclusive) and 99.9 (inclusive), always with one fractional digit
    
    // We use SIMD instructions to search for the newline char. We need to be careful about buffer
    // overruns when doing this. We avoid the overrun by stopping calling split_next when we're
    // within max_size_of_entry bytes of the end of the buffer. This is because as long as we're
    // at least max_size_of_entry bytes away from the end we know we'll find a newline and not
    // overrun. We then handle the leftover buffer with a slow path.
    size_t max_size_of_entry = 100 + 1 + 4 + 1;
    size_t max_distance_fast_path = mapped_size - max_size_of_entry;
    
    // This might be faster as it might allow some unrolling (don't think it is after
    // experimenting)
    size_t min_loops = mapped_size / max_size_of_entry;
    for (size_t i = 0; i < min_loops; ++i) {
        p = split_next(h, p);
    }
    
    // Have to check the pointer from now on.
    const char* maxp = mapped_file + max_distance_fast_path;
    while (p < maxp) {
        p = split_next(h, p);
    }
    
    split_rest_slow(h, p, ((char*)mapped_file) + mapped_size);
}

static inline char* find_split_point(char* point) {
    while (*point != '\n') {
        ++point;
    }
    ++point;
    return point;
}

// WARNING: Calling this multiple times will screw your return values.
char** split_input(char* start, size_t size, int num_cores) {
    if (num_cores > MAX_THREADS) {
        fprintf(stderr, "Too many threads. Max is %d", MAX_THREADS);
        abort();
    }
    
    static char* splitpoints[MAX_THREADS+1];
    splitpoints[0] = start;
    size_t split_size = size / num_cores;
    for (int i = 1; i < num_cores; ++i) {
        splitpoints[i] = find_split_point(start + (i * split_size));
    }
    splitpoints[num_cores] = start + size;
    return splitpoints;
}

void* thread_function(void* arg) {
    thread_data* data = (thread_data*)arg;
    printf("Thread started with data: %p, %zd\n", data->start, data->length);
    
    parse_mapped_file_to_hash_map(data->start, data->length, data->h);
    printf("Thread exiting with data: %p, %zd\n", data->start, data->length);
    pthread_exit(NULL);
}

typedef struct thread_info {
    pthread_t thread;
    thread_data data;
} thread_info;

void merge_hash_tables(thread_info* info, int num_threads) {
    hash_map* haggregate = info[0].data.h;
    for (int i = 1; i < num_threads; ++i) {
        hash_merge(haggregate, info[i].data.h);
    }
}

inline void run_single_threaded(const char* start, size_t length) {
    hash_map* h = hash_create();
    parse_mapped_file_to_hash_map(start, length, h);
    unsigned int total_rows = hash_dump(h);
    printf("TOTAL ROWS: %u\n", total_rows);
}

inline void spin_up_threads(int num_threads, char** splitpoints) {
    int result;
    thread_info* threads = (thread_info*)calloc(num_threads, sizeof(thread_info));
    for (int i = 0; i < num_threads; ++i) {
        threads[i].data.length = splitpoints[i+1] - splitpoints[i];
        threads[i].data.start = splitpoints[i];
        threads[i].data.h = hash_create();
        
        result = pthread_create(&threads[i].thread, NULL, thread_function, (void*)&threads[i].data);
        if (result != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }
    
    unsigned int total_rows = 0;
    // TODO: This seems to add several seconds of overhead and I'm not even merging yet :(
    for (int i = 0; i < num_threads; ++i) {
        result = pthread_join(threads[i].thread, NULL);
        if (result != 0) {
            perror("Failed to join thread");
            exit(1);
        }
    }
    
    // TODO: One potential trick might be to use a single, multi-threaded hash. Use
    // atomics where possible, but for min/max, store those values on a per thread basis.
    // This might back final aggregation faster. Not sure how expensive this actually is in
    // the grand scheme though. Probably looking in the wrong place for optimizations
    merge_hash_tables(threads, num_threads);
    // TODO: Need to fix the dump to display data correctly, i.e. as per the challenge requiements/
    // Probably won't do that because it just doesn't matter.
    total_rows = hash_dump(threads[0].data.h);
    
    printf("TOTAL ROWS: %u\n", total_rows);

    printf("All threads done\n");
    // abort to get fast cleanup?
    abort();
    free(threads);
}

