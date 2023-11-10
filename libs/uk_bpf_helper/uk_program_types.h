#ifndef USHELL_TERMINAL_UK_PROGRAM_TYPES_H
#define USHELL_TERMINAL_UK_PROGRAM_TYPES_H

#include <stdint.h>

#define UK_EBPF_PROG_TYPE_UNSPECIFIED 0

#define MAX_EXECUTABLE_CTX_SIZE 1024

typedef struct uk_bpf_type_executable {
    char* data;
    char* data_end;
    uint64_t data_meta;

    char storage[MAX_EXECUTABLE_CTX_SIZE -
        sizeof(char*) - // data
        sizeof(char*) - // data_end
        sizeof(uint64_t) // data_meta
    ];
} uk_bpf_type_executable_t;

#endif //USHELL_TERMINAL_UK_PROGRAM_TYPES_H
