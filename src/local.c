#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

#include "log.h"
#include "error.h"

#define DI_DEVICE "/dev/DI"
#define DO_DEVICE "/dev/switch"
#define AI_DEVICE "/dev/AD2534"

#define HYZZDZ_GPIO_MAGIC  'k'

#define IOC_CMD_RELAY1	_IO(HYZZDZ_GPIO_MAGIC, 0)
#define IOC_CMD_RELAY2	_IO(HYZZDZ_GPIO_MAGIC, 1)
#define IOC_CMD_RELAY3	_IO(HYZZDZ_GPIO_MAGIC, 2)
#define IOC_CMD_RELAY4  _IO(HYZZDZ_GPIO_MAGIC, 3)
#define IOC_CMD_RELAY5	_IO(HYZZDZ_GPIO_MAGIC, 4)
#define IOC_CMD_RELAY6	_IO(HYZZDZ_GPIO_MAGIC, 5)
#define IOC_CMD_RELAY7	_IO(HYZZDZ_GPIO_MAGIC, 6)
#define IOC_CMD_RELAY8	_IO(HYZZDZ_GPIO_MAGIC, 7)

#define AD_MAGIC	'a'
#define AD_MAXNR	13
#define AD_READ_CHANNL	_IO(AD_MAGIC, 0)

static int difd;
static int dofd;
static int aifd;

int local_di(int addr)
{
	/* default return 1 */
	int r;
	unsigned char distatus = 0;

	r = read(difd, &distatus, sizeof(unsigned char));
	if (r < 0) {
		printf("read di devices failed!\n");
		return -1;
	}

	return ((distatus & (1 << (addr - 1))) ? 0 : 1);	
}

void local_do(int addr, int status)
{
	/* default status is always closed */
	int relay_status = status;
	
	switch(addr) {
		case 1:
			ioctl(dofd, IOC_CMD_RELAY1, &relay_status);
			break;
		case 2:
			ioctl(dofd, IOC_CMD_RELAY2, &relay_status);
			break;
		case 3:
			ioctl(dofd, IOC_CMD_RELAY3, &relay_status);
			break;
		case 4:
			ioctl(dofd, IOC_CMD_RELAY4, &relay_status);
			break;
		case 5:
			ioctl(dofd, IOC_CMD_RELAY5, &relay_status);
			break;
		case 6:
			ioctl(dofd, IOC_CMD_RELAY6, &relay_status);
			break;
		case 7:
			ioctl(dofd, IOC_CMD_RELAY7, &relay_status);
			break;
		case 8:
			ioctl(dofd, IOC_CMD_RELAY8, &relay_status);
			break;
		default :
			printf("input parameter error!\n");
			break;
	}
}

float local_ai(int addr)
{
	int nchannel;
	float fvalue;
	unsigned int unadvalue;
       	
	nchannel = addr;
	unadvalue = 0;
	fvalue = 0.0;

	if (nchannel < 1 || nchannel > 4) {
		printf("input parameter erroe!\n");
		return -9.99;
	}

	nchannel--;
	ioctl(aifd, AD_READ_CHANNL, &nchannel);
	read(aifd, &unadvalue, sizeof(unsigned int));
	fvalue = (float)(5 * unadvalue)/4096;

	return fvalue;
}

int open_di_device(void)
{
	difd = open(DI_DEVICE, O_RDWR);
	if (difd < 0) {
		printf("open /dev/DI device failed!\n");
		return -1;
	}

	printf("open /dev/DI device success!\n");
	
	return 0;
}

int open_do_device(void)
{
	dofd = open(DO_DEVICE, O_RDWR);
	if (dofd < 0) {
		printf("open /dev/switch device failed!\n");
		return -1;
	}

	printf("open /dev/switch device success!\n");
	
	return 0;
}

int open_ai_device(void)
{
	aifd = open(AI_DEVICE, O_RDWR);
	if (aifd < 0) {
		printf("open /dev/AD2543 device failed!\n");
		return -1;
	}

	printf("open /dev/AD2543 device success!\n");
	
	return 0;
}

void close_do_device(void)
{
	if (close(dofd) == -1)
		printf("close /dev/switch failed\n");
	else
		printf("close /dev/switch ok\n");
}

void close_di_device(void)
{
	if (close(difd) == -1)
		printf("close /dev/DI failed\n");
	else
		printf("close /dev/DI ok\n");
}

void close_ai_device(void)
{
	if (close(aifd) == -1)
		printf("close devAD2543 failed\n");
	else
		printf("close /dev/AD2543 ok\n");
}

