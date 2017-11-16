#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "log.h"
#include "error.h"
#include "config.h"
#include "database.h"
#include "server.h"
#include "packet.h"
#include "siteconf.h"

#define TIMEOUT			5
#define CONNECT_TIMES		200		
#define RESPOND_OK_FLAG		"9014"
#define RESPOND_HEARTBEAT       "2020"
#define RESPOND_DAY_DATA        "2031"
#define RESPOND_MIN_DATA        "2051"
#define RESPOND_HOUR_DATA       "2061"

server_t *p_server = NULL;
siteconf_t *p_siteconf = NULL;

pid_t gather_pid;
static mqd_t mq[3];
union sigval mysigval;

static int senddog;
static int flag;
static int reboot_flag = 0;
static volatile int sockfd;
static volatile int error;

static char history_min[1024];
static char history_hour[1024];
static char history_day[1024];

static int retry_count_min = 0;
static int retry_count_hour = 0;
static int retry_count_day = 0;
static int retry_count_heartbeat = 0;

static int trigger_min  = 0;
static int trigger_hour = 0;
static int trigger_day  = 0;

static char heartbeat_time[32];
static char minutedata_time[32];
static char hourdata_time[32];
static char daydata_time[32];

static timer_t timerid;
static timer_t timerid_min;
static timer_t timerid_hour;
static timer_t timerid_day;
static timer_t timerid_watchdog;

static struct sigevent evp;
static struct sigevent evp_min;
static struct sigevent evp_hour;
static struct sigevent evp_day;

static struct itimerspec it;
static struct itimerspec it_min;
static struct itimerspec it_hour;
static struct itimerspec it_day;

static pthread_t thr_reboot;
static pthread_t thr_sender;
static pthread_t thr_receiver;
static pthread_t thr_heartbeat;
static pthread_t thr_minutedata;
static pthread_t thr_hourdata;
static pthread_t thr_daydata;

static pthread_mutex_t mutex_send;
static pthread_mutex_t mutex_recv;

static char *client_mqfile[MAXSERVER + 1] = {"/mq_data0", "/mq_data1", "/mq_data2", \
						"/mq_data3", "/mq_data4", "/mq_data5","/mq_data6"};
static char *mqfile[MAXSERVER + 1] = {"/mqtest0", "/mqtest1", "/mqtest2", "/mqtest3", \
					"/mqtest4", "/mqtest5", "/mqtest6"};

typedef void (*sigfunc_t)(int);

static int is_error(void)
{
	return error ? error : 0;
}

static int msend(int sockfd, void *buf, size_t len, int flags)
{
	int n;

	pthread_mutex_lock(&mutex_send);
	n = send(sockfd, buf, len, flags);
	pthread_mutex_unlock(&mutex_send);

	return n;
}

static int mrecv(int sockfd, void *buf, size_t len, int flags)
{
	int n;

	pthread_mutex_lock(&mutex_recv);
	n = recv(sockfd, buf, len, flags);
	pthread_mutex_unlock(&mutex_recv);

	return n;
}

static void sig_pipe_handler(int sig)
{
	error = 2;
}

static void sig_usr1_handler(int sig)
{
	/**/
}

static sigfunc_t set_pipe_handler()
{
	struct sigaction act, oact;

	act.sa_handler = sig_pipe_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGPIPE);
	act.sa_flags = 0;
#ifdef MSA_RESTART
	act.sa_flags |= SA_RESTART;
