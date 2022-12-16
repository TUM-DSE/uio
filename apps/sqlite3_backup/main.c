#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#define DB_INIT      "CREATE TABLE tab(INT, VARCHAR);"
#define DB_QUERY     "INSERT INTO tab VALUES (null, 'value');"
#define OP_NUM      600000

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

void sqlite3_generate_table() {
	int rc;
	char *errmsg;
	if (!db) {
		printf("db is not initialized\n");
		return;
	}
  rc = sqlite3_exec(db, DB_INIT, NULL, NULL, &errmsg);
  if (rc) {
    printf("Error (init): %s\n", errmsg);
    (void)sqlite3_free(errmsg);
    return;
  }
  for (int i = 0; i < OP_NUM; i++) {
    rc = sqlite3_exec(db, DB_QUERY, NULL, NULL, &errmsg);
    if (rc) {
      printf("Error (insert): %s\n", errmsg);
      (void)sqlite3_free(errmsg);
      break;
    }
  }
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
		char *errmsg;
		printf("> ");
		fflush(stdout);
		fgets(buf, 128, stdin);
		if (!strncmp(buf, "exit\n", 5) || !strncmp(buf, "quit\n", 5)) {
			break;
		}
    if (!strncmp(buf, "generate_table\n", 16)) {
      sqlite3_generate_table();
      continue;
    }
		printf("%s", buf);
		rc = sqlite3_exec(db, buf, NULL, NULL, &errmsg);
		if (rc) {
			printf("Error: %s\n", errmsg);
			sqlite3_free(errmsg);
		}
	}

	sqlite3_save();

	sqlite3_close(db);

	return 0;
}
