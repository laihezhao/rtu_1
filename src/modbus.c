#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <modbus/modbus.h>

#include "log.h"
#include "error.h"
#include "config.h"
#include "devices.h"

static int devicesnum;
extern modbus_t *ctx[MAXDEVS];
extern devices_t *p_devices;

void modbus_get_uint16_sw(const float src, uint16_t *dest)
{
	uint32_t d1; 
	uint32_t d2;

	memcpy(&d1, &src, sizeof(float));
	memcpy(&d2, &src, sizeof(float));

	dest[0] =((d1 >> 16) & 0x0000FFFF);
	dest[1] =(d2 & 0x0000FFFF);
}

float modbus_get_float_sw(const uint16_t *sr)
{
	float f = 0.0f;
	uint32_t i;

	i = (((uint32_t) sr[0]) << 16) + sr[1];
	memcpy(&f, &i, sizeof(float));

	return f;
}

/* high word and low word need to change */
void modbus_set_float_retry(float f, uint16_t *dest)
{
	modbus_set_float(f, dest);
}

/* high word and low word need to change */
float modbus_get_float_retry(const uint16_t *src)
{
	return modbus_get_float(src);
}

/* 0x03H */
int modbus_read_registers_retry(modbus_t* ctx, int addr, int nb, uint16_t *data)
{
	int index = 0;
	int r;

	if ((!ctx) || (!data))
		return -1;

	while ((r = modbus_read_registers(ctx, addr, nb, data)) == -1) {
		sleep(DELAY);
		index++;
		if (index > RETRY_TIMES) {
			printf("modbus_read_registers error: %s regaddr: %d\n", modbus_strerror(errno), addr);
			break;
		}
	}

#ifdef __DBG__
	if (index > RETRY_TIMES) {
		printf("too many retry, at %s\n", __FUNCTION__);
		return -1;
	}

	if (index > 1) {
		printf("number of regs read: %d,  retry: %d, at %s\n", r,
		       index, __FUNCTION__);
	}
#endif
	return r;
}

/* 0X0FH */
int modbus_write_registers_retry(modbus_t* ctx, int addr, int nb, const uint16_t *src)
{
	int index = 0;
	int r;

	if ((!ctx) || (!src))
		return -1;

	while ((r = modbus_write_registers(ctx, addr, nb, src)) == -1) {
		sleep(DELAY);
		index++;
		if (index > RETRY_TIMES) {
			printf("modbus_write_registers error: %s regaddr: %d\n", modbus_strerror(errno), addr);
			break;
		}
	}

#ifdef __DBG__
	if (index > RETRY_TIMES) {
		printf("too many retry, at %s\n", __FUNCTION__);
		return -1;
	}

	if (index > 1) {
		printf("number of regs read: %d,  retry: %d, at %s\n", r,
		       index, __FUNCTION__);
	}
#endif
	return r;
}

/* 0x02H */
int modbus_read_input_bits_retry(modbus_t* ctx, int addr, int nb, uint8_t *data)
{
	int index = 0;
	int r;

	if ((!ctx) || (!data))
		return -1;

	while ((r = modbus_read_input_bits(ctx, addr, nb, data)) == -1) {
		sleep(DELAY);
		index++;
		if (index > RETRY_TIMES) {
			printf("modbus_read_input_bits error: %s regaddr: %d\n", modbus_strerror(errno), addr);
			break;
		}
	}

#ifdef __DBG__
	if (index > RETRY_TIMES) {
		printf("too many retry, at %s\n", __FUNCTION__);
		return -1;
	}

	if (index > 1) {
		printf("number of regs read: %d,  retry: %d, at %s\n", r,
		       index, __FUNCTION__);
	}
#endif
	return r;
}