#endif
	if (sigaction(SIGPIPE, &act, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}

static sigfunc_t set_usr1_handler()
{
	struct sigaction act, oact;

	act.sa_handler = sig_usr1_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR1);
	act.sa_flags = 0;
#ifdef MSA_RESTART
	act.sa_flags |= SA_RESTART;
#endif
	if (sigaction(SIGUSR1, &act, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}

static void respond_heartbeat_handler(const char *qn)
{	
	if (!strncmp(heartbeat_time, qn, 17)) {
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_nsec = 0;
		it.it_value.tv_sec = 0;
		it.it_value.tv_nsec = 0;

		if (timer_settime(timerid, 0, &it, NULL) == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
		
		retry_count_heartbeat = 0;
	}
}

static void respond_minutedata_handler(const char *qn)
{
	if (!strncmp(minutedata_time, qn, 17)) {
		it_min.it_interval.tv_sec = 0;
		it_min.it_interval.tv_nsec = 0;
		it_min.it_value.tv_sec = 0;
		it_min.it_value.tv_nsec = 0;

		if (timer_settime(timerid_min, 0, &it_min, NULL) == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));

		retry_count_min = 0;
	}
}

static void respond_hourdata_handler(const char *qn)
{
	if (!strncmp(hourdata_time, qn, 17)) {
		it_hour.it_interval.tv_sec = 0;
		it_hour.it_interval.tv_nsec = 0;
		it_hour.it_value.tv_sec = 0;
		it_hour.it_value.tv_nsec = 0;

		if (timer_settime(timerid_hour, 0, &it_hour, NULL) == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
		
		retry_count_hour = 0;
	}
}

static void respond_daydata_handler(const char *qn)
{
	if (!strncmp(daydata_time, qn, 17)) {
		it_day.it_interval.tv_sec = 0;
		it_day.it_interval.tv_nsec = 0;
		it_day.it_value.tv_sec = 0;
		it_day.it_value.tv_nsec = 0;

		if (timer_settime(timerid_day, 0, &it_day, NULL) == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));

		retry_count_day = 0;
	}
}

static void parse_recv_respond(const char *pkt_r)
{
	char qn[32] = {0};
	char cn[32] = {0};
	char respond[32] = {0};

	if (strncmp(pkt_r + 2 + 4, "ST", 2) != 0)
		return ;
	
	sscanf(pkt_r + 2 + 4, "%*6sCN=%4s", respond);
	sscanf(pkt_r + 2 + 4, "%*54sQN=%17s", qn);
	sscanf(pkt_r + 2 + 4, "%*75sCN=%4s",  cn);
	
	if (strcmp(RESPOND_OK_FLAG, respond) == 0) {
		/* 2020 */
		if (strcmp(RESPOND_HEARTBEAT, cn) == 0) {
			respond_heartbeat_handler(qn);
			printf("receive respond = %s\n", cn);
		}

		/* 2031 */
		if (strcmp(RESPOND_DAY_DATA, cn) == 0) {
			respond_daydata_handler(qn);
			printf("receive respond = %s\n", cn);
		}

		/* 2051 */
		if (strcmp(RESPOND_MIN_DATA, cn) == 0) {
			respond_minutedata_handler(qn);
			printf("receive respond = %s\n", cn);
		}

		/* 2061 */
		if (strcmp(RESPOND_HOUR_DATA, cn) == 0) {
			respond_hourdata_handler(qn);
			printf("receive respond = %s\n", cn);
		}
	}
}

static void *sender_routine(void *arg)
{
	int n;
	int size;
	int index = 0;
	char *pkt;
	
	pkt = (char *)malloc(MAX_DATA_PACKET);
	if (!pkt)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "allocate memory failed.");
	
	while (1) {
		if (!is_error()) {
			memset(pkt, 0, MAX_DATA_PACKET);
			size = mq_receive(mq[1], pkt, MAX_DATA_PACKET, NULL);
			if (size == -1) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_receive: %s.", strerror(errno));
				sleep(5);
				continue;
			}
		
			if (size >= 0) {	
				/* minute data */
				if (!strncmp(pkt + 2 + 4 + 9, "2051", 4)) {
					memset(history_min, 0, 1024);
					strncpy(history_min, pkt, 1024);
					strncpy(minutedata_time, pkt + 2 + 4 + 57, 17),

						trigger_min = 1;	
				}

				/* hour data */
				if (!strncmp(pkt + 2 + 4 + 9, "2061", 4)) {
					memset(history_hour, 0, 1024);
					strncpy(history_hour, pkt, 1024);
					strncpy(hourdata_time, pkt + 2 + 4 +57, 17);

					trigger_hour = 1;
				}

				/* day data `*/
				if (!strncmp(pkt + 2 + 4 + 9, "2031", 4)) {
					memset(history_day, 0, 1024);
					strncpy(history_day, pkt, 1024);
					strncpy(daydata_time, pkt + 2 + 4 + 57, 17);

					trigger_day = 1;
				}

				n = msend(sockfd, pkt, size, 0);
				if (n < 0) {
					perror("msend");
					sleep(5);
					continue;
				}

				printf("< %d >, client send %d bytes\n", index, n);
				index++;

				usleep(100000);
			}
		}
	}
	
	free(pkt);		
	pkt = NULL;
}

