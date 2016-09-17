#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/stacktrace.h>
#include <asm/stacktrace.h>

#define MAX_AEE_KERNEL_BT 16
#define MAX_AEE_KERNEL_SYMBOL 256

struct aee_process_bt {
	pid_t pid;

	int nr_entries;
	struct {
		unsigned long pc;
		char symbol[MAX_AEE_KERNEL_SYMBOL];
	} entries[MAX_AEE_KERNEL_BT];
};
#include <linux/pid.h>
#include "internal.h"
#include "aed.h"

static void print_task_state(struct seq_file *m, struct task_struct *p, char
			     *state)
{
	*state = (p->state == 0) ? 'R' :
		(p->state < 0) ? 'U' :
		(p->state & TASK_UNINTERRUPTIBLE) ? 'D' :
		(p->state & TASK_STOPPED) ? 'T' :
		(p->state & TASK_TRACED) ? 'C' :
		(p->exit_state & EXIT_ZOMBIE) ? 'Z' :
		(p->exit_state & EXIT_DEAD) ? 'E' : (p->state & TASK_INTERRUPTIBLE) ? 'S' : '?';

	SEQ_printf(m, "------------------------------------\n");
	SEQ_printf(m, "[%d:%s] state:%c\n", p->pid, p->comm, *state);
}

#include <linux/mt_export.h>
static void sem_traverse(struct seq_file *m)
{
	struct task_struct *g, *p;
	struct semaphore *sem;
	char state;
	struct aee_process_bt *pbt;
	int i;

	pbt = kmalloc(sizeof(struct aee_process_bt), GFP_KERNEL);
	if (!pbt)
		return;
	pr_err("[sem_traverse]\n");
	read_lock(&tasklist_lock);
	SEQ_printf(m, "============= Semaphore list ===============\n");
	do_each_thread(g, p) {
		/*
		 * It's not reliable to print a task's held locks
		 * if it's not sleeping (or if it's not the current
		 * task):
		 */

		if (list_empty(&p->sem_head))
			continue;

		print_task_state(m, p, &state);
		list_for_each_entry(sem, &p->sem_head, sem_list) {
			SEQ_printf(m, "\nSem Name:[%s], Address:[0x%8x]\nCaller:[%pS]\n",
				   sem->sem_name, (unsigned int)sem, sem->caller);
		}
		if (state != 'R') {
			SEQ_printf(m, "Backtrace:\n");
			pbt->pid = p->pid;
			aed_get_process_bt(pbt);
			for (i = 0; i < pbt->nr_entries - 1; i++)
				SEQ_printf(m, "  [%d]%s\n", i, (char *)&pbt->entries[i].symbol);
			/* proc_stack(m, p); */
		}
	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);
	SEQ_printf(m, "[Semaphore owner list End]\n");
	kfree(pbt);
}

static void lock_traverse(struct seq_file *m)
{
	struct task_struct *g, *p;
	struct mutex *lock;
	char state;
	int i;
	struct aee_process_bt *pbt;

	pbt = kmalloc(sizeof(struct aee_process_bt), GFP_KERNEL);
	if (!pbt)
		return;
	pr_err "[mutex_traverse]\n");
	read_lock(&tasklist_lock);
	SEQ_printf(m, "============== Mutex list ==============\n");
	do_each_thread(g, p) {
		/*
		 * It's not reliable to print a task's held locks
		 * if it's not sleeping (or if it's not the current
		 * task):
		 */

		if (list_empty(&p->mutex_head))
			continue;

		print_task_state(m, p, &state);
		list_for_each_entry(lock, &p->mutex_head, mutex_list) {
			SEQ_printf(m, "\nLock Name:[%s], Address:[0x%8x]\nCaller:[%pS]\n",
				   lock->mutex_name, (unsigned int)lock, (void *)lock->caller);
		}
		if (state != 'R') {
			SEQ_printf(m, "Backtrace:\n");
			pbt->pid = p->pid;
			aed_get_process_bt(pbt);
			for (i = 0; i < pbt->nr_entries - 1; i++)
				SEQ_printf(m, "  [%d]%s\n", i, (char *)&pbt->entries[i].symbol);
			/* proc_stack(m, p); */
		}
	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);
	SEQ_printf(m, "[Mutex owner list End]\n");
	kfree(pbt);
}

MT_DEBUG_ENTRY(locktb);
static int mt_locktb_show(struct seq_file *m, void *v)
{
	lock_traverse(m);
	pr_err("\n\n");
	sem_traverse(m);
	return 0;
}
static ssize_t mt_locktb_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	return cnt;
}
static int __init init_mtlock_prof(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("mtprof/locktb", 0664, NULL, &mt_locktb_fops);
	if (!pe)
		return -ENOMEM;
	 return 0;
}
late_initcall(init_mtlock_prof);
