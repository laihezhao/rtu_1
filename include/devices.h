#ifndef __DEVICES_H_
#define __DEVICES_H_

typedef struct devices_info {
	char devname[64];
	int devtype;
	int devaddr;
	int comtype;
	int port;
	int baud;
	int databits;
	int stopbits;
	char parit[4];
} devices_info_t;

typedef struct devices {
	int num;
	devices_info_t info[0];
} devices_t;

int get_devices_settings();
void put_devices_settings();

#endif //__DEVICES_H_
