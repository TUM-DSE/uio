#include <uk/config.h>

#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#include <uk/plat/bootstrap.h>

#ifdef CONFIG_LIBUKALLOC_IFSTATS
#include "../common/include/memstat.h"
#endif

#ifndef DB_NAME
#define DB_NAME     "database.db"
#endif

#ifndef DB_QUERY
#define DB_QUERY     "INSERT INTO tab VALUES (null, 'value')"
#endif

#ifndef OP_NUM
#define OP_NUM      60000
#endif

#ifndef VERIFY_Q
#define VERIFY_Q    0
#endif

sqlite3 *db;

// this is the same workaround as in ../sqlite3_backup
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

static int callback(
        void *unused __attribute__((unused)),
        int argc, char **argv, char **azColName) {
#ifdef VERIFY_Q
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
#endif
    return 0;
}

int main(int argc, char **argv){
#ifdef CONFIG_LIBUKALLOC_IFSTATS
	get_ukalloc_stat();
	while(1);
#endif

  char *zErrMsg = 0;
  int rc;

    rc = sqlite3_open(DB_NAME, &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(1);
    }

    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start)) {
        perror("Could not read start time!");
        sqlite3_close(db);
        return 1;
    }

    for (int i = 0; i < OP_NUM; i++) {
        rc = sqlite3_exec(db, DB_QUERY, callback, 0, &zErrMsg);

        if( rc!=SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            break;
        }
    }

    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end)) {
        perror("Could not read end time!");
        sqlite3_close(db);
        return 1;
    }

    time_t n_sec = end.tv_sec - start.tv_sec;
    long n_nsec = end.tv_nsec - start.tv_nsec;
    if (n_nsec < 0) {
        --n_sec;
        n_nsec += 1000000000L;
    }
    printf("\n%ld.%09ld\n", n_sec, n_nsec);

#if VERIFY_Q
    puts("========== Read back ===============");
    rc = sqlite3_exec(db, "SELECT * FROM tab", callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
#endif

    sqlite3_close(db);

    // Ensure output is printed and VM really quits
    fflush(stdout);
    ukplat_halt();
    return 0;
}
