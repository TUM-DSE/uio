#include <stdio.h>
#include <unistd.h>
#include <ubpf_tracer.h>

#include <sys/time.h>
#include <inttypes.h>

// void mcount(unsigned long frompc)
//{
//	printf("Function called from address 0x%lx\n", frompc);
// }

int64_t millis()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return ((int64_t)now.tv_sec) * 1000 + ((int64_t)now.tv_usec) / 1000;
}

int64_t start_myfun, end_myfun;
__attribute__((noinline)) void myfun()
{
	//	end_myfun = millis();
	//	printf("myfun: %ldms\n", end_myfun - start_myfun);
	printf("hello from myfun\n");
	return;
}

int64_t start_hello, end_hello;
__attribute__((noinline)) void hello()
{
	end_hello = millis();
	printf("hello: %ldms\n", end_hello - start_hello);
	return;
}

int main()
{
	printf("Hello world!!\n");

	void *addr = &myfun;
	for (uint32_t i = 0; i < 5; ++i) {
		for (uint32_t j = 0; j < 4; ++j) {
			printf("%02x ", *(uint8_t *)(addr + (4 * i) + j));
		}
		printf("\n");
	}

	for (uint32_t i = 0; i < 10; ++i) {
		myfun();
		hello();
		sleep(5);
	}

	return 0;
}
