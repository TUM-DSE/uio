#include <stdlib.h>
#include <stdint.h>
#include <string.h>

inline uint64_t rdtsc(void){
    union {
        uint64_t val;
        struct {
            uint32_t lo;
            uint32_t hi;
        };
    } tsc;
    asm volatile ("rdtsc" : "=a" (tsc.lo), "=d" (tsc.hi));
    return tsc.val;
}
#include <stdio.h>

void put_things_in_string(char* str, size_t n){
    for(int i=0; i<n; i++){
        char c = i%10;
        str[i] = c;
    }
    str[n-1] = '\0';
}

int main() {
    const uint64_t nb_op = 1000000000;
    const int repetitions = 1;
    char *og = (char*) malloc(4096*sizeof(char));
    char *dest = (char*) malloc(4096*sizeof(char));
    int sizes_memcpy[] = {0, 1, 2, 4, 8, 16, 32, 64, 72, 96, 128, 256, 512, 768, 4088};
    int sizes_memcmp[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 25, 30, 35, 40, 41};

    uint64_t acc_time=0;
    int acc = 0;
    put_things_in_string(og, 4096);
    put_things_in_string(dest, 4096);

    int i = 0;
    for(i = 0; i < sizeof(sizes_memcmp)/sizeof(int); i++){
        int cmpSize = sizes_memcmp[i];
        acc = 0;
        acc_time=0.0;
        for(int i=0; i<repetitions; i++){
            uint64_t start = rdtsc();
            //auto start = chrono::system_clock::now();
            for(volatile uint64_t j = 0; j<nb_op; j++){
                acc += memcmp(dest, og, cmpSize);
            }
            //auto stop = chrono::system_clock::now();
            //chrono::duration<double> sec = stop - start;
            //acc_time += sec.count();
            uint64_t stop = rdtsc();
            acc_time += (stop-start);
        }
        char *system = "unikraft";
        printf("memcmp,%d,%s,%f,%d\n", cmpSize, system, ((double)(acc_time))/(repetitions*nb_op), acc);
    }
}
