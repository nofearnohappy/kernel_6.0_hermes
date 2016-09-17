/*
 * arch/arm/kernel/topology.c
 *
 * Copyright (C) 2011 Linaro Limited.
 * Written by: Vincent Guittot
 *
 * based on arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/cputype.h>
#include <asm/smp_plat.h>
#include <asm/topology.h>

#include <trace/events/sched.h>

/*
 * cpu power scale management
 */

/*
 * cpu power table
 * This per cpu data structure describes the relative capacity of each core.
 * On a heteregenous system, cores don't have the same computation capacity
 * and we reflect that difference in the cpu_power field so the scheduler can
 * take this difference into account during load balance. A per cpu structure
 * is preferred because each CPU updates its own cpu_power field during the
 * load balance except for idle cores. One idle core is selected to run the
 * rebalance_domains for all idle cores and the cpu_power can be updated
 * during this sequence.
 */

/* when CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY is in use, a new measure of
 * compute capacity is available. This is limited to a maximum of 1024 and
 * scaled between 0 and 1023 according to frequency.
 * Cores with different base CPU powers are scaled in line with this.
 * CPU capacity for each core represents a comparable ratio to maximum
 * achievable core compute capacity for a core in this system.
 *
 * e.g.1 If all cores in the system have a base CPU power of 1024 according to
 * efficiency calculations and are DVFS scalable between 500MHz and 1GHz, the
 * cores currently at 1GHz will have CPU power of 1024 whilst the cores
 * currently at 500MHz will have CPU power of 512.
 *
 * e.g.2
 * If core 0 has a base CPU power of 2048 and runs at 500MHz & 1GHz whilst
 * core 1 has a base CPU power of 1024 and runs at 100MHz and 200MHz, then
 * the following possibilities are available:
 *
 * cpu power\| 1GHz:100Mhz | 1GHz : 200MHz | 500MHz:100MHz | 500MHz:200MHz |
 * ----------|-------------|---------------|---------------|---------------|
 *    core 0 |    1024     |     1024      |     512       |     512       |
 *    core 1 |     256     |      512      |     256       |     512       |
 *
 * This information may be useful to the scheduler when load balancing,
 * so that the compute capacity of the core a task ran on can be baked into
 * task load histories.
 */
static DEFINE_PER_CPU(unsigned long, cpu_scale);
#ifdef CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY
static DEFINE_PER_CPU(unsigned long, invariant_cpu_capacity);
static DEFINE_PER_CPU(unsigned long, prescaled_cpu_capacity);
#endif /* CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY */

static int frequency_invariant_power_enabled = 1;

/* >0=1, <=0=0 */
void arch_set_invariant_power_enabled(int val)
{
	if(val>0)
		frequency_invariant_power_enabled = 1;
	else
		frequency_invariant_power_enabled = 0;
}

int arch_get_invariant_power_enabled(void)
{
	return frequency_invariant_power_enabled;
}

unsigned long arch_scale_freq_power(struct sched_domain *sd, int cpu)
{
	return per_cpu(cpu_scale, cpu);
}

static void set_power_scale(unsigned int cpu, unsigned long power)
{
	per_cpu(cpu_scale, cpu) = power;
}

unsigned long arch_get_max_cpu_capacity(int cpu)
{
	return per_cpu(cpu_scale, cpu);
}

#ifdef CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY
unsigned long arch_get_cpu_capacity(int cpu)
{
	return per_cpu(invariant_cpu_capacity, cpu);
}
#else
unsigned long arch_get_cpu_capacity(int cpu)
{
	return per_cpu(cpu_scale, cpu);
}
#endif /* CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY */

#ifdef CONFIG_OF
struct cpu_efficiency {
	const char *compatible;
	unsigned long efficiency;
};

/*
 * Table of relative efficiency of each processors
 * The efficiency value must fit in 20bit and the final
 * cpu_scale value must be in the range
 *   0 < cpu_scale < 3*SCHED_POWER_SCALE/2
 * in order to return at most 1 when DIV_ROUND_CLOSEST
 * is used to compute the capacity of a CPU.
 * Processors that are not defined in the table,
 * use the default SCHED_POWER_SCALE value for cpu_scale.
 */
