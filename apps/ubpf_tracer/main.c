#include <stdio.h>
#include <unistd.h>
#include <ubpf_config.h>
#include <ubpf_tracer.h>

__attribute__((noinline)) void myfun()
{
	printf("hello from myfun\n");
	return;
}

int main()
{
	printf("Hello world!!\n");

	while (1) {
		myfun();
		sleep(10);
	}

	return 0;
}