static void *receiver_routine(void *arg)
{
	int r;
	int n;
	char *pkt;

	pkt = (char *)malloc(MAX_DATA_PACKET);
	if (!pkt)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "allocate memory failed.");

	while (1) {
		if (!is_error()) {
			memset(pkt, 0, MAX_DATA_PACKET);
			n =mrecv(sockfd, pkt, MAX_DATA_PACKET, 0);
			if (n < 0) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "recv: %s.", strerror(errno));
				sleep(5);
				continue;
			} 
			
			if (n == 0) {
				error = 3;
				/* wake up the sleeping heartbeat thread */
				pthread_kill(thr_heartbeat, SIGUSR1);
				sleep(5);
				continue;
			}

			if (n > 0) {
				r = verify_packet(pkt);
				if (r != 0) {
					log_printf(3, __FILE__, __FUNCTION__, __LINE__, "verify packet failed.");
					sleep(5);
					continue;
				}
				
				parse_recv_respond(pkt);

				r = mq_send(mq[2], pkt, n, 0);
				if (r  == -1)
					log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_send: %s.", strerror(errno));
			}
		}
	}

	free(pkt);
	pkt = NULL;
}

static void timer_thread_minutedata(union sigval v)
{
	int r;
	char buf[256];

	if (!is_error()) {
		retry_count_min++;

		snprintf(buf, 256, "2051%s", minutedata_time);

		r = msend(sockfd, history_min, strlen(history_min), 0);
		if (r == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "msend: %s.", strerror(errno));
		
		if (retry_count_min > RETRY_COUNT_MIN) {
			it_min.it_interval.tv_sec = 0;
			it_min.it_interval.tv_nsec = 0;
			it_min.it_value.tv_sec = 0;
			it_min.it_value.tv_nsec = 0;

			if (timer_settime(timerid_min, 0, &it_min, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
			
			r = mq_send(mq[2], buf, strlen(buf), 0);
			if (r != 0)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_send: %s.", strerror(errno));

			retry_count_min = 0;

			memset(minutedata_time, 0, 32);
		}
	}
}

static void timer_thread_hourdata(union sigval v)
{
	int r;
	char buf[256];

	if (!is_error()) {
		retry_count_hour++;

		snprintf(buf, 256, "2061%s", hourdata_time);

		r = msend(sockfd, history_hour, strlen(history_hour), 0);
		if (r == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "msend: %s.", strerror(errno));
		
		if (retry_count_hour > RETRY_COUNT_HOUR) {
			it_hour.it_interval.tv_sec = 0;
			it_hour.it_interval.tv_nsec = 0;
			it_hour.it_value.tv_sec = 0;
			it_hour.it_value.tv_nsec = 0;

			if (timer_settime(timerid_hour, 0, &it_hour, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
			
			r = mq_send(mq[2], buf, strlen(buf), 0);
			if (r != 0)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_send: %s.", strerror(errno));
			
			retry_count_hour = 0;

			memset(hourdata_time, 0, 32);
		}
	}
}

static void timer_thread_daydata(union sigval v)
{
	int r;
	char buf[256];

	if (!is_error()) {
		retry_count_day++;

		snprintf(buf, 256, "2031%s", daydata_time);

		r = msend(sockfd, history_day, strlen(history_day), 0);
		if (r == -1)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "msend: %s.", strerror(errno));

		if (retry_count_day > RETRY_COUNT_DAY) {
			it_day.it_interval.tv_sec = 0;
			it_day.it_interval.tv_nsec = 0;
			it_day.it_value.tv_sec = 0;
			it_day.it_value.tv_nsec = 0;

			if (timer_settime(timerid_day, 0, &it_day, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
			
			r = mq_send(mq[2], buf, strlen(buf), 0);
			if (r != 0)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_send: %s.", strerror(errno));

			retry_count_day = 0;

			memset(daydata_time, 0, 32);
		}
	}
}

static void timer_thread_heartbeat(union sigval v)
{
	if (retry_count_heartbeat > RETRY_COUNT_HEARTBEAT)
		error = 1;
}

static void *heartbeat_routine(void *arg)
{
	int n;
	int r;
	char buf[256];

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_int = 0;
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_thread_heartbeat;
	
	if (timer_create(CLOCKID, &evp, &timerid) == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_create: %s.", strerror(errno));
		return NULL;
	}

	while (1) {
		if (!is_error()) {
			memset(buf, 0, sizeof(buf));

			it.it_interval.tv_sec = HEARTBEAT_TIME;
			it.it_interval.tv_nsec = 0;
			it.it_value.tv_sec = HEARTBEAT_TIME;
			it.it_value.tv_nsec = 0;

			if (timer_settime(timerid, 0, &it, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));

			r = get_sys_time(heartbeat_time, NULL);
			if (r != 0) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "get_sys_time failed.");
				continue;
			}

			n = create_pkt_heartbeat(buf, heartbeat_time);
			r = msend(sockfd, buf, n, 0);
			if (r < 0) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "send: %s.", strerror(errno));
				sleep(5);
				continue;
			}
			
			senddog = 0;
			
			retry_count_heartbeat++;

			sleep(HEARTBEAT_TIME + 1);
			
			it.it_interval.tv_sec = 0;
			it.it_interval.tv_nsec = 0;
			it.it_value.tv_sec = 0;
			it.it_value.tv_nsec = 0;
		
			if (timer_settime(timerid, 0, &it, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
#ifdef __DBG__
			printf("clear heartbeat timer\n");
#endif
		}
	}
}

static void *minutedata_routine(void *arg)
{
	memset(&evp_min, 0, sizeof(struct sigevent));
	evp_min.sigev_value.sival_int = 0;
	evp_min.sigev_notify = SIGEV_THREAD;
	evp_min.sigev_notify_function = timer_thread_minutedata;
	
	if (timer_create(CLOCKID, &evp_min, &timerid_min) == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_create: %s.", strerror(errno));
		return NULL;
	}

	while (1) {
		if (trigger_min) {
			it_min.it_interval.tv_sec = RETRY_TIME_MIN;
			it_min.it_interval.tv_nsec = 0;
			it_min.it_value.tv_sec = RETRY_TIME_MIN;
			it_min.it_value.tv_nsec = 0;

			if (timer_settime(timerid_min, 0, &it_min, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));

			trigger_min = 0;
		}
	}
}

static void *hourdata_routine(void *arg)
{
	memset(&evp_hour, 0, sizeof(struct sigevent));
	evp_hour.sigev_value.sival_int = 0;
	evp_hour.sigev_notify = SIGEV_THREAD;
	evp_hour.sigev_notify_function = timer_thread_hourdata;
	
	if (timer_create(CLOCKID, &evp_hour, &timerid_hour) == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_create: %s.", strerror(errno));
		return NULL;
	}

	while (1) {
		if (trigger_hour) {
			it_hour.it_interval.tv_sec = RETRY_TIME_HOUR;
			it_hour.it_interval.tv_nsec = 0;
			it_hour.it_value.tv_sec = RETRY_TIME_HOUR;
			it_hour.it_value.tv_nsec = 0;

			if (timer_settime(timerid_hour, 0, &it_hour, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));

			trigger_hour = 0;
		}
	}
}

static void *daydata_routine(void *arg)
{
	memset(&evp_day, 0, sizeof(struct sigevent));
	evp_day.sigev_value.sival_int = 0;
	evp_day.sigev_notify = SIGEV_THREAD;
	evp_day.sigev_notify_function = timer_thread_daydata;
	
	if (timer_create(CLOCKID, &evp_day, &timerid_day) == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_create: %s.", strerror(errno));
		return NULL;
	}

	while (1) {
		if (trigger_day) {
			it_day.it_interval.tv_sec = RETRY_TIME_DAY;
			it_day.it_interval.tv_nsec = 0;
			it_day.it_value.tv_sec = RETRY_TIME_DAY;
			it_day.it_value.tv_nsec = 0;

			if (timer_settime(timerid_day, 0, &it_day, NULL) == -1)
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));

			trigger_day = 0;
		}
	}
}

static int delete_heartbeat_timer(void)
{
	int r;

	r = timer_delete(timerid);
	if (r == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_delete: %s.", strerror(errno));
		return -1;
	}

	return 0;
}

static int delete_minutedata_timer(void)
{
	int r;

	r = timer_delete(timerid_min);
	if (r == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_delete: %s.", strerror(errno));
		return -1;
	}

	return 0;
}

static int delete_hourdata_timer(void)
{
	int r;

	r = timer_delete(timerid_hour);
	if (r == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_delete: %s.", strerror(errno));
		return -1;
	}

	return 0;
}

static int delete_daydata_timer(void)
{
	int r;

	r = timer_delete(timerid_day);
	if (r == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_delete: %s.", strerror(errno));
		return -1;
	}

	return 0;
}

static int connect_to_server(const char *hostname, const int port)
{
	int r;
	int arg;
	int valopt;
	fd_set sock_set;
	struct timeval tv;
	struct sockaddr_in serv_addr;

	if ((!hostname) || (!port)) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "hostname or port error.");
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "socket: %s.", strerror(errno));
		return -1;
	}

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, hostname, &serv_addr.sin_addr);

	arg = fcntl(sockfd, F_GETFL, NULL);
	arg |= O_NONBLOCK;
	fcntl(sockfd, F_SETFL, arg);

	r = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (r == -1) {
		if (errno == EINPROGRESS) {
			tv.tv_sec = TIMEOUT;
			tv.tv_usec = 0;

			FD_ZERO(&sock_set);
			FD_SET(sockfd, &sock_set);

			r = select(sockfd + 1, NULL, &sock_set, NULL, &tv);
			if (r > 0) {
				socklen_t len = sizeof(int);
				getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
					   (void *)(&valopt), &len);

				if (valopt) {
					log_printf(3, __FILE__, __FUNCTION__, __LINE__, "select: %s.", strerror(valopt));
					close(sockfd);
					return -1;
				}
			} else if (r == 0) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "select timeout.");
				close(sockfd);
				return -1;
			} else if (r == -1) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "select: %s.", strerror(errno));
				close(sockfd);
				return -1;
			}
		} else {
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "connect: %s.", strerror(errno));
			close(sockfd);
			return -1;
		}
	}

	FD_CLR(sockfd, &sock_set);
	
	arg = fcntl(sockfd, F_GETFL, NULL);
	arg &= ~O_NONBLOCK;
	fcntl(sockfd, F_SETFL, arg);

	return sockfd;
}

