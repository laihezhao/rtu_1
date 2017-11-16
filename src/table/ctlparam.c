#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "ctlparam.h"
#include "database.h"

extern ctlparam_t *p_ctlparam;

int get_ctlparam_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	ctlparam_info_t *p;

	sql = "select * from ctlparam";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_ctlparam = (ctlparam_t *)malloc(row * sizeof(ctlparam_info_t) + sizeof(int));
	if (!p_ctlparam) {
		db_free_table(result);
		return -1;
	}

	memset(p_ctlparam, 0, row * sizeof(ctlparam_info_t) + sizeof(int));

	p_ctlparam->num = row;
	p = p_ctlparam->info;

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

		p++;
	}

	db_free_table(result);

	return 0;
}

void put_ctlparam_settings(void)
{
	if (p_ctlparam) {
		free(p_ctlparam);
		p_ctlparam = NULL;
	}
}

