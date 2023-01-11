#include <stdio.h>
#include <unistd.h>

int main()
{
	int count = 0;
	while(1) {
		printf("%d\n", count);
		count += 1;
		sleep(1);
	}
	return 0;
}
