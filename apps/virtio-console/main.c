#include <stdio.h>
#include <unistd.h>

#include <uk/console.h>

int main()
{
	char ch = uk_console_getc();
	printf("ch=%c\n", ch);
	uk_console_putc('a');

	printf("busy loop...\n");

	while (1) {
		sleep(1);
	}
	return 0;
}
