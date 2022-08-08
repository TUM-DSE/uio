#include <stdio.h>
#include <unistd.h>

#include <uk/console.h>

int main()
{
	char ch;

	while (1) {
		ch = uk_console_getc();
		printf("%c", ch);
		fflush(stdout);
		// uk_console_putc('a');
	}
	return 0;
}
