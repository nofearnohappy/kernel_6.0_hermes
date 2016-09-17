LINUXINCLUDE += -include $(srctree)/kernel/sched/sched.h

#
# RQ stats for TLP estimation
#
# For user space operation
obj-$(CONFIG_MTK_SCHED_RQAVG_US) += rq_stats.o
# For kernel space operation
obj-$(CONFIG_MTK_SCHED_RQAVG_KS) += sched_avg.o
# For query cpu topology
obj-y += cputopo.o

# For Enhance CPU affinity in hotplug
obj-$(CONFIG_MT_SCHED) += mt_cpu_affinity/
