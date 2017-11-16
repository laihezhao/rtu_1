#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "server.h"
#include "database.h"

extern server_t *p_server;

int get_server_settings(void)
{
	int i;
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	server_info_t *p;

	sql = "select * from server where enable='1';";

	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_server = (server_t *)malloc(row * sizeof(server_info_t) + sizeof(int));
	if (!p_server) {
		db_free_table(result);
		return -1;
	}

	memset(p_server, 0, row * sizeof(server_info_t) + sizeof(int));

	p_server->num = row;
	p = p_server->info;

	for (i = 0; i < row; i++) {
		strncpy(p->hostname,   *(result + col + i * col + 1) ? *(result + col + i * col + 1) : "NULL", 64);
		
		strncpy(p->ip,         *(result + col + i * col + 2) ? *(result + col + i * col + 2) : "NULL", 64);
		
		strncpy(buf,           *(result + col + i * col + 3) ? *(result + col + i * col + 3) : "NULL", 128);
		sscanf(buf, "%d", &p->port);
		
		strncpy(buf,           *(result + col + i * col + 4) ? *(result + col + i * col + 4) : "NULL", 128);
		sscanf(buf, "%d", &p->enable);
	
		p++;
	}

	db_free_table(result);

	return 0;
}

void put_server_settings(void)
{
	if (p_server) {
		free(p_server);
		p_server = NULL;
	}
}