static int set_localtime(const char *settime)
{
	int ret = 0;
	struct tm time_tm;
        struct timeval tv;	
	struct timeval time_tv;
	time_t timep;  
	const char *temp = settime;
	struct timezone tz;

	sscanf(temp, "%4d%2d%2d%2d%2d%2d", &time_tm.tm_year, &time_tm.tm_mon, &time_tm.tm_mday, &time_tm.tm_hour, \
			&time_tm.tm_min, &time_tm.tm_sec);  

	time_tm.tm_year -= 1900;	
	time_tm.tm_mon -= 1;  
	time_tm.tm_wday = 0;
	time_tm.tm_yday = 0;
	time_tm.tm_isdst = 0;

	timep = mktime(&time_tm);
	time_tv.tv_sec = timep;  
	time_tv.tv_usec = 0;  
	
	ret = gettimeofday(&tv, &tz);
	if (ret != 0) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "gettimeofday: %s.", strerror(errno));
		return -1;
	}

	ret = settimeofday(&time_tv, &tz);
	if(ret != 0) {  
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "settimeofday: %s.", strerror(errno));
		return -1;  
	}

	printf("update system time ok\n");

	return 0;  
}

static int parse_login_info(char *buf, const char *qn)
{
	char qn_r[24];
	char cn_r[24];
	char reply[24];
	char settime[32];

	memset(qn_r,  0, 24);
	memset(cn_r,  0, 24);
	memset(reply, 0, 24);
	memset(settime, 0, 32);

	sscanf(buf, "%*12sCN=%4s", reply);
	if (strcmp(reply, "9014") != 0) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "server reply error info.");
		return -1;
	}

	sscanf(buf, "%*55sCP=&&QN=%17s;CN=%4s;DataTime=%14s&&", qn_r, cn_r, settime);
	if ((strcmp(qn_r, qn) != 0) || (strcmp(cn_r, "1010") != 0)) {
		return (-1);
	}
	
