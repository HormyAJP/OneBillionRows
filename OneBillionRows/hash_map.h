//
//  hash_map.h
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#ifndef hash_map_h
#define hash_map_h

#include <sys/types.h>

#define HASH_SIZE 10000
#define MAX_STATION_NAME_LENGTH 100

typedef struct hash_data {
    char weather_station_name[MAX_STATION_NAME_LENGTH];
    int min;
    int max;
    int count;
    long long total;
} hash_data;

typedef struct hash_map {
    hash_data buckets[HASH_SIZE];
} hash_map;

size_t hash(const char *weather_station_name, size_t str_len);

hash_map* hash_create(void);
void hash_destroy(hash_map* h);

// TODO: Debug collisions and collision depths
hash_data* hash_get_bucket(hash_map* h, const char* weather_station_name, size_t str_len);

void hash_dump(hash_map* h);

#endif /* hash_map_h */
