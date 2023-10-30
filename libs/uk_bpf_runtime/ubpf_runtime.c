#include "ubpf_runtime.h"

#include <uk/plat/time.h>

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

#include <uk/assert.h>
#include <uk/essentials.h>
#define size_to_num_pages(size) \
	(ALIGN_UP((unsigned long)(size), __PAGE_SIZE) / __PAGE_SIZE)

static struct ubpf_vm *init_vm(FILE *logfile) {
    struct ubpf_vm *vm = ubpf_create();
    if (logfile != NULL) {
        fprintf(logfile, "attached BPF helpers:\n");
    }

    HelperFunctionList *helper_function_list = get_bpf_helpers();

    HelperFunctionEntry *unwind_function = NULL;

    for (HelperFunctionEntry *entry = helper_function_list->m_head;
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

struct bpf_vm_setup_result BPF_RUNTIME_INVALID = {
    .vm = NULL,
    .jitted = NULL,
};

struct bpf_vm_setup_result setup_bpf_vm(FILE *logfile, const char *bpf_prog_filename, const char* function_name,
                                        void (*print_fn)(char *str)) {
    struct ubpf_vm *vm = init_vm(logfile);
    size_t code_len;
    void *code = readfile(bpf_prog_filename, 1024 * 1024, &code_len);
    if (code == NULL) {
        wrap_print_fn(60 + strlen(bpf_prog_filename), ERR("Failed to read BPF program from: %s\n"), bpf_prog_filename);

        return BPF_RUNTIME_INVALID;
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
        wrap_print_fn(buf_size, ERR("Failed to load code: %s\n"), errmsg);
        if (logfile != NULL) {
            fprintf(logfile, "Failed to load code: %s\n", errmsg);
        }

        free(errmsg);
        ubpf_destroy(vm);

        return BPF_RUNTIME_INVALID;
    }

    // compile BPF program once needed
#ifdef CONFIG_LIB_UNIBPF_JIT_COMPILE
    uint64_t begin;
    uint64_t end;
    char* compileError;

    begin = ukplat_monotonic_clock();
    ubpf_jit_fn jitted_bpf = ubpf_compile(vm, &compileError);
    end = ukplat_monotonic_clock();

    if(!jitted_bpf) {
        print_fn(ERR("BPF program compile failed: "));
        print_fn(compileError);
        print_fn("\n");
        if (logfile != NULL) {
            fprintf(logfile, "BPF program compile failed: %s\n", compileError);
        }

        free(compileError);
        ubpf_destroy(vm);

        return BPF_RUNTIME_INVALID;
    }

    wrap_print_fn(128, YAY("BPF program compile took: %lu ns\n"), end - begin)
    if (logfile != NULL) {
        fprintf(logfile, "BPF program compile took: %lu ns\n", end - begin);
    }

    UK_ASSERT(((size_t)jitted_bpf) % __PAGE_SIZE == 0);

    // mark jitted BPF program as executable
    struct uk_pagetable *page_table = ukplat_pt_get_active();
    unsigned long pages = size_to_num_pages(vm->jitted_size);
    int set_page_attr_result = ukplat_page_set_attr(page_table, (__vaddr_t)jitted_bpf, pages, PAGE_ATTR_PROT_READ | PAGE_ATTR_PROT_WRITE | PAGE_ATTR_PROT_EXEC, 0);
    if(set_page_attr_result < 0) {
        wrap_print_fn(128, ERR("BPF program page jitted code executable failed (%d)\n"), set_page_attr_result)

        if (logfile != NULL) {
            fprintf(logfile, "BPF program page jitted code executable failed (%d)\n", set_page_attr_result);
        }

        free(jitted_bpf);
        ubpf_destroy(vm);

        return BPF_RUNTIME_INVALID;
    }

    return (struct bpf_vm_setup_result) {
        .vm = vm,
        .jitted = jitted_bpf,
    };
#else
    return (struct bpf_vm_setup_result) {
        .vm = vm,
        .jitted = NULL,
    };
#endif // endof if LIB_UNIBPF_JIT_COMPILE
}

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

    
    struct bpf_vm_setup_result bpf_runtime = setup_bpf_vm(logfile, filename, function_name, print_fn);
    
    if(bpf_runtime.vm == NULL) {
        return -1; // failed to setup bpf vm
    }

    // start bpf program
    uint64_t begin;
    uint64_t end;
    uint64_t bpf_program_ret;

#ifdef CONFIG_LIB_UNIBPF_JIT_COMPILE
    begin = ukplat_monotonic_clock();
    bpf_program_ret = bpf_runtime.jitted(&context, sizeof(context));
    end = ukplat_monotonic_clock();

#else
    print_fn(ERR("Using BPF Interpreter\n"));
    begin = ukplat_monotonic_clock();
    int interpreter_result = ubpf_exec(bpf_runtime.vm, &context, sizeof(context), &bpf_program_ret);
    end = ukplat_monotonic_clock();

    if (interpreter_result < 0) {
        print_fn(ERR("BPF program interpretation failed.\n"));
        if (logfile != NULL) {
            fprintf(logfile, "BPF program interpretation failed.\n");
        }

        goto clean_up;
    }
#endif // endof if LIB_UNIBPF_JIT_COMPILE

    wrap_print_fn(128, YAY("BPF program returned: %lu. Took: %ld ns\n"), bpf_program_ret, end - begin)
    if (logfile != NULL) {
        fprintf(logfile, "BPF program returned: %lu. Took: %ld ns\n", bpf_program_ret, end - begin);
    }

    // clean up
clean_up:
    ubpf_destroy(bpf_runtime.vm);

    if (logfile != NULL) {
        fclose(logfile);
    }

    return 0;
}

void destroy_bpf_runtime(struct bpf_vm_setup_result bpf_runtime) {
    ubpf_destroy(bpf_runtime.vm);
}