//	set_localtime(settime);

	return 0;
}

static int login(void)
{
	int n;
	int r;
	char qn[32];

	char *sbuf = (char *)malloc(MAX_DATA_PACKET);
	char *rbuf = (char *)malloc(MAX_DATA_PACKET);
	if ((!sbuf) || (!rbuf)) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "allocate memory failed.");
		return -1;
	}

	memset(sbuf, 0, MAX_DATA_PACKET);
	memset(rbuf, 0, MAX_DATA_PACKET);
	
	r = get_sys_time(qn, NULL);
	if (r != 0)
		goto fail;
	
	n = create_pkt_login(sbuf, qn);
	send(sockfd, sbuf, n, 0);

	usleep(1000);

	n = recv(sockfd, rbuf, MAX_DATA_PACKET, 0);
	if (n == -1 && errno == EAGAIN) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "login timeout.");
		goto fail;
	}
	
	r = verify_packet(rbuf);
	if (r == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "login packet verify failed.");
		goto fail;
	}

	r = parse_login_info(rbuf, qn);
	if (r != 0) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "login info error.");
		goto fail;	
	}

	return 0;
fail:
	free(sbuf);
	free(rbuf);
	sbuf = NULL;
	rbuf = NULL;
	return -1;
}

static void timer_thread_watchdog(union sigval v)
{
	int r;

	if (senddog > 5)
		error = 1;

	r = mq_send(mq[0], "hello", 6, 0);
	if (r != 0)
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "msend: %s.", strerror(errno));
	
	senddog++;
}