struct cpu_efficiency table_efficiency[] = {
	{"arm,cortex-a57", 3891},
	{"arm,cortex-a53", 2048},
	{NULL, },
};

struct cpu_capacity {
	unsigned long hwid;
	unsigned long capacity;
};

struct cpu_capacity *cpu_capacity;

unsigned long middle_capacity = 1;
unsigned long min_capacity = (unsigned long)(-1);
unsigned long max_capacity = 0;
/*
 * Iterate all CPUs' descriptor in DT and compute the efficiency
 * (as per table_efficiency). Also calculate a middle efficiency
 * as close as possible to  (max{eff_i} - min{eff_i}) / 2
 * This is later used to scale the cpu_power field such that an
 * 'average' CPU is of middle power. Also see the comments near
 * table_efficiency[] and update_cpu_power().
 */
static void __init parse_dt_topology(void)
{
	struct cpu_efficiency *cpu_eff;
	struct device_node *cn = NULL;
	unsigned long capacity = 0;
	int alloc_size, cpu = 0;

	alloc_size = nr_cpu_ids * sizeof(struct cpu_capacity);
	cpu_capacity = kzalloc(alloc_size, GFP_NOWAIT);

	while ((cn = of_find_node_by_type(cn, "cpu"))) {
		const u32 *rate, *reg;
		int len;

		if (cpu >= num_possible_cpus())
			break;

		for (cpu_eff = table_efficiency; cpu_eff->compatible; cpu_eff++)
			if (of_device_is_compatible(cn, cpu_eff->compatible))
				break;

		if (cpu_eff->compatible == NULL)
			continue;

		rate = of_get_property(cn, "clock-frequency", &len);
		if (!rate || len != 4) {
			pr_err("%s missing clock-frequency property\n",
				   cn->full_name);
			continue;
		}

		reg = of_get_property(cn, "reg", &len);
		if (!reg || len != 4) {
			pr_err("%s missing reg property\n", cn->full_name);
			continue;
		}

		capacity = ((be32_to_cpup(rate)) >> 20) * cpu_eff->efficiency;

		/* Save min capacity of the system */
		if (capacity < min_capacity)
			min_capacity = capacity;

		/* Save max capacity of the system */
		if (capacity > max_capacity)
			max_capacity = capacity;

		cpu_capacity[cpu].capacity = capacity;
		cpu_topology[cpu].core_id = be32_to_cpup(reg) & 0xFF;
		cpu_topology[cpu].socket_id = be32_to_cpup(reg) >> 0x8 & 0xFF;
		pr_debug("CPU%u: hwid(0x%0x) socket_id(%i) core_id(%i) cap(%lu)\n",
			cpu, be32_to_cpup(reg), cpu_topology[cpu].socket_id, cpu_topology[cpu].core_id, capacity);
		cpu_capacity[cpu++].hwid = be32_to_cpup(reg);
	}

	if (cpu < num_possible_cpus())
		cpu_capacity[cpu].hwid = (unsigned long)(-1);

	/* If min and max capacities are equals, we bypass the update of the
	 * cpu_scale because all CPUs have the same capacity. Otherwise, we
	 * compute a middle_capacity factor that will ensure that the capacity
	 * of an 'average' CPU of the system will be as close as possible to
	 * SCHED_POWER_SCALE, which is the default value, but with the
	 * constraint explained near table_efficiency[].
	 */
	if (min_capacity == max_capacity)
		cpu_capacity[0].hwid = (unsigned long)(-1);
	else if (4*max_capacity < (3*(max_capacity + min_capacity)))
		middle_capacity = (min_capacity + max_capacity)
			>> (SCHED_POWER_SHIFT+1);
	else
		middle_capacity = ((max_capacity / 3)
						   >> (SCHED_POWER_SHIFT-1)) + 1;

}

/*
 * Look for a customed capacity of a CPU in the cpu_capacity table during the
 * boot. The update of all CPUs is in O(n^2) for heteregeneous system but the
 * function returns directly for SMP system.
 */
