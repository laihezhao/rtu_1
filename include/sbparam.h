#ifndef __SBPARAM_H_
#define __SBPARAM_H_

typedef struct sbparam_info {
	char sbid[64];
	char paramname[64];
	char devname[64];
	int devaddr;
	char funcode[64];
	int datatype;
	int regaddr;
	int reglen;
	int databit;
	float polvalue;
	float upvalue;
	float lowvalue;
	float K;
	float B;
	int alarmtype;
} sbparam_info_t;

typedef struct sbparam {
	int num;
	sbparam_info_t info[0];
} sbparam_t;

int get_sbparam_settings();
void put_sbparam_settings();

#endif //__SBPARAM_H_
