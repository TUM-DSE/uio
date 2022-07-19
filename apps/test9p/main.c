#include <stdio.h>
#include <stdlib.h>

#define FILEPATH "/a"

int main()
{
	FILE *fp;
	char buffer[128];

	fp = fopen(FILEPATH, "rt");
	if (fp == NULL) {
		fprintf(stderr, "Error opening file %s", FILEPATH);
	}
	fgets(buffer, 128, fp);
	printf("File contents: %s\n", buffer);
	fclose(fp);

	return 0;
}
