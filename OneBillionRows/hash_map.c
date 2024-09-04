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

// Anecdotally djb2 hash seems faster than the crap I randomly came up with (no surprise)
// Need to debug the hashmap and see how many collisions we're getting.
size_t hash2(const char *weather_station_name, size_t str_len)
{
    unsigned long hash = 5381;
    const char* pend = weather_station_name + str_len;
    while (weather_station_name < pend) {
        hash = ((hash << 5) + hash) + *weather_station_name++; /* hash * 33 + c */
    }

    return hash & HASH_MOD;
}

// Anecdotally faster than djb2
// TODO: Try Murmur?
size_t hash(const char *weather_station_name, size_t str_len) {
    unsigned long hash = 0;
//    const char* pend = weather_station_name + str_len;
    // TODO: Did some adhoc testing. Seems like you only need the first 6 chars in general to avoid collisions
    // given our data set. Be cautious of this though and debug collisions properly once you get to
    // your final code.
    const char* pend = weather_station_name + MYMIN(str_len, 6);
    
    while (weather_station_name < pend) {
        hash = *weather_station_name++ + (hash << 6) + (hash << 16) - hash;
    }
    
    return hash & HASH_MOD;
}


hash_map* hash_create(void) {
    hash_map* h = (hash_map*)malloc(sizeof(hash_map));
    memset(h, 0, sizeof(hash_map)); // This is necessary to ensure the string ptr is zero and count, total are zero.
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        (h->buckets + i)->min = 999;
        (h->buckets + i)->max = -999;
    }
    pthread_mutex_init(&h->mutex, NULL);
    return h;
}

void hash_destroy(hash_map* h) {
    pthread_mutex_destroy(&h->mutex);
    free(h);
}

hash_data* hash_get_bucket_thread_safe(hash_map* h, const char* weather_station_name, size_t str_len) {
    pthread_mutex_lock(&h->mutex);
    
    // In the locked version, we have to start the search again because another thread might have created the
    // bucket whilst we were busy.
    // There should otherwise be no race condition. The danger area is when performing memcmp in the unsafe
    // hash_get_bucket function. However, there's no way for the memcmp to return 0 untill every character of
    // the bucket name has been copied using strncpy below. And once that happens, the bucket is good to go.
    size_t bucket = hash(weather_station_name, str_len);
    hash_data* data = &(h->buckets[bucket]);
    while (1) {
        if (likely(!memcmp(data->weather_station_name, weather_station_name, str_len))) {
            break;
        }

        if (data->weather_station_name[0] == 0) {
            strncpy(data->weather_station_name, weather_station_name, str_len);
            break;
        }

        if (unlikely(data >= h->buckets + HASH_SIZE - 1)) {
            data = h->buckets;
        } else {
            data += 1;
        }
    }
    
    pthread_mutex_unlock(&h->mutex);
    return data;
}

hash_data* hash_get_bucket(hash_map* h, const char* weather_station_name, size_t str_len, int* collision_depth) {
    size_t bucket = hash(weather_station_name, str_len);
    hash_data* data = &(h->buckets[bucket]);
    while (1) {
        if (likely(!memcmp(data->weather_station_name, weather_station_name, str_len))) {
            break;
        }

        if (data->weather_station_name[0] == 0) {
//            return hash_get_bucket_thread_safe(h, weather_station_name, str_len);
            strncpy(data->weather_station_name, weather_station_name, str_len);
            break;
        }

        if (unlikely(data >= h->buckets + HASH_SIZE - 1)) {
            data = h->buckets;
        } else {
            data += 1;
        }
#if HASH_MAP_DEBUGGING
        if (collision_depth) {
            *collision_depth += 1;
        }
#endif
    }
    return data;
    
}

void hash_merge(hash_map* into, hash_map* from) {
    for (int bucket = 0; bucket < HASH_SIZE; ++bucket) {
        hash_data* from_bucket = &from->buckets[bucket];
        hash_data* to_bucket = hash_get_bucket(into, from_bucket->weather_station_name, strlen(from_bucket->weather_station_name), NULL);
        to_bucket->count += from_bucket->count;
        to_bucket->total += from_bucket->total;
        to_bucket->min = MYMIN(to_bucket->min, from_bucket->min);
        to_bucket->max = MYMAX(to_bucket->max, from_bucket->max);
    }
}

unsigned int hash_dump(hash_map* h) {
    unsigned int count = 0;
#if HASH_MAP_DEBUGGING
    int depth;
    int num_collisions = 0;
    int max_collision_size = 0;
    float average_collision_size = 0;
#endif
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        if (h->buckets[i].weather_station_name[0] == 0) {
            continue;
        }
#if HASH_MAP_DEBUGGING
        depth = 0;
        hash_get_bucket(h, h->buckets[i].weather_station_name, strlen(h->buckets[i].weather_station_name), &depth);
        if (depth) {
            num_collisions += 1;
            average_collision_size += depth;
            max_collision_size = MYMAX(depth, max_collision_size);
        }
#endif
        count += h->buckets[i].count;
        printf("%s, min: %f, max: %f, average: %f\n",
               h->buckets[i].weather_station_name,
               ((float)h->buckets[i].min/10),
               ((float)h->buckets[i].max/10),
               ((float)h->buckets[i].total/10/((float)h->buckets[i].count))
               );
    }
    
#if HASH_MAP_DEBUGGING
    average_collision_size /= num_collisions;
    printf("Num collisions: %d, Max collision depth: %d, Ave Collision depth: %f\n", num_collisions, max_collision_size, average_collision_size);
#endif
    return count;
}
