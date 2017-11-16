#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>

#include "log.h"
#include "error.h"
#include "siteconf.h"

typedef int (*encap_packet_routine) (char *, const char *);
static int encap_packet(char *buf, const char *data);
static encap_packet_routine f = encap_packet;

int get_sys_time(char *datatime, char *temptime)
{
	int n;
	int r;
	struct tm *ptm;
	struct timeval tv;
	struct timezone tz;

	r = gettimeofday(&tv, &tz);
	if (r != 0) {
		printf("error: gettimeofday: %s\n", strerror(errno));
		return -1;
	}

	ptm = localtime(&(tv.tv_sec));
	if (ptm == NULL) {
		printf("error: localtime: %s\n", strerror(errno));	
		return -1;
	}

	if (datatime != NULL) {
		n = snprintf(datatime, 32,
				"%04d%02d%02d%02d%02d%02d%03d",
				ptm->tm_year + 1900,
				ptm->tm_mon + 1,
				ptm->tm_mday,
				ptm->tm_hour,
				ptm->tm_min,
				ptm->tm_sec,
				(int)tv.tv_usec/1000);
		datatime[n] = '\0';
	}
	
	if (temptime != NULL) {
		n = snprintf(temptime, 32,
				"%04d-%02d-%02d %02d:%02d:%02d:%03d",
				ptm->tm_year + 1900,
				ptm->tm_mon + 1,
				ptm->tm_mday,
				ptm->tm_hour,
				ptm->tm_min,
				ptm->tm_sec,
				(int)tv.tv_usec/1000);
		temptime[n] = '\0';
	}

	return 0;
}

void reverse_time(char *dest, char *src)
{
	char *temp;

	if ((!dest) || (!src))
		return ;

	temp = src;
	memset(dest, 0, 32);
	strncpy(dest, temp, 4);
	strncat(dest,"-", 1);
	strncat(dest, temp + 4, 2);
	strncat(dest,"-", 1);
	strncat(dest, temp + 6, 2);
	strncat(dest, " ", 1);
	strncat(dest, temp + 8, 2);
	strncat(dest, ":", 1);
	strncat(dest, temp + 10, 2);
	strncat(dest, ":", 1);
	strncat(dest, temp + 12, 2);
	strncat(dest, ":", 1);
	strncat(dest, temp + 14, 3);
	strncat(dest, "\0", 1);
}

void parse_time(char *buf, const char *src)
{
	int j;
	int i;
	int len;
       	
	if ((!buf) || (!src))
		return ;

	len = strlen(src);

	for (i = 0, j = 0; i < len; i++) {
		if(src[i] == ' ' || src[i] == ':' || src[i] == '-')
			continue;
		buf[j] = src[i];
		j++;
	}

	buf[j] = '\0';
}

static uint32_t crc16(uint32_t reg16, uint8_t *puchMsg, uint32_t usDataLen)
{
	uint8_t regHi, regLow;
	uint8_t charCheck, charOut;

	int i, j;

	for (i = 0; i < usDataLen; i++) {
		regHi = (reg16 >> 8) & 0x00FF;
		regLow = reg16 & 0x00FF;
		charCheck = puchMsg[i];
		reg16 = regHi ^ charCheck;
		for (j = 0; j < 8; j++) {
			charOut = reg16 & 0x0001;
			reg16 = reg16 >> 1;
			if (0x0001 == charOut) {
				reg16 = reg16 ^ 0xA001;
			}
		}
	}

	return reg16;
}

/* note: buf must be at least one packet length, otherwise buffer overruns. */
static int encap_packet(char *buf, const char *data)
{
	char *p;
	int n;
	uint16_t crc;
	char t[64];

	if ((!p) || (!data))
		return -1;

	p = buf;

	/* header */
	*p = '#';
	*(p + 1) = '#';

	/* set data */
	n = snprintf(p + 2 + 4, 1024, "%s", data);

	/* data seg length */
	snprintf(t, 64, "%04d", n);
	memcpy(p + 2, t, 4);

	/* crc */
	crc = crc16(0xffff, (uint8_t *)(p + 2 + 4), n);
	snprintf(t, 64, "%04x", crc);
	memcpy(p + 2 + 4 + n, t, 4);

	/*tail */
	*(char *)(p + 2 + 4 + n + 4) = '\r';
	*(char *)(p + 2 + 4 + n + 4 + 1) = '\n';

#ifdef __DBG__
	{
		int i;
		int l = (2 + 4 + n + 4 + 1 + 1);

		char *ptr = p;
		printf("<< ----- send packet info ----- >>\n");
		for (i = 0; i < l; i++) {
			printf("%c", *(ptr + i));
		}
		printf("\n");
		fflush(stdout);
	}
#endif

	return 2 + 4 + n + 4 + 1 + 1;
}