void update_cpu_power(unsigned int cpu, unsigned long hwid)
{
	unsigned int idx = 0;

	/* look for the cpu's hwid in the cpu capacity table */
	for (idx = 0; idx < num_possible_cpus(); idx++) {
		if (cpu_capacity[idx].hwid == (hwid&0xFFFF))
			break;

		if (cpu_capacity[idx].hwid == -1)
			return;
	}

	if (idx == num_possible_cpus())
		return;

	set_power_scale(cpu, (cpu_capacity[idx].capacity << SCHED_POWER_SHIFT) / max_capacity);

	printk(KERN_INFO "CPU%u: update cpu_power %lu\n",
		   cpu, arch_scale_freq_power(NULL, cpu));
}

#else
static inline void parse_dt_topology(void) {}
static inline void update_cpu_power(unsigned int cpuid, unsigned int mpidr) {}
#endif

/*
 * cpu topology table
 */
struct cputopo_arm cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

#if defined (CONFIG_MTK_SCHED_CMP_PACK_SMALL_TASK) || defined (CONFIG_HMP_PACK_SMALL_TASK)
int arch_sd_share_power_line(void)
{
	return 0*SD_SHARE_POWERLINE;
}
#endif /* CONFIG_MTK_SCHED_CMP_PACK_SMALL_TASK || CONFIG_HMP_PACK_SMALL_TASK  */

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

void update_siblings_masks(unsigned int cpuid)
{
	struct cputopo_arm *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];
	int cpu;

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->socket_id != cpu_topo->socket_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->core_sibling);

		if (cpuid_topo->core_id != cpu_topo->core_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->thread_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->thread_sibling);
	}
	smp_wmb();
}

/*
 * store_cpu_topology is called at boot when only one cpu is running
 * and with the mutex cpu_hotplug.lock locked, when several cpus have booted,
 * which prevents simultaneous write access to cpu_topology array
 */
void store_cpu_topology(unsigned int cpuid)
{
	struct cputopo_arm *cpuid_topo = &cpu_topology[cpuid];
	unsigned int mpidr;

	mpidr = read_cpuid_mpidr();

	/* If the cpu topology has been already set, just return */
	if (cpuid_topo->socket_id != -1)
		goto topology_populated;

	/* create cpu topology mapping */
	if ((mpidr & MPIDR_SMP_BITMASK) == MPIDR_SMP_VALUE) {
		/*
		 * This is a multiprocessor system
		 * multiprocessor format & multiprocessor mode field are set
		 */

		if (mpidr & MPIDR_MT_BITMASK) {
			/* core performance interdependency */
			cpuid_topo->thread_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 2);
		} else {
			/* largely independent cores */
			cpuid_topo->thread_id = -1;
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
		}
	} else {
		/*
		 * This is an uniprocessor system
		 * we are in multiprocessor format but uniprocessor system
		 * or in the old uniprocessor format
		 */
		cpuid_topo->thread_id = -1;
		cpuid_topo->core_id = 0;
		cpuid_topo->socket_id = -1;
	}

topology_populated:
	update_siblings_masks(cpuid);

	update_cpu_power(cpuid, mpidr & MPIDR_HWID_BITMASK);

	printk(KERN_INFO "CPU%u: thread %d, cpu %d, socket %d, mpidr %x\n",
		   cpuid, cpu_topology[cpuid].thread_id,
		   cpu_topology[cpuid].core_id,
		   cpu_topology[cpuid].socket_id, mpidr);
}

/*
 * cluster_to_logical_mask - return cpu logical mask of CPUs in a cluster
 * @socket_id:		cluster HW identifier
 * @cluster_mask:	the cpumask location to be initialized, modified by the
 *			function only if return value == 0
 *
 * Return:
 *
 * 0 on success
 * -EINVAL if cluster_mask is NULL or there is no record matching socket_id
 */
