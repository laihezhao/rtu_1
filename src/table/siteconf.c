#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "error.h"
#include "siteconf.h"
#include "database.h"

extern siteconf_t *p_siteconf;

char* get_siteconf_mn(void)
{
	if (p_siteconf) {
		return p_siteconf->mn;
	}
	return NULL;
}

char* get_siteconf_pw(void)
{
	if (p_siteconf) {
		return p_siteconf->pw;
	}
	return NULL;
}

int get_siteconf_overtime(void)
{
	if (p_siteconf) {
		return p_siteconf->overtime;
	}
	return -1;
}

int get_siteconf_recount(void)
{
	if (p_siteconf) {
		return p_siteconf->recount;
	}
	return -1;
}

char *get_siteconf_apn(void)
{
	if (p_siteconf) {
		return p_siteconf->apn;
	}
	return NULL;
}

char *get_siteconf_dialnumber(void)
{
	if (p_siteconf) {
		return p_siteconf->dialnumber;
	}
	return NULL;
}

char *get_siteconf_dialuser(void)
{
	if (p_siteconf) {
		return p_siteconf->dialuser;
	}
	return NULL;
}

char *get_siteconf_dialpassword(void)
{
	if (p_siteconf) {
		return p_siteconf->dialpassword;
	}
	return NULL;
}

int get_siteconf_rtdinterval(void)
{
	if (p_siteconf) {
		return p_siteconf->rtdinterval;
	}
	return -1;
}

int get_siteconf_rsinterval(void)
{
	if (p_siteconf) {
		return p_siteconf->rsinterval;
	}
	return -1;
}

int get_siteconf_settings(void)
{
	int r;
	int row;
	int col;
	char *sql;
	char **result;
	char buf[128];
	siteconf_t *p;	

	sql = "select * from siteconfig";
	
	r = db_get_table(sql, &result, &row, &col);
	if (r == -1) {
		return -1;
	}

	p_siteconf = (siteconf_t *) malloc(sizeof(siteconf_t));
	if (!p_siteconf) {
		db_free_table(result);
		return -1;
	}

	memset(p_siteconf, 0, sizeof(siteconf_t));

	p = p_siteconf;

	strncpy(p->mn,          *(result + col + 0 * col + 2) ? *(result + col + 0 * col + 2) : "NULL", 64);
	
	strncpy(p->pw,          *(result + col + 1 * col + 2) ? *(result + col + 1 * col + 2) : "NULL", 64);
	
	strncpy(buf,            *(result + col + 2 * col + 2) ? *(result + col + 2 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->sitetype);
	
	strncpy(buf,            *(result + col + 3 * col + 2) ? *(result + col + 3 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->sampleinterval);
	
	strncpy(buf,            *(result + col + 4 * col + 2) ? *(result + col + 4 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->overtime);
	
	strncpy(buf,            *(result + col + 5 * col + 2) ? *(result + col + 5 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->recount);
	
	strncpy(buf,            *(result + col + 6 * col + 2) ? *(result + col + 6 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->dinums);
	
	strncpy(buf,            *(result + col + 7 * col + 2) ? *(result + col + 7 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->ainums);
	
	strncpy(buf,            *(result + col + 8 * col + 2) ? *(result + col + 8 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->comnums);
	
	strncpy(p->apn,         *(result + col + 9 * col + 2) ? *(result + col + 9 * col + 2) : "NULL", 64);
	
	strncpy(p->dialnumber,  *(result + col + 10 * col + 2) ? *(result + col + 10 * col + 2) : "NULL", 64);
	
	strncpy(p->dialuser,    *(result + col + 11 * col + 2) ? *(result + col + 11 * col + 2) : "NULL", 64);
	
	strncpy(p->dialpassword,*(result + col + 12 * col + 2) ? *(result + col + 12 * col + 2) : "NULL", 64);
	
	strncpy(buf,            *(result + col + 13 * col + 2) ? *(result + col + 13 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->rtdinterval);
	
	strncpy(buf,            *(result + col + 14 * col + 2) ? *(result + col + 14 * col + 2) : "NULL", 128);
	sscanf(buf, "%d", &p->rsinterval);
	
	db_free_table(result);

	return 0;
}

void put_siteconf_settings(void)
{
	if (p_siteconf) {
		free(p_siteconf);
		p_siteconf = NULL;
	}
}

