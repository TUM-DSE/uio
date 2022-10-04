#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

int main()
{
	sqlite3 *db;
	int rc;

	rc = sqlite3_open(":memory:", &db);
	if (rc) {
		printf("Failed to open db\n");
		return 1;
	}

	while (1) {
		char buf[128];
		printf("> ");
		fflush(stdout);
		fgets(buf, 128, stdin);
		if (!strncmp(buf, "exit\n", 5) || !strncmp(buf, "quit\n", 5)) {
			break;
		}
		printf("%s", buf);
	}

	sqlite3_close(db);

	return 0;
}
