//
//  logic.h
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#ifndef logic_h
#define logic_h

#include "hash_map.h"
#include <pthread.h>
#include <sys/types.h>

//#if DEBUG
//#define CORES 1
//#else
#define CORES physical_cores()
//#endif

extern const char* FILE_NAME;

int physical_cores(void);
void spin_up_threads(int num_threads, char** splitpoints);
void parse_mapped_file_to_hash_map(const void* mapped_file, size_t mapped_size, hash_map* h);
void spin_up_threads(int num_threads, char** splitpoints);
char** split_input(char* start, size_t size, int num_cores);

#endif /* logic_h */
