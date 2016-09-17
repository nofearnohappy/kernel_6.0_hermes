#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/stacktrace.h>
#include <asm/stacktrace.h>

#include <linux/pid.h>
#include <linux/debug_locks.h>
#include "internal.h"

/*************/
/* sample code */
static DEFINE_MUTEX(mtx_a);
static DEFINE_MUTEX(mtx_b);
static DEFINE_MUTEX(mtx_c);
MT_DEBUG_ENTRY(pvlk);
static int mt_pvlk_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "debug_locks = %d\n", debug_locks);
	return 0;
}

static ssize_t mt_pvlk_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;

	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;
	if (val == 0) {
		debug_locks_off();
	} else if (val == 2) {
		pr_err("==== circular lock test=====\n");
		mutex_lock(&mtx_a);
		mutex_lock(&mtx_b);
		mutex_lock(&mtx_c);
		mutex_unlock(&mtx_c);
		mutex_unlock(&mtx_b);
		mutex_unlock(&mtx_a);

		mutex_lock(&mtx_c);
		mutex_lock(&mtx_a);
		mutex_lock(&mtx_b);
		mutex_unlock(&mtx_b);
		mutex_unlock(&mtx_a);
		mutex_unlock(&mtx_c);

	}
	pr_err("[MT prove locking] debug_locks = %d\n", debug_locks);
	return cnt;
}

static int __init init_pvlk_prof(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("mtprof/pvlk", 0664, NULL, &mt_pvlk_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}
late_initcall(init_pvlk_prof);
