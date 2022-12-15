#include <stdio.h>
#include <unistd.h>

int count;

/* This function is exported (see exportsyms.uk), and therefore
 * ushell program can call this function (see fs0/set_count_func.c).
 * This function is also used in unikraft/lib/ushell/ushell.c
 * as an example of creating ushell's built-in command.
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
