#ifndef __DATABASE_H__
#define __DATABASE_H__

int db_open(const char *name);
int db_exec(const char *sql,
	    int (*callback) (void *, int, char **, char **), void *arg);
int db_get_table(const char *zSql,
		 char ***pazResult, int *pnRow, int *pnColumn);
void db_free_table(char **result);
void db_close();

#endif //__DATABASE_H_
