#include <stdio.h>
#include <unistd.h>

int count;

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
