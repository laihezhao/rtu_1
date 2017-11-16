#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "sitesbs.h"
#include "database.h"

extern sitesbs_t *p_sitesbs;

int get_sitesbs_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	sitesbs_info_t *p;

	sql = "select * from sitesbs";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_sitesbs = (sitesbs_t *)malloc(row * sizeof(sitesbs_info_t) + sizeof(int));
	if (!p_sitesbs) {
		db_free_table(result);
		return -1;
	}

	memset(p_sitesbs, 0, row * sizeof(sitesbs_info_t) + sizeof(int));

	p_sitesbs->num = row;
	p = p_sitesbs->info;

	for (i = 0; i < row; i++) {
		strncpy(p->sbid,   *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(p->sbname, *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 64);
		
		strncpy(buf,       *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 128);
		sscanf(buf, "%d", &p->sbtype);
		
		strncpy(p->description, *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 64);
		
		p++;
	}

	db_free_table(result);

	return 0;
}

void put_sitesbs_settings(void)
{
	if (p_sitesbs) {
		free(p_sitesbs);
		p_sitesbs = NULL;
	}
}