int cluster_to_logical_mask(unsigned int socket_id, cpumask_t *cluster_mask)
{
	int cpu;

	if (!cluster_mask)
		return -EINVAL;

	for_each_online_cpu(cpu)
		if (socket_id == topology_physical_package_id(cpu)) {
			cpumask_copy(cluster_mask, topology_core_cpumask(cpu));
			return 0;
		}

	return -EINVAL;
}

#ifdef CONFIG_SCHED_HMP
void __init arch_get_fast_and_slow_cpus(struct cpumask *fast,
										struct cpumask *slow)
{
	unsigned int cpu;

	cpumask_clear(fast);
	cpumask_clear(slow);

	/*
	 * Use the config options if they are given. This helps testing
	 * HMP scheduling on systems without a big.LITTLE architecture.
	 */
	if (strlen(CONFIG_HMP_FAST_CPU_MASK) && strlen(CONFIG_HMP_SLOW_CPU_MASK)) {
		if (cpulist_parse(CONFIG_HMP_FAST_CPU_MASK, fast))
			WARN(1, "Failed to parse HMP fast cpu mask!\n");
		if (cpulist_parse(CONFIG_HMP_SLOW_CPU_MASK, slow))
			WARN(1, "Failed to parse HMP slow cpu mask!\n");
		return;
	}

	/* check by capacity */
	for_each_possible_cpu(cpu) {
		if (cpu_capacity[cpu].capacity > min_capacity)
			cpumask_set_cpu(cpu, fast);
		else
			cpumask_set_cpu(cpu, slow);
	}

	if (!cpumask_empty(fast) && !cpumask_empty(slow))
		return;

	/*
	 * We didn't find both big and little cores so let's call all cores
	 * fast as this will keep the system running, with all cores being
	 * treated equal.
	 */
	cpumask_setall(slow);
	cpumask_clear(fast);
}

struct cpumask hmp_fast_cpu_mask;
struct cpumask hmp_slow_cpu_mask;

void __init arch_get_hmp_domains(struct list_head *hmp_domains_list)
{
	struct hmp_domain *domain;

	arch_get_fast_and_slow_cpus(&hmp_fast_cpu_mask, &hmp_slow_cpu_mask);

	/*
	 * Initialize hmp_domains
	 * Must be ordered with respect to compute capacity.
	 * Fastest domain at head of list.
	 */
	if(!cpumask_empty(&hmp_slow_cpu_mask)) {
		domain = (struct hmp_domain *)
			kmalloc(sizeof(struct hmp_domain), GFP_KERNEL);
		cpumask_copy(&domain->possible_cpus, &hmp_slow_cpu_mask);
		cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
		list_add(&domain->hmp_domains, hmp_domains_list);
	}
	domain = (struct hmp_domain *)
		kmalloc(sizeof(struct hmp_domain), GFP_KERNEL);
	cpumask_copy(&domain->possible_cpus, &hmp_fast_cpu_mask);
	cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
	list_add(&domain->hmp_domains, hmp_domains_list);
}
#endif /* CONFIG_SCHED_HMP */

static int cpu_topology_init;
/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */
void __init init_cpu_topology(void)
{
	unsigned int cpu;

	if (cpu_topology_init)
		return;
	/* init core mask and power*/
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		cpu_topo->thread_id = -1;
		cpu_topo->core_id =  -1;
		cpu_topo->socket_id = -1;
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);

		set_power_scale(cpu, SCHED_POWER_SCALE);
	}
	smp_wmb();

	parse_dt_topology();
}

void __init arch_build_cpu_topology_domain(void)
{
	init_cpu_topology();
	cpu_topology_init = 1;
}


#ifdef CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY
#include <linux/cpufreq.h>

struct cpufreq_extents {
	u32 max;
	u32 flags;
	u32 const_max;
	u32 throttling;
};
/* Flag set when the governor in use only allows one frequency.
 * Disables scaling.
 */
#define CPUPOWER_FREQINVAR_SINGLEFREQ 0x01
static struct cpufreq_extents freq_scale[CONFIG_NR_CPUS];

int arch_get_cpu_throttling(int cpu)
{
	return freq_scale[cpu].throttling;
}

/* Called when the CPU Frequency is changed.
 * Once for each CPU.
 */
