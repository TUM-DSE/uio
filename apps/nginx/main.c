#include <uk/config.h>

#ifdef CONFIG_LIBUKALLOC_IFSTATS
#include "../common/include/memstat.h"
#endif

extern int nginx_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#ifdef CONFIG_LIBUKALLOC_IFSTATS
	get_ukalloc_stat();
#endif
	return nginx_main(argc, argv);
}
