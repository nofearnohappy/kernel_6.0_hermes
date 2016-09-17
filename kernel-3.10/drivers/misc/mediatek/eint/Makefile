include $(srctree)/drivers/misc/mediatek/Makefile.custom
ifneq ($(CONFIG_MTK_EIC),y)
obj-y += eint_drv.o
endif
obj-$(CONFIG_ARCH_MT6595) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_ARCH_MT6580) += $(subst ",,$(CONFIG_MTK_PLATFORM))/

