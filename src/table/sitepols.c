#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "sitepols.h"
#include "database.h"

extern sitepols_t *p_sitepols;

int get_sitepols_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	sitepols_info_t *p;

	sql = "select * from sitepols";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_sitepols = (sitepols_t *)malloc(row * sizeof(sitepols_info_t) + sizeof(int));
	if (!p_sitepols) {
		db_free_table(result);
		return -1;
	}

	memset(p_sitepols, 0, row * sizeof(sitepols_info_t) + sizeof(int));

	p_sitepols->num = row;
	p = p_sitepols->info;
	
	for (i = 0; i < row; i++) {
		strncpy(p->polid,   *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(p->polname, *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 64);
		
		strncpy(p->devname, *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 64);

		strncpy(buf,        *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 128);
		sscanf(buf, "%d", &p->devaddr);
		
		strncpy(p->funcode, *(result + col + i * col + 5) ? *(result + col + i * col + 5) : "NULL", 64);
		
		strncpy(buf,        *(result + col + i * col + 6) ? *(result + col + i * col + 6) : "NULL", 128);
		sscanf(buf, "%d", &p->datatype);
		
		strncpy(buf,        *(result + col + i * col + 7) ? *(result + col + i * col + 7) : "NULL", 128);
		sscanf(buf, "%d", &p->regaddr);
		
		strncpy(buf,        *(result + col + i * col + 8) ? *(result + col + i * col + 8) : "NULL", 128);
		sscanf(buf, "%d", &p->reglen);
		
		strncpy(buf,        *(result + col + i * col + 9) ? *(result + col + i * col + 9) : "NULL", 128);
		sscanf(buf, "%d", &p->databit);
		
		strncpy(buf,        *(result + col + i * col + 10) ? *(result + col + i * col + 10) : "NULL", 128);
		sscanf(buf, "%f", &p->polvalue);
		
		strncpy(buf,        *(result + col + i * col + 11) ? *(result + col + i * col + 11) : "NULL", 128);
		sscanf(buf, "%f", &p->upvalue);
		
		strncpy(buf,        *(result + col + i * col + 12) ? *(result + col + i * col + 12) : "NULL", 128);
		sscanf(buf, "%f", &p->lowvalue);
		
		strncpy(buf,        *(result + col + i * col + 13) ? *(result + col + i * col + 13) : "NULL", 128);
		sscanf(buf, "%f", &p->K);
		
		strncpy(buf,        *(result + col + i * col + 14) ? *(result + col + i * col + 14) : "NULL", 128);
		sscanf(buf, "%f", &p->B);
		
		strncpy(buf,        *(result + col + i * col + 15) ? *(result + col + i * col + 15) : "NULL", 128);
		sscanf(buf, "%d", &p->alarmtype);	
		
		strncpy(buf,        *(result + col + i * col + 16) ? *(result + col + i * col + 16) : "NULL", 128);
		sscanf(buf, "%d", &p->flag);

		p++;
	}

	db_free_table(result);

	return 0;
}

void put_sitepols_settings(void)
{
	if (p_sitepols) {
		free(p_sitepols);
		p_sitepols = NULL;
	}
}
