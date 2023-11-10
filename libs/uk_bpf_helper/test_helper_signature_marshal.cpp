extern "C" {
#include "uk_bpf_helper_utils.h"
}

#include <cstdlib>
#include <string>
#include <cassert>
#include <iostream>
#include <sstream>

std::string buffer;

static void append_result(const char *result) {
    buffer += result;
}

void assert_empty_list() {
    buffer.clear();

    HelperFunctionList *list = helper_function_list_init();

    marshall_bpf_helper_definitions(list, append_result);
    assert(buffer.empty());

    helper_function_list_destroy(list);
}

void assert_empty_arg_list() {
    buffer.clear();

    auto *list = helper_function_list_init();
    auto *result = helper_function_list_emplace_back(list, 1, 11, "test", nullptr,
                                                     UK_EBPF_RETURN_TYPE_INTEGER, 0, nullptr);

    assert(list->m_tail == result);

    marshall_bpf_helper_definitions(list, append_result);
    assert("1,b:test()->" + std::to_string(UK_EBPF_RETURN_TYPE_INTEGER) == buffer);

    helper_function_list_destroy(list);
}

void assert_many_args() {
    buffer.clear();

    HelperFunctionList *list = helper_function_list_init();

    uk_ebpf_argument_type_t arg_types[] = {
            UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM_OR_NULL,
            UK_EBPF_ARGUMENT_TYPE_CONST_SIZE,
            UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO};
    helper_function_list_emplace_back(list, 1, 11, "test", nullptr,
                                      UK_EBPF_RETURN_TYPE_UNSUPPORTED, 3,
                                      arg_types);

    marshall_bpf_helper_definitions(list, append_result);
    std::stringstream stream;
    stream << std::hex << UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM_OR_NULL;

    assert("1,b:test(" + stream.str() + ","
           + std::to_string(UK_EBPF_ARGUMENT_TYPE_CONST_SIZE) + ","
           + std::to_string(UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO)
           + ")->" + std::to_string(UK_EBPF_RETURN_TYPE_UNSUPPORTED)
           == buffer);

    helper_function_list_destroy(list);
}

void assert_many_functions() {
    buffer.clear();

    HelperFunctionList *list = helper_function_list_init();
    helper_function_list_emplace_back(list, 1, 11, "test", nullptr,
                                      UK_EBPF_RETURN_TYPE_INTEGER, 0, nullptr);

    uk_ebpf_argument_type_t arg_types[] = {
            UK_EBPF_ARGUMENT_TYPE_ANYTHING,
            UK_EBPF_ARGUMENT_TYPE_CONST_SIZE,
            UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO};
    helper_function_list_emplace_back(list, 2, 12, "test2", nullptr,
                                      UK_EBPF_RETURN_TYPE_UNSUPPORTED, 3,
                                      arg_types);

    marshall_bpf_helper_definitions(list, append_result);
    assert("1,b:test()->" + std::to_string(UK_EBPF_RETURN_TYPE_INTEGER) + ";"
           + "2,c:test2(" + std::to_string(UK_EBPF_ARGUMENT_TYPE_ANYTHING)
           + "," + std::to_string(UK_EBPF_ARGUMENT_TYPE_CONST_SIZE) + ","
           + std::to_string(UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO)
           + ")->" + std::to_string(UK_EBPF_RETURN_TYPE_UNSUPPORTED)
           == buffer);

    helper_function_list_destroy(list);
}

void assert_unmarshal_empty() {
    assert_empty_list();

    auto *result = unmarshall_bpf_helper_definitions(buffer.c_str());

    assert(result != nullptr);
    helper_function_list_destroy(result);
}

void assert_unmarshal_hex_ret_type() {
    auto *result2 = unmarshall_bpf_helper_definitions("1,b:test()->a");
    assert(result2 != nullptr);
    assert(result2->m_head->m_function_signature.m_return_type == (uk_ebpf_return_type_t) 10);
    assert(result2->m_head->m_index == 1);
    assert(result2->m_head->m_prog_type_id == 11);

    helper_function_list_destroy(result2);
}

