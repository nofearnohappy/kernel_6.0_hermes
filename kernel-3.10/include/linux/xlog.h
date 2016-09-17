#if !defined(_LINUX_XLOG_H)
#define _LINUX_XLOG_H

#include <linux/linkage.h>

enum android_log_priority {
	ANDROID_LOG_UNKNOWN = 0,
	ANDROID_LOG_DEFAULT,	/* only for SetMinPriority() */
	ANDROID_LOG_VERBOSE,
	ANDROID_LOG_DEBUG,
	ANDROID_LOG_INFO,
	ANDROID_LOG_WARN,
	ANDROID_LOG_ERROR,
	ANDROID_LOG_FATAL,
	ANDROID_LOG_SILENT,	/* only for SetMinPriority(); must be last */
};

#define LOGGER_ALE_ARGS_MAX 16

struct ale_convert {
	const char *tag_str;
	const char *fmt_ptr;
	const char *filename;
	int lineno;

	unsigned int hash;
	char params[LOGGER_ALE_ARGS_MAX];
};

struct xlog_record {
	const char *tag_str;
	const char *fmt_str;
	int prio;
};

#define xlog_printk(prio, tag, fmt, ...) ((void)0)
#define xlog_ksystem_printk(prio, tag, fmt, ...)    ((void)0)

#endif
