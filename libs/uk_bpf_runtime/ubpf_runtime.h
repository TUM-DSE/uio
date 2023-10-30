#ifndef USHELL_TERMINAL_UBPF_RUNTIME_H
#define USHELL_TERMINAL_UBPF_RUNTIME_H

#include <ubpf.h>
#include <sys/types.h>
#include <stdlib.h>

#define ERR(st) "\033[0m\033[1;31m" st "\033[0m"
#define YAY(st) "\033[0m\033[1;32m" st "\033[0m"
#define wrap_print_fn(BUF_SIZE, ...)                                   \
	{                                                                  \
		char buf[BUF_SIZE];                                            \
		snprintf(buf, BUF_SIZE, __VA_ARGS__);                          \
		print_fn(buf);                                                 \
	}

struct bpf_vm_setup_result 
	   setup_bpf_vm(FILE *logfile, const char *bpf_prog_filename, const char* function_name,
                                        void (*print_fn)(char *str));

void *readfile(const char *path, size_t maxlen, size_t *len);

// shell commands
int bpf_exec(const char *filename, const char* function_name, void *args, size_t args_size, int debug,
	     void (*print_fn)(char *str));

struct bpf_vm_setup_result {
	struct ubpf_vm* vm; // if this is set to NULL, setup VM was failed
	ubpf_jit_fn jitted; // jitted will exist only if "CONFIG_LIB_UNIBPF_JIT_COMPILE" is defined
};

extern struct bpf_vm_setup_result BPF_RUNTIME_INVALID;

void destroy_bpf_runtime(struct bpf_vm_setup_result bpf_runtime);

#endif // USHELL_TERMINAL_UBPF_RUNTIME_H
