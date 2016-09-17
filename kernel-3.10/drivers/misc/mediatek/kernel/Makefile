include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)
obj-$(CONFIG_KGDB_KDB) += kdb_enhance.o
obj-$(CONFIG_ARM) += mt_cache_v7.o
obj-$(CONFIG_ARM64) += mt_cache_v8.o
obj-y += fiq_smp_call.o
#obj-y += sec_osal.o
