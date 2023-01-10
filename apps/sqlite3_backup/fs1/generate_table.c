#include "unicall_wrapper.h"

extern void sqlite3_generate_table();

__attribute__((section(".text"))) int main()
{
    unikraft_call_wrapper(sqlite3_generate_table);
    return 0;
}
