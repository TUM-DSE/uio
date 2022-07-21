#include <stdio.h>

int main(int argc, char *argv[])
{
	printf("argc: %d\n", argc);
	printf("argv: ");
	int i = 0;
	for (i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	return 0;
}
