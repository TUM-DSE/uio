#ifndef UK_BPF_HELPER_UTILS_H
#define UK_BPF_HELPER_UTILS_H

#include "helper_function_list.h"
#include "prog_type_list.h"

#define UK_BPF_HELPER_DEFINITION_INDEX_END ","

#define UK_BPF_HELPER_DEFINITION_PROG_TYPE_ID_END ":"

#define UK_BPF_HELPER_DEFINITION_ARGUMENT_TYPE_START "("
#define UK_BPF_HELPER_DEFINITION_ARGUMENT_TYPE_SPLIT ","
#define UK_BPF_HELPER_DEFINITION_ARGUMENT_TYPE_END ")"

#define UK_BPF_HELPER_DEFINITION_RETURN_TYPE_INDICATOR "->"

#define UK_BPF_HELPER_DEFINITION_FUNCTION_SPLIT ";"

void marshall_bpf_helper_definitions(HelperFunctionList *instance,
                                     void (*append_result)(const char *));

HelperFunctionList *unmarshall_bpf_helper_definitions(const char *input);

#define UK_BPF_PROG_TYPE_ID_END ","
#define UK_BPF_PROG_TYPE_DEFINITIONS_START_INDICATOR ":"
#define UK_BPF_PROG_TYPE_DEFINITIONS_SPLIT ","
#define UK_BPF_PROG_TYPE_LIST_SPLIT ";"

void marshall_bpf_prog_types(BpfProgTypeList *instance,
                             void (*append_result)(const char *));

BpfProgTypeList *unmarshall_bpf_prog_types(const char *input);

#endif // UK_BPF_HELPER_UTILS_H
