//
//  logic.c
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#include "logic.h"
#include "shared_definitions.h"

#include <emmintrin.h>

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

static const size_t MAX_CORES = 32;

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

static inline int fast_atoi( const char * str )
{
    int isminus = 0;
    if (unlikely(*str == '-')) {
        isminus=1;
        str++;
    }
    int val = 0;
    while( *str ) {
        val = val*10 + (*str++ - '0');
    }
    return isminus ? -val : val;
}

int parse_number(const char* start) {
    static char NUM_BUFFER[5];
    int i = 0;
    while (*start != '.') {
        NUM_BUFFER[i++] = *start;
        ++start;
    }
    NUM_BUFFER[i++] = *(start+1);
    NUM_BUFFER[i] = 0;
    return fast_atoi(NUM_BUFFER);
}

int index_of_newline(const char* p) {
    // TODO: Make const??
    __m128i new_lines = _mm_set_epi64x(0x0A0A0A0A0A0A0A0A, 0x0A0A0A0A0A0A0A0A);
    __m128i bytes_to_check = _mm_set_epi64x(*((long long*)p + 1), *(long long*)p);
    __m128i comparison_result = _mm_cmpeq_epi8(bytes_to_check, new_lines);
    int mask = _mm_movemask_epi8(comparison_result);
    if (unlikely(!mask)) {
        return -1;
    }
    return __builtin_ctz(mask);
}

//inline int compute_average(hash_data* data, int temp) {
//
//}

void enter_data_in_hash_map(hash_map* h, const char* weather_station_name, int temp) {
    hash_data* data = hash_get_bucket(h, weather_station_name);
    data->max = MAX(temp, data->max);
    data->min = MIN(temp, data->min);
    data->total += temp;
    data->count++;
}

void split_rest_slow(hash_map* h, const char* start, const char* end) {
    // 101 because 100 is the max weather station name.
    char BUFFER[101];
    const char* p = start;
    while (p < end) {
        while (*p != ';') {
            ++p;
        }
        
        size_t size = p - start;
        strncpy(BUFFER, start, size);
        BUFFER[size] = 0;
        ++p;
        int num = parse_number(p);
        enter_data_in_hash_map(h, BUFFER, num);
        while (*p != '\n' && p < end) {
            ++p;
        }
        ++p;
        start = p;
    }
}

char* split_next(hash_map* h, const char* start) {
    const char* p = start;
    int index;
    while (1) {
        index = index_of_newline(p);
        if (likely(index != -1))
            break;
        // It should be impossible to overflow here. We ensure that the buffer is always a multiple of 16 bytes.
        p += 8;
    }
    const char* pend = start + index + (p-start) + 1;
    const char* index_of_semicolon = pend - 5; // Guaranteed always 4 chars in the numnber ("x.x\n")
    while (*(index_of_semicolon) != ';') {
        --index_of_semicolon;
    }
    int temp = parse_number(index_of_semicolon+1);
    char BUFFER[101];
    size_t size = index_of_semicolon-start;
    strncpy(BUFFER, start, size);
    BUFFER[size] = 0;
#ifdef DEBUG
//    printf("%s : %d\n", BUFFER, temp);
#endif
    enter_data_in_hash_map(h, BUFFER, temp);
    return pend;
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
    size_t max_size_of_entry = (mapped_size - (100 + 1 + 4 + 1));
    const char* maxp = mapped_file + max_size_of_entry;
    while (p < maxp) {
        p = split_next(h, p);
    }
    
    split_rest_slow(h, p, ((char*)mapped_file) + mapped_size);
    // Print p to stop everything getting optimized away
    printf("IGNORE THIS LINE - IT PREVENTS OPTIMIZATION %p\n", p);
}

static inline char* find_split_point(char* point) {
    while (*point != '\n') {
        ++point;
    }
    ++point;
    return point;
}

char** split_input(char* start, size_t size, int num_cores) {
    static char* splitpoints[MAX_CORES+1];
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
    pthread_exit(NULL);
}

typedef struct thread_info {
    pthread_t thread;
    thread_data data;
} thread_info;

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
    // TODO: This seems to add several seconds of overhead and I'm not even merging yet :(
    for (int i = 0; i < num_threads; ++i) {
        result = pthread_join(threads[i].thread, NULL);
        if (result != 0) {
            perror("Failed to join thread");
            exit(1);
        }
        printf("================================================================================\n");
        hash_dump(threads[i].data.h);
    }
    


    printf("All threads done\n");
    // TODO: abort to get fast cleanup?
//    abort();
    free(threads);
}

void test_index_of_newline(void) {
    char BUFFER[17];
    BUFFER[16] = 0;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < i; j++) {
            BUFFER[j] = 'X';
        }
        BUFFER[i] = '\n';
        for (int j = i+1; j < 16; j++) {
            BUFFER[j] = 'X';
        }
        
        int index = index_of_newline(BUFFER);
        printf("Got %d when testing with newline at %d: '%s'\n", index, i, BUFFER);
        assert(index == i);
    }
}

void test_atoi(void) {
    int k=0;
    char BUFFER[16];
    for (int i = 0; i < 100000000; ++i) {
        sprintf(BUFFER, "%d", i);
        k += fast_atoi(BUFFER);
    }
    printf("%d\n", k);
}
