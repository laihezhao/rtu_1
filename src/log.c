#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#define _GNU_SOURCE

/// @brief 日志等级 - 调试
static int const LEVEL_DBG = 0;
/// @brief 日志等级 - 信息
static int const LEVEL_INF = 1;
/// @brief 日志等级 - 警告
static int const LEVEL_WAR = 2;
/// @brief 日志等级 - 一般错误
static int const LEVEL_ERR = 3;
/// @brief 日志等级 - 致命错误
static int const LEVEL_CRT = 4;

static char const* log_levels[] = {"dbg", "inf", "war", "err", "crt"};

void log_printf(int level, char const *file, char const *function, int line, char const *format, ...)
{
	if (level < LEVEL_CRT) {
		char datetime[32];
		time_t now;
		struct tm local;

		now = time(NULL);
		strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime_r(&now, &local));

		char header[256];
		snprintf (header, sizeof (header),
				"[%s] [%s] [pid=%d tid=%ld] [%s:%s:%d]",
				datetime, log_levels[level], getpid (),
				syscall (SYS_gettid), file, function, line);

		char content[1024];
		va_list ap;
		va_start (ap, format);
		vsnprintf (content, sizeof (content), format, ap);
		va_end (ap);

		fprintf (stderr, "%s %s\n\n", header, content);

		fflush(stderr);

	} else {
		exit (EXIT_FAILURE);
	}
}
