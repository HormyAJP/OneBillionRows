//
//  main.c
//  TestOneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#include "logic.h"
#include <assert.h>
#include <string.h>

const char* test_data1 = "Kingston;31.4\nEdinburgh;10.9";

#define TEST_CASE_START(INPUT_STRING) \
    { \
        hash_data* d; \
        hash_map* h = hash_create(); \
        parse_mapped_file_to_hash_map(INPUT_STRING, strlen(INPUT_STRING), h);

#define TEST_CASE_ASSERT(NAME, COUNT, MAX, MIN, AVERAGE) \
    d = hash_get_bucket(h, NAME, strlen(NAME)); \
    assert(d->count == COUNT); \
    assert(d->max == MAX); \
    assert(d->min == MIN); \
    assert(d->total == AVERAGE);

#define TEST_CASE_END() \
        hash_destroy(h); \
    }

void test_short_strings(void) {
    TEST_CASE_START("Kingston;31.4\nEdinburgh;10.9");
    TEST_CASE_ASSERT("Kingston", 1, 314, 314, 314);
    TEST_CASE_ASSERT("Edinburgh", 1, 109, 109, 109);
    TEST_CASE_END();
    
    TEST_CASE_START("Kingston;31.4\nKingston;30.0");
    TEST_CASE_ASSERT("Kingston", 2, 314, 300, 614);
    TEST_CASE_END();

    TEST_CASE_START("Kingston;31.4\nKingston;0.0");
    TEST_CASE_ASSERT("Kingston", 2, 314, 0, 314);
    TEST_CASE_END();
    
    TEST_CASE_START("Kingston;31.4\nKingston;32.1");
    TEST_CASE_ASSERT("Kingston", 2, 321, 314, 635);
    TEST_CASE_END();
    
    // Test that things still work if we have a trailing newline
    TEST_CASE_START("Kingston;31.4\nKingston;32.1\n");
    TEST_CASE_ASSERT("Kingston", 2, 321, 314, 635);
    TEST_CASE_END();
}

// Strings must be longer than (100 + 1 + 4 + 1) == 106 bytes to trigger SIMD path.
void test_long_strings(void) {
    TEST_CASE_START("Kingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4");
    TEST_CASE_ASSERT("Kingston", 8, 314, 314, 314*8);
    TEST_CASE_END();

    TEST_CASE_START("Ankara;15.3\nAstana;3.0\nBaguio;27.2\nColombo;14.0\nDikson;-12.4\nDushanbe;11.8\nEdinburgh;10.9\nGaborone;18.5\nHo Chi Minh City;38.0\nKansas City;6.9\nKingston;31.4\nKinshasa;29.1\nKuwait City;17.9\nLa Paz;26.4\nLibreville;33.9\nMilwaukee;17.2\nNakhon Ratchasima;28.1\nNgaoundéré;12.4\nPort-Gentil;27.0\nPorto;13.3\nPrague;16.6\nRabat;10.8\nSan Jose;13.3\nSeattle;-5.6\nSydney;16.2\nVancouver;19.6\nVientiane;8.3\nWinnipeg;-1.3\nÜrümqi;1.5");
    TEST_CASE_ASSERT("Ankara", 1, 153, 153, 153);
    TEST_CASE_ASSERT("Astana", 1, 30, 30, 30);
    TEST_CASE_ASSERT("Baguio", 1, 272, 272, 272);
    TEST_CASE_ASSERT("Colombo", 1, 140, 140, 140);
    TEST_CASE_ASSERT("Dikson", 1, -124, -124, -124);
    TEST_CASE_ASSERT("Dushanbe", 1, 118, 118, 118);
    TEST_CASE_ASSERT("Edinburgh", 1, 109, 109, 109);
    TEST_CASE_ASSERT("Gaborone", 1, 185, 185, 185);
    TEST_CASE_ASSERT("Ho Chi Minh City", 1, 380, 380, 380);
    TEST_CASE_ASSERT("Kansas City", 1, 69, 69, 69);
    TEST_CASE_ASSERT("Kingston", 1, 314, 314, 314);
    TEST_CASE_ASSERT("Kinshasa", 1, 291, 291, 291);
    TEST_CASE_ASSERT("Kuwait City", 1, 179, 179, 179);
    TEST_CASE_ASSERT("La Paz", 1, 264, 264, 264);
    TEST_CASE_ASSERT("Libreville", 1, 339, 339, 339);
    TEST_CASE_ASSERT("Milwaukee", 1, 172, 172, 172);
    TEST_CASE_ASSERT("Nakhon Ratchasima", 1, 281, 281, 281);
    TEST_CASE_ASSERT("Ngaoundéré", 1, 124, 124, 124);
    TEST_CASE_ASSERT("Port-Gentil", 1, 270, 270, 270);
    TEST_CASE_ASSERT("Porto", 1, 133, 133, 133);
    TEST_CASE_ASSERT("Prague", 1, 166, 166, 166);
    TEST_CASE_ASSERT("Rabat", 1, 108, 108, 108);
    TEST_CASE_ASSERT("San Jose", 1, 133, 133, 133);
    TEST_CASE_ASSERT("Seattle", 1, -56, -56, -56);
    TEST_CASE_ASSERT("Sydney", 1, 162, 162, 162);
    TEST_CASE_ASSERT("Vancouver", 1, 196, 196, 196);
    TEST_CASE_ASSERT("Vientiane", 1, 83, 83, 83);
    TEST_CASE_ASSERT("Winnipeg", 1, -13, -13, -13);
    TEST_CASE_ASSERT("Ürümqi", 1, 15, 15, 15);

    TEST_CASE_END();
    
    TEST_CASE_START("Kingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4");
    TEST_CASE_ASSERT("Kingston", 16, 314, 314, 314*16);
    TEST_CASE_END();
    
    // Test that things still work if we have a trailing newline
    TEST_CASE_START("Kingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\n");
    TEST_CASE_ASSERT("Kingston", 16, 314, 314, 314*16);
    TEST_CASE_END();
}

//char** split_input(char* start, size_t size, int num_cores) {
void test_split_input(void) {
    const char* input = "Something\nElse";
    char** split = split_input(input, strlen(input), 1);
    assert(split[0] == input);
    assert(split[1] == input + strlen(input));

    split = split_input(input, strlen(input), 2);
    assert(split[0] == input);
    assert(split[1] == input + strlen("Something\n"));
    assert(split[2] == input + strlen(input));
    
    
    input = "Kingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\nKingston;31.4\n";
    
    split = split_input(input, strlen(input), 1);
    assert(split[0] == input);
    assert(split[1] == input + strlen(input));
    
    split = split_input(input, strlen(input), 2);
    assert(split[0] < split[1]);
    assert(split[1] < split[2]);
    assert(split[0] == input);
    assert(split[2] == input + strlen(input));
}

char** split_input(char* start, size_t size, int num_cores);

// Fix these tests
//    test_atoi();
//    exit(0);
//    test_index_of_newline();
//    exit(0);

int main(int argc, const char * argv[]) {
    test_short_strings();
    test_long_strings();
    test_split_input();
    return 0;
}
