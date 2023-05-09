#include "unicall_wrapper.h"

#include <stdlib.h>

__attribute__((section(".text")))
int main(int argc, char *argv[])
{
	int a = 0;
	int b = 0;
	if (argc >= 2) {
		a = atoi(argv[1]);
	}
	if (argc >= 3) {
		b = atoi(argv[2]);
	}
	return a + b;
}
