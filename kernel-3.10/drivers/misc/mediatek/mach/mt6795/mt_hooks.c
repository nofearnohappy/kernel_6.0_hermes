#include <linux/compat.h>
#include <linux/init.h>
#include <asm/traps.h>
#include <asm/ptrace.h>
#include <asm/cacheflush.h>

static DEFINE_PER_CPU(void *, __prev_undefinstr_pc) = 0;
static DEFINE_PER_CPU(int, __prev_undefinstr_counter) = 0;

/*
 * return 0 if code is fixed
 * return 1 if the undefined instruction cannot be fixed.
 */
int arm_undefinstr_retry(struct pt_regs *regs, unsigned int instr)
{
	void __user *pc;
	struct thread_info *thread = current_thread_info();
	//u32 insn = __opcode_to_mem_arm(BUG_INSTR_VALUE);
	u32 insn = 0;

	pc = (void __user *)instruction_pointer(regs);
	/* Place the SIGILL ICache Invalidate after the Debugger Undefined-Instruction Solution. */
	if (((processor_mode(regs) == USR_MODE) || (processor_mode(regs) == SVC_MODE)) &&
			(instr != insn)) {
		void **prev_undefinstr_pc = &get_cpu_var(__prev_undefinstr_pc);
		int *prev_undefinstr_counter = &get_cpu_var(__prev_undefinstr_counter);
                /* Only do it for User-Space Application. */
		pr_alert("USR_MODE/SVC_MODE Undefined Instruction Address curr:%p pc=%p:%p, instr: 0x%x\n",
			(void *)current, (void *)pc, (void *)*prev_undefinstr_pc,
			instr);
		if ((*prev_undefinstr_pc != pc)) {
			/* If the current process or program counter is changed......renew the counter. */
			pr_alert("First Time Recovery curr:%p pc=%p:%p\n",
				(void *)current, (void *)pc, (void *)*prev_undefinstr_pc);
			*prev_undefinstr_pc = pc;
			*prev_undefinstr_counter = 0;
			put_cpu_var(__prev_undefinstr_pc);
			put_cpu_var(__prev_undefinstr_counter);
			__cpuc_flush_icache_all();
			flush_cache_all();
			if (!user_mode(regs)) {
				thread->cpu_excp--;
			}
			return 0;
		}
		else if(*prev_undefinstr_counter < 1) {
			pr_alert("2nd Time Recovery curr:%p pc=%p:%p\n",
				(void *)current, (void *)pc,
				(void *)*prev_undefinstr_pc);
			*prev_undefinstr_counter += 1;
			put_cpu_var(__prev_undefinstr_pc);
			put_cpu_var(__prev_undefinstr_counter);
			__cpuc_flush_icache_all();
			flush_cache_all();
			if (!user_mode(regs)) {
				thread->cpu_excp--;
			}
			return 0;
		}
		*prev_undefinstr_counter += 1;
		if(*prev_undefinstr_counter >= 4) {
			/* 2=first time SigILL,3=2nd time NE-SigILL,4=3rd time CoreDump-SigILL */
			*prev_undefinstr_pc = 0;
			*prev_undefinstr_counter = 0;
		}
		put_cpu_var(__prev_undefinstr_pc);
		put_cpu_var(__prev_undefinstr_counter);
		pr_alert("Go to ARM Notify Die curr:%p pc=%p:%p\n",
			(void *)current, (void *)pc, (void *)*prev_undefinstr_pc);
	}
	return 1;
}
