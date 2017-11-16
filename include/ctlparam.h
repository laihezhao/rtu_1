#ifndef __CTLPARAM_H_
#define __CTLPARAM_H_

typedef struct ctlparam_info {
	char sbid[64];
	char paramname[64];
	char devname[64];
	int devaddr;
	char funcode[64];
	int datatype;
	int regaddr;
	int reglen;
	int databit;
} ctlparam_info_t;

typedef struct ctlparam {
	int num;
	ctlparam_info_t info[0];
} ctlparam_t;

int get_ctlparam_settings();
void put_ctlparam_settings();

#endif //__CTLPARAM_H_