/* 0x01H */
int modbus_read_bits_retry(modbus_t* ctx, int addr, int nb, uint8_t *dest)
{
	int index = 0;
	int r;

	if ((!ctx) || (!dest))
		return -1;

	while ((r = modbus_read_bits(ctx, addr, nb, dest)) == -1) {
		sleep(DELAY);
		index++;
		if (index > RETRY_TIMES) {
			printf("modbus_read_bits error: %s regaddr: %d\n", modbus_strerror(errno), addr);
			break;
		}
	}

#ifdef __DBG__
	if (index > RETRY_TIMES) {
		printf("too many retry, at %s\n", __FUNCTION__);
		return -1;
	}

	if (index > 1) {
		printf("number of regs read: %d,  retry: %d, at %s\n", r,
		       index, __FUNCTION__);
	}
#endif
	return r;
}

/* 0X05H */
int modbus_write_bit_retry(modbus_t* ctx, int addr, int status)
{
	int index = 0;
	int r;

	if (!ctx)
		return -1;

	while ((r = modbus_write_bit(ctx, addr, status)) == -1) {
		sleep(DELAY);
		index++;
		if (index > RETRY_TIMES)
			printf("modbus_write_bit error: %s regaddr: %d\n", modbus_strerror(errno), addr);
			break;
	}

#ifdef __DBG__
	if (index > RETRY_TIMES) {
		printf("too many retry, at %s\n", __FUNCTION__);
		return -1;
	}

	if (index > 1) {
		printf("number of regs writen: %d,  retry: %d, at %s\n", r,
		       index, __FUNCTION__);
	}
#endif
	return r;
}

int get_modbus_settings(void)
{
	int r;
	int i;
	struct timeval response_timeout;

	devices_info_t *pdevices;
	if (p_devices) {
        	pdevices = p_devices->info;
		devicesnum = MIN_V(p_devices->num, MAXDEVS);
	}

	for (i = 0; i < devicesnum; i++, pdevices++) {
		if (pdevices->devtype != 1) {
#ifdef __DBG__
			printf("%s device devtype parameter error\n", pdevices->devname);
#endif
			continue;
		}
		
		if (pdevices->comtype != 2) {
#ifdef __DBG__
			printf("%s device comtype parameter error\n", pdevices->devname);
#endif
			continue;
		}
			
		/*485*/
		char device[32];

		memset(device, 0, sizeof(device));
		snprintf(device, 32, "/dev/ttyS%d", pdevices->port);

		ctx[i] = modbus_new_rtu(device, pdevices->baud, toupper(pdevices->parit[0]), pdevices->databits, pdevices->stopbits);
		if (ctx[i] == NULL) {
#ifdef __DBG__
			printf("Unable to creata the libmodbus < %s > context\n", pdevices->devname);
#endif
			continue;
		}

		response_timeout.tv_sec = 1;
		response_timeout.tv_usec = 5000;

		modbus_set_response_timeout(ctx[i], &response_timeout);
		
		modbus_get_response_timeout(ctx[i], &response_timeout);	
#ifdef __DBG__	
		printf("modbus response timeout :%ld sec %ld usec \n", response_timeout.tv_sec, response_timeout.tv_usec);
#endif
		r = modbus_set_slave(ctx[i], pdevices->devaddr);
		if (r == -1) {
#ifdef __DBG__
			printf("Invalid slave ID < %d >\n", pdevices->devaddr);
#endif
			modbus_free(ctx[i]);
			ctx[i] = NULL;
			continue;
		}

		modbus_set_debug(ctx[i], FALSE);

		if ((r = modbus_connect(ctx[i])) == -1) {
#ifdef __DBG__
			printf("< %s > Connection failed: %s\n", pdevices->devname, modbus_strerror(errno));
#endif
			modbus_free(ctx[i]);
			ctx[i] = NULL;
			continue;
		}
	}

	/* if connect all modbus devices fail, return -1 */
	for (i = 0; i < devicesnum; i++) {
		if (ctx[i] != NULL)
			return 0;
	}
#ifdef __DBG__
	printf("init modbus devices failed\n");
#endif
	return -1;
}

void put_modbus_settings(void)
{
	int i;

	for (i = 0; i < devicesnum; i++) {
		if (ctx[i] != NULL) {
			modbus_close(ctx[i]);
			modbus_free(ctx[i]);
		}
	}
}

