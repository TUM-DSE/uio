#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

sqlite3 *db;

void sqlite3_save()
{
	int rc;
	sqlite3 *pFile;
	sqlite3_backup *pBackup;

	if (!db) {
		printf("db is not initialized\n");
		return;
	}

	rc = sqlite3_open("dump", &pFile);
	if (rc) {
		printf("Failed to open db\n");
		return;
	}

	pBackup = sqlite3_backup_init(pFile, "main", db, "main");
	if (pBackup) {
		(void)sqlite3_backup_step(pBackup, -1);
		(void)sqlite3_backup_finish(pBackup);
	}
	rc = sqlite3_errcode(pFile);
	if (rc) {
		printf("SQLite3 save error: %d\n", rc);
	}

	(void)sqlite3_close(pFile);
}

int main()
{
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

	sqlite3_save();

	sqlite3_close(db);

	return 0;
}
