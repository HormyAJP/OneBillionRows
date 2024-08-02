//
//  main.c
//  OneBillionRows
//
//  Created by Badger on 20/7/2024.
//

//The task is to write a Java program which reads the file, calculates the min, mean, and max
//temperature value per weather station, and emits the results on stdout like this (i.e. sorted
//alphabetically by station name, and the result values per station in the format
// <min>/<mean>/<max>, rounded to one fractional digit):
//
//{Abha=-23.0/18.0/59.2, Abidjan=-16.2/26.0/67.3, Abéché=-10.0/29.4/69.0, Accra=-10.1/26.4/66.4,
//Addis Ababa=-23.7/16.0/67.0, Adelaide=-27.8/17.3/58.5, ...}
//
//
// <string: station name>;<double: measurement>
//
//Input value ranges are as follows: Station name: non null UTF-8 string of min length 1 character
//and max length 100 bytes, containing neither ; nor \n characters. (i.e. this could be 100
//one-byte characters, or 50 two-byte characters, etc.) Temperature value: non null double
//between -99.9 (inclusive) and 99.9 (inclusive), always with one fractional digit
//
//
//There is a maximum of 10,000 unique station names
//
//The rounding of output values must be done using the semantics of IEEE 754
//rounding-direction "roundTowardPositive
//


// Target times against C reference
//
// cd ~/dev/git/1brc
// time bin/analyze /Users/badger/dev/git/python-1-billion-row-challenge/data/measurements_medium.txt > /dev/null
//
// Full: 27.907s
// Medium: 2.6s

// I need to be about 3 times faster just on the read and parse alone
// My tmes:
// Full: 105s

#include "logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#define SMALL "/Users/badger/dev/git/python-1-billion-row-challenge/data/measurements_small.txt"
#define MEDIUM  "/Users/badger/dev/git/python-1-billion-row-challenge/data/measurements_medium.txt"
#define FULL "/Users/badger/dev/git/python-1-billion-row-challenge/data/measurements.txt"

int main(int argc, const char * argv[]) {

//    test_atoi();
//    exit(0);
//    test_index_of_newline();
//    exit(0);
    
    const char* filename = SMALL;
    if (argv[1] == 0) {
        fprintf(stderr, "WARNING: No input file specified. Using default small file:\n%s\n\nYou might want:\n\n%s\n%s", SMALL, MEDIUM, FULL);
    } else {
        filename = argv[1];
    }
    
    const int num_cores = CORES;
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting the file size");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    size_t size = sb.st_size;//(sb.st_size / 16) * 16;
    void *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("Error mapping the file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    spin_up_threads(num_cores, split_input(map, size, num_cores));

    if (munmap(map, size) == -1) {
        perror("Error unmapping the file");
    }
    return 0;
}
