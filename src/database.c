#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <pthread.h>
#include <unistd.h>

static sqlite3 *db;

int db_open(const char *name)
{
	int r;
	int retry = 0;

	if (name == NULL) {
		printf("database name is NULL\n");
		return -1;
	}

	while (1) {
		retry++;
		if (retry > 10) {
			printf("try open database 10 times failed\n");
			return -1;
		}

		r = sqlite3_open(name, &db);
		if (r) {
			sleep(1);
			continue;
		} else {
			return 0;
		}
	}

	return r;
}

int db_exec(const char *sql,
	    int (*callback) (void *, int, char **, char **), void *arg)
{
	int r;
	char *zErr;

	if (!sql) {
		return -1;
	}

	r = sqlite3_exec(db, sql, callback, arg, &zErr);
	if (r != SQLITE_OK) {
		if (zErr != NULL) {
			printf("sqlite3_exec: %s\n", zErr);
			sqlite3_free(zErr);
			return -1;
		}
	}
	
	return 0;
}

int db_get_table(const char *zSql, char ***pazResult, int *pnRow, int *pnColumn)
{
	int r;
	char *zErr;

	r = sqlite3_get_table(db, zSql, pazResult, pnRow, pnColumn, &zErr);
	if (r != SQLITE_OK) {
		printf("sqlite3_get_table: %s\n", zErr);
		sqlite3_free(zErr);
		sqlite3_free_table(*pazResult);
		return -1;
	}
	
	return 0;
}

void db_free_table(char **result)
{
	sqlite3_free_table(result);
}

void db_close(void)
{
	sqlite3_close(db);
}
