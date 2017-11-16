#ifndef __SITECONF_H_
#define __SITECONF_H_

typedef struct siteconf {
	char mn[64];
	char pw[64];
	int sitetype;
	int sampleinterval;
	int overtime;
	int recount;
	int dinums;
	int ainums;
	int comnums;
	char apn[64];
	char dialnumber[64];
	char dialuser[64];
	char dialpassword[64];
	int rtdinterval;
	int rsinterval;
} siteconf_t;

char* get_siteconf_mn();
char* get_siteconf_pw();

int get_siteconf_overtime();
int get_siteconf_recount();

char *get_siteconf_apn();
char *get_siteconf_dialnumber();
char *get_siteconf_dialuser();
char *get_siteconf_dialpassword();

int get_siteconf_rtdinterval();
int get_siteconf_rsinterval();

int  get_siteconf_settings();
void put_siteconf_settings();

#endif //__SITECONF_H_
