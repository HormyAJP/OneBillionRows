//
//  hash_map.c
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#include "hash_map.h"
#include "shared_definitions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 10000
#define MAX_STATION_NAME_LENGTH 100

// Anecdotally djb2 hash seems faster than the crap I randomly came up with (no surprise)
// Need to debug the hashmap and see how many collisions we're getting.
size_t hash(const char *weather_station_name)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *weather_station_name++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % HASH_SIZE;
}

//size_t hash(const char* weather_station_name) {
//    return weather_station_name[0] * weather_station_name[1] * weather_station_name[2] % HASH_SIZE;
//}

hash_map* hash_create(void) {
    hash_map* h = (hash_map*)malloc(sizeof(hash_map));
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        (h->buckets + i)->weather_station_name[0] = 0;
        (h->buckets + i)->min = 999;
        (h->buckets + i)->max = -999;
        // Probably an irrelevant optimization. No need to initialize these though.
//        (h->buckets + i)->count = 0;
//        (h->buckets + i)->total = 0;
    }
    return h;
}

void hash_destroy(hash_map* h) {
    free(h);
}

// TODO: Debug collisions and collision depths
hash_data* hash_get_bucket(hash_map* h, const char* weather_station_name) {
    size_t bucket = hash(weather_station_name);
    hash_data* data = &(h->buckets[bucket]);
    while (1) {
        if (likely(!strcmp(data->weather_station_name, weather_station_name))) {
            return data;
        }

        if (data->weather_station_name[0] == 0) {
            strncpy(data->weather_station_name, weather_station_name, MAX_STATION_NAME_LENGTH-1);
            return data;
        }

        if (unlikely(data >= h->buckets + HASH_SIZE - 1)) {
            data = h->buckets;
        } else {
            data += 1;
        }
    }
}

void hash_dump(hash_map* h) {
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        if (h->buckets[i].weather_station_name[0] == 0) {
            continue;
        }
        printf("%s, min: %f, max: %f, average: %f\n",
               h->buckets[i].weather_station_name,
               ((float)h->buckets[i].min/10),
               ((float)h->buckets[i].max/10),
               ((float)h->buckets[i].total/10/((float)h->buckets[i].count))
               );
    }
}
