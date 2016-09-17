
#undef TRACE_SYSTEM
#define TRACE_SYSTEM met_fuse

#if !defined(__TRACE_MET_FTRACE_FUSE_H__) || defined(TRACE_HEADER_MULTI_READ)
#define __TRACE_MET_FTRACE_FUSE_H__

#include <linux/tracepoint.h>

#include <linux/met_drv.h>
#include <linux/mmc/host.h>

/*
 * Tracepoint for met_event_fuse
 */
#if 0
TRACE_EVENT(met_fuse,
	TP_PROTO(int t_pid, char *t_name, unsigned int op, unsigned int size, struct timespec s_time, struct timespec e_time),

	TP_ARGS(t_pid, t_name, op, size, s_time, e_time),

	TP_STRUCT__entry(
		__field(int, task_pid)
		__field(unsigned int, op_code)
		__field(unsigned int, op_size)
		__field(long, start_time_s)
		__field(long, start_time_ns)
		__field(long, end_time_s)
		__field(long, end_time_ns)
		__array(char, task_name, TASK_COMM_LEN)
	),

	TP_fast_assign(
		__entry->task_pid = t_pid;
		__entry->op_code = op;
		__entry->op_size = size;
		__entry->start_time_s = s_time.tv_sec;
		__entry->start_time_ns = s_time.tv_nsec;
		__entry->end_time_s = e_time.tv_sec;
		__entry->end_time_ns = e_time.tv_nsec;
		memcpy(__entry->task_name, t_name, TASK_COMM_LEN);
	),

	TP_printk("%d,%s,%u,%u,%ld,%ld,%ld,%ld",
		__entry->task_pid, __entry->task_name,
		__entry->op_code, __entry->op_size,
		__entry->start_time_s, __entry->start_time_ns,
		__entry->end_time_s, __entry->end_time_ns)
);
#endif

DECLARE_EVENT_CLASS(met_mmc_fuse_template,
	TP_PROTO(int t_pid, char *t_name, unsigned int op, unsigned int size),

	TP_ARGS(t_pid, t_name, op, size),

	TP_STRUCT__entry(
		__field(int, task_pid)
		__field(unsigned int, op_code)
		__field(unsigned int, op_size)
		__array(char, task_name, TASK_COMM_LEN)
	),

	TP_fast_assign(
		__entry->task_pid = t_pid;
		__entry->op_code = op;
		__entry->op_size = size;
		memcpy(__entry->task_name, t_name, TASK_COMM_LEN);
	),

	TP_printk("%d,%s,%u,%u",
		__entry->task_pid, __entry->task_name,
		__entry->op_code, __entry->op_size)
);

/*
 * Tracepoint for met_fuse_start
 */
DEFINE_EVENT(met_mmc_fuse_template, met_fuse_start,
	TP_PROTO(int t_pid, char *t_name, unsigned int op, unsigned int size),
	TP_ARGS(t_pid, t_name, op, size));

/*
 * Tracepoint for met_fuse_stop
 */
DEFINE_EVENT(met_mmc_fuse_template, met_fuse_stop,
	TP_PROTO(int t_pid, char *t_name, unsigned int op, unsigned int size),
	TP_ARGS(t_pid, t_name, op, size));

#endif /* __TRACE_MET_FTRACE_FUSE_H__ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#undef linux
#define TRACE_INCLUDE_PATH ../../include/linux
#define TRACE_INCLUDE_FILE met_ftrace_fuse
#include <trace/define_trace.h>
