#include <unistd.h>
#include <mqueue.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#include "log.h"
#include "error.h"
#include "config.h"
#include "server.h"
#include "database.h"
#include "siteconf.h"

typedef struct client_info {
	char num[32];
	char ip[32];
	char port[32];
	char mqfile[64];
} client_info_t;

client_info_t client[MAXSERVER + 1];

pid_t pid[MAXSERVER + 1];
mqd_t mq[MAXSERVER + 1];

static struct mq_attr wat_attr[MAXSERVER + 1];

server_t *p_server = NULL;
siteconf_t *p_siteconf = NULL;

static int servernum;
static volatile int flag = -1;

static mode_t mode = 0777;
static int oflag = O_RDWR | O_CREAT | O_NONBLOCK;

static char *gather_watchdog_mqfile = "/mq_watchdog";
static char *client_watchdog_mqfile[MAXSERVER + 1] = {"/mq_watchdog0", "/mq_watchdog1", "/mq_watchdog2", \
				"/mq_watchdog3", "/mq_watchdog4", "/mq_watchdog5", "/mq_watchdog6"};

typedef void (*sigfunc_t)(int);

static void sig_chld_handler(int sig)
{
	pid_t pid;
	int statue;

	while ((pid = waitpid(-1, &statue, WNOHANG)) > 0) ;
}

static  sigfunc_t set_chld_handler()
{
	struct sigaction act, oact;

	act.sa_handler = sig_chld_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGCHLD);
	act.sa_flags = 0;
#ifdef MSA_RESTART
	act.sa_flags |= SA_RESTART;
#endif
	if (sigaction(SIGCHLD, &act, &oact) < 0)
		return (SIG_ERR);
	
	return (oact.sa_handler);
}

static void *watchdog_routine(void *arg)
{
	int i;
	int r;
	char buf[32];

	while (1) {
		sleep(WATCHDOG_TIME);
		for (i = 0; i < (servernum + 1); i++) {
			memset(buf, 0, sizeof(buf));
			r = mq_receive(mq[i], buf, MAX_DATA_PACKET, NULL);
			if (r == -1) {
				sleep(5);
				flag = i;
				continue;
			} else {
#ifdef __DBG__
				printf("read buf = %s, from = [%d] process\n", buf, i);
#endif	
			}
		}
	}
}

/*
 * Don't forget to unlink the message queue.before running your program again.
 * if you don't unlink it, it will still use the old message queue settings.
 * this happens when you end your program with Ctrl+C.
 *
 */
static void unlink_mqueue(void)
{
	int i;
	int rc;

	rc = mq_unlink(gather_watchdog_mqfile);
	if (rc != 0 && errno != ENOENT) {
		log_printf(2, __FILE__, __FUNCTION__, __LINE__, "Message queue %s "
				"remove from system failed.", gather_watchdog_mqfile);
	} else {
#ifdef __DBG__
		printf("Message queue %s remove from system ok\n", gather_watchdog_mqfile);
#endif
	}

	for (i = 1; i < (servernum + 1); i++) {
		rc = mq_unlink(client_watchdog_mqfile[i]);
		if (rc != 0 && errno != ENOENT) {
			log_printf(2, __FILE__, __FUNCTION__, __LINE__, "Message queue %s "
					"remove from system failed.", client_watchdog_mqfile[i]);
		} else {
#ifdef __DBG__
			printf("Message queue %s remove from system ok\n", client_watchdog_mqfile[i]);
#endif
		}
	}
}

/*
 * close the message queue.
 *
 */ 
static void close_mqueue(void)
{
	
	int i;
	int rc;

	for (i = 0; i < (servernum + 1); i++) {
		rc = mq_close(mq[i]);
		if (rc != 0) {
			log_printf(2, __FILE__, __FUNCTION__, __LINE__, \
					"mq[%d] message queue close from system failed.", i);
		} else {
#ifdef __DBG__
			printf("mq[%d] message queue close from system ok\n", i);
#endif
		}
	}
}