static int cpufreq_callback(struct notifier_block *nb,
							unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	int cpu = freq->cpu;
	struct cpufreq_extents *extents;
	unsigned int curr_freq;

	if (freq->flags & CPUFREQ_CONST_LOOPS)
		return NOTIFY_OK;

	if (val != CPUFREQ_POSTCHANGE)
		return NOTIFY_OK;

	/* if dynamic load scale is disabled, set the load scale to 1.0 */
	if (!frequency_invariant_power_enabled) {
		per_cpu(invariant_cpu_capacity, cpu) = per_cpu(cpu_scale, cpu);
		return NOTIFY_OK;
	}

	extents = &freq_scale[cpu];
	if (extents->max < extents->const_max) {
		extents->throttling = 1;
	} else {
		extents->throttling = 0;
	}
	/* If our governor was recognised as a single-freq governor,
	 * use curr = max to be sure multiplier is 1.0
	 */
	if (extents->flags & CPUPOWER_FREQINVAR_SINGLEFREQ)
		curr_freq = extents->max;
	else
		curr_freq = freq->new >> CPUPOWER_FREQSCALE_SHIFT;

	per_cpu(invariant_cpu_capacity, cpu) = DIV_ROUND_UP(
		(curr_freq * per_cpu(prescaled_cpu_capacity, cpu)), CPUPOWER_FREQSCALE_DEFAULT);

	mt_sched_printf(sched_lb_info,
		"[%s] CPU%d: pre/inv(%lu/%lu) flags:0x%x freq: cur/new/max/const_max(%u/%u/%u/%u)", __func__,
		cpu, per_cpu(prescaled_cpu_capacity, cpu), per_cpu(invariant_cpu_capacity, cpu),
		extents->flags, curr_freq, freq->new, extents->max, extents->const_max);

	return NOTIFY_OK;
}

/* Called when the CPUFreq governor is changed.
 * Only called for the CPUs which are actually changed by the
 * userspace.
 */
