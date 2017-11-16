#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "pols.h"
#include "database.h"

extern pols_t *p_pols;

int get_pols_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	pols_info_t *p;

	sql = "select * from pols where isstat='1';";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_pols = (pols_t *)malloc(row * sizeof(pols_info_t) + sizeof(int));
	if (!p_pols) {
		db_free_table(result);
		return -1;
	}
	
	memset(p_pols, 0, row * sizeof(pols_info_t) + sizeof(int));

	p_pols->num = row;
	p = p_pols->info;

	for (i = 0; i < row; i++) {
		strncpy(p->polid,   *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(p->polname, *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 64);
		
		strncpy(buf,        *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 128);
		sscanf(buf, "%f", &p->upvalue);
		
		strncpy(buf,        *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 128);
		sscanf(buf, "%f", &p->lowvalue);
		
		strncpy(p->unit,    *(result + col + i * col + 5) ? *(result + col + i * col + 5) : "NULL", 64);
		
		strncpy(buf,        *(result + col + i * col + 6) ? *(result + col + i * col + 6) : "NULL", 128);
		sscanf(buf, "%d", &p->isstat);
		
		strncpy(buf,        *(result + col + i * col + 7) ? *(result + col + i * col + 7) : "NULL", 128);
		sscanf(buf, "%d", &p->poltype);

		p++;
	}
	
	db_free_table(result);

	return 0;
}

void put_pols_settings(void)
{
	if (p_pols) {
		free(p_pols);
		p_pols = NULL;
	}
}

