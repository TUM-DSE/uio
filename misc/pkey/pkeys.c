// based on an example on the man page

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>

#define errExit(msg)                                                           \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

int main(void)
{
	int status;
	int pkey;
	int *buffer;

	/*
	 * Allocate one page of memory.
	 */
	buffer = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
		      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (buffer == MAP_FAILED)
		errExit("mmap");

	/*
	 * Put some random data into the page (still OK to touch).
	 */
	*buffer = __LINE__;
	printf("buffer contains: %d\n", *buffer);

	/*
	 * Allocate a protection key
	 */
	pkey = pkey_alloc(0, 0);
	if (pkey == -1)
		errExit("pkey_alloc");

	/*
	 * Disable access to any memory with "pkey" set,
	 * even though there is none right now.
	 */
	status = pkey_set(pkey, PKEY_DISABLE_ACCESS);
	if (status)
		errExit("pkey_set");

	/*
	 * Set the protection key on "buffer".
	 * Note that it is still read/write as far as mprotect() is
	 * concerned and the previous pkey_set() overrides it.
	 */
	status =
	    pkey_mprotect(buffer, getpagesize(), PROT_READ | PROT_WRITE, pkey);
	if (status == -1)
		errExit("pkey_mprotect");

	printf("about to read buffer again...\n");

	/*
	 * This will crash, because we have disallowed access.
	 */
	printf("buffer contains: %d\n", *buffer);

	status = pkey_free(pkey);
	if (status == -1)
		errExit("pkey_free");

	exit(EXIT_SUCCESS);
}
