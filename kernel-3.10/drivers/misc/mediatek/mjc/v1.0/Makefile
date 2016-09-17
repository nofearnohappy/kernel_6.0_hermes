include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -D$(CONFIG_MTK_PLATFORM)

obj-y += mjc_kernel_driver.o
ifdef CONFIG_COMPAT
obj-y += mjc_kernel_compat_driver.o
endif