void assert_unmarshal_empty_arg_list() {
    assert_empty_arg_list();

    auto *result = unmarshall_bpf_helper_definitions(buffer.c_str());

    assert(result != nullptr);
    assert(result->m_length == 1);
    assert(result->m_tail->m_index == 1);
    assert(
            strcmp(result->m_tail->m_function_signature.m_function_name, "test")
            == 0);
    assert(result->m_tail->m_function_signature.m_return_type
           == UK_EBPF_RETURN_TYPE_INTEGER);
    assert(result->m_tail->m_function_signature.m_num_args == 0);

    helper_function_list_destroy(result);
}

void assert_unmarshal_many_args_list() {
    assert_many_args();

    auto *result = unmarshall_bpf_helper_definitions(buffer.c_str());

    assert(result != nullptr);
    assert(result->m_length == 1);
    assert(result->m_tail->m_index == 1);
    assert(
            strcmp(result->m_tail->m_function_signature.m_function_name, "test")
            == 0);
    assert(result->m_tail->m_function_signature.m_return_type
           == UK_EBPF_RETURN_TYPE_UNSUPPORTED);
    assert(result->m_tail->m_function_signature.m_num_args == 3);
    assert(result->m_tail->m_function_signature.m_arg_types[0]
           == UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM_OR_NULL);
    assert(result->m_tail->m_function_signature.m_arg_types[1]
           == UK_EBPF_ARGUMENT_TYPE_CONST_SIZE);
    assert(result->m_tail->m_function_signature.m_arg_types[2]
           == UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO);

    helper_function_list_destroy(result);
}

void assert_unmarshal_many_functions_list() {
    assert_many_functions();

    auto *result = unmarshall_bpf_helper_definitions(buffer.c_str());

    assert(result != nullptr);
    assert(result->m_length == 2);

    assert(result->m_head->m_index == 1);
    assert(
            strcmp(result->m_head->m_function_signature.m_function_name, "test")
            == 0);
    assert(result->m_head->m_function_signature.m_return_type
           == UK_EBPF_RETURN_TYPE_INTEGER);
    assert(result->m_head->m_function_signature.m_num_args == 0);

    assert(result->m_tail->m_index == 2);
    assert(strcmp(result->m_tail->m_function_signature.m_function_name,
                  "test2")
           == 0);
    assert(result->m_tail->m_function_signature.m_return_type
           == UK_EBPF_RETURN_TYPE_UNSUPPORTED);
    assert(result->m_tail->m_function_signature.m_num_args == 3);
    assert(result->m_tail->m_function_signature.m_arg_types[0]
           == UK_EBPF_ARGUMENT_TYPE_ANYTHING);
    assert(result->m_tail->m_function_signature.m_arg_types[1]
           == UK_EBPF_ARGUMENT_TYPE_CONST_SIZE);
    assert(result->m_tail->m_function_signature.m_arg_types[2]
           == UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO);

    helper_function_list_destroy(result);
}

void assert_reject_null_input() {
    buffer.clear();
    assert(unmarshall_bpf_helper_definitions(nullptr) == nullptr);
}

