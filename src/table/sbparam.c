#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "sbparam.h"
#include "database.h"

extern sbparam_t *p_sbparam;

int get_sbparam_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	sbparam_info_t *p;

	sql = "select * from sbparam";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_sbparam = (sbparam_t *)malloc(row * sizeof(sbparam_info_t) + sizeof(int));
	if (!p_sbparam) {
		db_free_table(result);
		return -1;
	}

	memset(p_sbparam, 0, row * sizeof(sbparam_info_t) + sizeof(int));

	p_sbparam->num = row;
	p = p_sbparam->info;

	for (i = 0; i < row; i++) {
		strncpy(p->sbid,       *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(p->paramname,  *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 64);
		
		strncpy(p->devname,    *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 64);
		
		strncpy(buf,           *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 128);
		sscanf(buf, "%d", &p->devaddr);
		
		strncpy(p->funcode,    *(result + col + i * col + 5) ? *(result + col + i * col + 5) : "NULL", 64);
		
		strncpy(buf,           *(result + col + i * col + 6) ? *(result + col + i * col + 6) : "NULL", 128);
		sscanf(buf, "%d", &p->datatype);
		
		strncpy(buf,           *(result + col + i * col + 7) ? *(result + col + i * col + 7) : "NULL", 128);
		sscanf(buf, "%d", &p->regaddr);
		
		strncpy(buf,           *(result + col + i * col + 8) ? *(result + col + i * col + 8) : "NULL", 128);
		sscanf(buf, "%d", &p->reglen);
		
		strncpy(buf,           *(result + col + i * col + 9) ? *(result + col + i * col + 9) : "NULL", 128);
		sscanf(buf, "%d", &p->databit);
		
		strncpy(buf,           *(result + col + i * col + 10) ? *(result + col + i * col + 10) : "NULL", 128);
		sscanf(buf, "%f", &p->polvalue);
		
		strncpy(buf,           *(result + col + i * col + 11) ? *(result + col + i * col + 11) : "NULL", 128);
		sscanf(buf, "%f", &p->upvalue);
		
		strncpy(buf,           *(result + col + i * col + 12) ? *(result + col + i * col + 12) : "NULL", 128);
		sscanf(buf, "%f", &p->lowvalue);
		
		strncpy(buf,           *(result + col + i * col + 13) ? *(result + col + i * col + 13) : "NULL", 128);
		sscanf(buf, "%f", &p->K);
		
		strncpy(buf,           *(result + col + i * col + 14) ? *(result + col + i * col + 14) : "NULL", 128);
		sscanf(buf, "%f", &p->B);
		
		strncpy(buf,           *(result + col + i * col + 15) ? *(result + col + i * col + 15) : "NULL", 128);
		sscanf(buf, "%d", &p->alarmtype);

		p++;
	}

	db_free_table(result);

	return 0;
}

void put_sbparam_settings(void)
{
	if (p_sbparam) {
		free(p_sbparam);
		p_sbparam = NULL;
	}
}

