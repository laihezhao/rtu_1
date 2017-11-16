#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "devices.h"
#include "database.h"

extern devices_t *p_devices;

int get_devices_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	devices_info_t *p;

	sql = "select * from devices";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_devices = (devices_t *) malloc(row * sizeof(devices_info_t) + sizeof(int));
	if (!p_devices) {
		db_free_table(result);
		return -1;
	}

	memset(p_devices, 0, row * sizeof(devices_info_t) + sizeof(int));

	p_devices->num = row;
	p = p_devices->info;

	for (i = 0; i < row; i++) {
		strncpy(p->devname, *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(buf,        *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 128);
		sscanf(buf, "%d", &p->devtype);
		
		strncpy(buf,        *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 128);
		sscanf(buf, "%d", &p->devaddr);
		
		strncpy(buf,        *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 128);
		sscanf(buf, "%d", &p->comtype);
		
		strncpy(buf,        *(result + col + i * col + 5) ? *(result + col + i * col + 5) : "NULL", 128);
		sscanf(buf, "%d", &p->port);
		
		strncpy(buf,        *(result + col + i * col + 6) ? *(result + col + i * col + 6) : "NULL", 128);
		sscanf(buf, "%d", &p->baud);
		
		strncpy(buf,        *(result + col + i * col + 7) ? *(result + col + i * col + 7) : "NULL", 128);
		sscanf(buf, "%d", &p->databits);
	
		strncpy(buf,        *(result + col + i * col + 8) ? *(result + col + i * col + 8) : "NULL", 128);
		sscanf(buf, "%d", &p->stopbits);
#if 0		
		strncpy(buf,        *(result + col + i * col + 9) ? *(result + col + i * col + 9) : "NULL", 128);
		sscanf(buf, "%c", &p->parit);
#endif
		strncpy(p->parit, *(result + col + i * col + 9) ? *(result + col + i * col + 9) : "NULL", 64);
		
		p++;
	}
	
	db_free_table(result);

	return 0;
}

void put_devices_settings(void)
{
	if (p_devices) {
		free(p_devices);
		p_devices = NULL;
	}
}