void assert_reject_empty_function_name() {
    buffer.clear();
    buffer.append("1,1:test()->0;2,2:()->1");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_empty_group_id() {
    buffer.clear();
    buffer.append("1,2:test()->0;2,:()->1");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1,2:test()->0;2:()->1");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}


void assert_reject_empty_index() {
    buffer.clear();
    buffer.append("1,1:test()->0;:()->1");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1,1:test()->0;,:()->1");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_unexpected_eof() {
    buffer.clear();
    buffer.append("0,1:test()->0;1,1:test2()->");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->0;1,1:test2()-");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->0;1,1:test2()");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->0;1,1:test2(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->0;1,1:test2");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->0;1,1:");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->0,1;");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_broken_syntax() {
    buffer.clear();
    buffer.append("0,1:test()-0;1,1:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test)->0;1,1:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test((->0;1,1:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test)(->0;1,1:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()-->0;0,1:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->>0;0,1:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:t-est(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:t>est(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:t)est(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:t:est(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);


    buffer.clear();
    buffer.append("0,1:t;est(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:t,est(");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()>-0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0:test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("01test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1;test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append(":test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append(",:test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append(";test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("test()>0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_invalid_return_type() {
    buffer.clear();
    buffer.append("0,1:test)->");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test)->x");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test)->xxx");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->+");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->A");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->x");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()-");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test()->;0,2:test2()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_invalid_arg_type() {
    buffer.clear();
    buffer.append("0,1:test(,)->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test(0,)->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test(,0)->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test(abc,xxx)->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append(
            "0,1:test(fffffffffffffffff)->0"); // overflow, not a valid uint64_t
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("0,1:test->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_invalid_index() {
    buffer.clear();
    buffer.append(",1:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append(":test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("x,1:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("aaaaaaaaa,1:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("A,1:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_reject_invalid_group_id() {
    buffer.clear();
    buffer.append(":test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1,:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1,x:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1,aaaaaaaaaaaaaaaaa:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);

    buffer.clear();
    buffer.append("1,A:test()->0");
    assert(unmarshall_bpf_helper_definitions(buffer.c_str()) == nullptr);
}

void assert_empty_prog_type_list() {
    buffer.clear();

    auto *list = bpf_prog_type_list_init();

    marshall_bpf_prog_types(list, append_result);
    assert(buffer.empty());

    bpf_prog_type_list_destroy(list);
}

void assert_null_prog_type_list() {
    buffer.clear();

    marshall_bpf_prog_types(nullptr, append_result);
    assert(buffer.empty());
}

void assert_prog_type_list_w_one_element() {
    buffer.clear();

    auto *list = bpf_prog_type_list_init();

    bpf_prog_type_list_emplace_back(list, 1, "test", true, 42, 43, 44, 45);

    marshall_bpf_prog_types(list, append_result);
    assert("1,test:1,2a,2b,2c,2d" == buffer);

    bpf_prog_type_list_destroy(list);
}

void assert_prog_type_list_w_multi_element() {
    buffer.clear();

    auto *list = bpf_prog_type_list_init();

    bpf_prog_type_list_emplace_back(list, 1, "test", true, 42, 43, 44, 45);
    bpf_prog_type_list_emplace_back(list, 11, "test2", false, 142, 143, 144, 145);

    marshall_bpf_prog_types(list, append_result);
    assert("1,test:1,2a,2b,2c,2d;b,test2:0,8e,8f,90,91" == buffer);

    bpf_prog_type_list_destroy(list);
}


void assert_unmarshal_empty_prog_type() {
    assert_empty_prog_type_list();

    auto *list = unmarshall_bpf_prog_types(buffer.c_str());
    assert(list != nullptr);

    assert(list->m_length == 0);
    assert(list->m_head == nullptr);
    assert(list->m_tail == nullptr);

    bpf_prog_type_list_destroy(list);
}


void assert_unmarshal_prog_type_null() {
    auto *list = unmarshall_bpf_prog_types(nullptr);
    assert(list == nullptr);
}

void assert_unmarshal_prog_type_w_one() {
    assert_prog_type_list_w_one_element();
    // bpf_prog_type_list_emplace_back(list, 1, "test", true, 42, 43, 44, 45);

    auto *list = unmarshall_bpf_prog_types(buffer.c_str());
    assert(list != nullptr);
    assert(list->m_head != nullptr);
    assert(list->m_tail != nullptr);
    assert(list->m_head == list->m_tail);

    assert(list->m_tail->m_next == nullptr);
    assert(list->m_tail->prog_type_id == 1);
    assert(strcmp(list->m_tail->m_prog_type_name, "test") == 0);
    assert(list->m_tail->privileged == 1);
    assert(list->m_tail->ctx_descriptor_struct_size == 42);
    assert(list->m_tail->offset_to_data_ptr == 43);
    assert(list->m_tail->offset_to_data_end_ptr == 44);
    assert(list->m_tail->offset_to_ctx_metadata == 45);

    bpf_prog_type_list_destroy(list);
}

void assert_unmarshal_prog_type_w_multi() {
    assert_prog_type_list_w_multi_element();

    // bpf_prog_type_list_emplace_back(list, 1, "test", true, 42, 43, 44, 45);
    // bpf_prog_type_list_emplace_back(list, 11, "test2", false, 142, 143, 144, 145);

    auto *list = unmarshall_bpf_prog_types(buffer.c_str());
    assert(list != nullptr);
    assert(list->m_head != nullptr);
    assert(list->m_tail != nullptr);
    assert(list->m_head != list->m_tail);

    assert(list->m_head->m_next == list->m_tail);
    assert(list->m_head->prog_type_id == 1);
    assert(strcmp(list->m_head->m_prog_type_name, "test") == 0);
    assert(list->m_head->privileged == 1);
    assert(list->m_head->ctx_descriptor_struct_size == 42);
    assert(list->m_head->offset_to_data_ptr == 43);
    assert(list->m_head->offset_to_data_end_ptr == 44);
    assert(list->m_head->offset_to_ctx_metadata == 45);

    assert(list->m_tail->m_next == nullptr);
    assert(list->m_tail->prog_type_id == 11);
    assert(strcmp(list->m_tail->m_prog_type_name, "test2") == 0);
    assert(list->m_tail->privileged == 0);
    assert(list->m_tail->ctx_descriptor_struct_size == 142);
    assert(list->m_tail->offset_to_data_ptr == 143);
    assert(list->m_tail->offset_to_data_end_ptr == 144);
    assert(list->m_tail->offset_to_ctx_metadata == 145);

    bpf_prog_type_list_destroy(list);
}

void assert_unmarshal_prog_type_name_w_slash() {
    auto *list = unmarshall_bpf_prog_types("2a,test/:1,1,2,3,4");
    assert(list != nullptr);
    assert(list->m_head != nullptr);
    assert(list->m_tail != nullptr);
    assert(list->m_head == list->m_tail);

    assert(list->m_tail->m_next == nullptr);
    assert(list->m_tail->prog_type_id == 42);
    assert(strcmp(list->m_tail->m_prog_type_name, "test/") == 0);
    assert(list->m_tail->privileged == true);
    assert(list->m_tail->ctx_descriptor_struct_size == 1);
    assert(list->m_tail->offset_to_data_ptr == 2);
    assert(list->m_tail->offset_to_data_end_ptr == 3);
    assert(list->m_tail->offset_to_ctx_metadata == 4);

    bpf_prog_type_list_destroy(list);
}

void assert_unmarshal_prog_type_reject_invalid_prog_type_id() {
    auto *list = unmarshall_bpf_prog_types("0,test/:1,2,3;,test/:1,2,3;");
    assert(list == nullptr);

    auto *list2 =unmarshall_bpf_prog_types("0,test/:1,2,3;1test/:1,2,3;");
    assert(list2 == nullptr);

    auto *list3 = unmarshall_bpf_prog_types("0,test/:1,2,3;:test/:1,2,3;");
    assert(list3 == nullptr);
}


int main() {
    //TODO assert arg list length <= sizeof(ukPrototypes[helper->m_index].argument_type) /
    //                                    sizeof(ukPrototypes[helper->m_index].argument_type[0]
    /*assert_empty_list();
    assert_empty_arg_list();
    assert_many_args();
    assert_many_functions();
    assert_reject_empty_index();
    assert_unmarshal_empty();
    assert_unmarshal_hex_ret_type();
    assert_unmarshal_empty_arg_list();
    assert_unmarshal_many_args_list();
    assert_unmarshal_many_functions_list();

    assert_reject_null_input();
    assert_reject_empty_function_name();
    assert_reject_empty_group_id();
    assert_reject_unexpected_eof();
    assert_reject_broken_syntax();
    assert_reject_invalid_return_type();
    assert_reject_invalid_arg_type();
    assert_reject_invalid_index();
    assert_reject_invalid_group_id();

    // helper prog_type list tests
    assert_empty_prog_type_list();
    assert_null_prog_type_list();
    assert_prog_type_list_w_one_element();
    assert_prog_type_list_w_multi_element();

    assert_unmarshal_empty_prog_type();
    assert_unmarshal_prog_type_null();*/
    assert_unmarshal_prog_type_w_one();
    assert_unmarshal_prog_type_w_multi();
    assert_unmarshal_prog_type_name_w_slash();

    exit(EXIT_SUCCESS);
}