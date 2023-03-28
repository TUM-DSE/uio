#include <stdio.h>
#include <unistd.h>

#include <uk/alloc.h>
#include <uk/store.h>

void get_ukalloc_stat()
{
	struct uk_alloc_stats stat = {};
	struct uk_alloc *a = uk_alloc_get_default();
	uk_alloc_stats_get(a, &stat);

	printf("last_alloc_size: %ld\n", stat.last_alloc_size);
	printf("max_alloc_size: %ld\n", stat.max_alloc_size);
	printf("min_alloc_size: %ld\n", stat.min_alloc_size);

	printf("tot_nb_allocs: %ld\n", stat.tot_nb_allocs);
	printf("tot_nb_frees: %ld\n", stat.tot_nb_frees);
	printf("cur_nb_allocs: %ld\n", stat.cur_nb_allocs);
	printf("max_nb_allocs: %ld\n", stat.max_nb_allocs);

	printf("cur_mem_use: %ld\n", stat.cur_mem_use);
	printf("max_mem_use: %ld\n", stat.max_mem_use);

	printf("nb_enomem: %ld\n", stat.nb_enomem);
}

void get_stat_ukstore()
{
	__s64 val;
	static const struct uk_store_entry *entry;
	entry = uk_store_get_entry(libukalloc, NULL, "cur_mem_free");
	uk_store_get_value(entry, s64, &val);
	printf("cur_mem_free: %ld\n", val);
}

void get_stat()
{
	struct uk_alloc *a = uk_alloc_get_default();
	void *p = uk_do_malloc(a, 1024);
	get_ukalloc_stat();
	get_stat_ukstore();
	uk_free(a, p);
}

int count;

/* This function is exported (see exportsyms.uk), and therefore
 * ushell program can call this function (see fs0/set_count_func.c).
 * This function is also used in unikraft/lib/ushell/ushell.c
 * as an example of creating ushell's built-in command.
 */
__attribute__((section(".text.keep"))) void set_count(int c)
{
	count = c;
}

int main()
{
	count = 0;
	while (1) {
		printf("%d\n", count);
		count += 1;
		sleep(1);
		get_stat();
	}
	return 0;
}
