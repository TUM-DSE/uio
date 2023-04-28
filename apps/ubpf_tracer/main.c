#include <stdio.h>
#include <unistd.h>

__attribute__((noinline)) void function1()
{
	static int cnt = 0;
	printf("hello from function1: %d\n", cnt);
	cnt++;
	return;
}

__attribute__((noinline)) void function2()
{
	static int cnt = 0;
	printf("hello from function2: %d\n", cnt);
	cnt++;
	return;
}

__attribute__((noinline)) void function3()
{
	static int cnt = 0;
	printf("hello from function3: %d\n", cnt);
	cnt++;
	return;
}

__attribute__((noinline)) void function4()
{
	static int cnt = 0;
	printf("hello from function4: %d\n", cnt);
	cnt++;
	return;
}

int main()
{
	for (unsigned int i = 0; i < 10; ++i) {
		function1();
		function2();
		function3();
		function4();
		sleep(10);
	}

	return 0;
}
