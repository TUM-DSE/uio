#include <stdio.h>
#include <unistd.h>

int count;

/* this function is used in unikraft/lib/ushell/ushell.c
 * as an example of built-in command
 */
void set_count(int c)
{
	count = c;
}

int main()
{
	count = 0;
	while(1) {
		printf("%d\n", count);
		count += 1;
		sleep(1);
	}
	return 0;
}