int verify_packet(const char *buf)
{
	const char *ptr;
	uint32_t len;
	uint16_t hcrc;
	uint16_t ncrc;
	uint32_t t;

	if (!buf)
		return -1;

	ptr = buf;

	/* 判断是不是完整的报文 */
	if ((*ptr != '#') || (*(ptr + 1) != '#')) {
		printf("Error at %s: %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* 解析数据段大小 */
	sscanf((ptr + 2), "%04d", &t);
	len = t;
	if (len > 1024) {
		printf("!!! wrong segment, data length: %d bytes\n", len);
		return -1;
	}

	if ((*(char *)(ptr + 2 + 4 + len + 4) != '\r')
	    || (*(char *)(ptr + 2 + 4 + len + 4 + 1) != '\n')) {
		printf("Error at %s: %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	hcrc = crc16(0xffff, (uint8_t *) (ptr + 2 + 4), len);
	sscanf((ptr + 2 + 4 + len), "%04x", &t);
	ncrc = t;

	if (ncrc != hcrc) {
		printf("CRC Error at %s: %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

#ifdef __DBG__
	{
		int i;
		int l = (2 + 4 + len + 4 + 1 + 1);

		printf("<< ----- received packet info ----- >>\n");
		for (i = 0; i < l; i++) {
			printf("%c", *(ptr + i));
		}
		printf("\n");
		fflush(stdout);
	}
#endif

	return 0;
}

int create_pkt_login(char *buf, const char *data)
{
	int n = -1;
	char *st = "21";
	char *cn = "1010";
	char dest[256] = {0};
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	
	if ((!buf) || (!data))
		return -1;
	
	if (f) {
		n = snprintf(dest, 256,
		     "ST=%2s;CN=%4s;PW=%6s;MN=%14s;QN=%17s;Flag=1;CP=&&&&", st, cn, pw, mn, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_heartbeat(char *buf, const char *data)
{	
	int n = -1;
	char *st = "21";
	char *cn = "2020";
	char dest[256] = {0};
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	
	if ((!buf) || (!data))
		return -1;
	
	if (f) {
		n = snprintf(dest, 256,
		     "ST=%2s;CN=%4s;PW=%6s;MN=%14s;QN=%17s;Flag=1;CP=&&&&", st, cn, pw, mn, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;

}

int create_pkt_sampling_freq(char *buf, const char *data)
{
	int n = -1;
	char dest[256];
	char *st = "21";
	char *cn = "1081";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	int flag = 0;

	if ((!buf) || (!data))
		return -1;
	
	if (f) {
		n = snprintf(dest, 512,
			 "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=%1d;CP=&&QN=%17s;POLID=SB00;CTime=04,CTime=10,CTime=17&&",
			 st, cn, pw, mn, flag, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_gettime(char *buf, const char *data)
{
	int n = -1;
	char dest[256];
	char *st = "21";
	char *cn = "1011";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	int flag = 0;

	if ((!buf) || (!data))
		return -1;
	
	if (f) {
		n = snprintf(dest, 256,
			 "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=%1d;CP=&&%49s&&",
			 st, cn, pw, mn, flag, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_exec(char *buf, const char *data)
{
	int n = -1;
	char dest[256];
	char *st = "91";
	char *cn = "9012";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	int flag = 0;

	if ((!buf) || (!data))
		return -1;
	
	if (f) {
		n = snprintf(dest, 256,
			 "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=%1d;CP=&&QN=%17s;ExeRtn=1&&",
			 st, cn, pw, mn, flag, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_response(char *buf, const char *data)
{
	int n = -1;
	char dest[256];
	char *st = "91";
	char *cn = "9011";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	int flag = 0;

	if ((!buf) || (!data))
		return -1;

	if (f) {
		n = snprintf(dest, 256,
			 "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=%1d;CP=&&QN=%17s;QnRtn=1&&",
			 st, cn, pw, mn, flag, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_getlevel(char *buf, const char *data)
{
	int n = -1;
	char dest[1024];
	char *st = "21";
	char *cn = "1021";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	int flag = 0;
	
	if ((!buf) || (!data))
		return -1;
	
	if (f) {
		n = snprintf(dest, 1024,
			 "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=%1d;CP=&&%s&&",
			 st, cn, pw, mn, flag, data);
	
		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_realtime_data(char *buf, const char *poldata)
{
	int n = -1;
	char dest[512] = {0};
	char *st = "21";
	char *cn = "2011";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();

	if ((!buf) || (!poldata))
		return -1;

	if (f) {
		n = snprintf(dest, sizeof(dest),
			 "ST=%2s;CN=%4s;PW=%6s;"
			 "MN=%14s;Flag=0;CP=&&%s&&",
			 st, cn, pw, mn, poldata);

		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_sbs_change(char *buf, const char *data)
{
	int n = -1;
	char *st = "21";
	char *cn = "2081";
	char dest[1024];
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();

	if ((!buf) || (!data))
		return -1;

	if (f) {
		n = snprintf(dest, 1024, "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=0;CP=&&%s&&",
				st, cn, pw, mn, data);
	
		dest[n] = '\0';
		n = (*f) (buf, dest);
	}
	
	return n;
}

int create_pkt_sbs_status(char *buf, const char *poldata)
{
	int n = -1;
	char *st = "21";
	char *cn = "2021";
	char dest[1024]  = {0};
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();

	if ((!buf) || (!poldata))
		return -1;

	if (f) {
		n = snprintf(dest, 1024, "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=0;CP=&&%s&&",
				st, cn, pw, mn, poldata);
	
		dest[n] = '\0';
		n = (*f) (buf, dest);
	}
	
	return n;
}

int create_pkt_alarm_data(char *buf, const char *poldata)
{
	int n = -1;
	char dest[1024];
	char *st = "21";
	char *cn = "2072";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();

	if ((!buf) || (!poldata))
		return -1;

	memset(dest, 0, 1024);

	if (f) {
		n = snprintf(dest, 1024,"ST=%2s;CN=%4s;PW=%6s;MN=%14s;CP=&&%s&&",
			 st, cn, pw, mn, poldata);

		dest[n] = '\0';		
		n = (*f) (buf, dest);
	}

	return n;
}

int create_pkt_minute_data(char *buf, const char *poldata)
{
	int n = -1;
	char dest[1024];
	char *st = "21";
	char *cn = "2051";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();
	
	if ((!buf) || (!poldata))
		return -1;

	memset(dest, 0, sizeof(dest));

	if (f) {
		n = snprintf(dest, 1024,
			 "ST=%2s;CN=%4s;PW=%6s;"
			 "MN=%14s;Flag=1;CP=&&%s&&",
			 st, cn, pw, mn, poldata);

		dest[n] = '\0';		
		n = (*f) (buf, dest);
	}

	return n;
}

int create_pkt_hourly_data(char *buf, const char *poldata)
{
	int n = -1;
	char dest[1024];
	char *st = "21";
	char *cn = "2061";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();

	if ((!buf) || (!poldata))
		return -1;

	if (f) {
		n = snprintf(dest, 1024, "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=1;CP=&&%s&&",
			     st, cn, pw, mn, poldata);

		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_daily_data(char *buf, const char *poldata)
{
	int n = -1;
	char dest[1024];
	char *st = "21";
	char *cn = "2031";
	char *pw = get_siteconf_pw();
	char *mn = get_siteconf_mn();

	if ((!buf) || (!poldata))
		return -1;

	if (f) {
		n = snprintf(dest, 1024,
			     "ST=%2s;CN=%4s;PW=%6s;"
			     "MN=%14s;Flag=1;CP=&&%s&&",
			     st, cn, pw, mn, poldata);

		dest[n] = '\0';
		n = (*f)(buf, dest);
	}

	return n;
}

int create_pkt_res_login(char *buf, const char *data)
{
	int n = -1;
	char dest[256];
	char *st = "91";
	char *cn = "9014";
	char *pw = "123456";
	char *mn = "888888800000001";
	int flag = 0;

	if ((!buf) || (!data))
		return -1;

	if (f) {
		n = snprintf(dest, 256,
			     "ST=%2s;CN=%4s;PW=%6s;MN=%14s;Flag=%1d;CP=&&%s&&",
			     st, cn, pw, mn, flag, data);

		dest[n] = '\0';
		n = (*f) (buf, dest);
	}

	return n;
}

