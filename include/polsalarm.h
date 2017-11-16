#ifndef __POLSALARM_H_
#define __POLSALARM_H_

typedef struct polsalarm_info {
	char polid[64];
	char devname[64];
	int devaddr;
	char funcode[64];
	int datatype;
	int regaddr;
	int reglen;
	int databit;
	int alarmtype;
	int flag;
} polsalarm_info_t;

typedef struct polsalarm {
	int num;
	polsalarm_info_t info[0];
} polsalarm_t;

int get_polsalarm_settings();
void put_polsalarm_settings();

#endif //__POLSALARM_H_
