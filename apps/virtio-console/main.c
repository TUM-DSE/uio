#include <stdio.h>
#include <unistd.h>

#include <uk/console.h>

#define BUFSIZE 128

int main()
{
	int i = 0;
	char buf[BUFSIZE + 1];
	char ch = 0;

	while (1) {
		if (i == BUFSIZE || ch == '\n') {
			buf[i] = '\0';
			printf("%s", buf);
			uk_console_puts(buf, i);
			i = 0;
		}
		ch = uk_console_getc();
		buf[i++] = ch;
	}

	return 0;
}
