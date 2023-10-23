#include <asm/types.h>

#define MAX_EXECUTABLE_CTX_SIZE 1024

typedef struct uk_bpf_type_executable {
    char* data;
    char* data_end;
    __u64 data_meta;

    char storage[];
} uk_bpf_type_executable_t;

//#include "bpf_helpers.h"

// acutal:
// #define bpf_map_put ((void (*)(__u64 key1, __u64 key2, __u64 value))2)

// faked:
#define bpf_map_put ((void (*)(__u64 key1, __u64 key2))2)

__attribute__((section("executable"), used))
char helper_signature(uk_bpf_type_executable_t* context) {

    bpf_map_put(0,0);

    return 0;
}

// bpf_exec bpf-security/helper_signature.o helper_signature