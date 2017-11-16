#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "polsalarm.h"
#include "database.h"

extern polsalarm_t *p_polsalarm;

int get_polsalarm_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	polsalarm_info_t *p;

	sql = "select * from polsalarm";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_polsalarm = (polsalarm_t *)malloc(row * sizeof(polsalarm_info_t) + sizeof(int));
	if (!p_polsalarm) {
		db_free_table(result);
		return -1;
	}

	memset(p_polsalarm, 0, row * sizeof(polsalarm_info_t) + sizeof(int));

	p_polsalarm->num = row;
	p = p_polsalarm->info;

	for (i = 0; i < row; i++) {
		strncpy(p->polid,      *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(p->devname,    *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 64);
		
		strncpy(buf,           *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 128);
		sscanf(buf, "%d", &p->devaddr);
		
		strncpy(p->funcode,    *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 64);
		
		strncpy(buf,           *(result + col + i * col + 5) ? *(result + col + i * col + 5) : "NULL", 128);
		sscanf(buf, "%d", &p->datatype);

		strncpy(buf,           *(result + col + i * col + 6) ? *(result + col + i * col + 6) : "NULL", 128);
		sscanf(buf, "%d", &p->regaddr);
		
		strncpy(buf,           *(result + col + i * col + 7) ? *(result + col + i * col + 7) : "NULL", 128);
		sscanf(buf, "%d", &p->reglen);
		
		strncpy(buf,           *(result + col + i * col + 8) ? *(result + col + i * col + 8) : "NULL", 128);
		sscanf(buf, "%d", &p->databit);
		
		strncpy(buf,           *(result + col + i * col + 9) ? *(result + col + i * col + 9) : "NULL", 128);
		sscanf(buf, "%d", &p->alarmtype);
		
		strncpy(buf,           *(result + col + i * col + 10) ? *(result + col + i * col + 10) : "NULL", 128);
		sscanf(buf, "%d", &p->flag);

		p++;
	}

	db_free_table(result);

	return 0;
}

void put_polsalarm_settings(void)
{
	if (p_polsalarm) {
		free(p_polsalarm);
		p_polsalarm = NULL;
	}
}

