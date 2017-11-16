#ifndef __SITESBS_H_
#define __SITESBS_H_

typedef struct sitesbs_info {
	char sbid[64];
	char sbname[64];
	int sbtype;
	char description[64];
} sitesbs_info_t;

typedef struct sitesbs {
	int num;
	sitesbs_info_t info[0];
} sitesbs_t;

int get_sitesbs_settings();
void put_sitesbs_settings();

#endif //__SITESBS_H_
