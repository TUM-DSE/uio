#include "unicall_wrapper.h"

extern unsigned int sleep(unsigned);

__attribute__((section(".text"))) int main()
{
	unikraft_call_wrapper(sleep, 1);
	return 0;
}
