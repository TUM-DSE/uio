/*
 * Comment out for linux build
 * Comment for Unikraft build
 * */
//#define LNX 1

#ifdef LNX
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sys/mman.h>
#include <errno.h>
#else
#include <uk/pku.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#include <uk/alloc.h>
#include <uk/sched.h>
#include <uk/thread.h>
#endif

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

int *mem = NULL;

__attribute__((always_inline)) static inline void wrpkru(uint32_t val)
{
	asm volatile (  "mov %0, %%eax;"
			"xor %%ecx, %%ecx;"
			"xor %%edx, %%edx;"
			"wrpkru;"
			"lfence"
			:: "r"(val) : "eax", "ecx", "edx");
}

#ifndef LNX
void thread_func(void *arg)
{
	struct uk_thread *par = (struct uk_thread *) arg;

	printf("Hello from thread\n");
	uk_sched_yield();
	printf("thread mem = %d\n", *mem);
	printf("I am gonna change value\n");
	*mem = 7;
	printf("thread mem = %d\n", *mem);

	uk_thread_wake(par);
	return;
}
#endif

int main()
{
	int key0, key1, key2, key3, key4;
	int rc;
	struct uk_thread *thr = NULL;

	key0 = pkey_alloc(0, 0);
	if (key0 < 0) {
		printf("key0 returned %d", errno);
		return -1;
	}
	printf("key0 = %d\n", key0);

	key1 = pkey_alloc(0, 0);
	if (key1 < 0) {
		printf("key1 returned %d", errno);
		return -1;
	}
	printf("key1 = %d\n", key1);
	key2 = pkey_alloc(0, 0);
	if (key2 < 0) {
		printf("key2 returned %d", errno);
		return -1;
	}
	printf("key2 = %d\n", key2);
#if 0
	if (pkey_free(key0) < 0) 
		printf("key0 returned %d", errno);
#endif
	key3 = pkey_alloc(0, 0);
	if (key3 < 0) {
		printf("key3 returned %d", errno);
		return -1;
	}
	printf("key3 = %d\n", key3);

	key4 = pkey_alloc(0, 0);
	if (key4 < 0) {
		printf("key4 returned %d", errno);
		return -1;
	}
	printf("key4 = %d\n", key4);

#ifdef LNX
	mem = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (mem == MAP_FAILED) {
		printf("mmap failed\n");
		return -1;
	}
#else
	mem = uk_memalign(uk_alloc_get_default(), __PAGE_SIZE, PAGE_SIZE);
	if (!mem) {
		printf("could not get page\n");	
	}
#endif
	*mem = __LINE__;
	printf("mem = %d\n", *mem);
	rc = pkey_mprotect(mem, getpagesize(), PROT_READ, key4);
	if (rc < 0)
		printf("error in mprotect %d\n", rc);
#ifndef LNX
	thr = uk_sched_thread_create(uk_sched_get_default(), "test_pku",
					NULL, thread_func, uk_thread_current());
	printf("mem = %d\n", *mem);
	uk_sched_yield();
	printf("mem = %d\n", *mem);
	uk_sched_yield();
#endif
	*mem = __LINE__;
	printf("mem = %d\n", *mem);
	if (pkey_free(key1) < 0) 
		printf("key1 returned %d", errno);
	if (pkey_free(key2) < 0) 
		printf("key2 returned %d", errno);
	if (pkey_free(key3) < 0) 
		printf("key3 returned %d", errno);
	if (pkey_free(key4) < 0) 
		printf("key4 returned %d", errno);

	return 0;
}
