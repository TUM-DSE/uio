//
// Created by ken on 05.06.23.
//

#ifndef USHELL_TERMINAL_UBPF_RUNTIME_H
#define USHELL_TERMINAL_UBPF_RUNTIME_H

#include <ubpf.h>
#include <sys/types.h>
#include <stdlib.h>

#define ERR(st) "\033[0m\033[1;31m" st "\033[0m"
#define YAY(st) "\033[0m\033[1;32m" st "\033[0m"
#define wrap_print_fn(BUF_SIZE, ...)                                           \
	{                                                                      \
		char *buf = calloc(BUF_SIZE, sizeof(char));                    \
		snprintf(buf, BUF_SIZE, __VA_ARGS__);                          \
		print_fn(buf);                                                 \
		free(buf);                                                     \
	}

struct ubpf_vm *init_vm(FILE *logfile);
void *readfile(const char *path, size_t maxlen, size_t *len);

// shell commands
int bpf_exec(const char *filename, const char* function_name, void *args, size_t args_size, int debug,
	     void (*print_fn)(char *str));

#endif // USHELL_TERMINAL_UBPF_RUNTIME_H
