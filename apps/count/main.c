#include <uk/config.h>

#include <stdio.h>
#include <unistd.h>

#ifdef CONFIG_LIBUKALLOC_IFSTATS
#include "../common/include/memstat.h"
#endif

int count;

#ifdef CONFIG_LIBUSHELL
/* This function is exported (see exportsyms.uk), and therefore
 * ushell program can call this function (see fs0/set_count_func.c).
 * This function is also used in unikraft/lib/ushell/ushell.c
 * as an example of creating ushell's built-in command.
 */
__attribute__ ((section (".text.keep")))
void set_count(int c)
{
	count = c;
}
#endif

int main()
{
#ifdef CONFIG_LIBUKALLOC_IFSTATS
	get_ukalloc_stat();
	while(1);
#endif
	count = 0;
	while(1) {
		printf("%d\n", count);
		count += 1;
		sleep(1);
	}
	return 0;
}
