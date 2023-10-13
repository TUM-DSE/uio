#include "ubpf_runtime.h"

#include <uk/plat/time.h>

#include <ubpf.h>
#include <ubpf_int.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <helper_function_list.h>
#include <uk_program_types.h>
#include <uk/plat/paging.h>
#include "ubpf_helpers.h"

// private helper functions
static inline void register_helper(FILE *logfile, struct ubpf_vm *vm,
                                   const UK_UBPF_INDEX_t index,
                                   const char *function_name,
                                   const void *function_ptr) {
    if (logfile != NULL) {
        fprintf(logfile, " - [%u]: %s\n", index, function_name);
    }
    ubpf_register(vm, index, function_name, (void *) function_ptr);
}
// end of private helper functions


void *readfile(const char *path, size_t maxlen, size_t *len) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", path,
                strerror(errno));
        return NULL;
    }

    char *data = calloc(maxlen, 1);
    size_t offset = 0;
    size_t rv;
    while ((rv = fread(data + offset, 1, maxlen - offset, file)) > 0) {
        offset += rv;
    }

    if (ferror(file)) {
        fprintf(stderr, "Failed to read %s: %s\n", path,
                strerror(errno));
        fclose(file);
        free(data);
        return NULL;
    }

    if (!feof(file)) {
        fprintf(stderr,
                "Failed to read %s because it is too large (max %u "
                "bytes)\n",
                path, (unsigned) maxlen);
        fclose(file);
        free(data);
        return NULL;
    }

    fclose(file);
    if (len) {
        *len = offset;
    }
    return (void *) data;
}


struct ubpf_vm *init_vm(FILE *logfile) {
    struct ubpf_vm *vm = ubpf_create();
    if (logfile != NULL) {
        fprintf(logfile, "attached BPF helpers:\n");
    }

    HelperFunctionList *builtin_helpers = init_builtin_bpf_helpers();
    HelperFunctionEntry *unwind_function = NULL;

    for (HelperFunctionEntry *entry = builtin_helpers->m_head;
         entry != NULL; entry = entry->m_next) {
        register_helper(logfile, vm, entry->m_index,
                        entry->m_function_signature.m_function_name,
                        entry->m_function_addr);

        if (entry->m_function_signature.m_return_type
            == UK_EBPF_RETURN_TYPE_INTEGER_OR_NO_RETURN_IF_SUCCEED) {
            unwind_function = entry;
        }
    }

    if (unwind_function) {
        ubpf_set_unwind_function_index(vm, unwind_function->m_index);
    }

    return vm;
}

#include <uk/assert.h>
#include <uk/essentials.h>
#define size_to_num_pages(size) \
	(ALIGN_UP((unsigned long)(size), __PAGE_SIZE) / __PAGE_SIZE)

int bpf_exec(const char *filename, const char *function_name, void *args, size_t args_size, int debug,
             void (*print_fn)(char *str)) {
    FILE *logfile = NULL;
    if (debug != 0) {
        logfile = fopen("bpf_exec.log", "a");
    }

    if (logfile != NULL) {
        fprintf(logfile, "\n# bpf_exec %s", filename);
        if (args != NULL) {
            fprintf(logfile, " %s", (char *) args);
        }
        fprintf(logfile, "\n");
    }

    struct ubpf_vm *vm = init_vm(logfile);
    size_t code_len;
    void *code = readfile(filename, 1024 * 1024, &code_len);
    if (code == NULL) {
        fclose(logfile);
        return 1;
    }
    char *errmsg;
    int rv;
#if defined(UBPF_HAS_ELF_H)
    rv = ubpf_load_elf_ex(vm, code, code_len, function_name, &errmsg);
#else
    rv = ubpf_load(vm, code, code_len, &errmsg);
#endif
    free(code);

    if (rv < 0) {
        size_t buf_size = 100 + strlen(errmsg);
        wrap_print_fn(buf_size, ERR("Failed to load code: %s\n"),
                      errmsg);
        if (logfile != NULL) {
            fprintf(logfile, "Failed to load code: %s\n", errmsg);
        }

        free(errmsg);
        ubpf_destroy(vm);
        if (logfile != NULL) {
            fclose(logfile);
        }
        return 1;
    }

    // create context on local stack
    uk_bpf_type_executable_t context;
    context.data = context.storage;
    context.data_meta = 0;

    const size_t max_data_size = sizeof(context.storage);
    const size_t data_size = args_size > max_data_size ? max_data_size - 1 : args_size;

    context.data_end = context.storage + data_size - 1;

    strncpy(context.storage, args, data_size);
    if (data_size == max_data_size - 1) {
        context.storage[data_size] = '\0';
    }
    // start bpf program

    char buf[16];
    uint64_t begin;
    uint64_t end;

    uint64_t ret;
#ifdef CONFIG_LIB_UNIBPF_JIT_COMPILE
    char* compileError;
    begin = ukplat_monotonic_clock();
    ubpf_jit_fn jitted_bpf = ubpf_compile(vm, &compileError);
    end = ukplat_monotonic_clock();

    wrap_print_fn(128, YAY("BPF program compile took: %lu ns\n"), end - begin)
    if (logfile != NULL) {
        fprintf(logfile, "BPF program compile took: %lu ns\n", end - begin);
    }

    UK_ASSERT(((size_t)jitted_bpf) % __PAGE_SIZE == 0);

    struct uk_pagetable *page_table = ukplat_pt_get_active();
    unsigned long pages = size_to_num_pages(vm->jitted_size);
    int set_page_attr_result = ukplat_page_set_attr(page_table, (__vaddr_t)jitted_bpf, pages, PAGE_ATTR_PROT_READ | PAGE_ATTR_PROT_WRITE | PAGE_ATTR_PROT_EXEC, 0);
    if(set_page_attr_result < 0) {
        wrap_print_fn(128, ERR("BPF program page jitted code executable failed (%d)\n"), set_page_attr_result)

        if (logfile != NULL) {
            fprintf(logfile, "BPF program page jitted code executable failed (%d)\n", set_page_attr_result);
        }

        goto clean_up;
    }

    if(!jitted_bpf) {
        print_fn(ERR("BPF program compile failed: "));
        print_fn(compileError);
        print_fn("\n");
        if (logfile != NULL) {
            fprintf(logfile, "BPF program compile failed: %s\n", compileError);
        }

        free(compileError);
        goto clean_up;
    }

    begin = ukplat_monotonic_clock();
    ret = jitted_bpf(&context, sizeof(context));
    end = ukplat_monotonic_clock();

#else
    print_fn(ERR("Using BPF Interpreter\n"));
    begin = ukplat_monotonic_clock();
    int interpret_result = ubpf_exec(vm, &context, sizeof(context), &ret);
    end = ukplat_monotonic_clock();

    if (interpret_result < 0) {
        print_fn(ERR("BPF program interpretation failed.\n"));
        if (logfile != NULL) {
            fprintf(logfile, "BPF program interpretation failed.\n");
        }

        goto clean_up;
    }
#endif // endof if LIB_UNIBPF_JIT_COMPILE
    wrap_print_fn(128, YAY("BPF program returned: %lu. Took: %d ns\n"), ret, end - begin)
    if (logfile != NULL) {
        fprintf(logfile, "BPF program returned: %lu. Took: %d ns\n", ret, end - begin);
    }

    clean_up:
    ubpf_destroy(vm);

    if (logfile != NULL) {
        fclose(logfile);
    }

    return 0;
}
