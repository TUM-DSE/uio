#include <stdio.h>
#include <unistd.h>

int main()
{
	printf("busy loop...\n");
	while (1) {
		sleep(1);
	}
	return 0;
}