static int cpufreq_policy_callback(struct notifier_block *nb,
								   unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	struct cpufreq_extents *extents;
	int cpu, singleFreq = 0;
	static const char performance_governor[] = "performance";
	static const char powersave_governor[] = "powersave";

	if (event == CPUFREQ_START)
		return NOTIFY_OK;

	if (event != CPUFREQ_INCOMPATIBLE)
		return NOTIFY_OK;

	/* CPUFreq governors do not accurately report the range of
	 * CPU Frequencies they will choose from.
	 * We recognise performance and powersave governors as
	 * single-frequency only.
	 */
	if (!strncmp(policy->governor->name, performance_governor,
				 strlen(performance_governor)) ||
		!strncmp(policy->governor->name, powersave_governor,
				 strlen(powersave_governor)))
		singleFreq = 1;

	/* Make sure that all CPUs impacted by this policy are
	 * updated since we will only get a notification when the
	 * user explicitly changes the policy on a CPU.
	 */
	for_each_cpu(cpu, policy->cpus) {
		extents = &freq_scale[cpu];
		extents->max = policy->max >> CPUPOWER_FREQSCALE_SHIFT;
		extents->const_max = policy->cpuinfo.max_freq >> CPUPOWER_FREQSCALE_SHIFT;
		if (!frequency_invariant_power_enabled) {
			/* when disabled, invariant_cpu_scale = cpu_scale */
			per_cpu(invariant_cpu_capacity, cpu) = per_cpu(cpu_scale, cpu);
			/* unused when disabled */
			per_cpu(prescaled_cpu_capacity, cpu) = per_cpu(cpu_scale, cpu);
		} else {
			if (singleFreq)
				extents->flags |= CPUPOWER_FREQINVAR_SINGLEFREQ;
			else
				extents->flags &= ~CPUPOWER_FREQINVAR_SINGLEFREQ;
			per_cpu(prescaled_cpu_capacity, cpu) =
				((per_cpu(cpu_scale, cpu) << CPUPOWER_FREQSCALE_SHIFT) / extents->max);

			per_cpu(invariant_cpu_capacity, cpu) = DIV_ROUND_UP(
				((policy->cur>>CPUPOWER_FREQSCALE_SHIFT) *
				 per_cpu(prescaled_cpu_capacity, cpu)), CPUPOWER_FREQSCALE_DEFAULT);
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block cpufreq_notifier = {
	.notifier_call  = cpufreq_callback,
};
static struct notifier_block cpufreq_policy_notifier = {
	.notifier_call  = cpufreq_policy_callback,
};

static int __init register_topology_cpufreq_notifier(void)
{
	int ret;

	/* init safe defaults since there are no policies at registration */
	for (ret = 0; ret < CONFIG_NR_CPUS; ret++) {
		/* safe defaults */
		freq_scale[ret].max = CPUPOWER_FREQSCALE_DEFAULT;
		per_cpu(invariant_cpu_capacity, ret) = CPUPOWER_FREQSCALE_DEFAULT;
		per_cpu(prescaled_cpu_capacity, ret) = CPUPOWER_FREQSCALE_DEFAULT;
	}

	pr_info("topology: registering cpufreq notifiers for scale-invariant CPU Power\n");
	ret = cpufreq_register_notifier(&cpufreq_policy_notifier,
									CPUFREQ_POLICY_NOTIFIER);

	if (ret != -EINVAL)
		ret = cpufreq_register_notifier(&cpufreq_notifier,
										CPUFREQ_TRANSITION_NOTIFIER);

	return ret;
}

core_initcall(register_topology_cpufreq_notifier);

#else /* !CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY */

int arch_get_cpu_throttling(int cpu)
{
	return 0;
}

#endif /* CONFIG_ARCH_SCALE_INVARIANT_CPU_CAPACITY */


/*
 * Extras of CPU & Cluster functions
 */
/* range: 1 ~  (1 << SCHED_POWER_SHIFT) */
int arch_cpu_cap_ratio(unsigned int cpu)
{
	unsigned long ratio = (cpu_capacity[cpu].capacity << SCHED_POWER_SHIFT) / max_capacity;
	BUG_ON(cpu >= num_possible_cpus());
	return (int)ratio;
}

int arch_is_smp(void)
{
	static int __arch_smp = -1;

	if (__arch_smp != -1)
		return __arch_smp;

	__arch_smp = (max_capacity != min_capacity) ? 0 : 1;

	return __arch_smp;
}

int arch_get_nr_clusters(void)
{
	static int __arch_nr_clusters = -1;
	int max_id = 0;
	unsigned int cpu;

	if (__arch_nr_clusters != -1)
		return __arch_nr_clusters;

	/* assume socket id is monotonic increasing without gap. */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];
		if (arm_cputopo->socket_id > max_id)
			max_id = arm_cputopo->socket_id;
	}
	__arch_nr_clusters = max_id + 1;
	return __arch_nr_clusters;
}

int arch_is_multi_cluster(void)
{
	return arch_get_nr_clusters() > 1 ? 1 : 0;
}

int arch_get_cluster_id(unsigned int cpu)
{
	struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];
	return arm_cputopo->socket_id < 0 ? 0 : arm_cputopo->socket_id;
}

void arch_get_cluster_cpus(struct cpumask *cpus, int cluster_id)
{
	unsigned int cpu;

	cpumask_clear(cpus);
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *arm_cputopo = &cpu_topology[cpu];
		if (arm_cputopo->socket_id == cluster_id)
			cpumask_set_cpu(cpu, cpus);
	}
}

void arch_get_big_little_cpus(struct cpumask *big, struct cpumask *little)
{
	unsigned int cpu;
	cpumask_clear(big);
	cpumask_clear(little);
	for_each_possible_cpu(cpu) {
		if (cpu_capacity[cpu].capacity > min_capacity)
			cpumask_set_cpu(cpu, big);
		else
			cpumask_set_cpu(cpu, little);
	}
}

int arch_better_capacity(unsigned int cpu)
{
	BUG_ON(cpu >= num_possible_cpus());
	return cpu_capacity[cpu].capacity > min_capacity;
}
