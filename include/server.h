#ifndef __SERVER_H_
#define __SERVER_H_

typedef struct server_info {
	char hostname[64];
	char ip[64];
	int port;
	int enable;
} server_info_t;

typedef struct server {
	int num;
	server_info_t info[0];
} server_t;

int get_server_settings();
void put_server_settings();

#endif //__SERVER_H_