int main(int argc, char *argv[])
{
	int i;
	int r;
	char gather_pid[32];
	server_info_t *pserver;

	int fd = open(LOG_FILE, O_RDWR | O_CREAT | O_APPEND, 0660);
	if (fd == -1)
		err_ret("open");

	dup2(fd, STDERR_FILENO);

	r = db_open(FILENAME);
	if (r != 0)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "open %s database failed.", FILENAME);

	printf("open %s database ok\n", FILENAME);
	
	r = get_server_settings();
	if (r == 0) {
		pserver = p_server->info;
		servernum = MIN_V(p_server->num, MAXSERVER);
	} else {
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "select server table failed.");
	}

	unlink_mqueue();

	for (i = 1; i < (servernum + 1); i++, pserver++) {
		snprintf(client[i].num,    32, "%d", i);
		snprintf(client[i].ip,     32, "%s", pserver->ip);
		snprintf(client[i].port,   32, "%d", pserver->port);
		snprintf(client[i].mqfile, 64, "%s", client_watchdog_mqfile[i]);
	}

	for (i = 0; i < (servernum + 1); i++) {
		memset(&wat_attr[i], 0, sizeof(struct mq_attr));
		wat_attr[i].mq_flags = 0;
		wat_attr[i].mq_maxmsg = 10;
		wat_attr[i].mq_msgsize = 1024;
		wat_attr[i].mq_curmsgs = 0;
	}

	/* for gather process */
	mq[0] = mq_open(gather_watchdog_mqfile, oflag, mode, &wat_attr[0]);
	if (mq[0] == ((mqd_t) -1)) {
		log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_open: %s.", strerror(errno));
		return 1;
	}

	for (i = 1; i < (servernum + 1); i++) {
		mq[i] = mq_open(client_watchdog_mqfile[i], oflag, mode, &wat_attr[i]);
		if (mq[i] == ((mqd_t) -1)) {
			log_printf(3, __FILE__, __FUNCTION__, __LINE__, "mq_open<%d>: %s.", i, strerror(errno));
			return 1;
		}
	}

	set_chld_handler();

	pid[0] = fork();
	if (pid[0] == -1)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "fork: %s", strerror(errno));

	snprintf(gather_pid, 32, "%d", pid[0]);

	if (pid[0] == 0) {
		printf("create gather process ok\n");
		execl("./gather", gather_watchdog_mqfile, NULL);
		exit(0);
	}

	/* gather process runs first */
	sleep(1);

	db_close();

	put_server_settings();

	for (i = 1; i < (servernum + 1); i++) {
		pid[i] = fork();
		if (pid[i] == -1)
			log_printf(4, __FILE__, __FUNCTION__, __LINE__, "fork<%d>: %s.", i, strerror(errno));
	
		if (pid[i] == 0) {
			printf("create client[%d] process ok\n", i);
			execl("./client", client[i].num, client[i].ip, client[i].port, client[i].mqfile, gather_pid, NULL);
			exit(0);
		}

		/* parent process do to loop next */
	}	

	pthread_t thr_watchdog;
	r = pthread_create(&thr_watchdog, NULL, watchdog_routine, NULL);
	if (r != 0)
		log_printf(4, __FILE__, __FUNCTION__, __LINE__, "create watchdog routine failed.");
	pthread_detach(thr_watchdog);

	while (1) {
		if (flag >= 0) {
			if (flag == 0) {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "the system reboot.");
				system("reboot");
				flag = -1;			
			} else {
				log_printf(3, __FILE__, __FUNCTION__, __LINE__, "client[%d] process died!!!", flag);
			
				pid[flag] = fork();
				if (pid[flag] == -1) {
					log_printf(3, __FILE__, __FUNCTION__, __LINE__, "fork<%d>: %s.", flag, strerror(errno));
					continue;	
				}

				if (pid[flag] == 0) {
					printf("recreate client[%d] process ok\n", flag);
					execl("./client", client[flag].num, client[flag].ip, client[flag].port, client[flag].mqfile, gather_pid, NULL);
					exit(0);
				}

				flag = -1;
				continue;
			}			
		}
	}

	return 0;
}