static int create_watchdog_timer()
{
	struct sigevent evp;
	struct itimerspec it;
	
	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_int = 0;
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_thread_watchdog;

	if (timer_create(CLOCKID, &evp, &timerid_watchdog) == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_create: %s.", strerror(errno));
		return -1;
	}
	
	/* be sure that data is sended ahead of */
	it.it_interval.tv_sec = WATCHDOG_TIME;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = WATCHDOG_TIME - 2;
	it.it_value.tv_nsec = 0;

	if (timer_settime(timerid_watchdog, 0, &it, NULL) == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_settime: %s.", strerror(errno));
		return -1;
	}

	return 0;
}

static int delete_watchdog_timer(void)
{
	int r;

	r = timer_delete(timerid_watchdog);
	if (r == -1) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "timer_delete: %s.", strerror(errno));
		return -1;
	}

	return 0;
}

static void *reboot_routine(void *arg)
{
	while (1) {
		if (reboot_flag > CONNECT_TIMES)
			system("reboot");
		sleep(5);
	}
}

int main(int argc, char *argv[])
{
	int r;
	int once = 0;

	int fd = open(LOG_FILE, O_RDWR | O_CREAT | O_APPEND, 0660);
	if (fd == -1)
		err_ret("open");

	dup2(fd, STDERR_FILENO);
	
	if (argc != 5)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "client process parameters error.");
	
	if ((!argv[0]) || (!argv[1]) || (!argv[2]) || (!argv[3]) || (!argv[4]))
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "client process parameters is NULL.");
	
	r = db_open(FILENAME);
	if (r != 0)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "open %s database failed.", FILENAME);
	
	printf("open %s database ok\n", FILENAME);
	
	r = get_siteconf_settings();
	if (r != 0)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "select siteconf table failed.");
	
	flag = atoi(argv[0]);
	gather_pid = atoi(argv[4]);

	mysigval.sival_int = flag;

	/* for watchdog */
	mq[0] = mq_open(argv[3], O_WRONLY);
	if (mq[0] == ((mqd_t) -1))
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "open client[%d] process watchdog mqueue failed: %s.", \
				flag, strerror(errno));

	/* for receiving data from gather process */
	mq[1] = mq_open(client_mqfile[flag], O_RDONLY);
	if (mq[1] == ((mqd_t) -1))
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "open client[%d] process receiving mqueue failed: %s.", \
				flag, strerror(errno));

	/* for sending data to gather process */
	mq[2] = mq_open(mqfile[flag], O_WRONLY);
	if (mq[2] == ((mqd_t) -1)) 
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "open client[%d] process sending mqueue failed: %s.", \
				flag, strerror(errno));
	
	set_pipe_handler();
	set_usr1_handler();

	r = create_watchdog_timer();
	if (r != 0)
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "create watchdog timer failed.");
	
	pthread_mutex_init(&mutex_send, NULL);
	pthread_mutex_init(&mutex_recv, NULL);

	r = pthread_create(&thr_reboot, NULL, reboot_routine, NULL);
	if (r != 0)
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "create reboot routine failed.");
	
	pthread_detach(thr_reboot);

	/* if once time connect server failed, send SIGRTMIN+1 signal */
	int a_flag = 0;
	int b_flag = 0;

	while (1) {
		sockfd = connect_to_server(argv[1], atoi(argv[2]));
		if (sockfd == -1) {
			if (a_flag == 0) {
				r = sigqueue(gather_pid, SIGRTMIN+1, mysigval);
				if (r != 0)
					log_printf(3, __FILE__, __FUNCTION__, __LINE__, "sigqueue: %s.", strerror(errno));

				a_flag = 1;
			}

			log_printf(1, __FILE__, __FUNCTION__, __LINE__, "connect server failed %d times.", reboot_flag);
			reboot_flag++;
			sleep(5);
			continue;
		}

		log_printf(1, __FILE__, __FUNCTION__, __LINE__, "connect server %s ok.", argv[1]);
		
		r = login();
		if (r == -1) {
			if (b_flag == 0 && a_flag == 0) {
				r = sigqueue(gather_pid, SIGRTMIN+1, mysigval);
				if (r != 0)
					log_printf(3, __FILE__, __FUNCTION__, __LINE__, "sigqueue: %s.", strerror(errno));

				b_flag = 1;
			}
			close(sockfd);
			log_printf(1, __FILE__, __FUNCTION__, __LINE__, "connect server failed %d times.", reboot_flag);
			reboot_flag++;	
			sleep(5);
			continue;
		}
		
		reboot_flag = 0;
		
		log_printf(1, __FILE__, __FUNCTION__, __LINE__, "login server %s ok.", argv[1]);
		
		/* if one time succeed, next not send singal */	
		a_flag = 1;
		b_flag = 1;

		error = 0;	
		senddog = 0;

		r = sigqueue(gather_pid, SIGRTMIN+2, mysigval);
		if (r != 0)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "sigqueue: %s.", strerror(errno));
		
		/* be sure gather process receive the signal */
		sleep(10);

		while (1) {
			int flags = is_error();
			if (flags == 1) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "heartbeat timeout.");
				break;
			}
			
			if (flags == 2) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "Catch SIGPIPE signal.");
				break;
			}
			
			if (flags == 3) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "the server shutdown.");
				break;
			}

			if (once == 0) {
				r = pthread_create(&thr_sender, NULL, sender_routine, NULL);
				if (r != 0)
					log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create sender routine failed.");

				r = pthread_create(&thr_receiver, NULL, receiver_routine, NULL);
				if (r != 0)
					log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create receiver routine failed.");
				
				r = pthread_create(&thr_heartbeat, NULL, heartbeat_routine, NULL);
				if (r != 0)
					log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create heartbeat routine failed.");
			
				r = pthread_create(&thr_minutedata, NULL, minutedata_routine, NULL);
				if (r != 0)
					log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create minutedate routine failed.");
				
				r = pthread_create(&thr_hourdata, NULL, hourdata_routine, NULL);
				if (r != 0)
					log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create hourdate routine failed.");
				
				r = pthread_create(&thr_daydata, NULL, daydata_routine, NULL);
				if (r != 0)
					log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create daydate routine failed.");
				
				once = 1;
			}
		}
		
		sleep(10);
		
		close(sockfd);
		
		r = sigqueue(gather_pid, SIGRTMIN+1, mysigval);
		if (r != 0)
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "sigqueue: %s.", strerror(errno));
	
		sleep(10);
	}

	/* never run here */
	pthread_join(thr_sender, NULL);
	pthread_join(thr_receiver, NULL);
	pthread_join(thr_heartbeat, NULL);
	pthread_join(thr_minutedata, NULL);
	pthread_join(thr_hourdata, NULL);
	pthread_join(thr_daydata, NULL);

	return 0;
}

