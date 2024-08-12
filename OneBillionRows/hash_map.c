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
size_t hash(const char *weather_station_name, size_t str_len)
{
    unsigned long hash = 5381;
    // TODO: Expriment to see if we can just hash off the first 3 letters
    // Might need to refine this via debugging the hash map
    const char* pend = weather_station_name + MYMIN(str_len, 3);
    while (weather_station_name < pend) {
        hash = ((hash << 5) + hash) + *weather_station_name++; /* hash * 33 + c */
    }

    return hash % HASH_SIZE;
}

hash_map* hash_create(void) {
    hash_map* h = (hash_map*)malloc(sizeof(hash_map));
    memset(h, 0, sizeof(hash_map)); // This is necessary to ensure the string ptr is zero and count, total are zero.
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        (h->buckets + i)->min = 999;
        (h->buckets + i)->max = -999;
    }
    return h;
}

void hash_destroy(hash_map* h) {
    free(h);
}

// TODO: Debug collisions and collision depths
hash_data* hash_get_bucket(hash_map* h, const char* weather_station_name, size_t str_len) {
    size_t bucket = hash(weather_station_name, str_len);
    hash_data* data = &(h->buckets[bucket]);
    while (1) {
        if (likely(!strncmp(data->weather_station_name, weather_station_name, str_len))) {
            return data;
        }

        if (data->weather_station_name[0] == 0) {
            strncpy(data->weather_station_name, weather_station_name, str_len);
            return data;
        }

        if (unlikely(data >= h->buckets + HASH_SIZE - 1)) {
            data = h->buckets;
        } else {
            data += 1;
        }
    }
}

void hash_merge(hash_map* into, hash_map* from) {
    for (int bucket = 0; bucket < HASH_SIZE; ++bucket) {
        hash_data* from_bucket = &from->buckets[bucket];
        hash_data* to_bucket = hash_get_bucket(into, from_bucket->weather_station_name, strlen(from_bucket->weather_station_name));
        to_bucket->count += from_bucket->count;
        to_bucket->total += from_bucket->total;
        to_bucket->min = MYMIN(to_bucket->min, from_bucket->min);
        to_bucket->max = MYMAX(to_bucket->max, from_bucket->max);
    }
}

unsigned int hash_dump(hash_map* h) {
    unsigned int count = 0;
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        if (h->buckets[i].weather_station_name[0] == 0) {
            continue;
        }
        count += h->buckets[i].count;
        printf("%s, min: %f, max: %f, average: %f\n",
               h->buckets[i].weather_station_name,
               ((float)h->buckets[i].min/10),
               ((float)h->buckets[i].max/10),
               ((float)h->buckets[i].total/10/((float)h->buckets[i].count))
               );
    }
    return count;
}
