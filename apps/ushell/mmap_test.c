#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>

int main()
{
	FILE *fp = fopen("fs0/f", "rt");
	assert(fp != NULL);
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	printf("size: %ld\n", size);

	void *code =
	    mmap(NULL, size, PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(code != MAP_FAILED);
	printf("mmap addr=%#lx\n", (long)code);

	assert(fread(code, size, 1, fp) != 0);
	fclose(fp);

	// run
	int (*func)() = code;
	int r = func();
	printf("%d\n", r);

	munmap(code, size);

	return 0;
}
