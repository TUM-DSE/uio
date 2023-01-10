#include "unicall_wrapper.h"

extern void sqlite3_save();

__attribute__((section(".text"))) int main()
{
    unikraft_call_wrapper(sqlite3_save);
    return 0;
}
