#ifndef _MEMSTAT_H
#define _MEMSTAT_H

#include <stdio.h>
#include <uk/alloc.h>

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

#endif /* _MEMSTAT_H */
