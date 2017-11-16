#ifndef __SITEPOLS_H_
#define __SITEPOLS_H_

typedef struct sitepols_info {
	char polid[64];
	char polname[64];
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
	int flag;
} sitepols_info_t;

typedef struct sitepols {
	int num;
	sitepols_info_t info[0];
} sitepols_t;

int get_sitepols_settings();
void put_sitepols_settings();

#endif //__SITEPOLS_H_
