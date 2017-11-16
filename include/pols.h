#ifndef __POLS_H_
#define __POLS_H_

typedef struct pols_info {
	char polid[64];
	char polname[64];
	float upvalue;
	float lowvalue;
	char unit[64];
	int isstat;
	int poltype;
} pols_info_t;

typedef struct pols {
	int num;
	pols_info_t info[0];
} pols_t;

int get_pols_settings();
void put_pols_settings();

#endif //__POLS_H_

