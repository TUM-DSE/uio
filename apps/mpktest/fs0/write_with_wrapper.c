#include "unicall_wrapper.h"
extern int ushell_mpk_test_var;

__attribute__((section(".text"))) int main()
{
	unikraft_write_var(ushell_mpk_test_var, 42);
	return 0;
}
